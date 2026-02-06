# 36 - AI平台增强可观测性

> **适用版本**: Kubernetes v1.25 - v1.32 | **AI栈版本**: Prometheus 2.40+ | **最后更新**: 2026-02 | **质量等级**: 专家级

## 一、AI平台可观测性全景架构

### 1.1 五维可观测性模型

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    AI Platform Observability Framework                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  📊 指标监控 (Metrics)                                                 │
│  ├─ 系统指标: CPU、内存、GPU、网络                                      │
│  ├─ 应用指标: QPS、延迟、错误率                                         │
│  ├─ AI指标: 准确率、困惑度、生成质量                                    │
│  ├─ 业务指标: 收入、转化率、用户满意度                                  │
│  └─ 成本指标: 资源消耗、单位成本                                        │
│                                                                         │
│  🪵 日志分析 (Logs)                                                    │
│  ├─ 应用日志: 业务逻辑、错误信息                                        │
│  ├─ 系统日志: 内核、容器运行时                                          │
│  ├─ 安全日志: 访问控制、威胁检测                                        │
│  ├─ 审计日志: 操作记录、合规追踪                                        │
│  └─ 调试日志: 开发调试、性能分析                                        │
│                                                                         │
│  🔍 链路追踪 (Tracing)                                                 │
│  ├─ 请求链路: 端到端调用轨迹                                            │
│  ├─ 依赖关系: 服务间调用图                                              │
│  ├─ 性能瓶颈: 热点分析、延迟分解                                        │
│  ├─ 错误传播: 异常溯源、影响分析                                        │
│  └─ AI链路: Prompt处理、推理过程                                        │
│                                                                         │
│  🚨 告警管理 (Alerting)                                                │
│  ├─ 阈值告警: 静态阈值监控                                              │
│  ├─ 异常检测: 机器学习异常识别                                          │
│  ├─ 预测告警: 趋势预测、容量预警                                        │
│  ├─ 智能告警: 根因分析、关联告警                                        │
│  └─ 自愈能力: 自动修复、降级处理                                        │
│                                                                         │
│  📈 可视化展示 (Visualization)                                         │
│  ├─ 实时仪表板: 运行状态、关键指标                                      │
│  ├─ 历史趋势: 性能演变、容量规划                                        │
│  ├─ 对比分析: AB测试、版本对比                                          │
│  ├─ 下钻分析: 问题定位、根因查找                                        │
│  └─ 报告生成: 自动报表、合规报告                                        │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 1.2 AI可观测性特殊挑战

| 挑战类型 | 具体问题 | 影响 | 解决思路 |
|---------|---------|------|---------|
| **维度复杂性** | 多模态输入输出 | 监控指标爆炸 | 统一指标框架、维度抽象 |
| **实时性要求** | 推理延迟敏感 | 用户体验影响大 | 边缘计算、流式处理 |
| **语义理解** | 非结构化数据 | 传统监控失效 | AI辅助分析、语义监控 |
| **动态特性** | 模型持续演进 | 基线不断变化 | 自适应阈值、在线学习 |
| **成本敏感** | 大规模部署 | 监控成本高昂 | 智能采样、分层监控 |

## 二、企业级AI监控指标体系

### 2.1 核心指标分类

