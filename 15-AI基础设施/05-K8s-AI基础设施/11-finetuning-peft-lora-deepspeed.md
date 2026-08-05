---
title: "K8s 上的模型微调基础设施（PEFT/LoRA/DeepSpeed/FSDP）"
description: "Kubernetes 上 LLM 微调任务的基础设施规划：PEFT/LoRA/QLoRA 方法对比、DeepSpeed ZeRO 部署、FSDP 配置及 GPU 资源规划"
summary: "覆盖 Full fine-tuning vs PEFT vs LoRA vs QLoRA 对比，DeepSpeed ZeRO-1/2/3 在 K8s 的部署，PyTorch FSDP 配置，Kubeflow PyTorchJob 微调任务编排，GPU 内存估算，Checkpoint 管理及故障排查"
category: AI基础设施
tags:
- fine-tuning
- peft
- lora
- qlora
- deepspeed
- fsdp
- pytorchjob
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- MLOps 工程师
- SRE
estimated_read_time: 20min
intent_queries:
- "LoRA 微调需要多少 GPU 内存"
- "DeepSpeed ZeRO-3 在 K8s 怎么部署"
- "PyTorchJob 微调任务怎么配置"
trigger_keywords:
- lora
- qlora
- peft
- deepspeed
- fsdp
- fine-tuning
- pytorchjob
prerequisites:
- kubectl-basics
- helm-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# K8s 上的模型微调基础设施（PEFT/LoRA/DeepSpeed/FSDP）

## 概述

大语言模型微调是将预训练模型适配到特定业务场景的关键步骤。随着模型规模从 7B 增长到 70B 乃至更大，微调对 GPU 内存、分布式训练框架和基础设施编排提出了严峻挑战。Kubernetes 作为 AI 基础设施的底座，需要为微调任务提供弹性资源调度、分布式训练编排、Checkpoint 管理和故障恢复能力。

本文覆盖微调方法对比（Full fine-tuning / PEFT / LoRA / QLoRA）、DeepSpeed ZeRO 各阶段的 K8s 部署、PyTorch FSDP 配置、Kubeflow PyTorchJob 任务编排、GPU 内存精确估算、数据管线设计以及 Checkpoint 生命周期管理。

相关页面：[[15-AI基础设施/05-K8s-AI基础设施/02-gpu-cluster-scheduling-inference-serving|GPU调度与资源管理]]、[[23-实体/11-AI与边缘/kubeflow|Kubeflow训练平台]]、[[15-AI基础设施/05-K8s-AI基础设施/08-batch-scheduling-kueue-yunikorn|Kueue与YuniKorn批量调度]]、[[15-AI基础设施/05-K8s-AI基础设施/10-rdma-infiniband-gpudirect-networking|AI高性能网络]]、[[17-系统基础/06-知识字典/storage/persistent-volume|K8s存储与PV管理]]

## 架构与核心概念

### 微调范式对比

| 方法 | 可训练参数 | 7B 模型 GPU 内存 | 70B 模型 GPU 内存 | 精度损失 | 适用场景 |
|------|-----------|-----------------|-----------------|---------|---------|
| Full Fine-tuning | 100% (~7B) | 8×A100 80GB | 64×A100 80GB | 无 | 数据充足、追求最优 |
| PEFT (Adapter) | ~1-5% | 1×A100 80GB | 4×A100 80GB | 极小 | 多任务适配 |
| LoRA (r=16) | ~0.1-1% | 1×A100 40GB | 2×A100 80GB | 极小 | 资源受限、快速迭代 |
| QLoRA (4bit+LoRA) | ~0.1-1% | 1×A100 24GB | 1×A100 80GB | 小 | 单卡微调大模型 |
| Prefix Tuning | ~0.1% | 1×A100 40GB | 2×A100 80GB | 中等 | NLU 任务 |

### GPU 内存估算公式

