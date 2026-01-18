# 57 - LLM微调技术与实践

> **适用版本**: v1.25 - v1.32 | **参考**: [PEFT](https://huggingface.co/docs/peft/)

## 一、微调技术对比

| 技术 | 全称 | 显存需求 | 训练速度 | 适用场景 |
|-----|------|---------|---------|---------|
| **Full FT** | Full Fine-tuning | 极高 (模型×7) | 慢 | 领域垂直化 |
| **LoRA** | Low-Rank Adaptation | 低 (0.1%参数) | 快 | 通用指令微调 |
| **QLoRA** | Quantized LoRA | 极低 (4-bit) | 中 | 消费级GPU |
| **Adapter** | Adapter Tuning | 低 | 快 | 多任务适配 |
| **P-Tuning** | Prompt Tuning | 极低 | 极快 | Few-shot场景 |

## 二、显存估算 (Llama2-7B)

| 方法 | 显存 | GPU配置 | 成本/小时 |
|-----|------|---------|-----------|
| Full FT | 112GB | 2×A100 80GB | $65 |
| LoRA | 24GB | 1×A10G | $1 |
| QLoRA | 12GB | 1×RTX 4090 | $0.5 |

## 三、LoRA配置示例

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: lora-config
data:
  lora_config.yaml: |
    lora_r: 8
    lora_alpha: 16
    lora_dropout: 0.05
    target_modules:
      - q_proj
      - v_proj
      - k_proj
      - o_proj
---
apiVersion: batch/v1
kind: Job
metadata:
  name: llama2-lora-finetuning
spec:
  template:
    spec:
      containers:
      - name: trainer
        image: huggingface/transformers-pytorch-gpu
        command:
        - python
        - train_lora.py
        resources:
          limits:
            nvidia.com/gpu: 1
```

## 四、性能优化

| 优化技术 | 效果 | 实现 |
|---------|------|------|
| **Flash Attention 2** | 速度↑2倍 | `pip install flash-attn` |
| **梯度累积** | 显存↓50% | `gradient_accumulation_steps=4` |
| **混合精度** | 显存↓40% | `fp16=True` |
| **DeepSpeed ZeRO** | 显存↓75% | `deepspeed_config.json` |

## 五、训练监控

```python
import wandb
from transformers import TrainingArguments

args = TrainingArguments(
    output_dir="./output",
    learning_rate=3e-4,
    per_device_train_batch_size=4,
    num_train_epochs=3,
    report_to="wandb",
    logging_steps=10
)
```

## 六、成本优化

**Spot实例策略:**
- 训练任务: 100% Spot (节省70%)
- Checkpoint: 每500步保存到S3
- 中断恢复: 自动从最新checkpoint恢复

**示例:**
- On-Demand: $1,000 (100小时 × $10/小时)
- Spot: $300 (节省70%)

## 七、最佳实践

1. **数据集大小**: LoRA最少1000条，Full FT需10万+
2. **学习率**: LoRA用3e-4，Full FT用1e-5
3. **Epoch数**: 通常3-5个epoch足够
4. **验证集**: 保留10-20%用于验证
5. **Early Stopping**: 防止过拟合

---
**相关**: [112-分布式训练](../112-distributed-training-frameworks.md) | **版本**: PEFT 0.7.0+
