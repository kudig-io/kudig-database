# 143 - LLM微调技术与实践 (LLM Fine-tuning Techniques & Practices)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-01 | **参考**: [PEFT](https://huggingface.co/docs/peft/), [TRL](https://huggingface.co/docs/trl/)

---

## 一、微调技术全景 (Fine-tuning Landscape)

### 1.1 微调方法分类

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      LLM 微调技术全景                                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                    全参数微调 (Full Fine-tuning)                       │ │
│  │  ┌─────────────────────────────────────────────────────────────────┐ │ │
│  │  │  更新所有参数 | 显存需求极高 | 效果最佳 | 适合领域垂直化           │ │ │
│  │  │  典型场景: 预训练续训、领域适配、多语言扩展                       │ │ │
│  │  └─────────────────────────────────────────────────────────────────┘ │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                                                             │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                    参数高效微调 (PEFT)                                 │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐               │ │
│  │  │    LoRA      │  │   QLoRA      │  │   Adapter    │               │ │
│  │  │  低秩分解    │  │ 量化+LoRA    │  │  适配器层    │               │ │
│  │  │  0.1%参数    │  │  4-bit量化   │  │  插入模块    │               │ │
│  │  └──────────────┘  └──────────────┘  └──────────────┘               │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐               │ │
│  │  │  IA³         │  │  Prefix      │  │  P-Tuning    │               │ │
│  │  │  激活缩放    │  │  前缀微调    │  │  软提示词    │               │ │
│  │  │  0.01%参数   │  │  虚拟token   │  │  可学习嵌入  │               │ │
│  │  └──────────────┘  └──────────────┘  └──────────────┘               │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                                                             │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                    对齐微调 (Alignment)                                │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐               │ │
│  │  │    SFT       │  │    RLHF      │  │    DPO       │               │ │
│  │  │  监督微调    │  │ 人类反馈强化  │  │ 直接偏好优化  │               │ │
│  │  │  指令-响应   │  │  奖励模型    │  │  无需RM      │               │ │
│  │  └──────────────┘  └──────────────┘  └──────────────┘               │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐               │ │
│  │  │    PPO       │  │    ORPO      │  │    KTO       │               │ │
│  │  │  近端策略    │  │ 奇异比偏好   │  │ Kahneman-T   │               │ │
│  │  │  复杂但稳定  │  │  无需参考模型│  │  单边数据    │               │ │
│  │  └──────────────┘  └──────────────┘  └──────────────┘               │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 微调方法对比

| 方法 | 可训练参数 | 显存需求(7B) | 训练速度 | 效果 | 适用场景 |
|-----|-----------|-------------|---------|------|---------|
| **Full FT** | 100% | 112GB+ | 慢 | 最佳 | 领域垂直化 |
| **LoRA** | 0.1-1% | 16-24GB | 快 | 优秀 | 通用指令微调 |
| **QLoRA** | 0.1-1% | 6-12GB | 中 | 优秀 | 资源受限场景 |
| **Adapter** | 1-5% | 20-30GB | 快 | 良好 | 多任务适配 |
| **IA³** | 0.01% | 14-16GB | 极快 | 良好 | 轻量级适配 |
| **Prefix Tuning** | <0.1% | 14-16GB | 极快 | 中等 | Few-shot增强 |
| **P-Tuning v2** | <0.1% | 14-16GB | 极快 | 良好 | NLU任务 |

### 1.3 显存估算公式

| 组件 | 计算公式 | 7B模型估算 |
|-----|---------|-----------|
| **模型权重** | 参数量 × 精度字节 | 7B × 2B = 14GB (FP16) |
| **梯度** | 参数量 × 4B | 7B × 4B = 28GB |
| **优化器状态** | 参数量 × 8B (AdamW) | 7B × 8B = 56GB |
| **激活值** | batch × seq × hidden × layers | ~10-20GB |
| **LoRA显存** | 基础模型 + rank × hidden × 2 | 14GB + 1GB |

---

## 二、LoRA微调详解 (LoRA Fine-tuning)