```
微调 GPU 内存组成:

1. 模型参数 (Model Parameters):
   - FP32: 4 bytes × params
   - FP16/BF16: 2 bytes × params
   - INT8: 1 byte × params
   - INT4 (QLoRA): 0.5 bytes × params

2. 优化器状态 (Optimizer States):
   - Adam: 8 bytes × params (FP32 momentum + variance)
   - AdamW (ZeRO-1 分片): 8 bytes × params / N_gpus
   - 8-bit Adam: 2 bytes × params

3. 梯度 (Gradients):
   - FP16: 2 bytes × params
   - FP32: 4 bytes × params

4. 激活值 (Activations):
   - 与 batch_size × seq_len × hidden_dim 成正比
   - Gradient Checkpointing 可减少 60-70%

示例（LLaMA-7B, LoRA r=16, BF16, batch=4, seq=2048）:
  模型参数 (BF16): 7B × 2 = 14 GB
  LoRA 参数: ~20M × 2 = 40 MB（可忽略）
  优化器 (仅 LoRA): ~20M × 8 = 160 MB
  激活值 (gradient ckpt): ~4 GB
  总计: ~19 GB → 单张 A100 40GB 即可

示例（LLaMA-70B, QLoRA 4bit, batch=1, seq=2048）:
  模型参数 (INT4): 70B × 0.5 = 35 GB
  LoRA 参数: ~50M × 2 = 100 MB
  优化器: ~50M × 8 = 400 MB
  激活值: ~6 GB
  总计: ~42 GB → 单张 A100 80GB 可行
```

### DeepSpeed ZeRO 阶段

```
ZeRO (Zero Redundancy Optimizer) 内存优化:

ZeRO Stage 1 (优化器状态分片):
  - 将 Adam 状态分片到 N 个 GPU
  - 内存节省: 优化器部分减少到 1/N
  - 通信量: 与 DDP 相同
  - 适用: 模型可放入单卡但优化器状态过大

ZeRO Stage 2 (优化器 + 梯度分片):
  - 在 Stage 1 基础上分片梯度
  - 内存节省: 优化器 + 梯度减少到 1/N
  - 通信量: 与 DDP 相同
  - 适用: 中等规模模型（7B-13B）多卡训练

ZeRO Stage 3 (全分片):
  - 参数 + 梯度 + 优化器全部分片
  - 内存节省: 所有状态减少到 1/N
  - 通信量: 增加 1.5x（需要 AllGather 参数）
  - 适用: 大模型（30B+）多卡/多节点训练
  - 注意: 通信密集，需要高速网络（RDMA）

ZeRO-Offload / ZeRO-Infinity:
  - 将部分状态卸载到 CPU 内存或 NVMe
  - 进一步减少 GPU 内存需求
  - 代价: 训练速度降低 20-50%
```

## 生产部署

### Kubeflow Training Operator 安装

```bash
# 🟡 中风险：安装 Kubeflow Training Operator
kubectl apply --server-side -k "github.com/kubeflow/training-operator/manifests/overlays/standalone?ref=v1.8.1"

# 验证
kubectl get pods -n kubeflow -l app=training-operator
kubectl get crd | grep kubeflow
```

### DeepSpeed ZeRO-3 微调任务

