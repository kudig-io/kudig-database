---
title: "AI 可观测性平台"
description: "K8s 上 AI 可观测性平台部署运维：Arize Phoenix、Langfuse、OpenTelemetry for LLM、模型质量监控与 GPU 性能关联"
summary: "覆盖 AI 可观测性与传统可观测性差异，Arize Phoenix 部署（tracing/evaluation/embedding drift），Langfuse K8s 运维，OpenTelemetry LLM 追踪，模型质量监控（drift/hallucination），GPU 利用率关联分析及告警设计"
category: AI基础设施
tags:
- observability
- arize-phoenix
- langfuse
- opentelemetry
- llm-tracing
- model-monitoring
- drift-detection
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
- "Arize Phoenix 怎么在 K8s 上部署"
- "LLM 调用链路追踪怎么做"
- "如何监控模型质量漂移"
trigger_keywords:
- arize-phoenix
- langfuse
- opentelemetry
- llm-tracing
- observability
- drift-detection
- model-monitoring
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

# AI 可观测性平台

## 概述

AI 应用的可观测性与传统微服务有本质区别。传统可观测性关注请求延迟、错误率和吞吐量（RED 指标），而 AI 应用还需要追踪 Token 用量、模型输出质量、语义漂移、幻觉率、检索准确率等 AI 特有指标。一次 LLM 调用可能涉及多步推理、工具调用、RAG 检索，其调用链路远比 REST API 复杂。

本文覆盖 AI 可观测性的核心概念、Arize Phoenix 和 Langfuse 的 K8s 生产部署、OpenTelemetry for LLM 追踪集成、模型质量监控（drift detection / hallucination rate）、GPU 利用率与推理性能关联分析、告警规则设计以及故障排查。

相关页面：[[Agent可观测性]]、[[Prometheus监控体系]]、[[LLM Gateway与推理路由]]、[[vLLM推理引擎部署]]、[[GPU调度与资源管理]]

## 架构与核心概念

### AI 可观测性 vs 传统可观测性

| 维度 | 传统可观测性 | AI 可观测性 |
|------|------------|------------|
| 核心指标 | 延迟/错误率/吞吐量 | Token 用量/成本/质量/漂移 |
| 追踪粒度 | HTTP 请求 Span | LLM 调用 + Tool + Retrieval + Agent 步骤 |
| 日志内容 | 结构化事件 | Prompt/Completion 全文（含 PII 风险） |
| 质量评估 | 功能正确性（pass/fail） | 语义质量（相关性/准确性/幻觉） |
| 漂移检测 | 不适用 | Embedding drift / 输出分布变化 |
| 成本追踪 | 基础设施成本 | Token 级成本（input/output 分别计费） |
| 告警触发 | 错误率/延迟阈值 | 质量分数下降/成本异常/漂移超阈值 |
| 数据量 | 中等（结构化指标） | 大（Prompt 文本 + Embedding 向量） |

### AI 可观测性数据模型

```
AI 可观测性三层追踪模型:

Trace（追踪）:
  - 一次完整的用户请求处理
  - 包含多个 Span
  - 元数据: user_id, session_id, model, total_tokens, cost

Span（跨度）:
  - 一个处理步骤
  - 类型: LLM / Retrieval / Tool / Agent / Chain
  - 属性: input, output, model, tokens, latency, status

Generation（生成，LLM 特有）:
  - 一次 LLM API 调用
  - 属性: model, prompt, completion, input_tokens, output_tokens
  - 指标: latency, cost, quality_score

Evaluation（评估）:
  - 对输出质量的自动/人工评分
  - 维度: relevance, faithfulness, hallucination, toxicity
  - 方法: LLM-as-judge, 规则匹配, 人工标注

Embedding（嵌入）:
  - 文本的向量表示
  - 用于 drift detection 和相似度分析
  - 存储: 向量数据库或专用存储
```

### 平台架构