### 2.1 LoRA原理

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         LoRA 低秩分解原理                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  原始权重矩阵 W (d × k)                                                     │
│  ┌───────────────────────────────┐                                         │
│  │                               │                                         │
│  │           W₀ (冻结)           │                                         │
│  │         (d × k)               │                                         │
│  │                               │                                         │
│  └───────────────────────────────┘                                         │
│                  +                                                          │
│  ┌───────────────────────────────┐                                         │
│  │  LoRA 增量: ΔW = B × A        │                                         │
│  │  ┌─────┐     ┌───────────┐   │                                         │
│  │  │  B  │  ×  │     A     │   │                                         │
│  │  │(d×r)│     │   (r×k)   │   │                                         │
│  │  └─────┘     └───────────┘   │                                         │
│  │                               │                                         │
│  │  r << min(d, k), 如 r=8      │                                         │
│  │  可训练参数: r×(d+k)          │                                         │
│  └───────────────────────────────┘                                         │
│                                                                             │
│  前向计算: h = W₀x + ΔWx = W₀x + BAx                                       │
│  缩放因子: h = W₀x + (α/r) × BAx                                           │
│                                                                             │
│  参数量对比 (Llama-7B, Attention):                                          │
│  - 原始: 4096 × 4096 = 16.8M/层                                            │
│  - LoRA(r=8): 8 × (4096 + 4096) = 65K/层 (节省99.6%)                       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 LoRA配置最佳实践

```yaml
# LoRA微调Kubernetes Job
apiVersion: batch/v1
kind: Job
metadata:
  name: llama2-7b-lora-finetune
  namespace: ml-training
spec:
  backoffLimit: 3
  template:
    metadata:
      labels:
        app: lora-training
    spec:
      restartPolicy: OnFailure
      
      nodeSelector:
        nvidia.com/gpu.product: "NVIDIA-A100-SXM4-40GB"
      tolerations:
      - key: "nvidia.com/gpu"
        operator: "Exists"
        effect: "NoSchedule"
        
      containers:
      - name: trainer
        image: huggingface/transformers-pytorch-gpu:latest
        
        command: ["python", "-m", "torch.distributed.launch"]
        args:
        - "--nproc_per_node=4"
        - "train_lora.py"
        - "--model_name_or_path=meta-llama/Llama-2-7b-hf"
        - "--dataset_path=/data/training_data"
        - "--output_dir=/output/llama2-7b-lora"
        - "--lora_r=16"
        - "--lora_alpha=32"
        - "--lora_dropout=0.05"
        - "--target_modules=q_proj,k_proj,v_proj,o_proj,gate_proj,up_proj,down_proj"
        - "--per_device_train_batch_size=4"
        - "--gradient_accumulation_steps=4"
        - "--learning_rate=2e-4"
        - "--num_train_epochs=3"
        - "--warmup_ratio=0.03"
        - "--lr_scheduler_type=cosine"
        - "--bf16"
        - "--gradient_checkpointing"
        - "--logging_steps=10"
        - "--save_steps=500"
        - "--save_total_limit=3"
        - "--report_to=wandb"
        
        resources:
          requests:
            cpu: "32"
            memory: "128Gi"
            nvidia.com/gpu: "4"
          limits:
            cpu: "64"
            memory: "256Gi"
            nvidia.com/gpu: "4"
            
        env:
        - name: HF_TOKEN
          valueFrom:
            secretKeyRef:
              name: hf-token
              key: token
        - name: WANDB_API_KEY
          valueFrom:
            secretKeyRef:
              name: wandb-secret
              key: api-key
        - name: WANDB_PROJECT
          value: "llm-finetuning"
        - name: CUDA_VISIBLE_DEVICES
          value: "0,1,2,3"
          
        volumeMounts:
        - name: training-data
          mountPath: /data
        - name: output
          mountPath: /output
        - name: shm
          mountPath: /dev/shm
        - name: hf-cache
          mountPath: /root/.cache/huggingface
          
      volumes:
      - name: training-data
        persistentVolumeClaim:
          claimName: training-data-pvc
      - name: output
        persistentVolumeClaim:
          claimName: model-output-pvc
      - name: shm
        emptyDir:
          medium: Memory
          sizeLimit: "32Gi"
      - name: hf-cache
        persistentVolumeClaim:
          claimName: hf-cache-pvc
```

### 2.3 LoRA训练脚本

