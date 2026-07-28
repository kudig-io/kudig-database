---
title: K8s 分布式追踪实践指南 (Jaeger / Tempo / OpenTelemetry)
description: '# K8s 分布式追踪实践指南 (Jaeger / Tempo / OpenTelemetry)'
summary: 'helm repo add open-telemetry https://open-telemetry.github.io/opentelemetry-helm-charts'
category: enterprise-monitoring-alerting
tags:
- k8s
- monitoring
- alerting
- prometheus
- grafana
- jaeger
- helm
- docker
- mysql
- elasticsearch
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 监控工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- K8s 分布式追踪实践指南 (Jaeger / Tempo / OpenTelemetry) 是什么
- 如何 K8s 分布式追踪实践指南 (Jaeger / Tempo / OpenTelemetry)
- Kubernetes 20 enterprise monitoring alerting 最佳实践
trigger_keywords:
- K8s
- 分布式追踪实践指南
- Jaeger
- Tempo
- OpenTelemetry
- enterprise
- monitoring
- alerting
prerequisites:
- kubectl-basics
- observability-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- mysql-basics
- logging-basics
- tracing-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/promql.md
  label: '速查卡: promql'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8s 分布式追踪实践指南 ([[jaeger|Jaeger]] / Tempo / [[opentelemetry|OpenTelemetry]])

> **适用版本**: Jaeger v1.65 / Grafana Tempo v2.7 / OpenTelemetry Collector v0.120  
> **最后更新**: 2026-04-24  
> **难度**: 中级

---

<!-- chunk: 📋 目录 -->## 📋 目录