```
AI 可观测性平台架构:

数据采集层:
  - OpenTelemetry SDK（自动/手动 instrumentation）
  - Langfuse SDK（Python/JS）
  - OpenInference（Arize 标准）
  - 自定义 middleware（FastAPI/Flask）

数据处理层:
  - OTel Collector（接收、转换、路由）
  - 异步写入（避免影响主链路延迟）
  - PII 脱敏（Prompt 中的敏感信息）
  - 采样策略（高流量时按比例采样）

存储层:
  - Traces: ClickHouse / PostgreSQL（Langfuse）
  - Metrics: Prometheus / VictoriaMetrics
  - Embeddings: 向量数据库（Phoenix 内置）
  - Logs: Loki / Elasticsearch

展示层:
  - Phoenix UI（Trace 查看、Evaluation、Drift）
  - Langfuse Dashboard（成本、延迟、质量）
  - Grafana（指标可视化、告警）
  - 自定义报表（成本归因、质量趋势）
```

## 生产部署

### Arize Phoenix 部署

```yaml
# 🟡 中风险：Arize Phoenix K8s 部署
apiVersion: apps/v1
kind: Deployment
metadata:
  name: arize-phoenix
  namespace: ai-observability
  labels:
    app: arize-phoenix
spec:
  replicas: 2
  selector:
    matchLabels:
      app: arize-phoenix
  template:
    metadata:
      labels:
        app: arize-phoenix
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "6006"
    spec:
      containers:
      - name: phoenix
        image: arizephoenix/phoenix:7.12.0
        ports:
        - containerPort: 6006
          name: http
        - containerPort: 4317
          name: otel-grpc
        - containerPort: 4318
          name: otel-http
        env:
        - name: PHOENIX_SQL_DATABASE_URL
          valueFrom:
            secretKeyRef:
              name: phoenix-secrets
              key: database-url
        - name: PHOENIX_WORKING_DIR
          value: /data/phoenix
        - name: PHOENIX_OTEL_COLLECTOR_ENDPOINT
          value: "http://otel-collector.ai-observability.svc:4317"
        - name: PHOENIX_ENABLE_AUTH
          value: "true"
        - name: PHOENIX_SECRET
          valueFrom:
            secretKeyRef:
              name: phoenix-secrets
              key: secret
        resources:
          requests:
            cpu: "4"
            memory: "16Gi"
          limits:
            cpu: "8"
            memory: "32Gi"
        volumeMounts:
        - name: phoenix-data
          mountPath: /data/phoenix
        livenessProbe:
          httpGet:
            path: /healthz
            port: 6006
          initialDelaySeconds: 15
          periodSeconds: 20
        readinessProbe:
          httpGet:
            path: /readyz
            port: 6006
          initialDelaySeconds: 10
          periodSeconds: 10
      volumes:
      - name: phoenix-data
        persistentVolumeClaim:
          claimName: phoenix-data-pvc
---
apiVersion: v1
kind: Service
metadata:
  name: arize-phoenix
  namespace: ai-observability
spec:
  selector:
    app: arize-phoenix
  ports:
  - name: http
    port: 6006
    targetPort: 6006
  - name: otel-grpc
    port: 4317
    targetPort: 4317
  - name: otel-http
    port: 4318
    targetPort: 4318
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: phoenix-data-pvc
  namespace: ai-observability
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: gp3-encrypted
  resources:
    requests:
      storage: 200Gi
```

### Langfuse K8s 部署

