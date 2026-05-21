---
title: OpenTelemetry
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- prometheus
- jaeger
- job
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- OpenTelemetry 是什么
- 如何 OpenTelemetry
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- OpenTelemetry
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- logging-basics
- tracing-basics
- observability-basics
---

title: OpenTelemetry
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- jaeger
- job
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- OpenTelemetry 是什么
- 如何 OpenTelemetry
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- OpenTelemetry
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# OpenTelemetry

> **成熟度**: Incubating | **加入时间**: 2019-05 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://opentelemetry.io |
| **GitHub** | https://github.com/open-telemetry |
| **文档** | https://opentelemetry.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go, Java, Python, JS 等 |
| **CNCF 分类** | Observability |

---

## 项目概述

### 简介
OpenTelemetry (OTel) 是 CNCF 的可观测性框架，提供统一的 API、SDK 和工具来采集、处理和导出遥测数据（Traces、Metrics、Logs）。它是 OpenTracing 和 OpenCensus 项目的合并继承者。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2019-05 | OpenTracing + OpenCensus 合并成立 |
| 2019-05 | 加入 CNCF Sandbox |
| 2021-02 | 晋升为 CNCF Incubating |
| 2021-02 | Tracing API/SDK 达到 1.0 稳定 |
| 2023 | Metrics 和 Logs 达到稳定 |

### 核心定位
OpenTelemetry 是云原生可观测性的统一标准，解决了遥测数据采集的碎片化问题，是构建可观测性平台的基础。

---

## 架构设计

### 三大信号 (Three Pillars)

```
┌─────────────────────────────────────────────────────────────────┐
│                 OpenTelemetry 三大信号                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                      Traces (追踪)                          ││
│  │                                                              ││
│  │   Request ────► Service A ────► Service B ────► Database    ││
│  │      │            │                │                │       ││
│  │      ▼            ▼                ▼                ▼       ││
│  │   [Span 1]     [Span 2]         [Span 3]        [Span 4]    ││
│  │      └────────────┴────────────────┴────────────────┘       ││
│  │                      Trace ID: abc123                        ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                     Metrics (指标)                          ││
│  │                                                              ││
│  │   Counter:    http_requests_total{method="GET"} = 1000     ││
│  │   Gauge:      memory_usage_bytes = 512000000               ││
│  │   Histogram:  http_request_duration_seconds{le="0.1"} = 95  ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                      Logs (日志)                            ││
│  │                                                              ││
│  │   {"timestamp": "...", "severity": "ERROR",                 ││
│  │    "body": "Connection failed", "trace_id": "abc123",       ││
│  │    "span_id": "def456", "attributes": {...}}                ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 数据流架构

```
┌─────────────────────────────────────────────────────────────────┐
│                   OpenTelemetry 数据流                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  应用程序                                                        │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                                                              ││
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐     ││
│  │  │   OTel API  │───►│  OTel SDK   │───►│  Exporter   │     ││
│  │  │  (接口定义) │    │ (实现+处理) │    │ (数据导出)  │     ││
│  │  └─────────────┘    └─────────────┘    └─────────────┘     ││
│  │                                               │              ││
│  └───────────────────────────────────────────────┼──────────────┘│
│                                                  │               │
│                                                  ▼               │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                   OTel Collector (可选)                      ││
│  │  ┌───────────┐    ┌───────────┐    ┌───────────┐           ││
│  │  │ Receivers │───►│ Processors│───►│ Exporters │           ││
│  │  │           │    │           │    │           │           ││
│  │  │ • OTLP    │    │ • Batch   │    │ • OTLP    │           ││
│  │  │ • Jaeger  │    │ • Filter  │    │ • Jaeger  │           ││
│  │  │ • Prometheus│   │ • Transform│  │ • Prometheus│          ││
│  │  └───────────┘    └───────────┘    └───────────┘           ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                  │               │
│                   ┌──────────────────────────────┼──────────────┐│
│                   ▼                              ▼              ▼│
│            ┌───────────┐               ┌───────────┐    ┌───────────┐
│            │  Jaeger   │               │Prometheus │    │   Loki    │
│            │ (Traces)  │               │ (Metrics) │    │  (Logs)   │
│            └───────────┘               └───────────┘    └───────────┘
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 使用示例

### Go SDK

```go
package main

import (
    "context"
    "go.opentelemetry.io/otel"
    "go.opentelemetry.io/otel/exporters/otlp/otlptrace"
    "go.opentelemetry.io/otel/sdk/trace"
    "go.opentelemetry.io/otel/attribute"
)

func initTracer() func() {
    exporter, _ := otlptrace.New(context.Background())
    tp := trace.NewTracerProvider(
        trace.WithBatcher(exporter),
    )
    otel.SetTracerProvider(tp)
    return func() { tp.Shutdown(context.Background()) }
}

func main() {
    cleanup := initTracer()
    defer cleanup()

    tracer := otel.Tracer("my-service")
    ctx, span := tracer.Start(context.Background(), "my-operation")
    defer span.End()

    // 添加属性
    span.SetAttributes(
        attribute.String("user.id", "12345"),
        attribute.Int("items.count", 3),
    )

    // 记录事件
    span.AddEvent("Processing started")
    
    // 子 Span
    _, childSpan := tracer.Start(ctx, "child-operation")
    childSpan.End()
}
```