- [一、可观测性三大支柱](#一可观测性三大支柱)
- [二、OpenTelemetry 架构](#二opentelemetry-架构)
- [三、OpenTelemetry Collector 部署](#三opentelemetry-collector-部署)
- [四、Jaeger 全链路追踪](#四jaeger-全链路追踪)
- [五、Grafana Tempo 轻量追踪](#五grafana-tempo-轻量追踪)
- [六、应用自动埋点](#六应用自动埋点)
- [七、追踪与日志关联](#七追踪与日志关联)
- [八、采样策略与成本控制](#八采样策略与成本控制)
- [九、选型对比](#九选型对比)

---

<!-- chunk: 一、可观测性三大支柱 -->## 一、可观测性三大支柱

```
可观测性栈
├── Metrics (指标)
│   ├── Prometheus (拉取)
│   ├── Grafana (可视化)
│   └── 问: 发生了什么? (WHAT)
│
├── Logs (日志)
│   ├── Loki / ELK / Fluentd
│   └── 问: 为什么发生? (WHY)
│
└── Traces (追踪)  ◄── 本指南重点
    ├── Jaeger / Tempo / Zipkin
    └── 问: 在哪里发生? (WHERE)
        └── 跨服务请求的完整调用链
```

## Trace 核心概念

| 概念 | 说明 | 类比 |
|:---|:---|:---|
| Trace | 一次完整请求的调用链 | 一封信的完整邮寄路径 |
| Span | 调用链中的一个操作单元 | 邮局的每个处理节点 |
| SpanContext | 传播上下文 (TraceID, SpanID) | 信封上的追踪条码 |
| Baggage | 跨 Span 传递的键值对 | 随信附带的备注 |

---

<!-- chunk: 二、OpenTelemetry 架构 -->## 二、OpenTelemetry 架构

```
OpenTelemetry (CNCF Graduated)
├── API / SDK (应用集成)
│   ├── Auto-Instrumentation (零代码)
│   └── Manual Instrumentation (自定义 Span)
│
├── Collector (统一收集)
│   ├── Receivers (接收: OTLP / Jaeger / Zipkin)
│   ├── Processors (处理: Batch / Memory Limit)
│   └── Exporters (导出: Jaeger / Tempo / Prometheus)
│
└── Protocol (OTLP)
    ├── gRPC (默认)
    └── HTTP/Protobuf
```

---

<!-- chunk: 三、OpenTelemetry Collector 部署 -->## 三、OpenTelemetry Collector 部署

## 3.1 Helm 安装

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
helm repo add open-telemetry https://open-telemetry.github.io/opentelemetry-helm-charts
helm install otel-collector open-telemetry/opentelemetry-collector \
  --namespace observability \
  --create-namespace \
  --set mode=deployment
```
## 3.2 生产级配置

```yaml
# values-otel-collector.yaml
mode: deployment
replicaCount: 2

resources:
  requests:
    cpu: 200m
    memory: 512Mi
  limits:
    cpu: 1000m
    memory: 2Gi

config:
  receivers:
    otlp:
      protocols:
        grpc:
          endpoint: 0.0.0.0:4317
        http:
          endpoint: 0.0.0.0:4318
    
    prometheus:
      config:
        scrape_configs:
        - job_name: 'otel-collector'
          scrape_interval: 10s
          static_configs:
          - targets: ['0.0.0.0:8888']
  
  processors:
    batch:
      timeout: 1s
      send_batch_size: 1024
    
    memory_limiter:
      limit_mib: 1500
      spike_limit_mib: 512
      check_interval: 5s
    
    resource:
      attributes:
      - key: k8s.cluster.name
        value: production
        action: upsert
      - key: environment
        value: production
        action: upsert
  
  exporters:
    # 导出到 Jaeger
    otlp/jaeger:
      endpoint: jaeger-collector.observability.svc.cluster.local:4317
      tls:
        insecure: true
    
    # 导出到 Tempo
    otlp/tempo:
      endpoint: tempo.observability.svc.cluster.local:4317
      tls:
        insecure: true
    
    # 导出到 Prometheus (Metrics)
    prometheusremotewrite:
      endpoint: http://prometheus.monitoring.svc.cluster.local:9090/api/v1/write
    
    # 调试输出
    logging:
      loglevel: warn
  
  service:
    pipelines:
      traces:
        receivers: [otlp]
        processors: [memory_limiter, resource, batch]
        exporters: [otlp/jaeger, otlp/tempo]
      
      metrics:
        receivers: [otlp, prometheus]
        processors: [memory_limiter, resource, batch]
        exporters: [prometheusremotewrite]
```

---

<!-- chunk: 四、Jaeger 全链路追踪 -->## 四、Jaeger 全链路追踪

## 4.1 部署

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
helm repo add jaegertracing https://jaegertracing.github.io/helm-charts
helm install jaeger jaegertracing/jaeger \
  --namespace observability \
  --create-namespace \
  --set provisionDataStore.cassandra=false \
  --set provisionDataStore.elasticsearch=true \
  --set storage.type=elasticsearch \
  --set elasticsearch.replicas=1
```
## 4.2 生产级配置 (使用外部存储)

```yaml
# values-jaeger.yaml
provisionDataStore:
  cassandra: false
  elasticsearch: false  # 使用外部 ES

storage:
  type: elasticsearch
  elasticsearch:
    serverUrls: http://elasticsearch.monitoring.svc.cluster.local:9200

agent:
  enabled: false  # 使用 OpenTelemetry Collector

collector:
  service:
    otlp:
      grpc:
        enabled: true
      http:
        enabled: true
  
  resources:
    requests:
      cpu: 500m
      memory: 1Gi
    limits:
      cpu: 2000m
      memory: 4Gi

query:
  enabled: true
  service:
    type: ClusterIP
  ingress:
    enabled: true
    hosts:
      - jaeger.example.com
```

## 4.3 访问 Jaeger UI

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl port-forward -n observability svc/jaeger-query 16686:16686
# 打开 http://localhost:16686
```
---

<!-- chunk: 五、Grafana Tempo 轻量追踪 -->## 五、Grafana Tempo 轻量追踪

## 5.1 部署

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
helm repo add grafana https://grafana.github.io/helm-charts
helm install tempo grafana/tempo \
  --namespace observability \
  --create-namespace \
  --set tempo.storage.trace.backend=local
```
## 5.2 生产级配置 (对象存储)

```yaml
# values-tempo.yaml
tempo:
  storage:
    trace:
      backend: s3
      s3:
        bucket: tempo-traces
        endpoint: s3.us-east-1.amazonaws.com
        region: us-east-1
        access_key: ${AWS_ACCESS_KEY_ID}
        secret_key: ${AWS_SECRET_ACCESS_KEY}
  
  # 保留策略
  compactor:
    compaction:
      block_retention: 168h  # 7 天
  
  # 资源限制
  resources:
    requests:
      cpu: 500m
      memory: 2Gi
    limits:
      cpu: 2000m
      memory: 8Gi

# Grafana 数据源配置
datasources:
  - name: Tempo
    type: tempo
    url: http://tempo.observability.svc.cluster.local:3100
    isDefault: false
```

## 5.3 Tempo 优势

| 特性 | Jaeger | Tempo |
|:---|:---|:---|
| 存储 | Elasticsearch / Cassandra | S3 / GCS / Azure Blob |
| 成本 | 高 (索引存储) | 低 (对象存储) |
| 依赖查询 | 内置 | 通过 Grafana + TraceQL |
| 告警 | 不支持 | 通过 Grafana |
| 学习曲线 | 低 | 中 (需 TraceQL) |

---

<!-- chunk: 六、应用自动埋点 -->## 六、应用自动埋点

## 6.1 Java (OpenTelemetry Agent)

```dockerfile
# Dockerfile
FROM openjdk:17-jdk
COPY --from=ghcr.io/open-telemetry/opentelemetry-java-instrumentation:latest \
  /opentelemetry-javaagent.jar /opt/opentelemetry-javaagent.jar
ENV JAVA_TOOL_OPTIONS="-javaagent:/opt/opentelemetry-javaagent.jar"
ENV OTEL_SERVICE_NAME=myapp
ENV OTEL_EXPORTER_OTLP_ENDPOINT=http://otel-collector.observability.svc.cluster.local:4317
COPY target/*.jar app.jar
ENTRYPOINT ["java", "-jar", "/app.jar"]
```

## 6.2 Node.js

```bash
npm install @opentelemetry/auto-instrumentations-node
```

```javascript
// tracing.js
const { NodeSDK } = require('@opentelemetry/sdk-node');
const { OTLPTraceExporter } = require('@opentelemetry/exporter-trace-otlp-grpc');
const { getNodeAutoInstrumentations } = require('@opentelemetry/auto-instrumentations-node');

const sdk = new NodeSDK({
  traceExporter: new OTLPTraceExporter({
    url: 'http://otel-collector.observability.svc.cluster.local:4317'
  }),
  instrumentations: [getNodeAutoInstrumentations()]
});

sdk.start();
```

## 6.3 Python

```bash
pip install opentelemetry-distro opentelemetry-exporter-otlp
opentelemetry-bootstrap -a install
```

```bash
# 启动应用
OTEL_SERVICE_NAME=myapp \
OTEL_EXPORTER_OTLP_ENDPOINT=http://otel-collector.observability.svc.cluster.local:4317 \
opentelemetry-instrument python app.py
```

## 6.4 Go (手动埋点)

```go
import (
    "go.opentelemetry.io/otel"
    "go.opentelemetry.io/otel/exporters/otlp/otlptrace/otlptracegrpc"
    "go.opentelemetry.io/otel/sdk/trace"
)

func initTracer() (*trace.TracerProvider, error) {
    exporter, err := otlptracegrpc.New(context.Background(),
        otlptracegrpc.WithEndpoint("otel-collector.observability.svc.cluster.local:4317"),
        otlptracegrpc.WithInsecure(),
    )
    if err != nil {
        return nil, err
    }
    
    tp := trace.NewTracerProvider(
        trace.WithBatcher(exporter),
    )
    otel.SetTracerProvider(tp)
    return tp, nil
}
```

---

<!-- chunk: 七、追踪与日志关联 -->## 七、追踪与日志关联

## 7.1 TraceID 注入日志

```python
# Python 示例
import logging
from opentelemetry import trace

tracer = trace.get_tracer(__name__)

class TraceIdFilter(logging.Filter):
    def filter(self, record):
        current_span = trace.get_current_span()
        record.trace_id = format(current_span.get_span_context().trace_id, '032x') if current_span else 'N/A'
        record.span_id = format(current_span.get_span_context().span_id, '016x') if current_span else 'N/A'
        return True

logging.basicConfig(
    format='%(asctime)s - %(name)s - %(levelname)s - trace_id=%(trace_id)s - %(message)s'
)
logger = logging.getLogger(__name__)
logger.addFilter(TraceIdFilter())
```

## 7.2 Loki 日志标签关联

```yaml
# Promtail 配置
scrape_configs:
- job_name: kubernetes-pods
  pipeline_stages:
  - json:
      expressions:
        trace_id: trace_id
  - labels:
      trace_id:
```

---

<!-- chunk: 八、采样策略与成本控制 -->## 八、采样策略与成本控制

## 8.1 Head-based 采样 (Collector 端)

```yaml
processors:
  probabilistic_sampler:
    sampling_percentage: 10.0  # 10% 采样
  
  tail_sampling:
    decision_wait: 10s
    num_traces: 100000
    expected_new_traces_per_sec: 1000
    policies:
      - name: errors
        type: status_code
        status_code: {status_codes: [ERROR]}
      - name: slow_requests
        type: latency
        latency: {threshold_ms: 1000}
```

## 8.2 采样策略对比

| 策略 | 实现位置 | 优点 | 缺点 |
|:---|:---|:---|:---|
| Head-based | SDK/Agent | 简单、低开销 | 无法基于结果采样 |
| Tail-based | Collector | 可基于错误/延迟采样 | 内存开销大 |
| Adaptive | Collector | 动态调整采样率 | 配置复杂 |

## 8.3 成本控制

```
追踪成本 ≈ 存储成本 + 网络成本 + 计算成本

优化手段:
1. 合理采样率 (生产 1-10%, 开发 100%)
2. 短保留周期 (7天 vs 30天)
3. 对象存储 (Tempo vs Jaeger+ES)
4. 丢弃健康 Span (仅保留异常)
5. 压缩 (gzip OTLP)
```

---

<!-- chunk: 九、选型对比 -->## 九、选型对比

| 维度 | Jaeger | Tempo | Zipkin |
|:---|:---|:---|:---|
| **CNCF 状态** | Graduated | 非 CNCF | Incubating |
| **存储后端** | ES/Cassandra/Badger | S3/GCS/Azure | ES/MySQL/Cassandra |
| **查询语言** | 原生 UI | TraceQL (Grafana) | 原生 UI |
| **依赖分析** | 内置 | Grafana Tempo + 插件 | 内置 |
| **告警** | 不支持 | Grafana 告警 | 不支持 |
| **多租户** | 有限 | 支持 | 有限 |
| **存储成本** | 高 | 低 | 中 |
| **推荐场景** | 全功能追踪 | 成本敏感 / Grafana 生态 | 简单场景 / Spring |

## 推荐架构

```
应用 (Auto-Instrumentation)
    |
    ├── OTLP/gRPC ──► OpenTelemetry Collector
    |                       |
    |                       ├── traces ──► Tempo (低成本长期存储)
    |                       ├── traces ──► Jaeger (实时查询)
    |                       └── metrics ──► Prometheus
    |
    └── Logs (trace_id) ──► Loki
                                |
                                └── Grafana (统一查询: Metrics + Logs + Traces)
```

---

<!-- chunk: 参考链接 -->## 参考链接

- [OpenTelemetry 官方](https://opentelemetry.io/)
- [Jaeger 文档](https://www.jaegertracing.io/docs/)
- [Grafana Tempo 文档](https://grafana.com/docs/tempo/)
- [OpenTelemetry Collector](https://opentelemetry.io/docs/collector/)
- [TraceQL 查询语言](https://grafana.com/docs/tempo/latest/traceql/)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-20-enterprise-monitoring-alerting MOC
- [[09-可观测性/README.md|Domain 06: 企业级监控与告警 (Enterprise Monitoring & Alerting)]]
- [[09-可观测性/01-总览/00-open-source-projects-index.md|Domain-20 企业监控与告警 — 开源项目索引]]
- Prometheus企业级监控系统深度实践
- Grafana Enterprise Observability Platform 深度实践
- OpenTelemetry分布式追踪与可观测性深度实践
- Thanos Enterprise Metrics Federation and Long-term Storage
- Datadog企业级APM深度实践
- Datadog 企业级监控平台深度实践
- Elastic Stack企业级日志分析深度实践
- Elastic Stack企业级可观测性平台深度实践
- Zabbix Enterprise Monitoring Platform 深度实践

## See Also

- 07-zabbix-enterprise-monitoring
- 08-new-relic-enterprise-apm
- 99-prometheus-enterprise-guide
- 01-prometheus-enterprise-monitoring

- [[09-可观测性/README.md|返回目录]]

## Related

- [[21-生态参考/03-领域索引/observability-index.md|Observability 可观测性知识图谱索引]]

```

<!-- risk-assessed -->