```yaml
# 🟡 中风险：Langfuse 生产部署（Helm）
# langfuse-values.yaml
# 使用 Helm Chart: https://github.com/langfuse/langfuse-k8s
langfuse:
  replicas: 2
  image:
    repository: langfuse/langfuse
    tag: "2.93.0"
  resources:
    requests:
      cpu: "2"
      memory: "4Gi"
    limits:
      cpu: "4"
      memory: "8Gi"
  env:
    DATABASE_URL: "postgresql://langfuse:password@postgres-rw.ai-observability.svc:5432/langfuse"
    NEXTAUTH_SECRET: "${LANGFUSE_NEXTAUTH_SECRET}"
    SALT: "${LANGFUSE_SALT}"
    NEXTAUTH_URL: "https://langfuse.internal.company.com"
    TELEMETRY_ENABLED: "false"
    LANGFUSE_ENABLE_EXPERIMENTAL_FEATURES: "false"
    # S3 存储（大型 trace 数据）
    LANGFUSE_S3_EVENT_UPLOAD_BUCKET: "langfuse-events"
    LANGFUSE_S3_EVENT_UPLOAD_REGION: "us-east-1"
    LANGFUSE_S3_EVENT_UPLOAD_ACCESS_KEY_ID: "${S3_ACCESS_KEY}"
    LANGFUSE_S3_EVENT_UPLOAD_SECRET_ACCESS_KEY: "${S3_SECRET_KEY}"
    LANGFUSE_S3_EVENT_UPLOAD_ENDPOINT: "https://s3.us-east-1.amazonaws.com"

postgresql:
  enabled: false  # 使用外部 CloudNativePG

clickhouse:
  enabled: true
  shards: 2
  replicaCount: 2
  persistence:
    size: 100Gi
  resources:
    requests:
      cpu: "4"
      memory: "16Gi"

redis:
  enabled: true
  architecture: standalone
  auth:
    enabled: true
```

```bash
# 🟡 中风险：安装 Langfuse
helm repo add langfuse https://langfuse.github.io/langfuse-k8s
helm repo update

helm install langfuse langfuse/langfuse \
  --namespace ai-observability \
  --create-namespace \
  -f langfuse-values.yaml \
  --wait --timeout 300s

# 验证
kubectl get pods -n ai-observability -l app=langfuse
kubectl logs -n ai-observability -l app=langfuse --tail=20
```

### OpenTelemetry Collector 配置

```yaml
# 🟡 中风险：OTel Collector 配置（接收 LLM trace 并路由）
apiVersion: v1
kind: ConfigMap
metadata:
  name: otel-collector-config
  namespace: ai-observability
data:
  config.yaml: |
    receivers:
      otlp:
        protocols:
          grpc:
            endpoint: 0.0.0.0:4317
          http:
            endpoint: 0.0.0.0:4318

    processors:
      batch:
        timeout: 5s
        send_batch_size: 1024
      memory_limiter:
        check_interval: 1s
        limit_mib: 2048
        spike_limit_mib: 512
      # PII 脱敏（移除 prompt 中的敏感信息）
      attributes/redact:
        actions:
        - key: llm.prompt
          action: hash
        - key: user.email
          action: delete
      # 采样（高流量时 10% 采样）
      probabilistic_sampler:
        sampling_percentage: 10

    exporters:
      # Trace 数据发送到 Phoenix
      otlp/phoenix:
        endpoint: arize-phoenix.ai-observability.svc:4317
        tls:
          insecure: true
      # Trace 数据发送到 Langfuse
      otlp/langfuse:
        endpoint: langfuse.ai-observability.svc:4318
        headers:
          Authorization: "Basic ${LANGFUSE_OTEL_AUTH}"
      # 指标发送到 Prometheus
      prometheus:
        endpoint: 0.0.0.0:8889
      # 日志发送到 Loki
      loki:
        endpoint: http://loki.monitoring.svc:3100/loki/api/v1/push

    service:
      pipelines:
        traces:
          receivers: [otlp]
          processors: [memory_limiter, attributes/redact, probabilistic_sampler, batch]
          exporters: [otlp/phoenix, otlp/langfuse]
        metrics:
          receivers: [otlp]
          processors: [memory_limiter, batch]
          exporters: [prometheus]
        logs:
          receivers: [otlp]
          processors: [memory_limiter, batch]
          exporters: [loki]
```

### 应用端 Instrumentation