### Python SDK

```python
from opentelemetry import trace
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import BatchSpanProcessor
from opentelemetry.exporter.otlp.proto.grpc.trace_exporter import OTLPSpanExporter

# 初始化
trace.set_tracer_provider(TracerProvider())
tracer = trace.get_tracer(__name__)

# 配置导出器
otlp_exporter = OTLPSpanExporter(endpoint="localhost:4317")
trace.get_tracer_provider().add_span_processor(
    BatchSpanProcessor(otlp_exporter)
)

# 使用
with tracer.start_as_current_span("my-operation") as span:
    span.set_attribute("user.id", "12345")
    span.add_event("Processing started")
    
    # 嵌套 Span
    with tracer.start_as_current_span("child-operation"):
        pass
```

---

## OTel Collector

### 配置示例

```yaml
# otel-collector-config.yaml
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
          static_configs:
            - targets: ['localhost:8888']

processors:
  batch:
    timeout: 10s
    send_batch_size: 1000
  
  memory_limiter:
    check_interval: 1s
    limit_mib: 1000

exporters:
  otlp:
    endpoint: "jaeger:4317"
    tls:
      insecure: true
  
  prometheus:
    endpoint: "0.0.0.0:8889"

service:
  pipelines:
    traces:
      receivers: [otlp]
      processors: [batch, memory_limiter]
      exporters: [otlp]
    
    metrics:
      receivers: [otlp, prometheus]
      processors: [batch]
      exporters: [prometheus]
```

### Kubernetes 部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: otel-collector
spec:
  replicas: 1
  selector:
    matchLabels:
      app: otel-collector
  template:
    spec:
      containers:
        - name: collector
          image: otel/opentelemetry-collector-contrib:latest
          args: ["--config=/etc/otel/config.yaml"]
          ports:
            - containerPort: 4317  # OTLP gRPC
            - containerPort: 4318  # OTLP HTTP
            - containerPort: 8889  # Prometheus metrics
          volumeMounts:
            - name: config
              mountPath: /etc/otel/
      volumes:
        - name: config
          configMap:
            name: otel-collector-config
```

---

## 自动插桩

```bash
# Java Agent
java -javaagent:opentelemetry-javaagent.jar \
  -Dotel.service.name=my-service \
  -Dotel.exporter.otlp.endpoint=http://collector:4317 \
  -jar my-app.jar

# Python
opentelemetry-instrument \
  --service_name my-service \
  python my-app.py

# Node.js
node --require @opentelemetry/auto-instrumentations-node/register \
  app.js