```python
# train_lora.py
import torch
from transformers import (
    AutoModelForCausalLM,
    AutoTokenizer,
    TrainingArguments,
    Trainer,
    DataCollatorForLanguageModeling
)
from peft import (
    LoraConfig,
    get_peft_model,
    prepare_model_for_kbit_training,
    TaskType
)
from datasets import load_from_disk
import argparse

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--model_name_or_path", type=str, required=True)
    parser.add_argument("--dataset_path", type=str, required=True)
    parser.add_argument("--output_dir", type=str, required=True)
    parser.add_argument("--lora_r", type=int, default=16)
    parser.add_argument("--lora_alpha", type=int, default=32)
    parser.add_argument("--lora_dropout", type=float, default=0.05)
    parser.add_argument("--target_modules", type=str, default="q_proj,v_proj")
    parser.add_argument("--per_device_train_batch_size", type=int, default=4)
    parser.add_argument("--gradient_accumulation_steps", type=int, default=4)
    parser.add_argument("--learning_rate", type=float, default=2e-4)
    parser.add_argument("--num_train_epochs", type=int, default=3)
    parser.add_argument("--warmup_ratio", type=float, default=0.03)
    parser.add_argument("--lr_scheduler_type", type=str, default="cosine")
    parser.add_argument("--bf16", action="store_true")
    parser.add_argument("--gradient_checkpointing", action="store_true")
    parser.add_argument("--logging_steps", type=int, default=10)
    parser.add_argument("--save_steps", type=int, default=500)
    parser.add_argument("--save_total_limit", type=int, default=3)
    parser.add_argument("--report_to", type=str, default="wandb")
    args = parser.parse_args()

    # 加载tokenizer
    tokenizer = AutoTokenizer.from_pretrained(args.model_name_or_path)
    tokenizer.pad_token = tokenizer.eos_token
    tokenizer.padding_side = "right"

    # 加载模型
    model = AutoModelForCausalLM.from_pretrained(
        args.model_name_or_path,
        torch_dtype=torch.bfloat16 if args.bf16 else torch.float16,
        device_map="auto",
        trust_remote_code=True
    )

    # 启用梯度检查点
    if args.gradient_checkpointing:
        model.gradient_checkpointing_enable()
        model.enable_input_require_grads()

    # LoRA配置
    lora_config = LoraConfig(
        r=args.lora_r,
        lora_alpha=args.lora_alpha,
        lora_dropout=args.lora_dropout,
        target_modules=args.target_modules.split(","),
        bias="none",
        task_type=TaskType.CAUSAL_LM
    )

    # 应用LoRA
    model = get_peft_model(model, lora_config)
    model.print_trainable_parameters()

    # 加载数据集
    dataset = load_from_disk(args.dataset_path)

    # 训练参数
    training_args = TrainingArguments(
        output_dir=args.output_dir,
        per_device_train_batch_size=args.per_device_train_batch_size,
        gradient_accumulation_steps=args.gradient_accumulation_steps,
        learning_rate=args.learning_rate,
        num_train_epochs=args.num_train_epochs,
        warmup_ratio=args.warmup_ratio,
        lr_scheduler_type=args.lr_scheduler_type,
        bf16=args.bf16,
        logging_steps=args.logging_steps,
        save_steps=args.save_steps,
        save_total_limit=args.save_total_limit,
        report_to=args.report_to,
        ddp_find_unused_parameters=False,
        group_by_length=True,
        dataloader_num_workers=4,
        dataloader_pin_memory=True
    )

    # 数据整理器
    data_collator = DataCollatorForLanguageModeling(
        tokenizer=tokenizer,
        mlm=False
    )

    # Trainer
    trainer = Trainer(
        model=model,
        args=training_args,
        train_dataset=dataset["train"],
        eval_dataset=dataset.get("validation"),
        data_collator=data_collator
    )

    # 开始训练
    trainer.train()

    # 保存模型
    trainer.save_model()
    tokenizer.save_pretrained(args.output_dir)

if __name__ == "__main__":
    main()
```

---

## 三、QLoRA量化微调 (QLoRA)

### 3.1 QLoRA配置