```python
# 🟢 低风险：Python 应用集成 OpenTelemetry + Langfuse 示例
# 使用 OpenInference 标准（兼容 Phoenix 和 Langfuse）

from opentelemetry import trace
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import BatchSpanProcessor
from opentelemetry.exporter.otlp.proto.grpc.trace_exporter import OTLPSpanExporter
from openinference.instrumentation.openai import OpenAIInstrumentor

# 配置 Tracer
tracer_provider = TracerProvider()
tracer_provider.add_span_processor(
    BatchSpanProcessor(
        OTLPSpanExporter(endpoint="http://otel-collector.ai-observability.svc:4317")
    )
)
trace.set_tracer_provider(tracer_provider)

# 自动 instrument OpenAI 调用
OpenAIInstrumentor().instrument()

# 手动追踪 RAG 检索
tracer = trace.get_tracer("rag-pipeline")

@tracer.start_as_current_span("retrieve_documents")
def retrieve_documents(query: str):
    span = trace.get_current_span()
    span.set_attribute("retrieval.query", query)
    span.set_attribute("retrieval.top_k", 5)
    # ... 检索逻辑
    span.set_attribute("retrieval.num_results", len(results))
    return results
```

## 运维操作

### 模型质量监控

```bash
# 🟢 低风险：查看 Phoenix 中的 Evaluation 结果
PHOENIX_URL="http://arize-phoenix.ai-observability.svc:6006"

# 查看项目列表
curl -s "${PHOENIX_URL}/v1/projects" -H "Authorization: Bearer ${PHOENIX_API_KEY}" | jq .

# 查看最近的 trace 和质量评分
curl -s "${PHOENIX_URL}/v1/projects/default/traces?limit=20" \
  -H "Authorization: Bearer ${PHOENIX_API_KEY}" | \
  jq '.data[] | {trace_id, latency_ms, status, evaluations}'

# 🟢 低风险：Langfuse 质量指标查询
LANGFUSE_URL="http://langfuse.ai-observability.svc:3000"

# 查看评分统计
curl -s "${LANGFUSE_URL}/api/public/scores?name=hallucination&fromTimestamp=2026-07-12" \
  -H "Authorization: Bearer ${LANGFUSE_PUBLIC_KEY}:${LANGFUSE_SECRET_KEY}" | \
  jq '.data | group_by(.value) | map({score: .[0].value, count: length})'
```

### Drift Detection 配置

```yaml
# 🟡 中风险：Phoenix Embedding Drift 监控配置
# 通过 Phoenix Python SDK 配置 drift 检测
apiVersion: batch/v1
kind: CronJob
metadata:
  name: embedding-drift-check
  namespace: ai-observability
spec:
  schedule: "0 */6 * * *"  # 每 6 小时检查一次
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: drift-checker
            image: registry.internal/ai/observability-tools:latest
            command:
            - python
            - drift_check.py
            - --phoenix-endpoint=http://arize-phoenix.ai-observability.svc:6006
            - --project-name=production-rag
            - --baseline-window=7d
            - --current-window=6h
            - --drift-threshold=0.15
            - --alert-webhook=https://hooks.slack.com/services/xxx
            resources:
              requests:
                cpu: "2"
                memory: "8Gi"
            env:
            - name: PHOENIX_API_KEY
              valueFrom:
                secretKeyRef:
                  name: phoenix-secrets
                  key: api-key
          restartPolicy: Never
```

### GPU 利用率与推理性能关联

```bash
# 🟢 低风险：关联 GPU 指标与推理延迟
# 查询 Prometheus：GPU 利用率 vs 推理 P95 延迟
# PromQL:
# GPU 利用率
# avg(nvidia_gpu_utilization{namespace="ai-inference"}) by (pod)
# 推理 P95 延迟
# histogram_quantile(0.95, rate(vllm_request_duration_seconds_bucket[5m]))

# 查看 DCGM 指标
kubectl get --raw /apis/custom.metrics.k8s.io/v1beta1/namespaces/ai-inference/pods/*/DCGM_FI_DEV_GPU_UTIL 2>/dev/null | jq .

# 🟢 低风险：检查推理服务 GPU 状态
kubectl exec -n ai-inference deploy/vllm-llama70b -- nvidia-smi --query-gpu=utilization.gpu,memory.used,temperature.gpu,power.draw --format=csv -l 10
```

### 告警规则设计

