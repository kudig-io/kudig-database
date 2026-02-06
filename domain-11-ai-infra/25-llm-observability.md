# 25 - LLM可观测性与监控体系

> **适用版本**: Kubernetes v1.25 - v1.32 | **难度**: 专家级 | **参考**: [Prometheus](https://prometheus.io/) | [OpenTelemetry](https://opentelemetry.io/) | [Grafana](https://grafana.com/) | [Elasticsearch](https://www.elastic.co/)

## 一、企业级LLM可观测性架构

### 1.1 五维可观测性模型

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                     Enterprise LLM Observability Framework                          │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                           Metrics Collection                                  │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │  │
│  │  │ Performance │  │ Resource    │  │ Business    │  │ Cost        │          │  │
│  │  │ Metrics     │  │ Metrics     │  │ Metrics     │  │ Metrics     │          │  │
│  │  │ (Latency)   │  │ (GPU/CPU)   │  │ (Accuracy)  │  │ ($/token)   │          │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘          │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                           Log Aggregation                                     │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │  │
│  │  │ Application │  │ System      │  │ Audit       │  │ Security    │          │  │
│  │  │ Logs        │  │ Logs        │  │ Logs        │  │ Logs        │          │  │
│  │  │ (业务逻辑)  │  │ (系统状态)  │  │ (操作审计)  │  │ (安全事件)  │          │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘          │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                           Trace Analysis                                      │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │  │
│  │  │ Request     │  │ Model       │  │ Data        │  │ User        │          │  │
│  │  │ Tracing     │  │ Inference   │  │ Processing  │  │ Experience  │          │  │
│  │  │ (调用链路)  │  │ (推理过程)  │  │ (数据流)    │  │ (用户体验)  │          │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘          │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                           Alert & Action                                      │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │  │
│  │  │ Real-time   │  │ Automated   │  │ Human       │  │ Escalation  │          │  │
│  │  │ Alerts      │  │ Remediation │  │ Review      │  │ Process     │          │  │
│  │  │ (实时告警)  │  │ (自动修复)  │  │ (人工审核)  │  │ (升级流程)  │          │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘          │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 关键可观测性指标体系

| 指标类别 | 核心指标 | 监控阈值 | 告警级别 | 业务影响 |
|----------|----------|----------|----------|----------|
| **性能指标** | P50/P95/P99延迟 | P99<2s | Critical | 用户体验 |
| | QPS/TPS | 根据SLA | Warning | 系统容量 |
| | 吞吐量 | tokens/sec | Info | 效率监控 |
| **质量指标** | 准确率 | >90% | Critical | 业务正确性 |
| | 相关性得分 | >0.8 | Warning | 结果质量 |
| | 幻觉率 | <5% | Critical | 可信度 |
| **资源指标** | GPU利用率 | 70-90% | Warning | 资源效率 |
| | 内存使用率 | <85% | Critical | 系统稳定 |
| | 显存占用 | <90% | Warning | OOM风险 |
| **成本指标** | $/1K tokens | 预算阈值 | Info | 成本控制 |
| | 实例成本 | 预算阈值 | Warning | 财务监控 |
| | Spot中断率 | <10% | Info | 成本优化 |
| **用户体验** | 首token时间 | <300ms | Warning | 响应速度 |
| | 生成速率 | >10 tokens/sec | Info | 流畅度 |
| | 错误率 | <1% | Critical | 服务可用性 |

## 二、Prometheus指标体系实现

### 2.1 核心指标定义

```python
# llm_metrics.py
from prometheus_client import Counter, Histogram, Gauge, Summary
import time
from typing import Dict, Optional
import asyncio

class LLMMetricsCollector:
    def __init__(self, model_name: str = "default"):
        self.model_name = model_name
        
        # 请求计数器
        self.requests_total = Counter(
            'llm_requests_total',
            'Total number of LLM requests',
            ['model', 'status', 'endpoint']
        )
        
        # 延迟直方图
        self.request_duration = Histogram(
            'llm_request_duration_seconds',
            'LLM request duration in seconds',
            ['model', 'endpoint'],
            buckets=[0.01, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 30.0]
        )
        
        # Token计数器
        self.tokens_processed = Counter(
            'llm_tokens_processed_total',
            'Total number of tokens processed',
            ['model', 'token_type']  # input/output
        )
        
        # GPU指标
        self.gpu_utilization = Gauge(
            'llm_gpu_utilization_percent',
            'GPU utilization percentage',
            ['model', 'gpu_id']
        )
        
        self.gpu_memory_used = Gauge(
            'llm_gpu_memory_used_bytes',
            'GPU memory used in bytes',
            ['model', 'gpu_id']
        )
        
        self.gpu_temperature = Gauge(
            'llm_gpu_temperature_celsius',
            'GPU temperature in celsius',
            ['model', 'gpu_id']
        )
        
        # 模型质量指标
        self.model_accuracy = Gauge(
            'llm_model_accuracy_score',
            'Model accuracy score (0-1)',
            ['model']
        )
        
        self.hallucination_rate = Gauge(
            'llm_hallucination_rate_percent',
            'Percentage of hallucinated responses',
            ['model']
        )
        
        # 用户体验指标
        self.time_to_first_token = Histogram(
            'llm_time_to_first_token_seconds',
            'Time to first token in seconds',
            ['model'],
            buckets=[0.01, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0]
        )
        
        self.tokens_per_second = Gauge(
            'llm_tokens_per_second',
            'Tokens generated per second',
            ['model']
        )
        
        # 成本指标
        self.cost_per_thousand_tokens = Gauge(
            'llm_cost_per_thousand_tokens',
            'Cost per thousand tokens in USD',
            ['model']
        )
        
        self.instance_cost_hourly = Gauge(
            'llm_instance_cost_hourly_usd',
            'Hourly instance cost in USD',
            ['model', 'instance_type']
        )

    def record_request(self, endpoint: str, status: str = "success"):
        """记录请求"""
        self.requests_total.labels(
            model=self.model_name,
            status=status,
            endpoint=endpoint
        ).inc()
    
    def record_duration(self, endpoint: str, duration: float):
        """记录请求持续时间"""
        self.request_duration.labels(
            model=self.model_name,
            endpoint=endpoint
        ).observe(duration)
    
    def record_tokens(self, input_tokens: int, output_tokens: int):
        """记录token使用"""
        self.tokens_processed.labels(
            model=self.model_name,
            token_type="input"
        ).inc(input_tokens)
        
        self.tokens_processed.labels(
            model=self.model_name,
            token_type="output"
        ).inc(output_tokens)
    
    def record_gpu_metrics(self, gpu_id: str, utilization: float, 
                          memory_used: int, temperature: float):
        """记录GPU指标"""
        self.gpu_utilization.labels(
            model=self.model_name,
            gpu_id=gpu_id
        ).set(utilization)
        
        self.gpu_memory_used.labels(
            model=self.model_name,
            gpu_id=gpu_id
        ).set(memory_used)
        
        self.gpu_temperature.labels(
            model=self.model_name,
            gpu_id=gpu_id
        ).set(temperature)
    
    def record_model_quality(self, accuracy: float, hallucination_rate: float):
        """记录模型质量指标"""
        self.model_accuracy.labels(model=self.model_name).set(accuracy)
        self.hallucination_rate.labels(model=self.model_name).set(hallucination_rate)
    
    def record_user_experience(self, time_to_first_token: float, tokens_per_sec: float):
        """记录用户体验指标"""
        self.time_to_first_token.labels(model=self.model_name).observe(time_to_first_token)
        self.tokens_per_second.labels(model=self.model_name).set(tokens_per_sec)
    
    def record_cost(self, cost_per_1k_tokens: float, instance_cost: float, instance_type: str):
        """记录成本指标"""
        self.cost_per_thousand_tokens.labels(model=self.model_name).set(cost_per_1k_tokens)
        self.instance_cost_hourly.labels(
            model=self.model_name,
            instance_type=instance_type
        ).set(instance_cost)

# 使用示例
metrics_collector = LLMMetricsCollector("llama2-7b-chat")

class LLMService:
    def __init__(self, model_name: str):
        self.metrics = LLMMetricsCollector(model_name)
        self.model_name = model_name
    
    async def generate_response(self, prompt: str, max_tokens: int = 1000) -> Dict:
        """生成响应并记录指标"""
        start_time = time.time()
        
        try:
            # 模拟模型推理
            response = await self._inference(prompt, max_tokens)
            
            # 记录性能指标
            duration = time.time() - start_time
            self.metrics.record_duration("generate", duration)
            self.metrics.record_request("generate", "success")
            
            # 记录token使用
            input_tokens = len(prompt.split())
            output_tokens = len(response["text"].split())
            self.metrics.record_tokens(input_tokens, output_tokens)
            
            # 记录用户体验指标
            ttft = response.get("time_to_first_token", 0.1)
            tps = output_tokens / (duration - ttft) if duration > ttft else 0
            self.metrics.record_user_experience(ttft, tps)
            
            # 记录模型质量（模拟）
            accuracy = response.get("accuracy_score", 0.95)
            hallucination_rate = response.get("hallucination_rate", 0.02)
            self.metrics.record_model_quality(accuracy, hallucination_rate)
            
            # 记录成本（模拟）
            cost_per_1k = 0.002  # $0.002 per 1K tokens
            instance_cost = 1.5  # $1.5/hour
            self.metrics.record_cost(cost_per_1k, instance_cost, "g5.2xlarge")
            
            return response
            
        except Exception as e:
            self.metrics.record_request("generate", "error")
            raise
    
    async def _inference(self, prompt: str, max_tokens: int) -> Dict:
        """模拟推理过程"""
        # 模拟推理延迟
        await asyncio.sleep(0.1 + max_tokens * 0.001)
        
        return {
            "text": "This is a simulated response to: " + prompt[:50] + "...",
            "time_to_first_token": 0.05,
            "accuracy_score": 0.95,
            "hallucination_rate": 0.02
        }

# FastAPI集成示例
from fastapi import FastAPI, HTTPException
import uvicorn

app = FastAPI(title="LLM Observability Service")

@app.post("/generate")
async def generate(prompt: str, max_tokens: int = 100):
    service = LLMService("llama2-7b-chat")
    try:
        result = await service.generate_response(prompt, max_tokens)
        return result
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/metrics")
async def metrics():
    from prometheus_client import generate_latest
    return generate_latest()

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)
```

### 2.2 高级告警规则配置

```yaml
# llm-alerting-rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: llm-observability-alerts
  namespace: monitoring
spec:
  groups:
  - name: llm-performance.rules
    rules:
    # 性能告警
    - alert: HighLLMLatency
      expr: |
        histogram_quantile(0.99, rate(llm_request_duration_seconds_bucket[5m])) > 2
      for: 2m
      labels:
        severity: critical
        team: ml-platform
      annotations:
        summary: "LLM P99延迟超过2秒"
        description: "模型 {{ $labels.model }} 在端点 {{ $labels.endpoint }} 的P99延迟为 {{ $value }}秒，超过阈值2秒"
        runbook_url: "https://wiki.company.com/ml-ops/llm-performance-troubleshooting"
    
    - alert: LowLLMThroughput
      expr: |
        rate(llm_requests_total[5m]) < 10
      for: 5m
      labels:
        severity: warning
        team: ml-platform
      annotations:
        summary: "LLM请求吞吐量偏低"
        description: "模型 {{ $labels.model }} 的请求率 {{ $value | printf \"%.2f\" }}/分钟，低于阈值10/min"
    
    - alert: HighErrorRate
      expr: |
        sum(rate(llm_requests_total{status="error"}[5m])) 
        / sum(rate(llm_requests_total[5m])) > 0.01
      for: 3m
      labels:
        severity: critical
        team: ml-platform
      annotations:
        summary: "LLM错误率超过1%"
        description: "模型 {{ $labels.model }} 错误率为 {{ $value | printf \"%.4f\" }}，超过阈值1%"

  - name: llm-quality.rules
    rules:
    # 质量告警
    - alert: LowModelAccuracy
      expr: |
        llm_model_accuracy_score < 0.85
      for: 10m
      labels:
        severity: critical
        team: data-science
      annotations:
        summary: "模型准确率低于85%"
        description: "模型 {{ $labels.model }} 准确率为 {{ $value | printf \"%.4f\" }}，低于阈值85%"
    
    - alert: HighHallucinationRate
      expr: |
        llm_hallucination_rate_percent > 5
      for: 5m
      labels:
        severity: critical
        team: data-science
      annotations:
        summary: "模型幻觉率超过5%"
        description: "模型 {{ $labels.model }} 幻觉率为 {{ $value | printf \"%.2f\" }}%，超过阈值5%"

  - name: llm-resource.rules
    rules:
    # 资源告警
    - alert: HighGPUUtilization
      expr: |
        llm_gpu_utilization_percent > 95
      for: 5m
      labels:
        severity: warning
        team: ml-platform
      annotations:
        summary: "GPU利用率超过95%"
        description: "模型 {{ $labels.model }} GPU {{ $labels.gpu_id }} 利用率为 {{ $value | printf \"%.1f\" }}%"
    
    - alert: HighGPUMemoryUsage
      expr: |
        llm_gpu_memory_used_bytes / (1024*1024*1024) > 20
      for: 3m
      labels:
        severity: critical
        team: ml-platform
      annotations:
        summary: "GPU显存使用超过20GB"
        description: "模型 {{ $labels.model }} GPU {{ $labels.gpu_id }} 显存使用 {{ $value | printf \"%.2f\" }}GB"
    
    - alert: HighGPUTemperature
      expr: |
        llm_gpu_temperature_celsius > 80
      for: 2m
      labels:
        severity: warning
        team: ml-platform
      annotations:
        summary: "GPU温度超过80°C"
        description: "模型 {{ $labels.model }} GPU {{ $labels.gpu_id }} 温度为 {{ $value | printf \"%.1f\" }}°C"

  - name: llm-cost.rules
    rules:
    # 成本告警
    - alert: HighCostPerToken
      expr: |
        llm_cost_per_thousand_tokens > 5
      for: 15m
      labels:
        severity: info
        team: finance
      annotations:
        summary: "每千token成本超过$5"
        description: "模型 {{ $labels.model }} 每千token成本为 ${{ $value | printf \"%.2f\" }}"
    
    - alert: HighInstanceCost
      expr: |
        llm_instance_cost_hourly_usd > 5
      for: 1h
      labels:
        severity: warning
        team: finance
      annotations:
        summary: "实例小时成本超过$5"
        description: "模型 {{ $labels.model }} 实例 {{ $labels.instance_type }} 小时成本为 ${{ $value | printf \"%.2f\" }}"

  - name: llm-user-experience.rules
    rules:
    # 用户体验告警
    - alert: SlowTimeToFirstToken
      expr: |
        histogram_quantile(0.95, rate(llm_time_to_first_token_seconds_bucket[5m])) > 0.5
      for: 3m
      labels:
        severity: warning
        team: product
      annotations:
        summary: "首token时间超过500ms"
        description: "模型 {{ $labels.model }} P95首token时间为 {{ $value | printf \"%.3f\" }}秒"
    
    - alert: LowTokensPerSecond
      expr: |
        llm_tokens_per_second < 5
      for: 5m
      labels:
        severity: warning
        team: product
      annotations:
        summary: "生成速率低于5 tokens/sec"
        description: "模型 {{ $labels.model }} 生成速率为 {{ $value | printf \"%.1f\" }} tokens/sec"
```

## 三、Grafana仪表板配置

### 3.1 核心仪表板JSON

```json
{
  "dashboard": {
    "id": null,
    "title": "LLM Observability Dashboard",
    "tags": ["llm", "ai", "observability"],
    "timezone": "browser",
    "schemaVersion": 38,
    "version": 1,
    "refresh": "30s",
    "panels": [
      {
        "type": "stat",
        "title": "Overall Health",
        "gridPos": {"x": 0, "y": 0, "w": 4, "h": 4},
        "targets": [
          {
            "expr": "sum(up{job=\"llm-service\"})",
            "instant": true
          }
        ],
        "pluginVersion": "10.2.2"
      },
      {
        "type": "graph",
        "title": "Request Rate and Latency",
        "gridPos": {"x": 4, "y": 0, "w": 8, "h": 8},
        "targets": [
          {
            "expr": "rate(llm_requests_total[1m])",
            "legendFormat": "Requests/sec - {{status}}"
          },
          {
            "expr": "histogram_quantile(0.95, rate(llm_request_duration_seconds_bucket[5m]))",
            "legendFormat": "P95 Latency (s)"
          },
          {
            "expr": "histogram_quantile(0.99, rate(llm_request_duration_seconds_bucket[5m]))",
            "legendFormat": "P99 Latency (s)"
          }
        ]
      },
      {
        "type": "heatmap",
        "title": "Latency Distribution",
        "gridPos": {"x": 12, "y": 0, "w": 12, "h": 8},
        "targets": [
          {
            "expr": "rate(llm_request_duration_seconds_bucket[5m])",
            "format": "heatmap",
            "legendFormat": "{{le}}"
          }
        ]
      },
      {
        "type": "graph",
        "title": "GPU Metrics",
        "gridPos": {"x": 0, "y": 8, "w": 12, "h": 8},
        "targets": [
          {
            "expr": "llm_gpu_utilization_percent",
            "legendFormat": "GPU {{gpu_id}} Utilization %"
          },
          {
            "expr": "llm_gpu_memory_used_bytes / (1024*1024*1024)",
            "legendFormat": "GPU {{gpu_id}} Memory (GB)"
          },
          {
            "expr": "llm_gpu_temperature_celsius",
            "legendFormat": "GPU {{gpu_id}} Temperature (°C)"
          }
        ]
      },
      {
        "type": "graph",
        "title": "Model Quality Metrics",
        "gridPos": {"x": 12, "y": 8, "w": 12, "h": 8},
        "targets": [
          {
            "expr": "llm_model_accuracy_score",
            "legendFormat": "{{model}} Accuracy"
          },
          {
            "expr": "llm_hallucination_rate_percent",
            "legendFormat": "{{model}} Hallucination Rate %"
          }
        ],
        "thresholds": [
          {"color": "green", "value": null},
          {"color": "yellow", "value": 85},
          {"color": "red", "value": 95}
        ]
      },
      {
        "type": "graph",
        "title": "User Experience Metrics",
        "gridPos": {"x": 0, "y": 16, "w": 12, "h": 8},
        "targets": [
          {
            "expr": "histogram_quantile(0.95, rate(llm_time_to_first_token_seconds_bucket[5m])) * 1000",
            "legendFormat": "P95 TTFT (ms)"
          },
          {
            "expr": "llm_tokens_per_second",
            "legendFormat": "{{model}} Tokens/sec"
          }
        ]
      },
      {
        "type": "graph",
        "title": "Cost Metrics",
        "gridPos": {"x": 12, "y": 16, "w": 12, "h": 8},
        "targets": [
          {
            "expr": "llm_cost_per_thousand_tokens",
            "legendFormat": "{{model}} Cost per 1K tokens ($)"
          },
          {
            "expr": "llm_instance_cost_hourly_usd",
            "legendFormat": "{{model}} {{instance_type}} Hourly Cost ($)"
          }
        ]
      },
      {
        "type": "table",
        "title": "Top Error Endpoints",
        "gridPos": {"x": 0, "y": 24, "w": 24, "h": 6},
        "targets": [
          {
            "expr": "topk(10, sum by(endpoint) (rate(llm_requests_total{status=\"error\"}[5m])))",
            "format": "table"
          }
        ]
      }
    ],
    "templating": {
      "list": [
        {
          "name": "model",
          "type": "query",
          "datasource": "Prometheus",
          "refresh": 1,
          "query": "label_values(llm_requests_total, model)"
        }
      ]
    }
  }
}
```

## 四、分布式追踪实现

### 4.1 OpenTelemetry集成

```python
# opentelemetry_tracing.py
from opentelemetry import trace
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import BatchSpanProcessor
from opentelemetry.exporter.otlp.proto.grpc.trace_exporter import OTLPSpanExporter
from opentelemetry.instrumentation.fastapi import FastAPIInstrumentor
from opentelemetry.instrumentation.requests import RequestsInstrumentor
import asyncio
import time
from typing import Dict

# 初始化追踪器
trace.set_tracer_provider(TracerProvider())
otlp_exporter = OTLPSpanExporter(endpoint="http://otel-collector:4317", insecure=True)
span_processor = BatchSpanProcessor(otlp_exporter)
trace.get_tracer_provider().add_span_processor(span_processor)

tracer = trace.get_tracer(__name__)

class LLMTracer:
    def __init__(self, model_name: str):
        self.model_name = model_name
        self.tracer = tracer
    
    def trace_inference(self, prompt: str, max_tokens: int) -> Dict:
        """追踪推理过程"""
        with self.tracer.start_as_current_span("llm_inference") as span:
            span.set_attribute("model.name", self.model_name)
            span.set_attribute("input.prompt_length", len(prompt))
            span.set_attribute("input.max_tokens", max_tokens)
            
            # Tokenization阶段
            with self.tracer.start_as_current_span("tokenization") as token_span:
                start_time = time.time()
                tokens = self._tokenize(prompt)
                token_span.set_attribute("processing.time_ms", (time.time() - start_time) * 1000)
                token_span.set_attribute("output.token_count", len(tokens))
            
            # 模型推理阶段
            with self.tracer.start_as_current_span("model_inference") as inference_span:
                start_time = time.time()
                model_output = self._model_forward(tokens, max_tokens)
                inference_time = time.time() - start_time
                inference_span.set_attribute("processing.time_ms", inference_time * 1000)
                inference_span.set_attribute("output.token_count", len(model_output["tokens"]))
                inference_span.set_attribute("throughput.tokens_per_sec", len(model_output["tokens"]) / inference_time)
            
            # 解码阶段
            with self.tracer.start_as_current_span("decoding") as decode_span:
                start_time = time.time()
                response_text = self._decode(model_output["tokens"])
                decode_span.set_attribute("processing.time_ms", (time.time() - start_time) * 1000)
                decode_span.set_attribute("output.text_length", len(response_text))
            
            # 记录整体指标
            span.set_attribute("output.text", response_text[:100] + "..." if len(response_text) > 100 else response_text)
            span.set_attribute("total.processing_time_ms", (time.time() - span.start_time) * 1000)
            
            return {
                "text": response_text,
                "tokens_generated": len(model_output["tokens"]),
                "processing_time_ms": (time.time() - span.start_time) * 1000
            }
    
    def trace_user_interaction(self, user_id: str, session_id: str, prompt: str) -> Dict:
        """追踪用户交互"""
        with self.tracer.start_as_current_span("user_interaction") as span:
            span.set_attribute("user.id", user_id)
            span.set_attribute("session.id", session_id)
            span.set_attribute("input.prompt", prompt[:200] + "..." if len(prompt) > 200 else prompt)
            
            # 记录用户上下文
            with self.tracer.start_as_current_span("context_retrieval") as context_span:
                context = self._retrieve_context(user_id, session_id)
                context_span.set_attribute("context.size", len(context))
            
            # 生成响应
            response = self.trace_inference(prompt, max_tokens=500)
            
            # 记录用户反馈追踪点
            span.add_event("response_generated", {
                "response_length": len(response["text"]),
                "tokens_generated": response["tokens_generated"]
            })
            
            return response
    
    def _tokenize(self, prompt: str) -> list:
        """模拟分词"""
        time.sleep(0.01)  # 模拟处理时间
        return prompt.split()
    
    def _model_forward(self, tokens: list, max_tokens: int) -> Dict:
        """模拟模型前向传播"""
        time.sleep(0.1 + max_tokens * 0.001)  # 模拟推理时间
        return {"tokens": ["token"] * min(max_tokens, 100)}
    
    def _decode(self, tokens: list) -> str:
        """模拟解码"""
        time.sleep(0.005)  # 模拟处理时间
        return " ".join(tokens)
    
    def _retrieve_context(self, user_id: str, session_id: str) -> str:
        """模拟上下文检索"""
        time.sleep(0.02)  # 模拟检索时间
        return f"Context for user {user_id} in session {session_id}"

# FastAPI集成
from fastapi import FastAPI, Request
import uvicorn

app = FastAPI(title="LLM Tracing Service")

# 仪器化FastAPI
FastAPIInstrumentor.instrument_app(app)
RequestsInstrumentor().instrument()

llm_tracer = LLMTracer("llama2-7b-chat")

@app.post("/chat")
async def chat(request: Request):
    body = await request.json()
    prompt = body.get("prompt", "")
    user_id = body.get("user_id", "anonymous")
    session_id = body.get("session_id", "default")
    
    # 追踪用户交互
    response = llm_tracer.trace_user_interaction(user_id, session_id, prompt)
    
    return {
        "response": response["text"],
        "tokens_generated": response["tokens_generated"],
        "processing_time_ms": response["processing_time_ms"]
    }

@app.get("/health")
async def health():
    return {"status": "healthy"}

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)
```

## 五、SLO和错误预算管理

### 5.1 SLO定义和实现

```yaml
# llm-slos.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: llm-slo-rules
  namespace: monitoring
spec:
  groups:
  - name: llm-slos
    rules:
    # 可用性SLO (99.9%)
    - record: slo:availability:ratio
      expr: |
        sum(rate(llm_requests_total{status="success"}[30d]))
        / sum(rate(llm_requests_total[30d]))
    
    - alert: SLAViolation-Availability
      expr: |
        slo:availability:ratio < 0.999
      for: 1h
      labels:
        severity: critical
        slo: "availability"
      annotations:
        summary: "LLM服务可用性SLO违规"
        description: "30天可用性 {{ $value | printf \"%.4f\" }} 低于目标 99.9%"
    
    # 延迟SLO (P95 < 1s)
    - record: slo:latency:p95
      expr: |
        histogram_quantile(0.95, rate(llm_request_duration_seconds_bucket[30d]))
    
    - alert: SLAViolation-Latency
      expr: |
        slo:latency:p95 > 1
      for: 30m
      labels:
        severity: warning
        slo: "latency"
      annotations:
        summary: "LLM服务延迟SLO违规"
        description: "30天P95延迟 {{ $value | printf \"%.3f\" }}s 超过目标 1s"
    
    # 质量SLO (准确率 > 90%)
    - record: slo:quality:accuracy
      expr: |
        avg_over_time(llm_model_accuracy_score[30d])
    
    - alert: SLAViolation-Quality
      expr: |
        slo:quality:accuracy < 0.90
      for: 6h
      labels:
        severity: critical
        slo: "quality"
      annotations:
        summary: "LLM服务质量SLO违规"
        description: "30天平均准确率 {{ $value | printf \"%.4f\" }} 低于目标 90%"

  - name: error-budget
    rules:
    # 错误预算计算
    - record: error_budget:availability:remaining
      expr: |
        0.001 - (1 - slo:availability:ratio)  # 0.1% error budget
    
    - record: error_budget:latency:remaining
      expr: |
        1 - slo:latency:p95  # Latency budget
    
    - alert: ErrorBudget-BurnRate
      expr: |
        # 快速燃烧率：1小时错误率预估30天错误预算的2%
        (1 - avg(rate(llm_requests_total{status="success"}[1h])) / avg(rate(llm_requests_total[1h])))
        > (0.001 * 2 * 30)  # 2% of monthly error budget
      for: 2m
      labels:
        severity: critical
        budget: "fast-burn"
      annotations:
        summary: "LLM错误预算快速燃烧"
        description: "错误率异常升高，可能影响SLO达成"
```

---

**维护者**: LLM Observability Team | **最后更新**: 2026-02 | **版本**: v2.0

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
