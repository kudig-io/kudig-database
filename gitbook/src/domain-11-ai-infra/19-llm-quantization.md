# 146 - LLM模型量化技术

> **适用版本**: Kubernetes v1.25 - v1.32 | **难度**: 高级 | **参考**: [GPTQ](https://github.com/IST-DASLab/gptq) | [AWQ](https://github.com/mit-han-lab/llm-awq) | [bitsandbytes](https://github.com/TimDettmers/bitsandbytes)

## 一、量化技术全景

### 1.1 量化方法分类架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                          LLM Quantization Taxonomy                                   │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌─────────────────────────────────────────────────────────────────────────────┐    │
│  │                        Post-Training Quantization (PTQ)                      │    │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐              │    │
│  │  │  Weight-Only    │  │  Weight+Activ   │  │  Dynamic        │              │    │
│  │  │  Quantization   │  │  Quantization   │  │  Quantization   │              │    │
│  │  │                 │  │                 │  │                 │              │    │
│  │  │  - GPTQ         │  │  - SmoothQuant  │  │  - bitsandbytes │              │    │
│  │  │  - AWQ          │  │  - ZeroQuant    │  │  - LLM.int8()   │              │    │
│  │  │  - GGUF/GGML    │  │  - RPTQ         │  │                 │              │    │
│  │  │  - HQQ          │  │  - OmniQuant    │  │                 │              │    │
│  │  │  - EXL2         │  │                 │  │                 │              │    │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘              │    │
│  └─────────────────────────────────────────────────────────────────────────────┘    │
│                                                                                      │
│  ┌─────────────────────────────────────────────────────────────────────────────┐    │
│  │                    Quantization-Aware Training (QAT)                         │    │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐              │    │
│  │  │  Full QAT       │  │  PEFT + Quant   │  │  Knowledge      │              │    │
│  │  │                 │  │                 │  │  Distillation   │              │    │
│  │  │  - LSQ          │  │  - QLoRA        │  │                 │              │    │
│  │  │  - PACT         │  │  - QA-LoRA      │  │  - DistilBERT   │              │    │
│  │  │  - DoReFa       │  │  - LoftQ        │  │  - TinyBERT     │              │    │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘              │    │
│  └─────────────────────────────────────────────────────────────────────────────┘    │
│                                                                                      │
│  ┌─────────────────────────────────────────────────────────────────────────────┐    │
│  │                        Hardware-Specific Quantization                        │    │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐              │    │
│  │  │  NVIDIA GPU     │  │  Intel CPU/GPU  │  │  Apple Silicon  │              │    │
│  │  │                 │  │                 │  │                 │              │    │
│  │  │  - TensorRT-LLM │  │  - IPEX-LLM     │  │  - MLX          │              │    │
│  │  │  - FP8 (H100)   │  │  - Neural Magic │  │  - CoreML       │              │    │
│  │  │  - INT8 Tensor  │  │  - OpenVINO     │  │  - GGUF Metal   │              │    │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘              │    │
│  └─────────────────────────────────────────────────────────────────────────────┘    │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 量化技术全面对比

| 技术 | 精度 | 压缩比 | 推理速度 | 精度损失 | GPU支持 | CPU支持 | 量化时间 | 适用场景 |
|-----|------|-------|---------|---------|---------|---------|---------|---------|
| **FP32** | 32-bit | 1x | 基准 | 0% | ✓ | ✓ | - | 训练基准 |
| **FP16** | 16-bit | 2x | 1.5-2x | <0.1% | ✓ | 慢 | - | 标准推理 |
| **BF16** | 16-bit | 2x | 1.5-2x | <0.1% | A100+ | ✗ | - | 训练推理 |
| **FP8** | 8-bit | 4x | 2-3x | <0.5% | H100+ | ✗ | 分钟级 | H100推理 |
| **INT8** | 8-bit | 4x | 2-3x | 0.5-1% | ✓ | ✓ | 分钟级 | 通用推理 |
| **GPTQ** | 4-bit | 8x | 1.5-2x | 1-3% | ✓ | 慢 | 小时级 | 大模型推理 |
| **AWQ** | 4-bit | 8x | 2-2.5x | 0.5-2% | ✓ | 慢 | 小时级 | 生产推理 |
| **GGUF Q4_K_M** | 4-bit | 8x | 1.5x | 1-2% | ✓ | ✓ | 小时级 | CPU/边缘 |
| **GGUF Q2_K** | 2-bit | 16x | 1.2x | 5-10% | ✓ | ✓ | 小时级 | 极限压缩 |
| **HQQ** | 2-4bit | 8-16x | 1.5-2x | 1-5% | ✓ | ✓ | 分钟级 | 快速量化 |
| **EXL2** | 2-8bit | 4-16x | 2-3x | 变化 | ✓ | ✗ | 小时级 | ExLlama |
| **NF4** | 4-bit | 8x | 1.5x | 1-2% | ✓ | ✗ | 即时 | QLoRA训练 |

### 1.3 显存需求对比 (各模型尺寸)

| 模型 | FP32 | FP16 | INT8 | INT4 (GPTQ/AWQ) | 推荐GPU |
|-----|------|------|------|-----------------|---------|
| **7B** | 28GB | 14GB | 7GB | 3.5GB | RTX 4090 / T4 |
| **13B** | 52GB | 26GB | 13GB | 6.5GB | A10G / RTX 4090 |
| **34B** | 136GB | 68GB | 34GB | 17GB | A100 40GB |
| **70B** | 280GB | 140GB | 70GB | 35GB | A100 80GB × 2 / H100 |
| **120B** | 480GB | 240GB | 120GB | 60GB | A100 80GB × 4 |
| **405B** | 1.6TB | 810GB | 405GB | 205GB | H100 × 8 |

---

## 二、GPTQ量化详解

### 2.1 GPTQ原理与算法

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         GPTQ Quantization Process                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  原理: 逐层量化 + Hessian矩阵指导的误差补偿                                    │
│                                                                              │
│  Step 1: 计算Hessian矩阵                                                     │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  H = 2 * X^T * X  (X是该层输入的校准数据)                            │    │
│  │  H^{-1} 用于误差补偿                                                  │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│  Step 2: 逐列量化权重                                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  for each column i in W:                                             │    │
│  │    1. 量化: w_q[i] = round(w[i] / scale) * scale                    │    │
│  │    2. 计算误差: δ = w[i] - w_q[i]                                    │    │
│  │    3. 误差补偿: W[:, i+1:] -= δ * H^{-1}[:, i+1:] / H^{-1}[i,i]     │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│  Step 3: Group量化 (可选)                                                    │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  将权重分组 (group_size=128), 每组独立的scale和zero_point             │    │
│  │  减少量化误差, 但增加存储开销                                          │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│  关键参数:                                                                   │
│  - bits: 量化位宽 (2/3/4/8)                                                  │
│  - group_size: 分组大小 (32/64/128/-1表示per-channel)                       │
│  - desc_act: 是否按激活值降序排列列 (提高精度)                               │
│  - damp_percent: Hessian对角线阻尼 (数值稳定性)                              │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 GPTQ量化实现

```python
# gptq_quantization.py - GPTQ量化完整流程
from auto_gptq import AutoGPTQForCausalLM, BaseQuantizeConfig
from transformers import AutoTokenizer
import torch
from datasets import load_dataset
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class GPTQQuantizer:
    """GPTQ量化器"""
    
    def __init__(
        self,
        model_name: str,
        bits: int = 4,
        group_size: int = 128,
        desc_act: bool = True,
        damp_percent: float = 0.01,
        sym: bool = True,
        true_sequential: bool = True,
    ):
        self.model_name = model_name
        self.quantize_config = BaseQuantizeConfig(
            bits=bits,
            group_size=group_size,
            desc_act=desc_act,
            damp_percent=damp_percent,
            sym=sym,
            true_sequential=true_sequential,
            model_file_base_name="model"
        )
        
        # 加载tokenizer
        self.tokenizer = AutoTokenizer.from_pretrained(
            model_name,
            trust_remote_code=True
        )
        if self.tokenizer.pad_token is None:
            self.tokenizer.pad_token = self.tokenizer.eos_token
    
    def prepare_calibration_data(
        self,
        dataset_name: str = "wikitext",
        dataset_config: str = "wikitext-2-raw-v1",
        n_samples: int = 128,
        seq_len: int = 2048,
    ):
        """准备校准数据"""
        logger.info(f"Loading calibration dataset: {dataset_name}")
        
        # 加载数据集
        dataset = load_dataset(dataset_name, dataset_config, split="train")
        
        # 合并文本
        text = "\n\n".join(dataset["text"])
        
        # 分词
        tokens = self.tokenizer(
            text,
            return_tensors="pt",
            truncation=False
        ).input_ids[0]
        
        # 切分为固定长度样本
        examples = []
        for i in range(0, len(tokens) - seq_len, seq_len):
            example = tokens[i:i + seq_len]
            examples.append({"input_ids": example.unsqueeze(0)})
            if len(examples) >= n_samples:
                break
        
        logger.info(f"Prepared {len(examples)} calibration samples")
        return examples
    
    def quantize(
        self,
        output_dir: str,
        calibration_data: list = None,
        use_triton: bool = False,
        batch_size: int = 1,
    ):
        """执行GPTQ量化"""
        logger.info(f"Loading model: {self.model_name}")
        
        # 加载模型
        model = AutoGPTQForCausalLM.from_pretrained(
            self.model_name,
            quantize_config=self.quantize_config,
            trust_remote_code=True,
            torch_dtype=torch.float16,
            device_map="auto"
        )
        
        # 准备校准数据
        if calibration_data is None:
            calibration_data = self.prepare_calibration_data()
        
        logger.info("Starting quantization...")
        
        # 执行量化
        model.quantize(
            calibration_data,
            use_triton=use_triton,
            batch_size=batch_size,
        )
        
        # 保存量化模型
        logger.info(f"Saving quantized model to: {output_dir}")
        model.save_quantized(output_dir, use_safetensors=True)
        self.tokenizer.save_pretrained(output_dir)
        
        # 验证
        self._validate_quantized_model(output_dir)
        
        return model
    
    def _validate_quantized_model(self, model_path: str):
        """验证量化模型"""
        logger.info("Validating quantized model...")
        
        # 重新加载
        model = AutoGPTQForCausalLM.from_quantized(
            model_path,
            device_map="auto",
            use_safetensors=True
        )
        
        # 简单推理测试
        test_input = self.tokenizer(
            "The capital of France is",
            return_tensors="pt"
        ).to(model.device)
        
        with torch.no_grad():
            output = model.generate(
                **test_input,
                max_new_tokens=20,
                do_sample=False
            )
        
        result = self.tokenizer.decode(output[0], skip_special_tokens=True)
        logger.info(f"Validation output: {result}")
        
        return True

# 使用示例
if __name__ == "__main__":
    quantizer = GPTQQuantizer(
        model_name="meta-llama/Meta-Llama-3-70B-Instruct",
        bits=4,
        group_size=128,
        desc_act=True
    )
    
    model = quantizer.quantize(
        output_dir="./llama3-70b-gptq-4bit",
        batch_size=1
    )
```

### 2.3 GPTQ Kubernetes部署

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: gptq-quantization-llama3-70b
  namespace: llm-inference
spec:
  backoffLimit: 2
  ttlSecondsAfterFinished: 86400
  template:
    spec:
      restartPolicy: OnFailure
      containers:
      - name: quantizer
        image: pytorch/pytorch:2.2.0-cuda12.1-cudnn8-devel
        command:
        - /bin/bash
        - -c
        - |
          set -ex
          
          # 安装依赖
          pip install auto-gptq transformers datasets accelerate
          
          # 执行量化
          python << 'EOF'
          from auto_gptq import AutoGPTQForCausalLM, BaseQuantizeConfig
          from transformers import AutoTokenizer
          from datasets import load_dataset
          import torch
          import os
          
          model_name = "meta-llama/Meta-Llama-3-70B-Instruct"
          output_dir = "/output/llama3-70b-gptq-4bit"
          
          # 量化配置
          quantize_config = BaseQuantizeConfig(
              bits=4,
              group_size=128,
              desc_act=True,
              damp_percent=0.01,
              sym=True
          )
          
          # 加载模型
          tokenizer = AutoTokenizer.from_pretrained(model_name)
          model = AutoGPTQForCausalLM.from_pretrained(
              model_name,
              quantize_config=quantize_config,
              torch_dtype=torch.float16,
              device_map="auto",
              token=os.environ.get("HF_TOKEN")
          )
          
          # 准备校准数据
          dataset = load_dataset("wikitext", "wikitext-2-raw-v1", split="train")
          text = "\n\n".join(dataset["text"][:1000])
          examples = []
          tokens = tokenizer(text, return_tensors="pt").input_ids[0]
          for i in range(0, len(tokens) - 2048, 2048):
              examples.append({"input_ids": tokens[i:i+2048].unsqueeze(0)})
              if len(examples) >= 128:
                  break
          
          # 执行量化
          model.quantize(examples, batch_size=1)
          
          # 保存
          model.save_quantized(output_dir, use_safetensors=True)
          tokenizer.save_pretrained(output_dir)
          
          print(f"Quantization complete! Model saved to {output_dir}")
          EOF
        
        env:
        - name: HF_TOKEN
          valueFrom:
            secretKeyRef:
              name: hf-secrets
              key: token
        - name: CUDA_VISIBLE_DEVICES
          value: "0,1,2,3"
        
        resources:
          requests:
            cpu: "16"
            memory: "256Gi"
            nvidia.com/gpu: "4"
          limits:
            cpu: "32"
            memory: "512Gi"
            nvidia.com/gpu: "4"
        
        volumeMounts:
        - name: output
          mountPath: /output
        - name: hf-cache
          mountPath: /root/.cache/huggingface
        - name: shm
          mountPath: /dev/shm
      
      volumes:
      - name: output
        persistentVolumeClaim:
          claimName: quantized-models-pvc
      - name: hf-cache
        persistentVolumeClaim:
          claimName: hf-cache-pvc
      - name: shm
        emptyDir:
          medium: Memory
          sizeLimit: 64Gi
      
      nodeSelector:
        nvidia.com/gpu.product: NVIDIA-A100-SXM4-80GB
```

---

## 三、AWQ量化详解

### 3.1 AWQ原理与优势

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    AWQ (Activation-aware Weight Quantization)                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  核心思想: 保护重要权重通道, 不进行激活量化                                   │
│                                                                              │
│  观察: 只有0.1%-1%的"显著"权重对模型输出影响巨大                              │
│  策略: 通过缩放激活来保护这些权重的精度                                       │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  Step 1: 识别显著权重                                                │    │
│  │  ┌───────────────────────────────────────────────────────────────┐  │    │
│  │  │  saliency = |W| * |X|  (权重绝对值 × 激活绝对值)               │  │    │
│  │  │  显著权重 = saliency > threshold                               │  │    │
│  │  └───────────────────────────────────────────────────────────────┘  │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  Step 2: 计算最优缩放因子                                            │    │
│  │  ┌───────────────────────────────────────────────────────────────┐  │    │
│  │  │  目标: 最小化量化误差 ||WX - Q(sW)·X/s||                       │  │    │
│  │  │  s* = argmin_s E[||WX - Q(sW)·X/s||²]                         │  │    │
│  │  │  通过网格搜索找到每个通道的最优s                                │  │    │
│  │  └───────────────────────────────────────────────────────────────┘  │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  Step 3: 应用缩放并量化                                              │    │
│  │  ┌───────────────────────────────────────────────────────────────┐  │    │
│  │  │  W' = W * s       (缩放权重)                                   │  │    │
│  │  │  W_q = Q(W')      (量化)                                       │  │    │
│  │  │  X' = X / s       (缩放激活, 运行时)                           │  │    │
│  │  └───────────────────────────────────────────────────────────────┘  │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│  AWQ vs GPTQ:                                                                │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  特性        │ AWQ         │ GPTQ                                   │    │
│  │  ─────────────────────────────────────────────────────────────────  │    │
│  │  量化速度    │ 更快        │ 较慢                                   │    │
│  │  推理速度    │ 更快        │ 稍慢                                   │    │
│  │  精度损失    │ 较小        │ 稍大                                   │    │
│  │  显存占用    │ 相同        │ 相同                                   │    │
│  │  硬件优化    │ 更好        │ 一般                                   │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 AWQ量化实现

```python
# awq_quantization.py - AWQ量化完整流程
from awq import AutoAWQForCausalLM
from transformers import AutoTokenizer
import torch
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class AWQQuantizer:
    """AWQ量化器"""
    
    def __init__(
        self,
        model_name: str,
        w_bit: int = 4,
        q_group_size: int = 128,
        version: str = "GEMM",  # GEMM或GEMV
        zero_point: bool = True,
    ):
        self.model_name = model_name
        self.quant_config = {
            "w_bit": w_bit,
            "q_group_size": q_group_size,
            "version": version,
            "zero_point": zero_point
        }
        
        # 加载tokenizer
        self.tokenizer = AutoTokenizer.from_pretrained(
            model_name,
            trust_remote_code=True
        )
    
    def prepare_calibration_data(
        self,
        n_samples: int = 128,
        seq_len: int = 512,
    ):
        """准备校准数据 (使用内置数据集)"""
        # AWQ使用内置的校准数据加载
        # 返回None时会使用默认的pileval数据集
        return None
    
    def quantize(
        self,
        output_dir: str,
        calib_data: str = "pileval",
        n_samples: int = 128,
        seqlen: int = 512,
    ):
        """执行AWQ量化"""
        logger.info(f"Loading model: {self.model_name}")
        
        # 加载模型
        model = AutoAWQForCausalLM.from_pretrained(
            self.model_name,
            trust_remote_code=True,
            safetensors=True,
            device_map="auto"
        )
        
        logger.info("Starting AWQ quantization...")
        logger.info(f"Config: {self.quant_config}")
        
        # 执行量化
        model.quantize(
            tokenizer=self.tokenizer,
            quant_config=self.quant_config,
            calib_data=calib_data,
            n_samples=n_samples,
            seqlen=seqlen
        )
        
        # 保存量化模型
        logger.info(f"Saving quantized model to: {output_dir}")
        model.save_quantized(output_dir)
        self.tokenizer.save_pretrained(output_dir)
        
        # 验证
        self._validate_quantized_model(output_dir)
        
        return model
    
    def _validate_quantized_model(self, model_path: str):
        """验证量化模型"""
        logger.info("Validating quantized model...")
        
        # 重新加载
        model = AutoAWQForCausalLM.from_quantized(
            model_path,
            device_map="auto",
            fuse_layers=True
        )
        
        # 简单推理测试
        test_input = self.tokenizer(
            "The capital of France is",
            return_tensors="pt"
        ).to(model.device)
        
        with torch.no_grad():
            output = model.generate(
                **test_input,
                max_new_tokens=20,
                do_sample=False
            )
        
        result = self.tokenizer.decode(output[0], skip_special_tokens=True)
        logger.info(f"Validation output: {result}")
        
        return True

# AWQ GEMM vs GEMV选择
AWQ_VERSION_GUIDE = """
AWQ版本选择指南:
- GEMM: 批量推理优化, 适合高吞吐场景
  - batch_size >= 8 时性能更好
  - 支持连续批处理
  
- GEMV: 单请求低延迟, 适合交互式场景
  - batch_size = 1 时性能更好
  - 更低的首token延迟
"""

# 使用示例
if __name__ == "__main__":
    quantizer = AWQQuantizer(
        model_name="meta-llama/Meta-Llama-3-70B-Instruct",
        w_bit=4,
        q_group_size=128,
        version="GEMM"
    )
    
    model = quantizer.quantize(
        output_dir="./llama3-70b-awq-4bit",
        calib_data="pileval",
        n_samples=128,
        seqlen=512
    )
```

### 3.3 AWQ部署配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: vllm-awq-llama3-70b
  namespace: llm-inference
spec:
  replicas: 4
  selector:
    matchLabels:
      app: vllm-awq
  template:
    metadata:
      labels:
        app: vllm-awq
    spec:
      containers:
      - name: vllm
        image: vllm/vllm-openai:v0.4.2
        args:
        - --model=/models/llama3-70b-awq
        - --quantization=awq
        - --dtype=float16
        - --tensor-parallel-size=2
        - --gpu-memory-utilization=0.90
        - --max-model-len=8192
        - --max-num-seqs=256
        - --enable-prefix-caching
        
        resources:
          requests:
            nvidia.com/gpu: "2"
          limits:
            nvidia.com/gpu: "2"
        
        volumeMounts:
        - name: models
          mountPath: /models
      
      volumes:
      - name: models
        persistentVolumeClaim:
          claimName: awq-models-pvc
      
      nodeSelector:
        nvidia.com/gpu.product: NVIDIA-A100-SXM4-80GB
```

---

## 四、GGUF量化 (llama.cpp)

### 4.1 GGUF格式说明

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          GGUF Quantization Types                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  量化类型命名: Q{bits}_{variant}_{size}                                      │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  类型         │ 位宽  │ 压缩比 │ 精度损失 │ 速度   │ 推荐场景      │    │
│  │  ─────────────────────────────────────────────────────────────────  │    │
│  │  F32          │ 32    │ 1x     │ 0%       │ 基准   │ 训练          │    │
│  │  F16          │ 16    │ 2x     │ <0.1%    │ 1.5x   │ GPU推理       │    │
│  │  Q8_0         │ 8     │ 4x     │ ~0.5%    │ 2x     │ 高精度需求    │    │
│  │  Q6_K         │ 6.56  │ 4.8x   │ ~1%      │ 2.2x   │ 平衡          │    │
│  │  Q5_K_M       │ 5.5   │ 5.8x   │ ~1.5%    │ 2.3x   │ 推荐默认      │    │
│  │  Q5_K_S       │ 5.5   │ 5.8x   │ ~1.8%    │ 2.4x   │ 更小体积      │    │
│  │  Q4_K_M       │ 4.85  │ 6.6x   │ ~2%      │ 2.5x   │ 推荐          │    │
│  │  Q4_K_S       │ 4.5   │ 7.1x   │ ~2.5%    │ 2.6x   │ 更小体积      │    │
│  │  Q4_0         │ 4     │ 8x     │ ~3%      │ 2.7x   │ 极限速度      │    │
│  │  Q3_K_M       │ 3.9   │ 8.2x   │ ~3%      │ 2.8x   │ 边缘设备      │    │
│  │  Q3_K_S       │ 3.5   │ 9.1x   │ ~4%      │ 2.9x   │ 极限压缩      │    │
│  │  Q2_K         │ 2.96  │ 10.8x  │ ~6%      │ 3x     │ 不推荐        │    │
│  │  IQ4_XS       │ 4.25  │ 7.5x   │ ~1.5%    │ 2.5x   │ i-Quant最佳   │    │
│  │  IQ3_XXS      │ 3.06  │ 10.4x  │ ~3%      │ 2.8x   │ 极小模型      │    │
│  │  IQ2_XXS      │ 2.06  │ 15.5x  │ ~8%      │ 3x     │ 研究用        │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│  K-Quants: K表示使用更大的超级块, M=Medium, S=Small, L=Large                  │
│  I-Quants: 基于重要性的量化, 更高压缩比下保持精度                             │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 GGUF量化脚本

```bash
#!/bin/bash
# gguf_quantize.sh - GGUF量化脚本

set -e

MODEL_NAME="meta-llama/Meta-Llama-3-70B-Instruct"
OUTPUT_DIR="./gguf-models"
LLAMA_CPP_DIR="./llama.cpp"

# 1. 克隆llama.cpp (如果需要)
if [ ! -d "$LLAMA_CPP_DIR" ]; then
    git clone https://github.com/ggerganov/llama.cpp.git
    cd llama.cpp
    make -j$(nproc) LLAMA_CUDA=1
    pip install -r requirements.txt
    cd ..
fi

# 2. 下载模型
echo "Downloading model..."
huggingface-cli download $MODEL_NAME --local-dir ./hf-model

# 3. 转换为GGUF格式
echo "Converting to GGUF..."
python $LLAMA_CPP_DIR/convert_hf_to_gguf.py \
    ./hf-model \
    --outfile $OUTPUT_DIR/llama3-70b-f16.gguf \
    --outtype f16

# 4. 量化为不同精度
echo "Quantizing models..."

# Q8_0 - 最高精度
$LLAMA_CPP_DIR/llama-quantize \
    $OUTPUT_DIR/llama3-70b-f16.gguf \
    $OUTPUT_DIR/llama3-70b-q8_0.gguf \
    q8_0

# Q5_K_M - 推荐平衡
$LLAMA_CPP_DIR/llama-quantize \
    $OUTPUT_DIR/llama3-70b-f16.gguf \
    $OUTPUT_DIR/llama3-70b-q5_k_m.gguf \
    q5_k_m

# Q4_K_M - 推荐大模型
$LLAMA_CPP_DIR/llama-quantize \
    $OUTPUT_DIR/llama3-70b-f16.gguf \
    $OUTPUT_DIR/llama3-70b-q4_k_m.gguf \
    q4_k_m

# Q4_0 - 最快速度
$LLAMA_CPP_DIR/llama-quantize \
    $OUTPUT_DIR/llama3-70b-f16.gguf \
    $OUTPUT_DIR/llama3-70b-q4_0.gguf \
    q4_0

# IQ4_XS - i-Quant最佳
$LLAMA_CPP_DIR/llama-quantize \
    $OUTPUT_DIR/llama3-70b-f16.gguf \
    $OUTPUT_DIR/llama3-70b-iq4_xs.gguf \
    iq4_xs

echo "Quantization complete!"
ls -lh $OUTPUT_DIR/
```

### 4.3 llama.cpp服务器部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: llama-cpp-server
  namespace: llm-inference
spec:
  replicas: 4
  selector:
    matchLabels:
      app: llama-cpp
  template:
    metadata:
      labels:
        app: llama-cpp
    spec:
      containers:
      - name: llama-cpp
        image: ghcr.io/ggerganov/llama.cpp:server-cuda
        
        args:
        - --model=/models/llama3-70b-q4_k_m.gguf
        - --host=0.0.0.0
        - --port=8080
        - --ctx-size=8192
        - --batch-size=512
        - --n-gpu-layers=80
        - --parallel=8
        - --cont-batching
        - --flash-attn
        - --mlock
        - --no-mmap
        
        ports:
        - containerPort: 8080
          name: http
        
        resources:
          requests:
            cpu: "8"
            memory: "64Gi"
            nvidia.com/gpu: "1"
          limits:
            cpu: "16"
            memory: "128Gi"
            nvidia.com/gpu: "1"
        
        volumeMounts:
        - name: models
          mountPath: /models
        
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 60
          periodSeconds: 30
        
        readinessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
      
      volumes:
      - name: models
        persistentVolumeClaim:
          claimName: gguf-models-pvc
```

---

## 五、FP8量化 (H100/Ada)

### 5.1 FP8格式说明

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              FP8 Data Format                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  FP8 E4M3 (用于权重和激活):                                                  │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  [Sign 1-bit][Exponent 4-bits][Mantissa 3-bits]                     │    │
│  │  范围: [-448, 448], 精度: 适合权重和激活                             │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│  FP8 E5M2 (用于梯度):                                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  [Sign 1-bit][Exponent 5-bits][Mantissa 2-bits]                     │    │
│  │  范围: [-57344, 57344], 精度: 较低, 适合梯度                         │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│  性能对比 (H100):                                                            │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  精度   │ Tensor Core TFLOPS │ 相对FP16  │ 显存占用              │    │
│  │  ─────────────────────────────────────────────────────────────────  │    │
│  │  FP32   │ 67                  │ 0.3x      │ 2x                    │    │
│  │  TF32   │ 989                 │ 0.5x      │ 1x                    │    │
│  │  FP16   │ 1979                │ 1x        │ 1x (baseline)         │    │
│  │  BF16   │ 1979                │ 1x        │ 1x                    │    │
│  │  FP8    │ 3958                │ 2x        │ 0.5x                  │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 5.2 FP8量化实现 (TensorRT-LLM)

```python
# fp8_quantization.py - FP8量化流程
import tensorrt_llm
from tensorrt_llm.quantization import QuantMode
from tensorrt_llm.models import LLaMAForCausalLM
import torch

class FP8Quantizer:
    """FP8量化器 (需要H100/Ada GPU)"""
    
    def __init__(self, model_dir: str, output_dir: str):
        self.model_dir = model_dir
        self.output_dir = output_dir
        
        # 检查GPU支持
        if not self._check_fp8_support():
            raise RuntimeError("FP8 requires H100/Ada GPU")
    
    def _check_fp8_support(self):
        """检查FP8支持"""
        if not torch.cuda.is_available():
            return False
        
        # 检查计算能力 >= 8.9 (Ada) 或 9.0 (Hopper)
        cc = torch.cuda.get_device_capability()
        return cc[0] >= 9 or (cc[0] == 8 and cc[1] >= 9)
    
    def quantize(
        self,
        tp_size: int = 4,
        pp_size: int = 1,
        max_batch_size: int = 64,
        max_input_len: int = 4096,
        max_output_len: int = 4096,
    ):
        """执行FP8量化并构建引擎"""
        
        # 量化模式
        quant_mode = QuantMode.from_description(
            quantize_weights=True,
            quantize_activations=True,
            per_token=True,
            per_channel=True,
            use_int4_weights=False,
            use_int8_kv_cache=False,
            use_fp8_kv_cache=True,
            use_fp8_qdq=True
        )
        
        # 转换检查点
        self._convert_checkpoint(tp_size, pp_size, quant_mode)
        
        # 构建引擎
        self._build_engine(
            tp_size,
            pp_size,
            max_batch_size,
            max_input_len,
            max_output_len,
            quant_mode
        )
    
    def _convert_checkpoint(self, tp_size, pp_size, quant_mode):
        """转换检查点为FP8格式"""
        import subprocess
        
        cmd = f"""
        python /opt/tensorrt_llm/examples/llama/convert_checkpoint.py \
            --model_dir {self.model_dir} \
            --output_dir {self.output_dir}/checkpoint \
            --dtype float16 \
            --tp_size {tp_size} \
            --pp_size {pp_size} \
            --use_fp8_rowwise \
            --calib_dataset cnn_dailymail \
            --calib_size 512
        """
        subprocess.run(cmd, shell=True, check=True)
    
    def _build_engine(
        self,
        tp_size,
        pp_size,
        max_batch_size,
        max_input_len,
        max_output_len,
        quant_mode
    ):
        """构建TensorRT引擎"""
        import subprocess
        
        cmd = f"""
        trtllm-build \
            --checkpoint_dir {self.output_dir}/checkpoint \
            --output_dir {self.output_dir}/engine \
            --gemm_plugin float16 \
            --gpt_attention_plugin float16 \
            --max_batch_size {max_batch_size} \
            --max_input_len {max_input_len} \
            --max_output_len {max_output_len} \
            --max_num_tokens 65536 \
            --use_fp8_context_fmha enable \
            --paged_kv_cache enable \
            --remove_input_padding enable \
            --workers {tp_size}
        """
        subprocess.run(cmd, shell=True, check=True)

# FP8部署配置
FP8_DEPLOYMENT_CONFIG = {
    "engine_config": {
        "max_batch_size": 64,
        "max_input_len": 4096,
        "max_output_len": 4096,
        "max_beam_width": 1,
        "kv_cache_type": "paged",
        "kv_cache_free_gpu_mem_fraction": 0.90
    },
    "runtime_config": {
        "batch_scheduler_policy": "max_utilization",
        "decoding_config": {
            "decoding_mode": "auto",
            "lookahead_config": None
        }
    }
}
```

### 5.3 FP8 Triton部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: triton-fp8-llama3-70b
  namespace: llm-inference
spec:
  replicas: 2
  selector:
    matchLabels:
      app: triton-fp8
  template:
    metadata:
      labels:
        app: triton-fp8
    spec:
      containers:
      - name: triton
        image: nvcr.io/nvidia/tritonserver:24.03-trtllm-python-py3
        
        args:
        - tritonserver
        - --model-repository=/models
        - --http-port=8000
        - --grpc-port=8001
        - --metrics-port=8002
        - --backend-config=tensorrtllm,decoupled_mode=true
        - --backend-config=tensorrtllm,batching_type=inflight_fused
        
        env:
        - name: CUDA_VISIBLE_DEVICES
          value: "0,1,2,3"
        
        ports:
        - containerPort: 8000
          name: http
        - containerPort: 8001
          name: grpc
        - containerPort: 8002
          name: metrics
        
        resources:
          requests:
            nvidia.com/gpu: "4"
          limits:
            nvidia.com/gpu: "4"
        
        volumeMounts:
        - name: model-repo
          mountPath: /models
        - name: shm
          mountPath: /dev/shm
      
      volumes:
      - name: model-repo
        persistentVolumeClaim:
          claimName: fp8-models-pvc
      - name: shm
        emptyDir:
          medium: Memory
          sizeLimit: 64Gi
      
      nodeSelector:
        nvidia.com/gpu.product: NVIDIA-H100-SXM5-80GB
```

---

## 六、SmoothQuant (W8A8)

### 6.1 SmoothQuant原理

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         SmoothQuant Algorithm                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  问题: 激活值的离群点(outliers)导致INT8量化误差大                            │
│  解决: 将量化难度从激活转移到权重                                            │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  原始: Y = X × W                                                     │    │
│  │                                                                       │    │
│  │  平滑后: Y = (X / s) × (s × W)                                       │    │
│  │         = X_smooth × W_smooth                                        │    │
│  │                                                                       │    │
│  │  其中 s 是per-channel平滑因子:                                        │    │
│  │  s_j = max(|X_j|)^α / max(|W_j|)^(1-α)                               │    │
│  │                                                                       │    │
│  │  α ∈ [0, 1] 控制平滑程度:                                            │    │
│  │  - α = 0: 不平滑 (全部在激活)                                         │    │
│  │  - α = 1: 完全平滑 (全部转移到权重)                                   │    │
│  │  - α = 0.5: 平衡 (推荐)                                               │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│  效果对比:                                                                   │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  方法          │ OPT-175B PPL │ 相对FP16 │ 显存 │ 速度             │    │
│  │  ─────────────────────────────────────────────────────────────────  │    │
│  │  FP16          │ 8.34         │ 基准     │ 350GB │ 1x              │    │
│  │  W8A8 naive    │ 发散         │ -        │ 175GB │ -               │    │
│  │  W8A8 + ZQ     │ 8.69         │ +4.2%    │ 175GB │ 1.8x            │    │
│  │  SmoothQuant   │ 8.42         │ +1.0%    │ 175GB │ 1.9x            │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 6.2 SmoothQuant实现

```python
# smoothquant.py - SmoothQuant实现
import torch
import torch.nn as nn
from transformers import AutoModelForCausalLM, AutoTokenizer
from datasets import load_dataset
import numpy as np

class SmoothQuantCalibrator:
    """SmoothQuant校准器"""
    
    def __init__(self, model, tokenizer, alpha=0.5):
        self.model = model
        self.tokenizer = tokenizer
        self.alpha = alpha
        self.act_scales = {}
        self.hooks = []
    
    def register_hooks(self):
        """注册钩子收集激活统计"""
        for name, module in self.model.named_modules():
            if isinstance(module, nn.Linear):
                hook = module.register_forward_hook(
                    lambda m, inp, out, name=name: self._collect_stats(name, inp[0])
                )
                self.hooks.append(hook)
    
    def remove_hooks(self):
        """移除钩子"""
        for hook in self.hooks:
            hook.remove()
        self.hooks = []
    
    def _collect_stats(self, name, activation):
        """收集激活统计"""
        if name not in self.act_scales:
            self.act_scales[name] = activation.abs().max(dim=0)[0].detach()
        else:
            self.act_scales[name] = torch.max(
                self.act_scales[name],
                activation.abs().max(dim=0)[0].detach()
            )
    
    def calibrate(self, calibration_data, n_samples=128):
        """执行校准"""
        self.register_hooks()
        
        self.model.eval()
        with torch.no_grad():
            for i, sample in enumerate(calibration_data):
                if i >= n_samples:
                    break
                inputs = self.tokenizer(
                    sample,
                    return_tensors="pt",
                    truncation=True,
                    max_length=512
                ).to(self.model.device)
                self.model(**inputs)
        
        self.remove_hooks()
    
    def compute_smooth_scales(self):
        """计算平滑缩放因子"""
        smooth_scales = {}
        
        for name, module in self.model.named_modules():
            if isinstance(module, nn.Linear) and name in self.act_scales:
                act_scale = self.act_scales[name]
                weight_scale = module.weight.abs().max(dim=0)[0]
                
                # s = act_scale^α / weight_scale^(1-α)
                scale = (act_scale.pow(self.alpha) / 
                        weight_scale.pow(1 - self.alpha).clamp(min=1e-5))
                
                smooth_scales[name] = scale.to(module.weight.device)
        
        return smooth_scales
    
    def apply_smoothing(self, smooth_scales):
        """应用平滑缩放"""
        for name, module in self.model.named_modules():
            if name in smooth_scales:
                scale = smooth_scales[name]
                # 缩放权重: W_smooth = W * s
                module.weight.data *= scale.unsqueeze(0)
    
    def quantize_to_int8(self):
        """量化到INT8"""
        for name, module in self.model.named_modules():
            if isinstance(module, nn.Linear):
                # 对称量化
                weight_max = module.weight.abs().max()
                scale = weight_max / 127.0
                
                # 量化并反量化 (模拟INT8)
                module.weight.data = (
                    (module.weight.data / scale).round().clamp(-127, 127) * scale
                )
                
                # 存储量化参数
                module.register_buffer('weight_scale', scale)

def smooth_quantize_model(model_name: str, output_dir: str, alpha: float = 0.5):
    """SmoothQuant量化流程"""
    
    # 加载模型
    model = AutoModelForCausalLM.from_pretrained(
        model_name,
        torch_dtype=torch.float16,
        device_map="auto"
    )
    tokenizer = AutoTokenizer.from_pretrained(model_name)
    
    # 准备校准数据
    dataset = load_dataset("wikitext", "wikitext-2-raw-v1", split="train")
    calibration_data = [text for text in dataset["text"] if len(text) > 100][:500]
    
    # 校准
    calibrator = SmoothQuantCalibrator(model, tokenizer, alpha=alpha)
    calibrator.calibrate(calibration_data)
    
    # 计算平滑缩放
    smooth_scales = calibrator.compute_smooth_scales()
    
    # 应用平滑
    calibrator.apply_smoothing(smooth_scales)
    
    # 量化
    calibrator.quantize_to_int8()
    
    # 保存
    model.save_pretrained(output_dir)
    tokenizer.save_pretrained(output_dir)
    
    # 保存平滑缩放因子
    torch.save(smooth_scales, f"{output_dir}/smooth_scales.pt")
    
    return model
```

---

## 七、bitsandbytes动态量化

### 7.1 LLM.int8()原理

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     LLM.int8() Mixed Precision Quantization                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  核心思想: 分离处理离群特征(outliers)和正常特征                               │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  Step 1: 检测离群特征                                                │    │
│  │  ┌───────────────────────────────────────────────────────────────┐  │    │
│  │  │  outlier_cols = {i : |X[:, i]|_max > threshold}               │  │    │
│  │  │  threshold = 6.0 (默认)                                        │  │    │
│  │  └───────────────────────────────────────────────────────────────┘  │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  Step 2: 分离计算                                                    │    │
│  │  ┌───────────────────────────────────────────────────────────────┐  │    │
│  │  │  X_outlier: 离群特征 → FP16矩阵乘法                           │  │    │
│  │  │  X_normal:  正常特征 → INT8矩阵乘法                           │  │    │
│  │  └───────────────────────────────────────────────────────────────┘  │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  Step 3: 合并结果                                                    │    │
│  │  ┌───────────────────────────────────────────────────────────────┐  │    │
│  │  │  Y = matmul_int8(X_normal, W_normal) +                        │  │    │
│  │  │      matmul_fp16(X_outlier, W_outlier)                        │  │    │
│  │  └───────────────────────────────────────────────────────────────┘  │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│  离群特征比例: 通常 < 0.1% (对显存影响很小)                                  │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 7.2 bitsandbytes使用

```python
# bitsandbytes_quantization.py - bitsandbytes量化
import torch
from transformers import AutoModelForCausalLM, AutoTokenizer, BitsAndBytesConfig

# INT8量化配置
INT8_CONFIG = BitsAndBytesConfig(
    load_in_8bit=True,
    llm_int8_threshold=6.0,  # 离群检测阈值
    llm_int8_skip_modules=["lm_head"],  # 跳过量化的模块
    llm_int8_enable_fp32_cpu_offload=False,  # CPU offload
    llm_int8_has_fp16_weight=False,
)

# NF4量化配置 (QLoRA使用)
NF4_CONFIG = BitsAndBytesConfig(
    load_in_4bit=True,
    bnb_4bit_quant_type="nf4",  # Normal Float 4-bit
    bnb_4bit_compute_dtype=torch.bfloat16,  # 计算精度
    bnb_4bit_use_double_quant=True,  # 双重量化
)

# FP4量化配置
FP4_CONFIG = BitsAndBytesConfig(
    load_in_4bit=True,
    bnb_4bit_quant_type="fp4",  # Float Point 4-bit
    bnb_4bit_compute_dtype=torch.float16,
)

def load_quantized_model(
    model_name: str,
    quantization: str = "int8",
    device_map: str = "auto"
):
    """加载量化模型"""
    
    config_map = {
        "int8": INT8_CONFIG,
        "nf4": NF4_CONFIG,
        "fp4": FP4_CONFIG,
    }
    
    quantization_config = config_map.get(quantization)
    if quantization_config is None:
        raise ValueError(f"Unknown quantization: {quantization}")
    
    model = AutoModelForCausalLM.from_pretrained(
        model_name,
        quantization_config=quantization_config,
        device_map=device_map,
        trust_remote_code=True,
    )
    
    tokenizer = AutoTokenizer.from_pretrained(model_name)
    
    return model, tokenizer

def print_model_size(model):
    """打印模型大小"""
    param_size = 0
    for param in model.parameters():
        param_size += param.nelement() * param.element_size()
    
    buffer_size = 0
    for buffer in model.buffers():
        buffer_size += buffer.nelement() * buffer.element_size()
    
    size_mb = (param_size + buffer_size) / 1024 / 1024
    print(f"Model size: {size_mb:.2f} MB")

# 使用示例
if __name__ == "__main__":
    # 加载INT8模型
    model_int8, tokenizer = load_quantized_model(
        "meta-llama/Meta-Llama-3-70B-Instruct",
        quantization="int8"
    )
    print_model_size(model_int8)
    
    # 加载NF4模型 (适合QLoRA微调)
    model_nf4, tokenizer = load_quantized_model(
        "meta-llama/Meta-Llama-3-70B-Instruct",
        quantization="nf4"
    )
    print_model_size(model_nf4)
```

### 7.3 bitsandbytes Kubernetes部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: bnb-inference
  namespace: llm-inference
spec:
  replicas: 4
  selector:
    matchLabels:
      app: bnb-inference
  template:
    metadata:
      labels:
        app: bnb-inference
    spec:
      containers:
      - name: inference
        image: pytorch/pytorch:2.2.0-cuda12.1-cudnn8-runtime
        command:
        - /bin/bash
        - -c
        - |
          pip install transformers accelerate bitsandbytes
          python << 'EOF'
          from transformers import AutoModelForCausalLM, AutoTokenizer, BitsAndBytesConfig
          from fastapi import FastAPI
          from pydantic import BaseModel
          import torch
          import uvicorn
          
          # 加载模型
          bnb_config = BitsAndBytesConfig(
              load_in_8bit=True,
              llm_int8_threshold=6.0
          )
          
          model = AutoModelForCausalLM.from_pretrained(
              "meta-llama/Meta-Llama-3-70B-Instruct",
              quantization_config=bnb_config,
              device_map="auto"
          )
          tokenizer = AutoTokenizer.from_pretrained(
              "meta-llama/Meta-Llama-3-70B-Instruct"
          )
          
          app = FastAPI()
          
          class GenerateRequest(BaseModel):
              prompt: str
              max_tokens: int = 512
          
          @app.post("/generate")
          async def generate(request: GenerateRequest):
              inputs = tokenizer(request.prompt, return_tensors="pt").to(model.device)
              outputs = model.generate(**inputs, max_new_tokens=request.max_tokens)
              return {"text": tokenizer.decode(outputs[0], skip_special_tokens=True)}
          
          uvicorn.run(app, host="0.0.0.0", port=8000)
          EOF
        
        env:
        - name: HF_TOKEN
          valueFrom:
            secretKeyRef:
              name: hf-secrets
              key: token
        
        resources:
          requests:
            nvidia.com/gpu: "2"
          limits:
            nvidia.com/gpu: "2"
        
        ports:
        - containerPort: 8000
```

---

## 八、量化精度评估

### 8.1 评估框架

```python
# quantization_eval.py - 量化精度评估
from lm_eval import evaluator
from lm_eval.models.huggingface import HFLM
import json
import pandas as pd

class QuantizationEvaluator:
    """量化模型评估器"""
    
    def __init__(self, model, tokenizer, model_name: str):
        self.model = model
        self.tokenizer = tokenizer
        self.model_name = model_name
    
    def evaluate(
        self,
        tasks: list = None,
        num_fewshot: int = 0,
        batch_size: int = 8,
    ):
        """运行评估"""
        
        if tasks is None:
            tasks = [
                "hellaswag",
                "arc_easy",
                "arc_challenge", 
                "winogrande",
                "piqa",
                "boolq",
                "openbookqa",
            ]
        
        # 创建lm-eval模型包装
        lm = HFLM(
            pretrained=self.model,
            tokenizer=self.tokenizer,
            batch_size=batch_size
        )
        
        # 运行评估
        results = evaluator.simple_evaluate(
            model=lm,
            tasks=tasks,
            num_fewshot=num_fewshot,
            batch_size=batch_size
        )
        
        return results
    
    def compare_with_baseline(
        self,
        baseline_results: dict,
        quantized_results: dict
    ):
        """与基线比较"""
        
        comparison = []
        for task in baseline_results["results"]:
            baseline_acc = baseline_results["results"][task].get("acc,none", 0)
            quantized_acc = quantized_results["results"][task].get("acc,none", 0)
            
            comparison.append({
                "task": task,
                "baseline_acc": baseline_acc,
                "quantized_acc": quantized_acc,
                "delta": quantized_acc - baseline_acc,
                "relative_change": (quantized_acc - baseline_acc) / baseline_acc * 100
            })
        
        df = pd.DataFrame(comparison)
        return df

# 评估任务说明
EVAL_TASKS = {
    # 常识推理
    "hellaswag": "句子补全",
    "winogrande": "代词消解",
    "piqa": "物理常识",
    
    # 推理能力
    "arc_easy": "科学推理(简单)",
    "arc_challenge": "科学推理(困难)",
    "openbookqa": "开放问答",
    
    # 阅读理解
    "boolq": "布尔问答",
    "race": "阅读理解",
    
    # 数学
    "gsm8k": "小学数学",
    "math": "高等数学",
    
    # 代码
    "humaneval": "代码生成",
    "mbpp": "代码生成",
}

# 精度损失阈值
ACCURACY_THRESHOLDS = {
    "acceptable": 0.02,      # <2% 可接受
    "marginal": 0.05,        # 2-5% 边际
    "significant": 0.10,     # 5-10% 显著
    "unacceptable": float("inf")  # >10% 不可接受
}
```

### 8.2 Perplexity评估

```python
# perplexity_eval.py - 困惑度评估
import torch
from datasets import load_dataset
from tqdm import tqdm
import math

def evaluate_perplexity(
    model,
    tokenizer,
    dataset_name: str = "wikitext",
    dataset_config: str = "wikitext-2-raw-v1",
    stride: int = 512,
    max_length: int = 2048,
):
    """评估模型困惑度"""
    
    # 加载数据集
    dataset = load_dataset(dataset_name, dataset_config, split="test")
    text = "\n\n".join(dataset["text"])
    
    # 分词
    encodings = tokenizer(text, return_tensors="pt")
    
    max_length = min(max_length, model.config.max_position_embeddings)
    seq_len = encodings.input_ids.size(1)
    
    nlls = []
    prev_end_loc = 0
    
    for begin_loc in tqdm(range(0, seq_len, stride)):
        end_loc = min(begin_loc + max_length, seq_len)
        trg_len = end_loc - prev_end_loc
        
        input_ids = encodings.input_ids[:, begin_loc:end_loc].to(model.device)
        target_ids = input_ids.clone()
        target_ids[:, :-trg_len] = -100
        
        with torch.no_grad():
            outputs = model(input_ids, labels=target_ids)
            neg_log_likelihood = outputs.loss
        
        nlls.append(neg_log_likelihood)
        
        prev_end_loc = end_loc
        if end_loc == seq_len:
            break
    
    ppl = torch.exp(torch.stack(nlls).mean())
    return ppl.item()

# 各模型基准PPL (WikiText-2)
BASELINE_PPL = {
    "llama3-8b": {"fp16": 6.24, "int8": 6.28, "int4": 6.45},
    "llama3-70b": {"fp16": 4.12, "int8": 4.15, "int4": 4.28},
    "mixtral-8x7b": {"fp16": 5.89, "int8": 5.94, "int4": 6.12},
}
```

---

## 九、性能基准对比

### 9.1 完整性能矩阵

| 模型 | 量化方法 | 显存 (GB) | 吞吐量 (tok/s) | P99延迟 (ms) | PPL变化 | 成本/1M tokens |
|-----|---------|----------|---------------|-------------|---------|---------------|
| **Llama3-8B** |
| | FP16 | 16 | 2,800 | 15 | 基准 | $0.020 |
| | INT8 (bnb) | 8 | 3,200 | 13 | +0.6% | $0.015 |
| | GPTQ-4bit | 4 | 3,800 | 11 | +2.1% | $0.012 |
| | AWQ-4bit | 4 | 4,200 | 10 | +1.5% | $0.010 |
| | GGUF Q4_K_M | 4.5 | 3,500 | 12 | +1.8% | $0.011 |
| **Llama3-70B** |
| | FP16 | 140 | 350 | 120 | 基准 | $0.180 |
| | INT8 | 70 | 420 | 100 | +0.7% | $0.120 |
| | GPTQ-4bit | 35 | 480 | 90 | +2.8% | $0.085 |
| | AWQ-4bit | 35 | 520 | 85 | +2.0% | $0.075 |
| | FP8 (H100) | 70 | 680 | 65 | +0.3% | $0.095 |
| **Llama3-405B** |
| | FP16 | 810 | 120 | 350 | 基准 | $0.650 |
| | FP8 (H100) | 420 | 220 | 200 | +0.5% | $0.350 |
| | INT4 | 210 | 280 | 160 | +3.5% | $0.220 |

### 9.2 量化方法选择指南

```yaml
# 量化选择决策树
quantization_selection:
  
  # 场景1: 最高精度要求
  high_accuracy:
    options:
    - method: FP8
      requirements: H100/Ada GPU
      accuracy_loss: "<0.5%"
      use_case: "医疗/金融/法律"
    - method: INT8 (SmoothQuant)
      requirements: "任何GPU"
      accuracy_loss: "<1%"
      use_case: "通用高精度"
  
  # 场景2: 成本优化
  cost_optimized:
    options:
    - method: AWQ-4bit
      accuracy_loss: "1-2%"
      memory_reduction: "75%"
      use_case: "生产推理"
    - method: GPTQ-4bit
      accuracy_loss: "2-3%"
      memory_reduction: "75%"
      use_case: "大批量推理"
  
  # 场景3: 边缘/CPU部署
  edge_deployment:
    options:
    - method: GGUF Q4_K_M
      accuracy_loss: "2%"
      cpu_friendly: true
      use_case: "笔记本/边缘"
    - method: GGUF Q5_K_M
      accuracy_loss: "1.5%"
      cpu_friendly: true
      use_case: "高精度边缘"
  
  # 场景4: 微调需求
  finetuning:
    options:
    - method: QLoRA (NF4)
      accuracy_loss: "即时量化"
      trainable: true
      use_case: "4-bit微调"
```

---

## 十、监控与告警

### 10.1 量化模型监控

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: quantization-monitoring
  namespace: llm-inference
spec:
  groups:
  - name: quantization-quality
    rules:
    # 量化精度监控
    - record: llm:quantization_accuracy_score
      expr: |
        1 - abs(llm_baseline_accuracy - llm_quantized_accuracy) / llm_baseline_accuracy
    
    # 量化模型吞吐量
    - record: llm:quantized_throughput
      expr: |
        sum(rate(vllm_generation_tokens_total{quantization!=""}[5m])) by (model, quantization)
    
    # 精度下降告警
    - alert: QuantizationAccuracyDegradation
      expr: |
        llm:quantization_accuracy_score < 0.95
      for: 30m
      labels:
        severity: warning
      annotations:
        summary: "量化模型精度下降超过5%"
        description: "模型 {{ $labels.model }} 量化后精度 {{ $value }}"
    
    # 性能下降告警
    - alert: QuantizedModelSlowdown
      expr: |
        llm:quantized_throughput < llm:baseline_throughput * 0.8
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "量化模型性能下降"
        description: "模型 {{ $labels.model }} 吞吐量低于预期"
```

---

## 十一、快速参考

### 11.1 量化方法速查表

| 场景 | 推荐方法 | 命令/配置 |
|-----|---------|----------|
| **快速测试** | bitsandbytes INT8 | `load_in_8bit=True` |
| **生产推理** | AWQ-4bit | `--quantization=awq` |
| **H100部署** | FP8 | `--quantization=fp8` |
| **CPU推理** | GGUF Q4_K_M | `llama.cpp --ngl 0` |
| **QLoRA微调** | NF4 | `load_in_4bit=True, bnb_4bit_quant_type="nf4"` |
| **极限压缩** | GGUF Q2_K | `llama-quantize model.gguf q2_k` |

### 11.2 显存估算公式

```python
def estimate_memory(
    params_b: float,  # 参数量 (十亿)
    precision_bits: int,  # 精度位数
    kv_cache_len: int = 2048,  # KV缓存长度
    batch_size: int = 1,
    hidden_size: int = 8192,
    num_layers: int = 80,
) -> float:
    """估算显存需求 (GB)"""
    
    # 模型权重
    model_memory = params_b * precision_bits / 8  # GB
    
    # KV缓存 (per layer: 2 * batch * seq * hidden * precision)
    kv_memory = (2 * batch_size * kv_cache_len * hidden_size * 
                 num_layers * 2 / 1024**3)  # FP16 KV cache
    
    # 激活内存 (近似)
    activation_memory = batch_size * 2  # GB
    
    # 总内存 (1.2x overhead)
    total = (model_memory + kv_memory + activation_memory) * 1.2
    
    return total

# 示例
print(f"Llama3-70B FP16: {estimate_memory(70, 16):.0f} GB")
print(f"Llama3-70B INT8: {estimate_memory(70, 8):.0f} GB")
print(f"Llama3-70B INT4: {estimate_memory(70, 4):.0f} GB")
```

### 11.3 常见问题解决

| 问题 | 原因 | 解决方案 |
|-----|------|---------|
| CUDA OOM | 模型太大 | 使用更低精度量化 / 更多GPU |
| 精度严重下降 | 量化过度 | 提高精度 / 使用AWQ替代GPTQ |
| 推理速度慢 | 量化未优化 | 使用GEMM kernel / vLLM优化 |
| 量化失败 | 校准数据不足 | 增加校准样本数 |
| 离群值溢出 | INT8阈值不当 | 调整llm_int8_threshold |

---

## 十二、最佳实践

### 量化部署检查清单

- [ ] **选择合适的量化方法**: 根据精度/性能/成本权衡
- [ ] **准备足够的校准数据**: 128-512样本,代表性数据
- [ ] **评估精度损失**: PPL变化<5%,任务精度变化<3%
- [ ] **性能基准测试**: 确保吞吐量和延迟满足SLA
- [ ] **监控量化模型**: 持续跟踪精度和性能指标
- [ ] **保留FP16模型**: 作为精度验证基准

---

**相关文档**: [144-LLM推理服务](144-llm-inference-serving.md) | [143-LLM微调技术](143-llm-finetuning.md) | [133-GPU调度管理](133-gpu-scheduling-management.md)

**版本**: AutoGPTQ 0.7+ | AWQ 0.2+ | bitsandbytes 0.43+ | llama.cpp b2500+