```

---

## 参考资源

- [官方文档](https://opentelemetry.io/docs)
- [GitHub](https://github.com/open-telemetry)
- [CNCF 项目页面](https://www.cncf.io/projects/opentelemetry/)
- [OTel Collector](https://github.com/open-telemetry/opentelemetry-collector)
- [语言 SDK 列表](https://opentelemetry.io/docs/instrumentation/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[log.md|log]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[references/release-notes-observability|发布说明索引 — 可观测性]] — Cross-reference
- [[concepts/bp-observability|最佳实践：Observability]] — Cross-reference
- [[concepts/ai-agent-README|AI Agent 工程专题]] — Cross-reference
- [[concepts/observability-stack-evolution|可观测性栈演进]] — Cross-reference
- [[entities/cncf-observability|CNCF 可观测性项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/observability-index|Observability 可观测性知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.43|opentelemetry-collector v0.43 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.12|opentelemetry-collector v0.12 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.129|opentelemetry-collector v0.129 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.26|opentelemetry-collector v0.26 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.77|opentelemetry-collector v0.77 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.84|opentelemetry-collector v0.84 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.94|opentelemetry-collector v0.94 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.67|opentelemetry-collector v0.67 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.36|opentelemetry-collector v0.36 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.139|opentelemetry-collector v0.139 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.53|opentelemetry-collector v0.53 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.22|opentelemetry-collector v0.22 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.80|opentelemetry-collector v0.80 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.119|opentelemetry-collector v0.119 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.16|opentelemetry-collector v0.16 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.47|opentelemetry-collector v0.47 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.148|opentelemetry-collector v0.148 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.57|opentelemetry-collector v0.57 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.109|opentelemetry-collector v0.109 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.90|opentelemetry-collector v0.90 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.32|opentelemetry-collector v0.32 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.63|opentelemetry-collector v0.63 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.23|opentelemetry-collector v0.23 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.81|opentelemetry-collector v0.81 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.17|opentelemetry-collector v0.17 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.118|opentelemetry-collector v0.118 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.149|opentelemetry-collector v0.149 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.46|opentelemetry-collector v0.46 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.56|opentelemetry-collector v0.56 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.108|opentelemetry-collector v0.108 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.91|opentelemetry-collector v0.91 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.33|opentelemetry-collector v0.33 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.62|opentelemetry-collector v0.62 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.42|opentelemetry-collector v0.42 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.27|opentelemetry-collector v0.27 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.128|opentelemetry-collector v0.128 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.76|opentelemetry-collector v0.76 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.85|opentelemetry-collector v0.85 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.95|opentelemetry-collector v0.95 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.66|opentelemetry-collector v0.66 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.138|opentelemetry-collector v0.138 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.37|opentelemetry-collector v0.37 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.52|opentelemetry-collector v0.52 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.146|opentelemetry-collector v0.146 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.49|opentelemetry-collector v0.49 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.18|opentelemetry-collector v0.18 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.117|opentelemetry-collector v0.117 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.123|opentelemetry-collector v0.123 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.133|opentelemetry-collector v0.133 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.107|opentelemetry-collector v0.107 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.59|opentelemetry-collector v0.59 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.79|opentelemetry-collector v0.79 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.28|opentelemetry-collector v0.28 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.127|opentelemetry-collector v0.127 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.113|opentelemetry-collector v0.113 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.142|opentelemetry-collector v0.142 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.103|opentelemetry-collector v0.103 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.137|opentelemetry-collector v0.137 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.38|opentelemetry-collector v0.38 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.69|opentelemetry-collector v0.69 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.78|opentelemetry-collector v0.78 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.126|opentelemetry-collector v0.126 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.29|opentelemetry-collector v0.29 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.112|opentelemetry-collector v0.112 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.143|opentelemetry-collector v0.143 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.102|opentelemetry-collector v0.102 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.39|opentelemetry-collector v0.39 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.136|opentelemetry-collector v0.136 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.68|opentelemetry-collector v0.68 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.48|opentelemetry-collector v0.48 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.147|opentelemetry-collector v0.147 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.116|opentelemetry-collector v0.116 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.19|opentelemetry-collector v0.19 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.122|opentelemetry-collector v0.122 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.132|opentelemetry-collector v0.132 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.106|opentelemetry-collector v0.106 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.58|opentelemetry-collector v0.58 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.125|opentelemetry-collector v0.125 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.88|opentelemetry-collector v0.88 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.140|opentelemetry-collector v0.140 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.111|opentelemetry-collector v0.111 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.101|opentelemetry-collector v0.101 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.98|opentelemetry-collector v0.98 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.135|opentelemetry-collector v0.135 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.115|opentelemetry-collector v0.115 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.144|opentelemetry-collector v0.144 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.121|opentelemetry-collector v0.121 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.131|opentelemetry-collector v0.131 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.0|opentelemetry-collector v0.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.105|opentelemetry-collector v0.105 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.114|opentelemetry-collector v0.114 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.145|opentelemetry-collector v0.145 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.120|opentelemetry-collector v0.120 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.130|opentelemetry-collector v0.130 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.104|opentelemetry-collector v0.104 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.124|opentelemetry-collector v0.124 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.89|opentelemetry-collector v0.89 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.141|opentelemetry-collector v0.141 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.110|opentelemetry-collector v0.110 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.100|opentelemetry-collector v0.100 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.99|opentelemetry-collector v0.99 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.134|opentelemetry-collector v0.134 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.82|opentelemetry-collector v0.82 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.20|opentelemetry-collector v0.20 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.45|opentelemetry-collector v0.45 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.55|opentelemetry-collector v0.55 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.61|opentelemetry-collector v0.61 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.30|opentelemetry-collector v0.30 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.92|opentelemetry-collector v0.92 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.41|opentelemetry-collector v0.41 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.86|opentelemetry-collector v0.86 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.75|opentelemetry-collector v0.75 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.24|opentelemetry-collector v0.24 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.34|opentelemetry-collector v0.34 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.65|opentelemetry-collector v0.65 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.96|opentelemetry-collector v0.96 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.51|opentelemetry-collector v0.51 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.40|opentelemetry-collector v0.40 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.87|opentelemetry-collector v0.87 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.74|opentelemetry-collector v0.74 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.25|opentelemetry-collector v0.25 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.35|opentelemetry-collector v0.35 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.64|opentelemetry-collector v0.64 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.97|opentelemetry-collector v0.97 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.50|opentelemetry-collector v0.50 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.83|opentelemetry-collector v0.83 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.21|opentelemetry-collector v0.21 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.70|opentelemetry-collector v0.70 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.44|opentelemetry-collector v0.44 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.15|opentelemetry-collector v0.15 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.54|opentelemetry-collector v0.54 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.60|opentelemetry-collector v0.60 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.31|opentelemetry-collector v0.31 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/RELEASE-NOTES-0.93|opentelemetry-collector v0.93 Release Notes]]