```yaml
# 🟢 低风险：AI 可观测性告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ai-observability-alerts
  namespace: monitoring
spec:
  groups:
  - name: ai-quality.rules
    rules:
    - alert: LLMHallucinationRateHigh
      expr: |
        avg_over_time(llm_evaluation_score{name="hallucination", direction="lower_better"}[1h]) > 0.15
      for: 30m
      labels:
        severity: warning
      annotations:
        summary: "LLM 幻觉率超过 15%（过去 1 小时平均）"
        runbook: "检查最近模型版本变更、prompt 模板变更、RAG 检索质量"

    - alert: EmbeddingDriftDetected
      expr: embedding_drift_score{project="production-rag"} > 0.15
      for: 1h
      labels:
        severity: warning
      annotations:
        summary: "Embedding 分布漂移超过阈值"
        runbook: "检查输入数据分布变化、是否需要更新向量索引"

    - alert: LLMCostAnomaly
      expr: |
        sum(rate(llm_token_cost_total[1h])) > 2 * sum(rate(llm_token_cost_total[1h] offset 1d))
      for: 30m
      labels:
        severity: warning
      annotations:
        summary: "LLM Token 成本异常（超过昨日同期 2 倍）"

    - alert: TraceLossRateHigh
      expr: |
        rate(otel_collector_export_send_failed_spans[5m]) / rate(otel_collector_export_sent_spans[5m]) > 0.05
      for: 10m
      labels:
        severity: critical
      annotations:
        summary: "Trace 丢失率超过 5%"

    - alert: PhoenixStorageNearFull
      expr: |
        kubelet_volume_stats_used_bytes{persistentvolumeclaim="phoenix-data-pvc"} /
        kubelet_volume_stats_capacity_bytes{persistentvolumeclaim="phoenix-data-pvc"} > 0.85
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "Phoenix 存储使用超过 85%"
```

## 故障排查

### Trace 丢失

```bash
# 🟢 低风险：诊断 Trace 丢失
# Step 1: 检查 OTel Collector 状态
kubectl get pods -n ai-observability -l app=otel-collector
kubectl logs -n ai-observability -l app=otel-collector --tail=50 | grep -i "error\|drop\|reject"

# Step 2: 检查 Collector 指标
kubectl exec -n ai-observability deploy/otel-collector -- \
  curl -s http://localhost:8888/metrics | grep -E "otelcol_exporter_sent|otelcol_exporter_send_failed|otelcol_processor_dropped"

# Step 3: 检查后端存储连通性
kubectl exec -n ai-observability deploy/otel-collector -- \
  curl -s -o /dev/null -w "%{http_code}" http://arize-phoenix.ai-observability.svc:6006/healthz

# Step 4: 检查应用端 SDK 配置
# 确认 OTEL_EXPORTER_OTLP_ENDPOINT 指向正确的 Collector 地址
# 确认采样率配置（避免 100% 采样导致 Collector 过载）

# 常见原因:
# 1. Collector 内存不足 → 增加 memory_limiter 或 Pod 内存
# 2. 后端不可达 → 检查 Service/DNS/NetworkPolicy
# 3. 采样率过低 → 调整 probabilistic_sampler 百分比
# 4. 应用端 BatchSpanProcessor 队列满 → 增加 max_queue_size
```

### 指标延迟

```bash
# 🟢 低风险：诊断指标延迟
# 检查 Phoenix 查询延迟
time curl -s "${PHOENIX_URL}/v1/projects/default/traces?limit=10" \
  -H "Authorization: Bearer ${PHOENIX_API_KEY}"

# 检查 ClickHouse（Langfuse 后端）性能
kubectl exec -n ai-observability statefulset/clickhouse -- \
  clickhouse-client --query "SELECT count() FROM traces WHERE timestamp > now() - INTERVAL 1 HOUR"

# 检查 PostgreSQL 连接数
kubectl exec -n ai-observability statefulset/postgres -- \
  psql -U langfuse -c "SELECT count(*) FROM pg_stat_activity WHERE state = 'active';"

# 解决方案:
# 1. ClickHouse 合并延迟 → 检查 merge 队列、增加资源
# 2. PostgreSQL 连接池耗尽 → 增加 max_connections 或 PgBouncer
# 3. Phoenix 索引膨胀 → 定期 VACUUM / 重建索引
```