```yaml
# QLoRA微调配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: qlora-config
data:
  qlora_config.yaml: |
    # 量化配置
    quantization:
      load_in_4bit: true
      bnb_4bit_compute_dtype: bfloat16
      bnb_4bit_use_double_quant: true
      bnb_4bit_quant_type: nf4
      
    # LoRA配置
    lora:
      r: 64
      lora_alpha: 16
      lora_dropout: 0.1
      target_modules:
        - q_proj
        - k_proj
        - v_proj
        - o_proj
        - gate_proj
        - up_proj
        - down_proj
      bias: none
      task_type: CAUSAL_LM
      
    # 训练配置
    training:
      per_device_train_batch_size: 1
      gradient_accumulation_steps: 16
      learning_rate: 2e-4
      max_steps: 10000
      warmup_steps: 100
      fp16: false
      bf16: true
      optim: paged_adamw_32bit
      gradient_checkpointing: true
      max_grad_norm: 0.3
      
---
apiVersion: batch/v1
kind: Job
metadata:
  name: llama2-70b-qlora
  namespace: ml-training
spec:
  template:
    spec:
      containers:
      - name: trainer
        image: ml-platform/qlora-trainer:latest
        command: ["python", "train_qlora.py"]
        args:
        - "--model_name=meta-llama/Llama-2-70b-hf"
        - "--dataset=/data/alpaca"
        - "--output_dir=/output/llama2-70b-qlora"
        - "--config=/config/qlora_config.yaml"
        
        resources:
          limits:
            nvidia.com/gpu: 1        # 单卡即可微调70B
            memory: "48Gi"
            
        volumeMounts:
        - name: config
          mountPath: /config
          
      volumes:
      - name: config
        configMap:
          name: qlora-config
```

### 3.2 QLoRA训练脚本

```python
# train_qlora.py
import torch
from transformers import (
    AutoModelForCausalLM,
    AutoTokenizer,
    BitsAndBytesConfig,
    TrainingArguments
)
from peft import (
    LoraConfig,
    get_peft_model,
    prepare_model_for_kbit_training
)
from trl import SFTTrainer
from datasets import load_from_disk
import yaml

def load_config(config_path):
    with open(config_path, 'r') as f:
        return yaml.safe_load(f)

def main(args):
    config = load_config(args.config)
    
    # 4bit量化配置
    bnb_config = BitsAndBytesConfig(
        load_in_4bit=config['quantization']['load_in_4bit'],
        bnb_4bit_compute_dtype=getattr(
            torch, 
            config['quantization']['bnb_4bit_compute_dtype']
        ),
        bnb_4bit_use_double_quant=config['quantization']['bnb_4bit_use_double_quant'],
        bnb_4bit_quant_type=config['quantization']['bnb_4bit_quant_type']
    )
    
    # 加载量化模型
    model = AutoModelForCausalLM.from_pretrained(
        args.model_name,
        quantization_config=bnb_config,
        device_map="auto",
        trust_remote_code=True
    )
    
    # 准备量化训练
    model = prepare_model_for_kbit_training(model)
    
    # LoRA配置
    lora_config = LoraConfig(
        r=config['lora']['r'],
        lora_alpha=config['lora']['lora_alpha'],
        lora_dropout=config['lora']['lora_dropout'],
        target_modules=config['lora']['target_modules'],
        bias=config['lora']['bias'],
        task_type=config['lora']['task_type']
    )
    
    model = get_peft_model(model, lora_config)
    model.print_trainable_parameters()
    
    # 加载tokenizer和数据
    tokenizer = AutoTokenizer.from_pretrained(args.model_name)
    tokenizer.pad_token = tokenizer.eos_token
    
    dataset = load_from_disk(args.dataset)
    
    # 训练参数
    training_args = TrainingArguments(
        output_dir=args.output_dir,
        per_device_train_batch_size=config['training']['per_device_train_batch_size'],
        gradient_accumulation_steps=config['training']['gradient_accumulation_steps'],
        learning_rate=config['training']['learning_rate'],
        max_steps=config['training']['max_steps'],
        warmup_steps=config['training']['warmup_steps'],
        bf16=config['training']['bf16'],
        optim=config['training']['optim'],
        gradient_checkpointing=config['training']['gradient_checkpointing'],
        max_grad_norm=config['training']['max_grad_norm'],
        logging_steps=10,
        save_steps=100,
        report_to="wandb"
    )
    
    # SFT Trainer
    trainer = SFTTrainer(
        model=model,
        train_dataset=dataset["train"],
        tokenizer=tokenizer,
        args=training_args,
        dataset_text_field="text",
        max_seq_length=2048,
        packing=True
    )
    
    trainer.train()
    trainer.save_model()

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--model_name", type=str, required=True)
    parser.add_argument("--dataset", type=str, required=True)
    parser.add_argument("--output_dir", type=str, required=True)
    parser.add_argument("--config", type=str, required=True)
    args = parser.parse_args()
    main(args)
```