```yaml
# 🟡 中风险：DeepSpeed ZeRO-3 多节点微调 PyTorchJob
apiVersion: kubeflow.org/v1
kind: PyTorchJob
metadata:
  name: llama-70b-sft-zero3
  namespace: ai-training
  labels:
    kueue.x-k8s.io/queue-name: ml-training-queue
spec:
  elasticPolicy:
    rdzvBackend: c10d
    minReplicas: 2
    maxReplicas: 8
    maxRestarts: 3
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      restartPolicy: OnFailure
      template:
        spec:
          containers:
          - name: pytorch
            image: registry.internal/ai/deepspeed-training:0.15-cuda12.4
            command:
            - deepspeed
            - --num_gpus=8
            - --num_nodes=4
            - --hostfile=/etc/deepspeed/hostfile
            - train_sft.py
            - --deepspeed=ds_config_zero3.json
            - --model_name_or_path=/models/llama-70b
            - --data_path=/data/sft-corpus
            - --output_dir=/checkpoints/llama-70b-sft
            - --per_device_train_batch_size=1
            - --gradient_accumulation_steps=16
            - --max_seq_length=4096
            - --num_train_epochs=3
            - --learning_rate=2e-5
            - --bf16=true
            - --gradient_checkpointing=true
            - --lora_r=64
            - --lora_alpha=128
            - --lora_target_modules=q_proj,k_proj,v_proj,o_proj,gate_proj,up_proj,down_proj
            resources:
              limits:
                nvidia.com/gpu: "8"
                memory: "512Gi"
              requests:
                nvidia.com/gpu: "8"
                cpu: "64"
                memory: "256Gi"
            volumeMounts:
            - name: model-storage
              mountPath: /models
            - name: training-data
              mountPath: /data
            - name: checkpoint-storage
              mountPath: /checkpoints
            - name: shm
              mountPath: /dev/shm
          volumes:
          - name: model-storage
            persistentVolumeClaim:
              claimName: model-weights-pvc
          - name: training-data
            persistentVolumeClaim:
              claimName: sft-corpus-pvc
          - name: checkpoint-storage
            persistentVolumeClaim:
              claimName: checkpoint-pvc
          - name: shm
            emptyDir:
              medium: Memory
              sizeLimit: "128Gi"
          tolerations:
          - key: nvidia.com/gpu
            operator: Exists
            effect: NoSchedule
    Worker:
      replicas: 3
      restartPolicy: OnFailure
      template:
        spec:
          containers:
          - name: pytorch
            image: registry.internal/ai/deepspeed-training:0.15-cuda12.4
            resources:
              limits:
                nvidia.com/gpu: "8"
                memory: "512Gi"
              requests:
                nvidia.com/gpu: "8"
                cpu: "64"
                memory: "256Gi"
            volumeMounts:
            - name: model-storage
              mountPath: /models
            - name: training-data
              mountPath: /data
            - name: checkpoint-storage
              mountPath: /checkpoints
            - name: shm
              mountPath: /dev/shm
          volumes:
          - name: model-storage
            persistentVolumeClaim:
              claimName: model-weights-pvc
          - name: training-data
            persistentVolumeClaim:
              claimName: sft-corpus-pvc
          - name: checkpoint-storage
            persistentVolumeClaim:
              claimName: checkpoint-pvc
          - name: shm
            emptyDir:
              medium: Memory
              sizeLimit: "128Gi"
          tolerations:
          - key: nvidia.com/gpu
            operator: Exists
            effect: NoSchedule
```

### DeepSpeed 配置文件

```json
{
  "train_batch_size": "auto",
  "train_micro_batch_size_per_gpu": "auto",
  "gradient_accumulation_steps": "auto",
  "gradient_clipping": 1.0,
  "zero_optimization": {
    "stage": 3,
    "offload_optimizer": {
      "device": "none"
    },
    "offload_param": {
      "device": "none"
    },
    "overlap_comm": true,
    "contiguous_gradients": true,
    "sub_group_size": 1e9,
    "reduce_bucket_size": "auto",
    "stage3_prefetch_bucket_size": "auto",
    "stage3_param_persistence_threshold": "auto",
    "stage3_max_live_parameters": 1e9,
    "stage3_max_reuse_distance": 1e9,
    "gather_16bit_weights_on_model_save": true
  },
  "bf16": {
    "enabled": true
  },
  "zero_allow_untested_optimizer": true,
  "wall_clock_breakdown": false,
  "steps_per_print": 100
}
```

### PyTorch FSDP 微调任务