```yaml
# ai-monitoring-metrics.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: ai-platform-metrics-spec
  namespace: monitoring
data:
  metrics-specification.yaml: |
    # AI平台监控指标规范
    metrics:
      system_level:
        - name: "node_gpu_utilization"
          type: "gauge"
          description: "GPU利用率百分比"
          labels: ["node", "gpu_id", "model"]
          sampling_interval: "15s"
          retention: "30d"
        
        - name: "node_gpu_temperature"
          type: "gauge"
          description: "GPU温度(摄氏度)"
          labels: ["node", "gpu_id"]
          sampling_interval: "30s"
          retention: "7d"
        
        - name: "container_memory_working_set_bytes"
          type: "gauge"
          description: "容器实际使用内存"
          labels: ["namespace", "pod", "container"]
          sampling_interval: "15s"
          retention: "14d"
      
      application_level:
        - name: "http_requests_total"
          type: "counter"
          description: "HTTP请求数总量"
          labels: ["service", "method", "status_code", "model_name"]
          sampling_interval: "5s"
          retention: "90d"
        
        - name: "http_request_duration_seconds"
          type: "histogram"
          description: "HTTP请求延迟分布"
          labels: ["service", "method", "model_name"]
          buckets: [0.01, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0]
          sampling_interval: "5s"
          retention: "30d"
        
        - name: "model_inference_duration_seconds"
          type: "histogram"
          description: "模型推理延迟"
          labels: ["model_name", "batch_size", "hardware_type"]
          buckets: [0.01, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0]
          sampling_interval: "1s"
          retention: "15d"
      
      ai_specific:
        - name: "model_accuracy"
          type: "gauge"
          description: "模型准确率"
          labels: ["model_name", "dataset", "version"]
          sampling_interval: "1h"
          retention: "365d"
        
        - name: "prompt_tokens_total"
          type: "counter"
          description: "Prompt token消耗总量"
          labels: ["model_name", "user_id", "application"]
          sampling_interval: "1m"
          retention: "90d"
        
        - name: "generation_tokens_total"
          type: "counter"
          description: "生成token消耗总量"
          labels: ["model_name", "user_id", "application"]
          sampling_interval: "1m"
          retention: "90d"
        
        - name: "model_drift_score"
          type: "gauge"
          description: "模型漂移检测分数"
          labels: ["model_name", "feature_name", "drift_type"]
          sampling_interval: "1h"
          retention: "180d"
      
      business_level:
        - name: "api_cost_usd"
          type: "counter"
          description: "API调用成本(美元)"
          labels: ["service", "model_name", "customer_tier"]
          sampling_interval: "1m"
          retention: "365d"
        
        - name: "user_satisfaction_score"
          type: "gauge"
          description: "用户满意度评分"
          labels: ["application", "model_name", "user_segment"]
          sampling_interval: "1d"
          retention: "365d"
        
        - name: "revenue_generated_usd"
          type: "counter"
          description: "产生的收入(美元)"
          labels: ["product", "model_name", "region"]
          sampling_interval: "1h"
          retention: "365d"
```

### 2.2 Prometheus监控规则

```yaml
# ai-prometheus-rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ai-platform-monitoring-rules
  namespace: monitoring
spec:
  groups:
  - name: ai-platform.rules
    rules:
    # 系统健康检查
    - alert: HighGPUMemoryUsage
      expr: |
        avg by(node, gpu_id) (
          nvidia_gpu_memory_used_bytes / nvidia_gpu_memory_total_bytes * 100
        ) > 90
      for: 5m
      labels:
        severity: critical
        team: ai-platform
      annotations:
        summary: "GPU内存使用率过高 ({{ $labels.node }}-{{ $labels.gpu_id }})"
        description: "GPU内存使用率达到 {{ $value }}%，可能导致OOM错误"
        runbook_url: "https://internal/wiki/gpu-troubleshooting"
    
    - alert: GPUPowerAnomaly
      expr: |
        stddev_over_time(nvidia_gpu_power_usage_watts[10m]) > 50
      for: 15m
      labels:
        severity: warning
        team: ai-platform
      annotations:
        summary: "GPU功耗出现异常波动"
        description: "GPU功耗标准差过大，可能存在硬件问题"
    
    # 应用性能监控
    - alert: HighInferenceLatency
      expr: |
        histogram_quantile(0.95, 
          sum by(model_name) (
            rate(model_inference_duration_seconds_bucket[5m])
          )
        ) > 2.0
      for: 2m
      labels:
        severity: warning
        team: ml-engineering
      annotations:
        summary: "模型推理延迟过高 ({{ $labels.model_name }})"
        description: "P95推理延迟达到 {{ $value }}秒，超出SLA要求"
    
    - alert: LowModelAccuracy
      expr: |
        model_accuracy < 0.85
      for: 1h
      labels:
        severity: critical
        team: ml-engineering
      annotations:
        summary: "模型准确率下降 ({{ $labels.model_name }})"
        description: "模型准确率 {{ $value }} 低于阈值0.85，需要调查"
    
    # 业务指标监控
    - alert: HighAPICost
      expr: |
        sum by(service) (
          rate(api_cost_usd[1h])
        ) > 100
      for: 30m
      labels:
        severity: warning
        team: finance
      annotations:
        summary: "API成本过高 ({{ $labels.service }})"
        description: "小时API成本达到 ${{ $value }}，超出预算"
    
    - alert: UserSatisfactionDrop
      expr: |
        user_satisfaction_score < 3.5
      for: 24h
      labels:
        severity: critical
        team: product
      annotations:
        summary: "用户满意度下降"
        description: "用户满意度评分 {{ $value }} 低于阈值3.5"
    
    # AI特有问题
    - alert: ModelDriftDetected
      expr: |
        model_drift_score > 0.1
      for: 6h
      labels:
        severity: warning
        team: ml-engineering
      annotations:
        summary: "检测到模型漂移 ({{ $labels.model_name }})"
        description: "模型漂移分数 {{ $value }}，建议重新训练模型"
    
    - alert: TokenConsumptionSpike
      expr: |
        rate(prompt_tokens_total[5m]) > 100000
      for: 10m
      labels:
        severity: info
        team: ml-engineering
      annotations:
        summary: "Token消耗激增"
        description: "Prompt token消耗速率异常增长，当前为 {{ $value }}/sec"

  - name: ai-platform-recording.rules
    rules:
    # 预计算常用指标
    - record: "node:gpu_utilization:avg5m"
      expr: |
        avg_over_time(nvidia_gpu_utilization[5m])
    
    - record: "model:inference_p95_latency:1h"
      expr: |
        histogram_quantile(0.95, 
          sum by(model_name) (
            rate(model_inference_duration_seconds_bucket[1h])
          )
        )
    
    - record: "service:daily_cost:usd"
      expr: |
        sum by(service) (
          increase(api_cost_usd[24h])
        )
    
    - record: "model:drift_trend:7d"
      expr: |
        avg_over_time(model_drift_score[7d])
```

