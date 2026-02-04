# 65 - LLM可观测性与监控

> **适用版本**: v1.25 - v1.32 | **参考**: [Prometheus](https://prometheus.io/)

## 一、监控指标体系

| 类型 | 指标 | 阈值 | 告警 |
|-----|------|------|------|
| **性能** | P99延迟 | <2s | 高 |
| **吞吐** | QPS | >100 | 中 |
| **质量** | 错误率 | <1% | 高 |
| **资源** | GPU利用率 | >70% | 低 |
| **成本** | $/1M tokens | 监控 | 信息 |

## 二、Prometheus指标

```python
from prometheus_client import Counter, Histogram, Gauge
import time

# 请求计数
request_count = Counter(
    'llm_requests_total',
    'Total LLM requests',
    ['model', 'status']
)

# 延迟分布
request_latency = Histogram(
    'llm_request_duration_seconds',
    'LLM request latency',
    ['model'],
    buckets=[0.1, 0.5, 1.0, 2.0, 5.0, 10.0]
)

# Token计数
token_count = Counter(
    'llm_tokens_total',
    'Total tokens processed',
    ['model', 'type']  # type: input/output
)

# GPU利用率
gpu_utilization = Gauge(
    'llm_gpu_utilization_percent',
    'GPU utilization',
    ['gpu_id']
)

# 使用示例
@app.post("/v1/chat/completions")
async def chat(request: dict):
    start = time.time()
    
    try:
        response = llm.generate(request["messages"])
        
        # 记录指标
        request_count.labels(model=model_name, status="success").inc()
        token_count.labels(model=model_name, type="input").inc(input_tokens)
        token_count.labels(model=model_name, type="output").inc(output_tokens)
        
        return response
    except Exception as e:
        request_count.labels(model=model_name, status="error").inc()
        raise
    finally:
        duration = time.time() - start
        request_latency.labels(model=model_name).observe(duration)
```

## 三、告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: llm-alerts
spec:
  groups:
  - name: llm_performance
    rules:
    - alert: HighInferenceLatency
      expr: histogram_quantile(0.99, rate(llm_request_duration_seconds_bucket[5m])) > 2
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "LLM P99延迟>2秒"
    
    - alert: HighErrorRate
      expr: |
        sum(rate(llm_requests_total{status="error"}[5m]))
        / sum(rate(llm_requests_total[5m]))
        > 0.01
      for: 10m
      labels:
        severity: critical
      annotations:
        summary: "错误率>1%"
    
    - alert: LowGPUUtilization
      expr: avg(llm_gpu_utilization_percent) < 30
      for: 30m
      labels:
        severity: info
      annotations:
        summary: "GPU利用率<30%，资源浪费"
```

## 四、Grafana Dashboard

```json
{
  "dashboard": {
    "title": "LLM Monitoring",
    "panels": [
      {
        "title": "Requests Per Second",
        "targets": [{
          "expr": "rate(llm_requests_total[1m])"
        }]
      },
      {
        "title": "P50/P95/P99 Latency",
        "targets": [
          {"expr": "histogram_quantile(0.50, rate(llm_request_duration_seconds_bucket[5m]))"},
          {"expr": "histogram_quantile(0.95, rate(llm_request_duration_seconds_bucket[5m]))"},
          {"expr": "histogram_quantile(0.99, rate(llm_request_duration_seconds_bucket[5m]))"}
        ]
      },
      {
        "title": "Tokens Throughput",
        "targets": [{
          "expr": "rate(llm_tokens_total[1m])"
        }]
      }
    ]
  }
}
```

## 五、分布式追踪

```python
from opentelemetry import trace
from opentelemetry.instrumentation.fastapi import FastAPIInstrumentor

# 初始化追踪
tracer = trace.get_tracer(__name__)

@app.post("/v1/chat/completions")
async def chat(request: dict):
    with tracer.start_as_current_span("llm_inference") as span:
        # 记录输入
        span.set_attribute("input_tokens", len(request["messages"]))
        span.set_attribute("model", model_name)
        
        # Embedding
        with tracer.start_as_current_span("tokenization"):
            tokens = tokenizer.encode(request["messages"])
        
        # 推理
        with tracer.start_as_current_span("generation"):
            output = model.generate(tokens)
        
        # 解码
        with tracer.start_as_current_span("decoding"):
            response = tokenizer.decode(output)
        
        span.set_attribute("output_tokens", len(output))
        
        return response
```

## 六、日志聚合

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluentd-config
data:
  fluent.conf: |
    <source>
      @type tail
      path /var/log/llm/*.log
      pos_file /var/log/llm.pos
      tag llm
      <parse>
        @type json
        time_key timestamp
      </parse>
    </source>
    
    <filter llm>
      @type record_transformer
      <record>
        cluster_id "#{ENV['CLUSTER_ID']}"
        namespace "#{ENV['NAMESPACE']}"
      </record>
    </filter>
    
    <match llm>
      @type elasticsearch
      host elasticsearch
      port 9200
      index_name llm-logs
    </match>
```

## 七、用户体验监控

```python
class UserExperienceMetrics:
    def __init__(self):
        self.time_to_first_token = Histogram(
            'llm_time_to_first_token_seconds',
            'Time to first token'
        )
        self.token_generation_rate = Histogram(
            'llm_tokens_per_second',
            'Token generation rate'
        )
    
    def track_streaming_response(self, request_id):
        start_time = time.time()
        first_token_time = None
        token_count = 0
        
        for token in llm.stream_generate():
            if first_token_time is None:
                first_token_time = time.time()
                ttft = first_token_time - start_time
                self.time_to_first_token.observe(ttft)
            
            token_count += 1
            yield token
        
        total_time = time.time() - first_token_time
        tokens_per_second = token_count / total_time
        self.token_generation_rate.observe(tokens_per_second)
```

## 八、成本监控

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: cost-tracking
spec:
  groups:
  - name: llm_cost
    rules:
    - record: llm_cost_per_million_tokens
      expr: |
        (
          sum(rate(container_cpu_usage_seconds_total{pod=~"llm.*"}[1h])) * 0.05
          + sum(gpu_duty_cycle{pod=~"llm.*"}) / 100 * 2.5
        ) / (rate(llm_tokens_total[1h]) / 1000000)
```

## 九、SLO定义

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: slo-definitions
spec:
  groups:
  - name: slo
    rules:
    - record: slo:availability:ratio
      expr: |
        sum(rate(llm_requests_total{status="success"}[30d]))
        / sum(rate(llm_requests_total[30d]))
      # 目标: 99.9% (允许43分钟/月故障)
    
    - record: slo:latency:p99
      expr: histogram_quantile(0.99, rate(llm_request_duration_seconds_bucket[30d]))
      # 目标: P99 < 2秒
```

## 十、最佳实践

1. **关键指标**: 延迟、吞吐、错误率、资源利用率
2. **告警分级**: Critical/Warning/Info三级
3. **追踪采样**: 1-10%采样率平衡性能与可见性
4. **日志保留**: 30天热存储 + 90天冷存储
5. **Dashboard**: 为不同角色定制Dashboard

---
**相关**: [114-GPU监控](../114-gpu-monitoring.md) | **版本**: Prometheus 2.45+