```yaml
# 🟡 中风险：FSDP 微调任务（无需 DeepSpeed 依赖）
apiVersion: kubeflow.org/v1
kind: PyTorchJob
metadata:
  name: llama-13b-fsdp-sft
  namespace: ai-training
spec:
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      restartPolicy: OnFailure
      template:
        spec:
          containers:
          - name: pytorch
            image: registry.internal/ai/pytorch-fsdp:2.4-cuda12.4
            command:
            - torchrun
            - --nproc_per_node=8
            - --nnodes=2
            - --node_rank=0
            - --master_addr=$(MASTER_ADDR)
            - --master_port=29500
            - train_fsdp.py
            - --model_name=/models/llama-13b
            - --fsdp_sharding_strategy=FULL_SHARD
            - --fsdp_auto_wrap_policy=TRANSFORMER_BASED_WRAP
            - --fsdp_transformer_layer_cls_to_wrap=LlamaDecoderLayer
            - --fsdp_backward_prefetch=BACKWARD_PRE
            - --fsdp_state_dict_type=SHARDED_STATE_DICT
            - --use_gradient_checkpointing=true
            - --precision=bf16
            env:
            - name: MASTER_ADDR
              valueFrom:
                fieldRef:
                  fieldPath: status.podIP
            resources:
              limits:
                nvidia.com/gpu: "8"
                memory: "256Gi"
            volumeMounts:
            - name: shm
              mountPath: /dev/shm
          volumes:
          - name: shm
            emptyDir:
              medium: Memory
              sizeLimit: "64Gi"
    Worker:
      replicas: 1
      restartPolicy: OnFailure
      template:
        spec:
          containers:
          - name: pytorch
            image: registry.internal/ai/pytorch-fsdp:2.4-cuda12.4
            command:
            - torchrun
            - --nproc_per_node=8
            - --nnodes=2
            - --node_rank=1
            - --master_addr=$(MASTER_ADDR)
            - --master_port=29500
            - train_fsdp.py
            resources:
              limits:
                nvidia.com/gpu: "8"
                memory: "256Gi"
```

## 运维操作

### 微调任务监控

```bash
# 🟢 低风险：查看 PyTorchJob 状态
kubectl get pytorchjobs -n ai-training
kubectl describe pytorchjob llama-70b-sft-zero3 -n ai-training

# 🟢 低风险：查看训练日志
kubectl logs -n ai-training llama-70b-sft-zero3-master-0 -c pytorch --tail=50 -f
# 关注: loss 曲线、throughput (samples/sec)、GPU 利用率

# 🟢 低风险：检查 GPU 利用率
kubectl exec -n ai-training llama-70b-sft-zero3-master-0 -- nvidia-smi
kubectl exec -n ai-training llama-70b-sft-zero3-master-0 -- nvidia-smi --query-gpu=utilization.gpu,memory.used,memory.total --format=csv -l 5

# 🟢 低风险：检查训练进度和 Checkpoint
kubectl exec -n ai-training llama-70b-sft-zero3-master-0 -- ls -la /checkpoints/llama-70b-sft/
kubectl exec -n ai-training llama-70b-sft-zero3-master-0 -- cat /checkpoints/llama-70b-sft/trainer_state.json | jq '.log_history[-5:]'
```

### Checkpoint 管理

```bash
# 🟢 低风险：查看 Checkpoint 大小和频率
kubectl exec -n ai-training llama-70b-sft-zero3-master-0 -- du -sh /checkpoints/llama-70b-sft/checkpoint-*/

# 🟡 中风险：手动保存 Checkpoint（通过信号）
kubectl exec -n ai-training llama-70b-sft-zero3-master-0 -- kill -USR1 1

# 🔴 高风险：清理旧 Checkpoint（释放存储空间）
kubectl exec -n ai-training llama-70b-sft-zero3-master-0 -- bash -c "
cd /checkpoints/llama-70b-sft
ls -dt checkpoint-* | tail -n +4 | xargs rm -rf
echo 'Kept latest 3 checkpoints'
"

# 🟡 中风险：LoRA 权重合并导出（用于推理部署）
kubectl exec -n ai-training llama-70b-sft-zero3-master-0 -- python3 -c "
from peft import PeftModel
from transformers import AutoModelForCausalLM, AutoTokenizer
import torch

base_model = AutoModelForCausalLM.from_pretrained('/models/llama-70b', torch_dtype=torch.bfloat16)
model = PeftModel.from_pretrained(base_model, '/checkpoints/llama-70b-sft/lora-adapter')
merged = model.merge_and_unload()
merged.save_pretrained('/checkpoints/llama-70b-sft/merged-model')
AutoTokenizer.from_pretrained('/models/llama-70b').save_pretrained('/checkpoints/llama-70b-sft/merged-model')
print('Model merged and saved')
"
```