## 三、分布式追踪与链路分析

### 3.1 AI服务链路追踪架构

```python
# ai-tracing-instrumentation.py
import asyncio
import time
from typing import Dict, List, Optional, Any
import uuid
from dataclasses import dataclass, field
from opentelemetry import trace
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import BatchSpanProcessor
from opentelemetry.exporter.otlp.proto.grpc.trace_exporter import OTLPSpanExporter
from opentelemetry.trace import SpanKind
import logging
import json

# 初始化OpenTelemetry
trace.set_tracer_provider(TracerProvider())
otlp_exporter = OTLPSpanExporter(endpoint="http://jaeger-collector:4317", insecure=True)
span_processor = BatchSpanProcessor(otlp_exporter)
trace.get_tracer_provider().add_span_processor(span_processor)

tracer = trace.get_tracer(__name__)

@dataclass
class AIRequestContext:
    request_id: str
    model_name: str
    user_id: str
    prompt: str
    timestamp: float = field(default_factory=time.time)
    metadata: Dict[str, Any] = field(default_factory=dict)

class AITracingInstrumentation:
    def __init__(self):
        self.logger = logging.getLogger(__name__)
    
    def trace_model_inference(self, context: AIRequestContext, batch_size: int = 1):
        """追踪模型推理全过程"""
        with tracer.start_as_current_span(
            f"model_inference_{context.model_name}",
            kind=SpanKind.SERVER,
            attributes={
                "ai.request_id": context.request_id,
                "ai.model_name": context.model_name,
                "ai.user_id": context.user_id,
                "ai.batch_size": batch_size,
                "ai.prompt_length": len(context.prompt),
            }
        ) as span:
            
            # 预处理阶段
            preprocessing_result = self._trace_preprocessing(context, span)
            
            # 模型推理阶段
            inference_result = self._trace_model_inference(preprocessing_result, span)
            
            # 后处理阶段
            postprocessing_result = self._trace_postprocessing(inference_result, span)
            
            # 设置最终属性
            span.set_attribute("ai.response_length", len(postprocessing_result.get("response", "")))
            span.set_attribute("ai.tokens_input", preprocessing_result.get("input_tokens", 0))
            span.set_attribute("ai.tokens_output", postprocessing_result.get("output_tokens", 0))
            
            return postprocessing_result
    
    def _trace_preprocessing(self, context: AIRequestContext, parent_span) -> Dict:
        """追踪预处理阶段"""
        with tracer.start_as_current_span(
            "preprocessing",
            context=trace.set_span_in_context(parent_span),
            attributes={
                "component": "tokenizer",
                "ai.model_name": context.model_name,
            }
        ) as span:
            
            start_time = time.time()
            
            # 模拟预处理逻辑
            tokens = self._tokenize_prompt(context.prompt, context.model_name)
            input_ids = tokens["input_ids"]
            
            processing_time = time.time() - start_time
            
            span.set_attribute("ai.input_tokens", len(input_ids))
            span.set_attribute("ai.preprocessing_time", processing_time)
            span.set_status(trace.Status(trace.StatusCode.OK))
            
            return {
                "input_ids": input_ids,
                "attention_mask": tokens["attention_mask"],
                "input_tokens": len(input_ids),
                "processing_time": processing_time
            }
    
    def _trace_model_inference(self, preprocessed_data: Dict, parent_span) -> Dict:
        """追踪模型推理阶段"""
        with tracer.start_as_current_span(
            "model_inference",
            context=trace.set_span_in_context(parent_span),
            attributes={
                "component": "model",
                "ai.model_name": preprocessed_data.get("model_name", "unknown"),
                "ai.input_tokens": preprocessed_data.get("input_tokens", 0),
            }
        ) as span:
            
            start_time = time.time()
            
            # 模拟模型推理
            logits, hidden_states = self._run_model_inference(
                preprocessed_data["input_ids"],
                preprocessed_data["attention_mask"]
            )
            
            inference_time = time.time() - start_time
            
            span.set_attribute("ai.inference_time", inference_time)
            span.set_attribute("ai.output_logits_shape", str(logits.shape))
            span.set_status(trace.Status(trace.StatusCode.OK))
            
            return {
                "logits": logits,
                "hidden_states": hidden_states,
                "inference_time": inference_time
            }
    
    def _trace_postprocessing(self, inference_result: Dict, parent_span) -> Dict:
        """追踪后处理阶段"""
        with tracer.start_as_current_span(
            "postprocessing",
            context=trace.set_span_in_context(parent_span),
            attributes={
                "component": "decoder",
                "ai.model_name": inference_result.get("model_name", "unknown"),
            }
        ) as span:
            
            start_time = time.time()
            
            # 模拟后处理逻辑
            response_text, output_tokens = self._decode_output(inference_result["logits"])
            
            processing_time = time.time() - start_time
            
            span.set_attribute("ai.output_tokens", output_tokens)
            span.set_attribute("ai.postprocessing_time", processing_time)
            span.set_status(trace.Status(trace.StatusCode.OK))
            
            return {
                "response": response_text,
                "output_tokens": output_tokens,
                "processing_time": processing_time
            }
    
    def _tokenize_prompt(self, prompt: str, model_name: str) -> Dict:
        """模拟分词"""
        # 简化的分词逻辑
        tokens = prompt.split()
        return {
            "input_ids": list(range(len(tokens))),
            "attention_mask": [1] * len(tokens)
        }
    
    def _run_model_inference(self, input_ids: List[int], attention_mask: List[int]):
        """模拟模型推理"""
        # 模拟推理延迟
        time.sleep(0.1 + len(input_ids) * 0.001)
        
        # 模拟输出
        import numpy as np
        batch_size = 1
        seq_len = len(input_ids)
        vocab_size = 32000
        
        logits = np.random.randn(batch_size, seq_len, vocab_size).astype(np.float32)
        hidden_states = np.random.randn(batch_size, seq_len, 4096).astype(np.float32)
        
        return logits, hidden_states
    
    def _decode_output(self, logits) -> tuple:
        """模拟解码输出"""
        # 简化的解码逻辑
        import numpy as np
        batch_size, seq_len, vocab_size = logits.shape
        
        # 选择最高概率的token
        output_ids = np.argmax(logits, axis=-1)[0]  # 取第一个样本
        output_tokens = len(output_ids)
        
        # 转换为文本（简化）
        response_text = " ".join([f"token_{token_id}" for token_id in output_ids[:50]])
        
        return response_text, output_tokens

class AIPerformanceAnalyzer:
    def __init__(self):
        self.tracer = trace.get_tracer(__name__)
        self.logger = logging.getLogger(__name__)
    
    def analyze_inference_performance(self, traces: List[Dict]) -> Dict:
        """分析推理性能"""
        if not traces:
            return {}
        
        # 提取关键性能指标
        preprocessing_times = []
        inference_times = []
        postprocessing_times = []
        total_times = []
        
        for trace_data in traces:
            spans = trace_data.get("spans", [])
            
            # 提取各阶段耗时
            for span in spans:
                if span.get("name") == "preprocessing":
                    preprocessing_times.append(span.get("attributes", {}).get("ai.preprocessing_time", 0))
                elif span.get("name") == "model_inference":
                    inference_times.append(span.get("attributes", {}).get("ai.inference_time", 0))
                elif span.get("name") == "postprocessing":
                    postprocessing_times.append(span.get("attributes", {}).get("ai.postprocessing_time", 0))
                elif span.get("name", "").startswith("model_inference_"):
                    total_times.append(span.get("duration", 0) / 1_000_000_000)  # 纳秒转秒
        
        # 计算统计信息
        analysis = {
            "total_requests": len(traces),
            "performance_metrics": {
                "preprocessing": self._calculate_stats(preprocessing_times),
                "inference": self._calculate_stats(inference_times),
                "postprocessing": self._calculate_stats(postprocessing_times),
                "total": self._calculate_stats(total_times)
            },
            "bottlenecks": self._identify_bottlenecks({
                "preprocessing": preprocessing_times,
                "inference": inference_times,
                "postprocessing": postprocessing_times
            }),
            "recommendations": self._generate_recommendations({
                "preprocessing": preprocessing_times,
                "inference": inference_times,
                "postprocessing": postprocessing_times
            })
        }
        
        return analysis
    
    def _calculate_stats(self, times: List[float]) -> Dict:
        """计算统计信息"""
        if not times:
            return {}
        
        import numpy as np
        times_array = np.array(times)
        
        return {
            "count": len(times),
            "mean": float(np.mean(times_array)),
            "median": float(np.median(times_array)),
            "p95": float(np.percentile(times_array, 95)),
            "p99": float(np.percentile(times_array, 99)),
            "min": float(np.min(times_array)),
            "max": float(np.max(times_array)),
            "std": float(np.std(times_array))
        }
    
    def _identify_bottlenecks(self, timing_data: Dict) -> List[str]:
        """识别性能瓶颈"""
        bottlenecks = []
        
        # 计算各阶段平均耗时占比
        total_avg = sum(np.mean(times) for times in timing_data.values() if times)
        
        if total_avg > 0:
            for stage, times in timing_data.items():
                if times:
                    avg_time = np.mean(times)
                    percentage = (avg_time / total_avg) * 100
                    if percentage > 50:  # 如果某阶段占总时间超过50%
                        bottlenecks.append(f"{stage}阶段是主要瓶颈 ({percentage:.1f}%)")
        
        return bottlenecks
    
    def _generate_recommendations(self, timing_data: Dict) -> List[str]:
        """生成优化建议"""
        recommendations = []
        
        # 预处理优化建议
        if timing_data.get("preprocessing") and np.mean(timing_data["preprocessing"]) > 0.05:
            recommendations.append("预处理耗时较长，考虑优化分词算法或使用更快的tokenizer")
        
        # 推理优化建议
        if timing_data.get("inference") and np.mean(timing_data["inference"]) > 0.5:
            recommendations.append("模型推理时间较长，考虑模型量化、批处理或使用更高效的推理引擎")
        
        # 后处理优化建议
        if timing_data.get("postprocessing") and np.mean(timing_data["postprocessing"]) > 0.05:
            recommendations.append("后处理耗时较高，考虑优化解码算法或并行处理")
        
        # 一般建议
        recommendations.append("启用持续性能监控，建立性能基线")
        recommendations.append("实施自动扩缩容以应对负载变化")
        
        return recommendations

# 使用示例
async def main():
    instrumentation = AITracingInstrumentation()
    analyzer = AIPerformanceAnalyzer()
    
    # 模拟多个推理请求
    traces_collected = []
    
    for i in range(10):
        request_context = AIRequestContext(
            request_id=str(uuid.uuid4()),
            model_name="llama2-7b",
            user_id=f"user_{i}",
            prompt="Write a short story about AI observability"
        )
        
        # 执行带追踪的推理
        result = instrumentation.trace_model_inference(request_context, batch_size=1)
        traces_collected.append(result)
        
        print(f"Request {i+1} completed in {result.get('total_time', 0):.3f}s")
    
    # 分析性能
    performance_analysis = analyzer.analyze_inference_performance(traces_collected)
    print("\nPerformance Analysis:")
    print(json.dumps(performance_analysis, indent=2))

if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    asyncio.run(main())
```