### 存储膨胀

| 故障现象 | 可能原因 | 排查命令 | 解决方案 |
|---------|---------|---------|---------|
| PVC 使用 > 85% | Trace 数据未设置 TTL | `du -sh /data/phoenix/*` | 配置数据保留策略（如 30 天） |
| ClickHouse 磁盘满 | 大量 Prompt 全文存储 | `SELECT sum(bytes_on_disk) FROM system.parts` | 启用 TTL、压缩、S3 分层 |
| 查询变慢 | 数据量过大索引效率低 | `EXPLAIN ANALYZE` 慢查询 | 分区表、增加副本、归档旧数据 |
| 内存 OOM | Embedding 向量全部加载 | `kubectl top pod` | 限制内存中向量数量、使用 mmap |
| 日志膨胀 | Prompt/Completion 全文记录 | 检查日志大小 | PII 脱敏 + 截断 + 采样 |

```bash
# 🔴 高风险：清理过期 Trace 数据（不可逆）
# Phoenix: 清理 30 天前的 trace
kubectl exec -n ai-observability deploy/arize-phoenix -- python3 -c "
import phoenix as px
from datetime import datetime, timedelta
client = px.Client(endpoint='http://localhost:6006')
cutoff = datetime.now() - timedelta(days=30)
# 使用 Phoenix API 清理
print(f'Cleaning traces before {cutoff}')
"

# 🔴 高风险：ClickHouse 手动合并和清理
kubectl exec -n ai-observability statefulset/clickhouse -- \
  clickhouse-client --query "
    ALTER TABLE traces DELETE WHERE timestamp < now() - INTERVAL 30 DAY;
    OPTIMIZE TABLE traces FINAL;
  "
```

## 最佳实践

### 数据保留策略

1. **Trace 数据**：热数据 7 天（SSD），温数据 30 天（HDD/S3），冷数据 90 天（归档）
2. **Prompt 全文**：仅保留 7 天（PII 风险），之后只保留 metadata 和评分
3. **Embedding 向量**：保留最新 baseline（用于 drift 对比），历史向量按周聚合
4. **指标数据**：15s 粒度保留 7 天，1min 粒度保留 30 天，1h 粒度保留 1 年

### 采样策略

```
生产环境采样建议:

高流量服务 (>1000 RPM):
  - 正常请求: 10% 采样
  - 错误请求: 100% 采样
  - 高延迟请求 (>P95): 100% 采样
  - 新模型/新版本: 100% 采样（灰度期间）

中流量服务 (100-1000 RPM):
  - 正常请求: 50% 采样
  - 错误/高延迟: 100%

低流量服务 (<100 RPM):
  - 全量采集: 100%

Tail-based Sampling:
  - 在 OTel Collector 配置 tail sampling
  - 保留所有包含 error 或高延迟的 trace
  - 丢弃正常完成的低价值 trace
```

### 安全与合规

1. **PII 脱敏**：在 OTel Collector 层对 Prompt/Completion 进行脱敏或哈希
2. **访问控制**：Phoenix/Langfuse 启用认证（OIDC 集成企业 SSO）
3. **数据隔离**：多团队使用不同 Project，RBAC 控制可见性
4. **审计**：记录谁在何时查看了哪些 Trace（含 Prompt 内容）
5. **合规**：GDPR 场景下 Prompt 数据保留不超过 7 天，支持用户数据删除

### 与现有监控体系集成

- **Grafana Dashboard**：将 LLM 指标（Token/cost/latency/quality）与基础设施指标（GPU/CPU/Network）统一展示
- **PagerDuty/Slack 告警**：质量下降、成本异常、Trace 丢失等告警接入现有 On-call 流程
- **CI/CD 集成**：模型上线前自动运行 Evaluation，质量分数低于阈值阻止发布
- **成本报表**：按团队/项目/模型维度生成周/月成本报表，接入 FinOps 流程

## Related

- [[Agent可观测性]]
- [[Prometheus监控体系]]
- [[LLM Gateway与推理路由]]
- [[vLLM推理引擎部署]]
- [[GPU调度与资源管理]]