---

## 四、对齐微调 (Alignment Fine-tuning)

### 4.1 SFT监督微调

```yaml
# SFT训练Job
apiVersion: "kubeflow.org/v1"
kind: PyTorchJob
metadata:
  name: llama2-sft
  namespace: ml-training
spec:
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      template:
        spec:
          containers:
          - name: pytorch
            image: ml-platform/sft-trainer:latest
            command: ["accelerate", "launch"]
            args:
            - "--config_file=/config/accelerate_config.yaml"
            - "train_sft.py"
            - "--model_name=meta-llama/Llama-2-7b-hf"
            - "--dataset_name=/data/sft_dataset"
            - "--output_dir=/output/llama2-sft"
            - "--num_train_epochs=3"
            - "--per_device_train_batch_size=4"
            - "--learning_rate=2e-5"
            resources:
              limits:
                nvidia.com/gpu: 8
                
    Worker:
      replicas: 3
      template:
        spec:
          containers:
          - name: pytorch
            image: ml-platform/sft-trainer:latest
            resources:
              limits:
                nvidia.com/gpu: 8
```

### 4.2 DPO直接偏好优化

```python
# DPO训练配置
from trl import DPOTrainer, DPOConfig
from transformers import AutoModelForCausalLM, AutoTokenizer
from peft import LoraConfig
from datasets import load_from_disk

def train_dpo():
    # 加载SFT模型
    model = AutoModelForCausalLM.from_pretrained(
        "llama2-7b-sft",
        torch_dtype=torch.bfloat16,
        device_map="auto"
    )
    
    # 参考模型 (冻结的SFT模型)
    ref_model = AutoModelForCausalLM.from_pretrained(
        "llama2-7b-sft",
        torch_dtype=torch.bfloat16,
        device_map="auto"
    )
    
    tokenizer = AutoTokenizer.from_pretrained("llama2-7b-sft")
    tokenizer.pad_token = tokenizer.eos_token
    
    # LoRA配置 (可选)
    peft_config = LoraConfig(
        r=16,
        lora_alpha=32,
        lora_dropout=0.05,
        target_modules=["q_proj", "v_proj", "k_proj", "o_proj"],
        bias="none",
        task_type="CAUSAL_LM"
    )
    
    # DPO配置
    dpo_config = DPOConfig(
        output_dir="llama2-7b-dpo",
        beta=0.1,                      # KL散度系数
        loss_type="sigmoid",           # sigmoid/hinge/ipo
        per_device_train_batch_size=4,
        gradient_accumulation_steps=4,
        learning_rate=5e-7,
        num_train_epochs=1,
        warmup_ratio=0.1,
        bf16=True,
        logging_steps=10,
        save_steps=100,
        gradient_checkpointing=True,
        max_length=1024,
        max_prompt_length=512,
        report_to="wandb"
    )
    
    # 加载偏好数据集
    # 格式: {"prompt": str, "chosen": str, "rejected": str}
    dataset = load_from_disk("/data/dpo_dataset")
    
    # DPO Trainer
    trainer = DPOTrainer(
        model=model,
        ref_model=ref_model,
        args=dpo_config,
        train_dataset=dataset["train"],
        eval_dataset=dataset["validation"],
        tokenizer=tokenizer,
        peft_config=peft_config
    )
    
    trainer.train()
    trainer.save_model()

if __name__ == "__main__":
    train_dpo()
```

### 4.3 RLHF训练Pipeline