### 3.2 Jaeger追踪配置

```yaml
# jaeger-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: jaeger-all-in-one
  namespace: observability
spec:
  replicas: 1
  selector:
    matchLabels:
      app: jaeger
  template:
    metadata:
      labels:
        app: jaeger
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "14269"
    spec:
      containers:
      - name: jaeger
        image: jaegertracing/all-in-one:1.42
        ports:
        - containerPort: 5775
          protocol: UDP
        - containerPort: 6831
          protocol: UDP
        - containerPort: 6832
          protocol: UDP
        - containerPort: 5778
          protocol: TCP
        - containerPort: 16686
          protocol: TCP
        - containerPort: 14250
          protocol: TCP
        - containerPort: 14268
          protocol: TCP
        - containerPort: 14269
          protocol: TCP
        - containerPort: 4317
          protocol: TCP
        - containerPort: 4318
          protocol: TCP
        env:
        - name: SPAN_STORAGE_TYPE
          value: badger
        - name: BADGER_EPHEMERAL
          value: "false"
        - name: BADGER_DIRECTORY_VALUE
          value: "/badger/data"
        - name: BADGER_DIRECTORY_KEY
          value: "/badger/key"
        - name: COLLECTOR_OTLP_ENABLED
          value: "true"
        - name: LOG_LEVEL
          value: info
        livenessProbe:
          httpGet:
            path: "/"
            port: 14269
          initialDelaySeconds: 5
        readinessProbe:
          httpGet:
            path: "/"
            port: 14269
          initialDelaySeconds: 1
        volumeMounts:
        - name: badger-data
          mountPath: /badger
      volumes:
      - name: badger-data
        persistentVolumeClaim:
          claimName: jaeger-pvc

---
apiVersion: v1
kind: Service
metadata:
  name: jaeger-query
  namespace: observability
spec:
  ports:
  - name: query-http
    port: 16686
    protocol: TCP
    targetPort: 16686
  selector:
    app: jaeger
  type: ClusterIP

---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: jaeger-pvc
  namespace: observability
spec:
  accessModes:
  - ReadWriteOnce
  resources:
    requests:
      storage: 50Gi
  storageClassName: fast-ssd
```

