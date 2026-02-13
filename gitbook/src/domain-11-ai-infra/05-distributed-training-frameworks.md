# 分布式训练框架

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [PyTorch Distributed](https://pytorch.org/tutorials/beginner/dist_overview.html) | [DeepSpeed](https://www.deepspeed.ai/)

## 分布式训练架构对比

```
┌─────────────────────────────────────────────────────────────┐
│             数据并行 (Data Parallelism)                      │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ 完整模型 │  │ 完整模型 │  │ 完整模型 │  │ 完整模型 │   │
│  │ GPU 0    │  │ GPU 1    │  │ GPU 2    │  │ GPU 3    │   │
│  │ 数据分片1│  │ 数据分片2│  │ 数据分片3│  │ 数据分片4│   │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘   │
│       └──────────────┴──────────────┴──────────────┘       │
│                    梯度同步 (AllReduce)                     │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│             模型并行 (Model Parallelism)                     │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ 层 1-10  │─→│ 层11-20  │─→│ 层21-30  │─→│ 层31-40  │   │
│  │ GPU 0    │  │ GPU 1    │  │ GPU 2    │  │ GPU 3    │   │
│  │ 完整数据 │  │ 完整数据 │  │ 完整数据 │  │ 完整数据 │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
│                    流水线并行 (Pipeline)                    │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│        3D并行 (DP + PP + TP - DeepSpeed/Megatron)          │
│  ┌────────────────────────────────────────────────────┐    │
│  │  Data Parallel Group 1                             │    │
│  │  ┌──────────┐  ┌──────────┐  (Tensor Parallel)    │    │
│  │  │层1-20切片│  │层21-40切片│                        │    │
│  │  │  GPU 0   │  │  GPU 1   │  ← Pipeline Stage 1  │    │
│  │  └──────────┘  └──────────┘                        │    │
│  │  ┌──────────┐  ┌──────────┐                        │    │
│  │  │层41-60切片│ │层61-80切片│                        │    │
│  │  │  GPU 2   │  │  GPU 3   │  ← Pipeline Stage 2  │    │
│  │  └──────────┘  └──────────┘                        │    │
│  └────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

---

## 一、PyTorch分布式训练

### 1. DistributedDataParallel (DDP)

#### 训练脚本

```python
import torch
import torch.distributed as dist
from torch.nn.parallel import DistributedDataParallel as DDP
from torch.utils.data.distributed import DistributedSampler

def setup(rank, world_size):
    """初始化分布式环境"""
    dist.init_process_group(
        backend="nccl",  # GPU推荐nccl
        init_method="env://",  # 从环境变量读取配置
        world_size=world_size,
        rank=rank
    )
    torch.cuda.set_device(rank)

def cleanup():
    dist.destroy_process_group()

def train(rank, world_size):
    setup(rank, world_size)
    
    # 模型包装
    model = YourModel().cuda(rank)
    model = DDP(model, device_ids=[rank])
    
    # 数据加载器(关键)
    train_dataset = YourDataset()
    train_sampler = DistributedSampler(
        train_dataset,
        num_replicas=world_size,
        rank=rank,
        shuffle=True
    )
    train_loader = DataLoader(
        train_dataset,
        batch_size=32,
        sampler=train_sampler,  # 使用DistributedSampler
        num_workers=4,
        pin_memory=True
    )
    
    optimizer = torch.optim.AdamW(model.parameters(), lr=1e-4)
    
    # 训练循环
    for epoch in range(10):
        train_sampler.set_epoch(epoch)  # 打乱数据
        
        for batch in train_loader:
            inputs, labels = batch
            inputs = inputs.cuda(rank)
            labels = labels.cuda(rank)
            
            outputs = model(inputs)
            loss = criterion(outputs, labels)
            
            optimizer.zero_grad()
            loss.backward()  # 自动梯度同步
            optimizer.step()
        
        # 仅rank 0保存检查点
        if rank == 0:
            torch.save({
                'epoch': epoch,
                'model_state_dict': model.module.state_dict(),
                'optimizer_state_dict': optimizer.state_dict(),
            }, f'checkpoint_epoch_{epoch}.pt')
    
    cleanup()

if __name__ == "__main__":
    world_size = int(os.environ['WORLD_SIZE'])
    rank = int(os.environ['RANK'])
    train(rank, world_size)
```

#### PyTorchJob配置

```yaml
apiVersion: kubeflow.org/v1
kind: PyTorchJob
metadata:
  name: ddp-training
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
              image: pytorch/pytorch:2.1.0-cuda12.1-cudnn8-runtime
              command:
                - torchrun
                - --nproc_per_node=8  # 单节点8卡
                - --nnodes=4          # 4个节点
                - --node_rank=$(RANK)
                - --master_addr=$(MASTER_ADDR)
                - --master_port=23456
                - train.py
              env:
                - name: NCCL_DEBUG
                  value: "INFO"
                - name: NCCL_IB_DISABLE
                  value: "0"  # 启用InfiniBand
              resources:
                limits:
                  nvidia.com/gpu: 8
              volumeMounts:
                - name: training-data
                  mountPath: /data
                - name: checkpoint
                  mountPath: /checkpoint
          volumes:
            - name: training-data
              persistentVolumeClaim:
                claimName: imagenet-pvc
            - name: checkpoint
              persistentVolumeClaim:
                claimName: checkpoint-pvc
    
    Worker:
      replicas: 3
      restartPolicy: OnFailure
      template:
        spec:
          containers:
            - name: pytorch
              image: pytorch/pytorch:2.1.0-cuda12.1-cudnn8-runtime
              command:
                - torchrun
                - --nproc_per_node=8
                - --nnodes=4
                - --node_rank=$(RANK)
                - --master_addr=$(MASTER_ADDR)
                - --master_port=23456
                - train.py
              resources:
                limits:
                  nvidia.com/gpu: 8
              volumeMounts:
                - name: training-data
                  mountPath: /data
                - name: checkpoint
                  mountPath: /checkpoint
          volumes:
            - name: training-data
              persistentVolumeClaim:
                claimName: imagenet-pvc
            - name: checkpoint
              persistentVolumeClaim:
                claimName: checkpoint-pvc
```

---

### 2. FSDP (Fully Sharded Data Parallel)

#### 核心优势

- **显存优化**: 模型参数、梯度、优化器状态全部分片
- **零冗余**: 相比DDP节省显存 8x (8卡场景)
- **通信优化**: AllGather + ReduceScatter

#### FSDP配置

```python
import torch
from torch.distributed.fsdp import (
    FullyShardedDataParallel as FSDP,
    CPUOffload,
    MixedPrecision,
    ShardingStrategy,
)
from torch.distributed.fsdp.wrap import (
    size_based_auto_wrap_policy,
    transformer_auto_wrap_policy,
)

# 混合精度配置
mixed_precision_policy = MixedPrecision(
    param_dtype=torch.float16,
    reduce_dtype=torch.float16,
    buffer_dtype=torch.float16,
)

# 自动包装策略(按层大小)
auto_wrap_policy = size_based_auto_wrap_policy(
    min_num_params=1e8  # 1亿参数以上的层独立分片
)

# 或者按Transformer层包装
from transformers.models.llama.modeling_llama import LlamaDecoderLayer
auto_wrap_policy = partial(
    transformer_auto_wrap_policy,
    transformer_layer_cls={LlamaDecoderLayer},
)

model = YourLargeModel()
model = FSDP(
    model,
    sharding_strategy=ShardingStrategy.FULL_SHARD,  # 全分片
    mixed_precision=mixed_precision_policy,
    auto_wrap_policy=auto_wrap_policy,
    cpu_offload=CPUOffload(offload_params=False),  # CPU offload(可选)
    device_id=torch.cuda.current_device(),
)

# 训练循环与DDP相同
optimizer = torch.optim.AdamW(model.parameters(), lr=1e-4)
```

#### FSDP vs DDP显存对比

| 模型规模 | DDP (8xA100) | FSDP (8xA100) | 节省 |
|---------|-------------|--------------|------|
| **7B** | 56GB | 20GB | 64% |
| **13B** | OOM | 35GB | 可训练 |
| **70B** | OOM | OOM(需CPU offload) | - |
| **70B+CPU Offload** | - | 60GB | 可训练 |

---

## 二、DeepSpeed

### ZeRO优化阶段

| ZeRO阶段 | 分片内容 | 显存节省 | 通信开销 | 适用场景 |
|---------|---------|---------|---------|---------|
| **ZeRO-0** | 无分片 | 1x | 1x | 基准 |
| **ZeRO-1** | 优化器状态 | 4x | 1.5x | <10B模型 |
| **ZeRO-2** | 优化器+梯度 | 8x | 2x | 10-50B模型 |
| **ZeRO-3** | 优化器+梯度+参数 | Nd倍 | 1.5x | 50B+模型 |

---

### DeepSpeed配置

#### ds_config.json

```json
{
  "train_batch_size": 256,
  "train_micro_batch_size_per_gpu": 4,
  "gradient_accumulation_steps": 8,
  
  "optimizer": {
    "type": "AdamW",
    "params": {
      "lr": 1e-5,
      "betas": [0.9, 0.999],
      "eps": 1e-8,
      "weight_decay": 0.01
    }
  },
  
  "scheduler": {
    "type": "WarmupDecayLR",
    "params": {
      "warmup_min_lr": 0,
      "warmup_max_lr": 1e-5,
      "warmup_num_steps": 1000,
      "total_num_steps": 100000
    }
  },
  
  "fp16": {
    "enabled": true,
    "loss_scale": 0,
    "loss_scale_window": 1000,
    "initial_scale_power": 16,
    "hysteresis": 2,
    "min_loss_scale": 1
  },
  
  "zero_optimization": {
    "stage": 3,
    "offload_optimizer": {
      "device": "cpu",
      "pin_memory": true
    },
    "offload_param": {
      "device": "cpu",
      "pin_memory": true
    },
    "overlap_comm": true,
    "contiguous_gradients": true,
    "sub_group_size": 1e9,
    "reduce_bucket_size": 5e8,
    "stage3_prefetch_bucket_size": 5e8,
    "stage3_param_persistence_threshold": 1e6,
    "stage3_max_live_parameters": 1e9,
    "stage3_max_reuse_distance": 1e9,
    "stage3_gather_16bit_weights_on_model_save": true
  },
  
  "gradient_clipping": 1.0,
  "prescale_gradients": false,
  "wall_clock_breakdown": false,
  
  "flops_profiler": {
    "enabled": true,
    "profile_step": 1,
    "module_depth": -1,
    "top_modules": 1,
    "detailed": true
  }
}
```

#### 训练脚本

```python
import deepspeed
import torch
from transformers import AutoModelForCausalLM, AutoTokenizer

# 模型加载
model = AutoModelForCausalLM.from_pretrained("meta-llama/Llama-2-70b-hf")
tokenizer = AutoTokenizer.from_pretrained("meta-llama/Llama-2-70b-hf")

# DeepSpeed初始化
model_engine, optimizer, _, _ = deepspeed.initialize(
    model=model,
    model_parameters=model.parameters(),
    config="ds_config.json"
)

# 训练循环
for step, batch in enumerate(train_dataloader):
    inputs = tokenizer(batch["text"], return_tensors="pt", padding=True)
    inputs = {k: v.to(model_engine.device) for k, v in inputs.items()}
    
    outputs = model_engine(**inputs, labels=inputs["input_ids"])
    loss = outputs.loss
    
    model_engine.backward(loss)
    model_engine.step()
    
    if step % 100 == 0:
        print(f"Step {step}, Loss: {loss.item()}")
    
    # 保存检查点
    if step % 1000 == 0:
        model_engine.save_checkpoint("/checkpoint", tag=f"step_{step}")
```

---

### DeepSpeed on Kubernetes

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: deepspeed-training
  namespace: ai-training
spec:
  parallelism: 4  # 4个节点
  completions: 4
  template:
    spec:
      containers:
        - name: deepspeed
          image: deepspeed/deepspeed:latest
          command:
            - deepspeed
            - --hostfile=/etc/deepspeed/hostfile
            - --num_gpus=8
            - --master_addr=$(MASTER_ADDR)
            - train.py
            - --deepspeed
            - --deepspeed_config=ds_config.json
          env:
            - name: NCCL_IB_DISABLE
              value: "0"
          resources:
            limits:
              nvidia.com/gpu: 8
          volumeMounts:
            - name: training-data
              mountPath: /data
            - name: checkpoint
              mountPath: /checkpoint
            - name: deepspeed-config
              mountPath: /etc/deepspeed
      volumes:
        - name: training-data
          persistentVolumeClaim:
            claimName: training-data-pvc
        - name: checkpoint
          persistentVolumeClaim:
            claimName: checkpoint-pvc
        - name: deepspeed-config
          configMap:
            name: deepspeed-hostfile
```

---

## 三、Megatron-LM (NVIDIA)

### 3D并行配置

```bash
# Megatron-LM训练命令
python pretrain_gpt.py \
  --tensor-model-parallel-size 8 \    # 张量并行度(单节点8卡)
  --pipeline-model-parallel-size 4 \  # 流水线并行度(4个stage)
  --num-layers 96 \                   # 模型层数
  --hidden-size 12288 \               # 隐藏层大小
  --num-attention-heads 96 \          # 注意力头数
  --seq-length 2048 \                 # 序列长度
  --max-position-embeddings 2048 \
  --micro-batch-size 4 \              # 微批次大小
  --global-batch-size 512 \           # 全局批次大小
  --train-iters 500000 \              # 训练步数
  --lr 1.5e-4 \                       # 学习率
  --lr-decay-style cosine \
  --min-lr 1.0e-5 \
  --weight-decay 0.1 \
  --clip-grad 1.0 \
  --fp16 \                            # 混合精度
  --data-path /data/my-dataset \
  --vocab-file /data/vocab.json \
  --merge-file /data/merges.txt \
  --save-interval 10000 \
  --save /checkpoint \
  --load /checkpoint
```

---

## 四、Ray Train (分布式训练编排)

### Ray Cluster on K8s

```yaml
apiVersion: ray.io/v1alpha1
kind: RayCluster
metadata:
  name: ray-cluster
  namespace: ai-training
spec:
  rayVersion: '2.9.0'
  
  # Head节点
  headGroupSpec:
    serviceType: ClusterIP
    rayStartParams:
      dashboard-host: '0.0.0.0'
      num-cpus: '0'  # Head节点不参与计算
    template:
      spec:
        containers:
          - name: ray-head
            image: rayproject/ray:2.9.0-py310-gpu
            ports:
              - containerPort: 6379  # Redis
              - containerPort: 8265  # Dashboard
            resources:
              requests:
                cpu: 4
                memory: 16Gi
              limits:
                cpu: 8
                memory: 32Gi
  
  # Worker节点
  workerGroupSpecs:
    - replicas: 8
      minReplicas: 4
      maxReplicas: 16
      groupName: gpu-workers
      rayStartParams:
        num-gpus: '8'
      template:
        spec:
          containers:
            - name: ray-worker
              image: rayproject/ray:2.9.0-py310-gpu
              resources:
                limits:
                  nvidia.com/gpu: 8
                  cpu: 32
                  memory: 256Gi
              volumeMounts:
                - name: training-data
                  mountPath: /data
          volumes:
            - name: training-data
              persistentVolumeClaim:
                claimName: training-data-pvc
```

### Ray Train训练脚本

```python
import ray
from ray import train
from ray.train.torch import TorchTrainer
from ray.train import ScalingConfig, RunConfig

def train_func(config):
    import torch
    from torch.nn.parallel import DistributedDataParallel as DDP
    
    # Ray自动处理分布式环境
    model = YourModel()
    model = train.torch.prepare_model(model)  # 自动DDP包装
    
    train_dataset = train.torch.prepare_data_loader(
        torch.utils.data.DataLoader(YourDataset(), batch_size=32)
    )
    
    optimizer = torch.optim.AdamW(model.parameters(), lr=1e-4)
    
    for epoch in range(10):
        for batch in train_dataset:
            loss = model(batch)
            optimizer.zero_grad()
            loss.backward()
            optimizer.step()
        
        # 报告指标
        train.report({"loss": loss.item(), "epoch": epoch})

# 配置训练器
trainer = TorchTrainer(
    train_func,
    scaling_config=ScalingConfig(
        num_workers=32,         # 32个worker
        use_gpu=True,
        resources_per_worker={"GPU": 1}
    ),
    run_config=RunConfig(
        name="my-training",
        storage_path="s3://my-bucket/ray_results",
        checkpoint_config=train.CheckpointConfig(
            num_to_keep=3,
            checkpoint_score_attribute="loss",
            checkpoint_score_order="min",
        ),
    ),
)

# 启动训练
result = trainer.fit()
```

---

## 五、通信后端优化

### NCCL配置最佳实践

```bash
# 基础配置
export NCCL_DEBUG=INFO
export NCCL_DEBUG_SUBSYS=ALL

# InfiniBand/RoCE配置
export NCCL_IB_DISABLE=0
export NCCL_IB_HCA=mlx5_0:1,mlx5_1:1,mlx5_2:1,mlx5_3:1
export NCCL_IB_GID_INDEX=3
export NCCL_NET_GDR_LEVEL=5  # GPU Direct RDMA
export NCCL_IB_TC=106        # 流量类别

# P2P通信
export NCCL_P2P_DISABLE=0
export NCCL_P2P_LEVEL=SYS    # NVLink优先

# 网络接口
export NCCL_SOCKET_IFNAME=eth0
export NCCL_IB_TIMEOUT=22

# 性能调优
export NCCL_BUFFSIZE=8388608       # 8MB buffer
export NCCL_NTHREADS=512           # 线程数
export NCCL_NSOCKS_PERTHREAD=8     # 每线程socket数
export NCCL_SOCKET_NTHREADS=8

# 拓扑优化
export NCCL_TOPO_FILE=/etc/nccl_topo.xml
export NCCL_GRAPH_FILE=/etc/nccl_graph.txt
```

### 通信性能测试

```bash
# NCCL Tests
git clone https://github.com/NVIDIA/nccl-tests.git
cd nccl-tests
make

# AllReduce测试(模拟梯度同步)
./build/all_reduce_perf -b 8 -e 128M -f 2 -g 8

# 输出示例:
# #    bytes   #iters  time(us)  algbw(GB/s)  busbw(GB/s)
#   8388608      100    1243.2       6.75       11.81
#  16777216      100    2198.4       7.63       13.35
#  33554432      100    4102.7       8.18       14.31
```

---

## 六、框架选型决策

### 训练规模推荐

| 模型规模 | GPU数量 | 推荐方案 | 配置要点 |
|---------|---------|---------|---------|
| **<1B** | 1-8 | PyTorch DDP | 标准数据并行 |
| **1B-10B** | 8-64 | PyTorch FSDP | ZeRO-2等价 |
| **10B-100B** | 64-512 | DeepSpeed ZeRO-3 | CPU offload |
| **100B+** | 512+ | Megatron-LM 3D | TP+PP+DP |

### 框架特性对比

| 特性 | PyTorch DDP | FSDP | DeepSpeed | Megatron |
|------|------------|------|-----------|----------|
| **易用性** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐ |
| **显存优化** | ⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **通信效率** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **模型规模** | <10B | <50B | <200B | 1T+ |
| **生态成熟度** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ |

---

## 七、生产最佳实践

### 训练稳定性

- ✅ 启用梯度裁剪(gradient clipping)
- ✅ 配置自动重启策略
- ✅ 频繁保存检查点(每N步)
- ✅ 监控GPU温度和ECC错误
- ✅ 配置OOM重试机制

### 性能优化

- ✅ 使用混合精度训练(FP16/BF16)
- ✅ 启用梯度累积(gradient accumulation)
- ✅ 优化DataLoader(num_workers/pin_memory)
- ✅ 启用编译优化(torch.compile)
- ✅ 使用Flash Attention 2

### 成本优化

- ✅ Spot实例+检查点容错
- ✅ 混合使用多代GPU(A100+V100)
- ✅ 动态调整batch size
- ✅ 数据预处理离线化
- ✅ 模型并行度自动调优

---

**表格维护**: Kusheet Project | **作者**: Allen Galler (allengaller@gmail.com)