### 数据加载管线

```yaml
# 🟡 中风险：数据预处理 Job（在训练前运行）
apiVersion: batch/v1
kind: Job
metadata:
  name: data-preprocess-sft
  namespace: ai-training
spec:
  parallelism: 4
  completions: 4
  template:
    spec:
      containers:
      - name: preprocess
        image: registry.internal/ai/data-pipeline:latest
        command:
        - python
        - preprocess.py
        - --input_dir=/raw-data/sft-corpus
        - --output_dir=/processed-data/sft-tokenized
        - --tokenizer=/models/llama-70b/tokenizer
        - --max_seq_length=4096
        - --num_workers=8
        resources:
          requests:
            cpu: "16"
            memory: "64Gi"
        volumeMounts:
        - name: raw-data
          mountPath: /raw-data
        - name: processed-data
          mountPath: /processed-data
        - name: model-storage
          mountPath: /models
      volumes:
      - name: raw-data
        persistentVolumeClaim:
          claimName: raw-corpus-pvc
      - name: processed-data
        persistentVolumeClaim:
          claimName: processed-corpus-pvc
      - name: model-storage
        persistentVolumeClaim:
          claimName: model-weights-pvc
      restartPolicy: Never
```

## 故障排查

### CUDA OOM

```bash
# 🟢 低风险：诊断 CUDA OOM
# Step 1: 确认 OOM 错误
kubectl logs <pod> -n ai-training --tail=100 | grep -i "out of memory\|CUDA OOM"
# 典型错误: "torch.cuda.OutOfMemoryError: CUDA out of memory. Tried to allocate X GiB"

# Step 2: 检查 GPU 内存使用
kubectl exec <pod> -n ai-training -- nvidia-smi --query-gpu=memory.used,memory.total --format=csv

# Step 3: 检查 DeepSpeed 内存报告
kubectl logs <pod> -n ai-training | grep -A20 "DeepSpeed Memory"

# 解决方案（按优先级）:
# 1. 减小 per_device_train_batch_size（如 4→2→1）
# 2. 增大 gradient_accumulation_steps（保持有效 batch 不变）
# 3. 启用 gradient_checkpointing
# 4. 升级 ZeRO stage（1→2→3）
# 5. 启用 ZeRO-Offload（CPU/NVMe）
# 6. 减小 max_seq_length
# 7. 使用 QLoRA 4bit 量化
```

### 梯度爆炸 / Loss 异常

```bash
# 🟢 低风险：诊断训练不稳定
kubectl logs <pod> -n ai-training --tail=200 | grep -E "loss|grad_norm|nan|inf"

# 常见原因及解决:
# 1. Loss 突然变为 NaN → 学习率过高，降低 lr 或增加 warmup
# 2. Grad norm 持续增大 → 启用 gradient_clipping (max_grad_norm=1.0)
# 3. Loss 不下降 → 检查数据质量、tokenizer 是否匹配
# 4. Loss 震荡 → batch size 过小，增加 gradient_accumulation_steps
```

### Checkpoint 不兼容

| 故障现象 | 可能原因 | 解决方案 |
|---------|---------|---------|
| 加载 Checkpoint 报 key mismatch | DeepSpeed ZeRO-3 分片保存 | 使用 `zero_to_fp32.py` 合并分片 |
| FSDP Checkpoint 无法单卡加载 | Sharded state dict | 使用 `torch.distributed.checkpoint` 加载 |
| LoRA adapter 版本不匹配 | PEFT 库版本升级 | 固定 PEFT 版本或重新训练 |
| Checkpoint 文件损坏 | 写入中断（Pod 被驱逐） | 从上一个完整 Checkpoint 恢复 |
| 合并后模型精度下降 | 量化精度损失 | 使用 BF16 合并，避免 FP16 溢出 |