## 四、智能告警与自愈系统

### 4.1 机器学习驱动的异常检测

```python
# ml-anomaly-detection.py
import pandas as pd
import numpy as np
from sklearn.ensemble import IsolationForest
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import classification_report
import joblib
from typing import Dict, List, Tuple, Optional
import asyncio
import logging
from datetime import datetime, timedelta
import json

class MLAnomalyDetector:
    def __init__(self, model_name: str = "isolation_forest"):
        self.model_name = model_name
        self.model = None
        self.scaler = StandardScaler()
        self.feature_columns = []
        self.is_fitted = False
        self.logger = logging.getLogger(__name__)
        
    def prepare_features(self, metrics_data: pd.DataFrame) -> pd.DataFrame:
        """准备特征数据"""
        # 选择数值型指标
        numeric_columns = metrics_data.select_dtypes(include=[np.number]).columns.tolist()
        
        # 移除时间戳列
        feature_columns = [col for col in numeric_columns if col != 'timestamp']
        self.feature_columns = feature_columns
        
        # 提取特征
        features = metrics_data[feature_columns].copy()
        
        # 添加派生特征
        for col in feature_columns:
            if col.startswith(('cpu_', 'memory_', 'gpu_')):
                # 计算变化率
                features[f'{col}_rate'] = features[col].diff().fillna(0)
                # 计算移动平均
                features[f'{col}_ma_5'] = features[col].rolling(window=5, min_periods=1).mean()
                features[f'{col}_ma_15'] = features[col].rolling(window=15, min_periods=1).mean()
        
        return features
    
    def train(self, training_data: pd.DataFrame, contamination: float = 0.1) -> Dict:
        """训练异常检测模型"""
        try:
            # 准备特征
            features = self.prepare_features(training_data)
            
            # 标准化
            scaled_features = self.scaler.fit_transform(features)
            
            # 训练模型
            self.model = IsolationForest(
                contamination=contamination,
                random_state=42,
                n_estimators=100
            )
            self.model.fit(scaled_features)
            
            self.is_fitted = True
            
            # 评估模型
            predictions = self.model.predict(scaled_features)
            anomaly_rate = np.sum(predictions == -1) / len(predictions)
            
            return {
                "status": "success",
                "anomaly_rate": float(anomaly_rate),
                "training_samples": len(training_data),
                "features_used": len(self.feature_columns),
                "model_parameters": {
                    "contamination": contamination,
                    "n_estimators": 100
                }
            }
            
        except Exception as e:
            self.logger.error(f"Training failed: {e}")
            return {"status": "error", "message": str(e)}
    
    def predict(self, test_data: pd.DataFrame) -> Dict:
        """预测异常"""
        if not self.is_fitted:
            raise ValueError("Model not trained yet")
        
        try:
            # 准备特征
            features = self.prepare_features(test_data)
            
            # 标准化
            scaled_features = self.scaler.transform(features)
            
            # 预测
            predictions = self.model.predict(scaled_features)
            anomaly_scores = self.model.decision_function(scaled_features)
            
            # 返回结果
            results = []
            for i, (pred, score) in enumerate(zip(predictions, anomaly_scores)):
                results.append({
                    "index": i,
                    "is_anomaly": pred == -1,
                    "anomaly_score": float(score),
                    "confidence": float(abs(score)),
                    "timestamp": test_data.iloc[i]['timestamp'] if 'timestamp' in test_data.columns else None
                })
            
            return {
                "predictions": results,
                "anomaly_count": int(np.sum(predictions == -1)),
                "anomaly_percentage": float(np.mean(predictions == -1) * 100)
            }
            
        except Exception as e:
            self.logger.error(f"Prediction failed: {e}")
            return {"status": "error", "message": str(e)}
    
    def save_model(self, filepath: str) -> bool:
        """保存模型"""
        try:
            model_data = {
                'model': self.model,
                'scaler': self.scaler,
                'feature_columns': self.feature_columns,
                'is_fitted': self.is_fitted
            }
            joblib.dump(model_data, filepath)
            return True
        except Exception as e:
            self.logger.error(f"Failed to save model: {e}")
            return False
    
    def load_model(self, filepath: str) -> bool:
        """加载模型"""
        try:
            model_data = joblib.load(filepath)
            self.model = model_data['model']
            self.scaler = model_data['scaler']
            self.feature_columns = model_data['feature_columns']
            self.is_fitted = model_data['is_fitted']
            return True
        except Exception as e:
            self.logger.error(f"Failed to load model: {e}")
            return False

class AlertCorrelationEngine:
    def __init__(self):
        self.alert_history = []
        self.correlation_window = timedelta(hours=1)
        self.logger = logging.getLogger(__name__)
    
    def correlate_alerts(self, new_alerts: List[Dict]) -> List[Dict]:
        """关联告警"""
        correlated_alerts = []
        
        for alert in new_alerts:
            # 查找相关历史告警
            related_alerts = self._find_related_alerts(alert)
            
            if related_alerts:
                # 创建关联告警
                correlated_alert = {
                    "alert_id": alert.get("alert_id"),
                    "type": "correlated",
                    "primary_alert": alert,
                    "related_alerts": related_alerts,
                    "correlation_score": self._calculate_correlation_score(alert, related_alerts),
                    "root_cause_analysis": self._analyze_root_cause(alert, related_alerts),
                    "recommended_actions": self._suggest_actions(alert, related_alerts)
                }
                correlated_alerts.append(correlated_alert)
            else:
                # 单独告警
                correlated_alerts.append({
                    "alert_id": alert.get("alert_id"),
                    "type": "isolated",
                    "alert": alert
                })
        
        # 更新历史记录
        self.alert_history.extend(new_alerts)
        self._cleanup_old_alerts()
        
        return correlated_alerts
    
    def _find_related_alerts(self, target_alert: Dict) -> List[Dict]:
        """查找相关告警"""
        related = []
        target_time = datetime.fromisoformat(target_alert.get("timestamp", datetime.now().isoformat()))
        
        for historical_alert in self.alert_history:
            hist_time = datetime.fromisoformat(historical_alert.get("timestamp", datetime.now().isoformat()))
            
            # 时间窗口检查
            if abs((target_time - hist_time).total_seconds()) <= self.correlation_window.total_seconds():
                # 相关性检查
                if self._are_alerts_related(target_alert, historical_alert):
                    related.append(historical_alert)
        
        return related
    
    def _are_alerts_related(self, alert1: Dict, alert2: Dict) -> bool:
        """判断两个告警是否相关"""
        # 基于标签的相关性
        labels1 = set(alert1.get("labels", {}).keys())
        labels2 = set(alert2.get("labels", {}).keys())
        
        common_labels = labels1.intersection(labels2)
        if len(common_labels) >= 2:  # 至少有两个共同标签
            return True
        
        # 基于服务的相关性
        service1 = alert1.get("labels", {}).get("service")
        service2 = alert2.get("labels", {}).get("service")
        if service1 and service2 and service1 == service2:
            return True
        
        # 基于节点的相关性
        node1 = alert1.get("labels", {}).get("node")
        node2 = alert2.get("labels", {}).get("node")
        if node1 and node2 and node1 == node2:
            return True
        
        return False
    
    def _calculate_correlation_score(self, target_alert: Dict, related_alerts: List[Dict]) -> float:
        """计算关联分数"""
        if not related_alerts:
            return 0.0
        
        scores = []
        target_labels = set(target_alert.get("labels", {}).keys())
        
        for related_alert in related_alerts:
            related_labels = set(related_alert.get("labels", {}).keys())
            common_labels = len(target_labels.intersection(related_labels))
            total_labels = len(target_labels.union(related_labels))
            
            if total_labels > 0:
                jaccard_similarity = common_labels / total_labels
                scores.append(jaccard_similarity)
        
        return float(np.mean(scores)) if scores else 0.0
    
    def _analyze_root_cause(self, target_alert: Dict, related_alerts: List[Dict]) -> Dict:
        """分析根本原因"""
        analysis = {
            "primary_indicators": [],
            "supporting_evidence": [],
            "likely_root_causes": []
        }
        
        # 分析主要指标
        target_severity = target_alert.get("labels", {}).get("severity", "")
        if target_severity == "critical":
            analysis["primary_indicators"].append("关键级别告警")
        
        # 收集支持证据
        for related in related_alerts:
            evidence = {
                "alert": related.get("alertname", "Unknown"),
                "severity": related.get("labels", {}).get("severity", ""),
                "description": related.get("annotations", {}).get("description", "")
            }
            analysis["supporting_evidence"].append(evidence)
        
        # 推断可能的根本原因
        services_involved = set()
        for alert in [target_alert] + related_alerts:
            service = alert.get("labels", {}).get("service")
            if service:
                services_involved.add(service)
        
        if len(services_involved) == 1:
            analysis["likely_root_causes"].append(f"服务 {list(services_involved)[0]} 的问题")
        else:
            analysis["likely_root_causes"].append("基础设施层面的问题")
        
        return analysis
    
    def _suggest_actions(self, target_alert: Dict, related_alerts: List[Dict]) -> List[str]:
        """建议行动方案"""
        actions = []
        
        # 基于严重程度的建议
        severity = target_alert.get("labels", {}).get("severity", "")
        if severity == "critical":
            actions.append("立即调查并采取紧急措施")
            actions.append("通知相关团队负责人")
        
        # 基于告警类型的建议
        alert_name = target_alert.get("alertname", "")
        if "HighLatency" in alert_name:
            actions.append("检查网络连接和服务依赖")
        elif "HighErrorRate" in alert_name:
            actions.append("查看应用日志和错误堆栈")
        elif "HighCPULoad" in alert_name:
            actions.append("分析CPU使用模式和进程活动")
        
        # 基于关联告警的建议
        if related_alerts:
            actions.append("执行关联告警的综合分析")
            actions.append("检查共享资源的使用情况")
        
        return actions
    
    def _cleanup_old_alerts(self):
        """清理旧告警记录"""
        cutoff_time = datetime.now() - timedelta(days=7)
        self.alert_history = [
            alert for alert in self.alert_history
            if datetime.fromisoformat(alert.get("timestamp", datetime.now().isoformat())) > cutoff_time
        ]

# 使用示例
async def main():
    # 模拟监控数据
    timestamps = pd.date_range(start='2024-01-01', periods=1000, freq='5T')
    metrics_data = pd.DataFrame({
        'timestamp': timestamps,
        'cpu_utilization': np.random.normal(50, 15, 1000),
        'memory_utilization': np.random.normal(60, 20, 1000),
        'gpu_utilization': np.random.normal(70, 25, 1000),
        'request_latency': np.random.exponential(0.1, 1000),
        'error_rate': np.random.beta(1, 50, 1000)  # 低错误率
    })
    
    # 注入一些异常数据
    anomaly_indices = np.random.choice(1000, 20, replace=False)
    metrics_data.loc[anomaly_indices, 'cpu_utilization'] += 40
    metrics_data.loc[anomaly_indices, 'request_latency'] *= 5
    
    # 训练模型
    detector = MLAnomalyDetector()
    training_result = detector.train(metrics_data.head(800))
    print(f"Training result: {training_result}")
    
    # 预测异常
    test_data = metrics_data.tail(200)
    prediction_result = detector.predict(test_data)
    print(f"Anomalies detected: {prediction_result['anomaly_count']} "
          f"({prediction_result['anomaly_percentage']:.1f}%)")
    
    # 告警关联分析
    engine = AlertCorrelationEngine()
    
    sample_alerts = [
        {
            "alert_id": "alert_001",
            "alertname": "HighCPULoad",
            "labels": {"severity": "warning", "service": "llm-inference", "node": "node-01"},
            "annotations": {"description": "CPU使用率超过80%"},
            "timestamp": datetime.now().isoformat()
        },
        {
            "alert_id": "alert_002",
            "alertname": "HighLatency",
            "labels": {"severity": "warning", "service": "llm-inference", "node": "node-01"},
            "annotations": {"description": "请求延迟超过1秒"},
            "timestamp": (datetime.now() - timedelta(minutes=5)).isoformat()
        }
    ]
    
    correlated_results = engine.correlate_alerts(sample_alerts)
    print(f"\nCorrelated alerts: {len(correlated_results)}")

if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    asyncio.run(main())
```

## 五、生产级可观测性最佳实践