```yaml
# RLHF三阶段训练Pipeline
apiVersion: argoproj.io/v1alpha1
kind: Workflow
metadata:
  name: rlhf-pipeline
  namespace: ml-training
spec:
  entrypoint: rlhf-training
  
  templates:
  - name: rlhf-training
    dag:
      tasks:
      # Stage 1: SFT
      - name: sft-training
        template: sft
        
      # Stage 2: 奖励模型训练
      - name: reward-model-training
        template: reward-model
        dependencies: [sft-training]
        
      # Stage 3: PPO训练
      - name: ppo-training
        template: ppo
        dependencies: [reward-model-training]
        
  - name: sft
    container:
      image: ml-platform/sft-trainer:latest
      command: ["python", "train_sft.py"]
      resources:
        limits:
          nvidia.com/gpu: 8
          
  - name: reward-model
    container:
      image: ml-platform/reward-trainer:latest
      command: ["python", "train_reward.py"]
      args:
      - "--base_model={{tasks.sft-training.outputs.result}}"
      resources:
        limits:
          nvidia.com/gpu: 4
          
  - name: ppo
    container:
      image: ml-platform/ppo-trainer:latest
      command: ["python", "train_ppo.py"]
      args:
      - "--sft_model={{tasks.sft-training.outputs.result}}"
      - "--reward_model={{tasks.reward-model-training.outputs.result}}"
      resources:
        limits:
          nvidia.com/gpu: 8
```

---

## 五、分布式微调 (Distributed Fine-tuning)

### 5.1 DeepSpeed ZeRO配置

```yaml
# DeepSpeed配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: deepspeed-config
data:
  ds_config_zero3.json: |
    {
      "train_batch_size": "auto",
      "train_micro_batch_size_per_gpu": "auto",
      "gradient_accumulation_steps": "auto",
      
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
        "reduce_bucket_size": "auto",
        "stage3_prefetch_bucket_size": "auto",
        "stage3_param_persistence_threshold": "auto",
        "sub_group_size": 1e9,
        "stage3_max_live_parameters": 1e9,
        "stage3_max_reuse_distance": 1e9,
        "stage3_gather_16bit_weights_on_model_save": true
      },
      
      "bf16": {
        "enabled": true
      },
      
      "gradient_clipping": 1.0,
      
      "optimizer": {
        "type": "AdamW",
        "params": {
          "lr": "auto",
          "betas": [0.9, 0.999],
          "eps": 1e-8,
          "weight_decay": "auto"
        }
      },
      
      "scheduler": {
        "type": "WarmupDecayLR",
        "params": {
          "warmup_min_lr": 0,
          "warmup_max_lr": "auto",
          "warmup_num_steps": "auto",
          "total_num_steps": "auto"
        }
      },
      
      "activation_checkpointing": {
        "partition_activations": true,
        "cpu_checkpointing": true,
        "contiguous_memory_optimization": true
      },
      
      "wall_clock_breakdown": false
    }
```

### 5.2 多节点训练

```yaml
# PyTorchJob多节点微调
apiVersion: "kubeflow.org/v1"
kind: PyTorchJob
metadata:
  name: llama2-70b-finetune
  namespace: ml-training
spec:
  elasticPolicy:
    rdzvBackend: c10d
    minReplicas: 4
    maxReplicas: 8
    
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      template:
        spec:
          containers:
          - name: pytorch
            image: nvcr.io/nvidia/pytorch:24.01-py3
            command: ["deepspeed"]
            args:
            - "--num_gpus=8"
            - "--num_nodes=$(WORLD_SIZE)"
            - "--hostfile=/etc/mpi/hostfile"
            - "train.py"
            - "--model_name=meta-llama/Llama-2-70b-hf"
            - "--deepspeed=/config/ds_config_zero3.json"
            - "--bf16"
            - "--gradient_checkpointing"
            
            resources:
              limits:
                nvidia.com/gpu: 8
                rdma/rdma_shared_device_a: 1
                
            env:
            - name: NCCL_IB_DISABLE
              value: "0"
            - name: NCCL_NET_GDR_LEVEL
              value: "5"
              
    Worker:
      replicas: 3
      template:
        spec:
          containers:
          - name: pytorch
            image: nvcr.io/nvidia/pytorch:24.01-py3
            resources:
              limits:
                nvidia.com/gpu: 8
                rdma/rdma_shared_device_a: 1
```

---

## 六、微调监控与评估 (Monitoring & Evaluation)

### 6.1 训练监控指标