```bash
# 🟡 中风险：DeepSpeed ZeRO-3 Checkpoint 合并
kubectl exec <pod> -n ai-training -- python3 -c "
import torch
from deepspeed.utils.zero_to_fp32 import convert_zero_checkpoint_to_fp32_state_dict
convert_zero_checkpoint_to_fp32_state_dict(
    '/checkpoints/llama-70b-sft/checkpoint-1000',
    '/checkpoints/llama-70b-sft/checkpoint-1000-fp32'
)
print('ZeRO checkpoint converted to FP32')
"
```

## 最佳实践

### 资源规划指南

| 模型规模 | 推荐方法 | GPU 配置 | 网络需求 | 存储需求 |
|---------|---------|---------|---------|---------|
| 7B | LoRA/QLoRA | 1-2× A100 80GB | 10GbE 足够 | 100GB SSD |
| 13B | LoRA + ZeRO-2 | 2-4× A100 80GB | 25GbE | 200GB SSD |
| 30B | LoRA + ZeRO-3 | 4-8× A100 80GB | 100GbE/RoCE | 500GB SSD |
| 70B | QLoRA 或 ZeRO-3 | 8× A100 80GB (QLoRA) / 16-32× (Full) | RDMA NDR | 1TB NVMe |
| 70B Full FT | ZeRO-3 + Offload | 64× A100 80GB | RDMA NDR | 2TB NVMe |

### 训练稳定性

1. **学习率调度**：使用 cosine schedule with warmup（warmup 3-5% steps）
2. **梯度裁剪**：`max_grad_norm=1.0` 防止梯度爆炸
3. **混合精度**：优先使用 BF16（动态范围大，不易溢出）
4. **Gradient Checkpointing**：牺牲 20-30% 速度换 60-70% 激活内存
5. **数据质量**：去重、过滤低质量样本、控制数据分布

### Checkpoint 策略

```yaml
# 推荐 Checkpoint 配置
save_strategy: "steps"
save_steps: 500           # 每 500 步保存
save_total_limit: 3       # 最多保留 3 个
load_best_model_at_end: true
metric_for_best_model: "eval_loss"
greater_is_better: false

# 存储建议:
# - 使用高性能并行文件系统（Lustre/GPFS/WekaFS）
# - Checkpoint 写入使用异步 IO（避免阻塞训练）
# - 定期将 Checkpoint 备份到对象存储（S3/GCS）
# - ZeRO-3 使用 gather_16bit_weights_on_model_save=true
```

### DeepSpeed vs FSDP 选型

- **选 DeepSpeed**：需要 ZeRO-Offload/Infinity（CPU/NVMe 卸载）；使用 HuggingFace Trainer 集成；需要更细粒度的内存控制
- **选 FSDP**：纯 PyTorch 原生（无额外依赖）；与 torch.compile 兼容更好；团队更熟悉 PyTorch 原生 API
- **性能对比**：两者在 ZeRO-3/FULL_SHARD 级别性能接近（<5% 差异），选择更多取决于生态和团队熟悉度

## Related

- [[15-AI基础设施/05-K8s-AI基础设施/02-gpu-cluster-scheduling-inference-serving|GPU调度与资源管理]]
- [[23-实体/11-AI与边缘/kubeflow|Kubeflow训练平台]]
- [[15-AI基础设施/05-K8s-AI基础设施/08-batch-scheduling-kueue-yunikorn|Kueue与YuniKorn批量调度]]
- [[15-AI基础设施/05-K8s-AI基础设施/10-rdma-infiniband-gpudirect-networking|AI高性能网络]]
- [[17-系统基础/06-知识字典/storage/persistent-volume|K8s存储与PV管理]]
