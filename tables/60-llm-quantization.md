# 60 - LLM模型量化技术

> **适用版本**: v1.25 - v1.32 | **参考**: [GPTQ](https://github.com/IST-DASLab/gptq)

## 一、量化技术对比

| 技术 | 精度 | 压缩比 | 速度 | 精度损失 | 适用场景 |
|-----|------|-------|------|---------|---------|
| **FP16** | 16-bit | 2x | 1.5x | <0.1% | 标准训练/推理 |
| **INT8** | 8-bit | 4x | 2x | <1% | 生产推理 |
| **GPTQ** | 4-bit | 8x | 1.8x | 1-3% | 大模型推理 |
| **AWQ** | 4-bit | 8x | 2x | <2% | 低延迟推理 |
| **GGUF** | 2-8bit | 4-16x | 变化 | 2-5% | CPU/边缘 |

## 二、显存对比 (Llama2-7B)

| 精度 | 模型大小 | 显存需求 | GPU配置 | 成本/小时 |
|-----|---------|---------|---------|-----------|
| FP32 | 28GB | 32GB | A100 40GB | $4 |
| FP16 | 14GB | 18GB | A10G 24GB | $1 |
| INT8 | 7GB | 10GB | T4 16GB | $0.5 |
| INT4 | 3.5GB | 6GB | RTX 4090 | $0.3 |

## 三、量化部署

### GPTQ量化
```python
from transformers import AutoModelForCausalLM
from auto_gptq import AutoGPTQForCausalLM

# 量化模型
model = AutoGPTQForCausalLM.from_pretrained(
    "meta-llama/Llama-2-7b-hf",
    quantize_config={
        "bits": 4,
        "group_size": 128,
        "desc_act": True
    }
)

# 保存
model.save_quantized("llama2-7b-gptq")
```

### TGI部署
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: tgi-llama2-gptq
spec:
  template:
    spec:
      containers:
      - name: tgi
        image: ghcr.io/huggingface/text-generation-inference:1.4.0
        args:
        - --model-id=TheBloke/Llama-2-7B-GPTQ
        - --quantize=gptq
        - --max-batch-size=128
        resources:
          limits:
            nvidia.com/gpu: 1
```

## 四、性能基准

### Llama2-7B 推理性能
| 配置 | 吞吐量 | P99延迟 | 显存 |
|-----|--------|---------|------|
| FP16 | 30 tok/s | 1200ms | 18GB |
| INT8 | 28 tok/s | 1100ms | 10GB |
| GPTQ-4bit | 25 tok/s | 1300ms | 6GB |
| AWQ-4bit | 27 tok/s | 1150ms | 6GB |

## 五、量化工具链

| 工具 | 支持格式 | 特点 | 使用场景 |
|-----|---------|------|---------|
| **AutoGPTQ** | GPTQ | 快速量化 | HuggingFace模型 |
| **llama.cpp** | GGUF | CPU优化 | 边缘部署 |
| **AWQ** | AWQ | 低损失 | 生产推理 |
| **bitsandbytes** | NF4 | 简单易用 | 快速测试 |

## 六、最佳实践

1. **选择策略**:
   - 生产推理: INT8或GPTQ
   - 边缘设备: GGUF (Q4_K_M)
   - 快速原型: FP16

2. **验证精度**:
   ```python
   from lm_eval import evaluator
   
   results = evaluator.simple_evaluate(
       model="gpt2",
       tasks=["hellaswag", "arc_easy"],
       num_fewshot=0
   )
   ```

3. **监控指标**:
   - 准确率下降<3%可接受
   - 延迟不应增加>20%
   - 吞吐量应提升>30%

## 七、成本收益

**Llama2-13B推理场景 (100 QPS):**

| 配置 | GPU数量 | 成本/月 | 节省 |
|-----|---------|---------|------|
| FP16 | 10×A100 | $28,800 | 基线 |
| INT8 | 5×A10G | $3,600 | 87% |
| GPTQ | 3×T4 | $1,080 | 96% |

---
**相关**: [116-LLM Serving](../116-llm-serving-architecture.md) | **版本**: AutoGPTQ 0.6.0+