| 指标 | 说明 | 告警阈值 |
|-----|------|---------|
| **train_loss** | 训练损失 | 停滞>1000步 |
| **eval_loss** | 验证损失 | 持续上升 |
| **learning_rate** | 学习率 | 异常归零 |
| **grad_norm** | 梯度范数 | > 10 (梯度爆炸) |
| **gpu_util** | GPU利用率 | < 50% |
| **gpu_memory** | 显存使用 | > 95% |
| **throughput** | 样本/秒 | 下降>20% |

### 6.2 评估Pipeline

```yaml
# 模型评估Job
apiVersion: batch/v1
kind: Job
metadata:
  name: model-evaluation
  namespace: ml-training
spec:
  template:
    spec:
      containers:
      - name: evaluator
        image: ml-platform/lm-evaluation:latest
        command: ["python", "-m", "lm_eval"]
        args:
        - "--model=hf"
        - "--model_args=pretrained=/models/llama2-7b-finetuned"
        - "--tasks=hellaswag,arc_challenge,winogrande,mmlu"
        - "--batch_size=auto"
        - "--output_path=/output/eval_results.json"
        
        resources:
          limits:
            nvidia.com/gpu: 1
            
        volumeMounts:
        - name: models
          mountPath: /models
        - name: output
          mountPath: /output
          
      volumes:
      - name: models
        persistentVolumeClaim:
          claimName: model-pvc
      - name: output
        persistentVolumeClaim:
          claimName: eval-output-pvc
```

---

## 七、成本优化 (Cost Optimization)

### 7.1 成本对比

| 配置 | GPU | 7B模型训练成本 | 70B模型训练成本 |
|-----|-----|---------------|----------------|
| **Full FT** | 2×A100 80GB | $200/天 | $1,600/天 |
| **LoRA** | 1×A100 40GB | $50/天 | $400/天 |
| **QLoRA** | 1×A10G 24GB | $8/天 | $64/天 |
| **Spot + QLoRA** | 1×A10G Spot | $2.4/天 | $19/天 |

### 7.2 Spot实例策略

```yaml
# Spot实例微调配置
apiVersion: karpenter.sh/v1alpha5
kind: Provisioner
metadata:
  name: finetuning-spot
spec:
  requirements:
  - key: "karpenter.sh/capacity-type"
    operator: In
    values: ["spot"]
  - key: "node.kubernetes.io/instance-type"
    operator: In
    values: ["g5.2xlarge", "g5.4xlarge", "p3.2xlarge"]
    
  limits:
    resources:
      nvidia.com/gpu: 32
      
  ttlSecondsAfterEmpty: 30
  
---
# 训练中断恢复
apiVersion: batch/v1
kind: Job
metadata:
  name: finetuning-with-checkpoint
spec:
  backoffLimit: 100  # 允许多次重试
  template:
    spec:
      containers:
      - name: trainer
        image: ml-platform/trainer:latest
        args:
        - "--resume_from_checkpoint=auto"  # 自动从最新checkpoint恢复
        - "--save_steps=100"                # 频繁保存
        
        volumeMounts:
        - name: checkpoints
          mountPath: /checkpoints
          
      volumes:
      - name: checkpoints
        persistentVolumeClaim:
          claimName: checkpoint-pvc  # 持久化checkpoint
```

---

## 八、快速参考 (Quick Reference)

### 8.1 LoRA参数选择

| 参数 | 小模型(<7B) | 中模型(7-13B) | 大模型(>30B) |
|-----|------------|--------------|-------------|
| **r (rank)** | 8-16 | 16-32 | 32-64 |
| **alpha** | 16-32 | 32-64 | 64-128 |
| **dropout** | 0.05-0.1 | 0.05-0.1 | 0.05 |
| **target_modules** | q,v | q,k,v,o | all linear |
| **学习率** | 1e-4 - 3e-4 | 1e-4 - 2e-4 | 5e-5 - 1e-4 |

### 8.2 常用命令

```bash
# 查看训练状态
kubectl logs -f job/lora-training -n ml-training

# 查看GPU使用
kubectl exec -it <pod> -- nvidia-smi

# 加载LoRA权重
from peft import PeftModel
model = PeftModel.from_pretrained(base_model, "path/to/lora")

# 合并LoRA到基础模型
merged_model = model.merge_and_unload()
merged_model.save_pretrained("merged_model")
```

---

**微调最佳实践**: 从QLoRA开始 → 验证效果后升级LoRA → 必要时Full FT → 持续评估

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
