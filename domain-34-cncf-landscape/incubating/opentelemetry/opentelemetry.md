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
