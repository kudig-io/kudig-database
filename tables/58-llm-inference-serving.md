# 58 - LLM推理服务部署

> **适用版本**: v1.25 - v1.32 | **参考**: [vLLM](https://docs.vllm.ai/)

## 一、推理引擎对比

| 引擎 | 吞吐量 | 延迟 | 特性 | 适用场景 |
|-----|--------|------|------|---------|
| **vLLM** | ★★★★★ | 中 | PagedAttention | 高吞吐生产 |
| **TGI** | ★★★★☆ | 低 | HF原生 | HF模型 |
| **TensorRT-LLM** | ★★★★★ | 极低 | NVIDIA优化 | A100/H100 |
| **llama.cpp** | ★★☆☆☆ | 低 | CPU推理 | 边缘设备 |

## 二、vLLM部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: vllm-llama2-7b
spec:
  replicas: 2
  template:
    spec:
      containers:
      - name: vllm
        image: vllm/vllm-openai:v0.3.0
        args:
        - --model=/models/llama-2-7b
        - --tensor-parallel-size=1
        - --dtype=float16
        resources:
          limits:
            nvidia.com/gpu: 1
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: vllm-hpa
spec:
  scaleTargetRef:
    name: vllm-llama2-7b
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Pods
    pods:
      metric:
        name: vllm_queue_size
      target:
        averageValue: "10"
```

## 三、性能优化

| 技术 | 效果 | 配置 |
|-----|------|------|
| **Continuous Batching** | 吞吐↑10x | vLLM默认 |
| **PagedAttention** | 显存利用率95% | `--gpu-memory-utilization=0.95` |
| **FlashAttention 2** | 速度↑2x | 自动启用 |
| **量化** | 显存↓75% | `--quantization=awq` |

## 四、性能基准 (Llama2-7B, A100)

| 配置 | 吞吐量 | P99延迟 | 成本/1M tokens |
|-----|--------|---------|---------------|
| vLLM FP16 | 4800 tok/s | 850ms | $0.12 |
| TensorRT FP16 | 7200 tok/s | 620ms | $0.08 |
| vLLM INT8 | 6400 tok/s | 720ms | $0.10 |

## 五、监控指标

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: vllm-alerts
spec:
  groups:
  - name: vllm
    rules:
    - alert: HighInferenceLatency
      expr: histogram_quantile(0.99, rate(vllm_request_duration_seconds_bucket[5m])) > 5
      annotations:
        summary: "P99延迟>5秒"
    
    - alert: HighQueueSize
      expr: vllm_num_requests_waiting > 50
      annotations:
        summary: "请求队列积压"
```

## 六、成本优化

**策略:**
1. **Spot实例**: 节省70%成本
2. **批处理**: 吞吐提升10倍
3. **量化**: INT8节省50%成本
4. **缓存**: 30%请求命中缓存

**示例 (100 QPS):**
- 无优化: 10个A100实例 = $400/小时
- 优化后: 2个A100实例 = $80/小时
- 节省: 80%

## 七、最佳实践

1. **副本数**: 最少2个副本保证高可用
2. **健康检查**: 配置liveness/readiness探针
3. **请求超时**: 设置30-60秒超时
4. **日志**: 记录所有请求用于审计
5. **限流**: 按用户/API key限流

---
**相关**: [116-LLM Serving架构](../116-llm-serving-architecture.md) | **版本**: vLLM 0.3.0+
