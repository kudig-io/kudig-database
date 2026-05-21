---
title: containerd 分布式追踪与可观测性
description: 'description: ''## 1. 可观测性概述'''
category: general
tags:
- cncf
- ecosystem
- observability
- etcd
- kubelet
- prometheus
- grafana
- jaeger
- containerd
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- containerd 分布式追踪与可观测性 是什么
- 如何 containerd 分布式追踪与可观测性
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- containerd
- 分布式追踪与可观测性
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- monitoring-basics
- etcd-basics
- logging-basics
- tracing-basics
- observability-basics
---

title: containerd 分布式追踪与可观测性
description: '## 1. 可观测性概述'
category: cncf-landscape
tags:
- k8s
- containerd
- observability
- tracing
- [[entities/opentelemetry.md|OpenTelemetry]]
- prometheus
- metrics
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 可观测性工程师
estimated_read_time: 10min
intent_queries:
- containerd 分布式追踪 配置
- containerd OpenTelemetry 集成
- containerd 可观测性 最佳实践
trigger_keywords:
- containerd 分布式追踪
- containerd 可观测性
- containerd OpenTelemetry
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
# containerd 分布式追踪与可观测性

> **版本**: v1.0 | **适用版本**: containerd 1.7+ / 2.0 | **最后更新**: 2026-05

---

## 1. 可观测性概述

### 1.1 containerd 可观测性框架

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         containerd 可观测性架构                                  │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                     containerd 运行时                                    │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐ │    │
│  │  │   Metrics   │ │   Traces    │ │    Logs     │ │      Events         │ │    │
│  │  │  (Prometheus)│ │ (OTLP)      │ │   (Loki)    │ │    (Watch Events)   │ │    │
│  │  └──────┬──────┘ └──────┬──────┘ └──────┬──────┘ └──────────┬────────┘ │    │
│  └─────────┼───────────────┼───────────────┼────────────────────┼─────────┘    │
│            │               │               │                    │              │
│            ▼               ▼               ▼                    ▼              │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                    OpenTelemetry Collector                              │    │
│  │  ┌────────────────┐ ┌────────────────┐ ┌────────────────┐              │    │
│  │  │   Receivers   │ │  Processors    │ │   Exporters    │              │    │
│  │  │ prometheus/otlp│ │ batch/memory   │ │ otlp/tempo      │              │    │
│  │  └────────────────┘ └────────────────┘ └────────────────┘              │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                    │                                            │
│                                    ▼                                            │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                         Backend Storage                                  │    │
│  │  ┌────────────────┐ ┌────────────────┐ ┌────────────────┐              │    │
│  │  │   Prometheus   │ │     Grafana    │ │     [[entities/jaeger.md|Jaeger]]     │              │    │
│  │  │   (Metrics)    │ │   (Dashboards) │ │   (Traces)     │              │    │
│  │  └────────────────┘ └────────────────┘ └────────────────┘              │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 可观测性数据类型

| 数据类型 | 采集方式 | 存储后端 | 使用场景 |
|----------|----------|----------|----------|
| **Metrics** | Prometheus pull | Prometheus/[[entities/thanos.md|thanos]] | 性能监控、告警 |
| **Traces** | OTLP push | Tempo/Jaeger | 分布式追踪 |
| **Logs** | File tail/[[entities/fluentd.md|Fluentd]] | Loki/Elasticsearch | 问题排查 |
| **Events** | [[entities/kubernetes.md|kubernetes]] Events | etcd | 状态变更追踪 |

---

## 2. Metrics 采集

### 2.1 containerd 内置指标

```toml
# /etc/containerd/config.toml
[metrics]
  # 启用 metrics 端点
  address = "127.0.0.1:1338"
  
  # gRPC 直方图（用于分析延迟分布）
  grpc_histogram = true
```

### 2.2 关键指标列表

| 指标名称 | 类型 | 说明 |
|----------|------|------|
| `containerd_runtime_container_create_duration_seconds` | Histogram | 容器创建延迟 |
| `containerd_runtime_container_start_duration_seconds` | Histogram | 容器启动延迟 |
| `containerd_container_healthcheck_duration_seconds` | Histogram | 健康检查延迟 |
| `containerd_images_pulled_total` | Counter | 镜像拉取总数 |
| `containerd_images_pulled_duration_seconds` | Histogram | 镜像拉取耗时 |
| `containerd_grpc_request_duration_seconds` | Histogram | gRPC 请求延迟 |
| `containerd_tasks_started_total` | Counter | 任务启动总数 |
| `containerd_tasks_oom_total` | Counter | OOM 事件总数 |

### 2.3 Prometheus 采集配置

```yaml
# prometheus scrape config
- job_name: containerd
  static_configs:
  - targets: ['localhost:1338']
  metrics_path: /v1/metrics
  scrape_interval: 15s
  
  relabel_configs:
  - source_labels: [__address__]
    target_label: instance
    regex: '(.*):1338'
    replacement: '${1}'
  
  # 添加 node 信息标签
  - target_label: cluster
    replacement: 'prod-cluster-1'
```

### 2.4 Grafana Dashboard

```json
{
  "dashboard": {
    "title": "containerd Runtime Dashboard",
    "panels": [
      {
        "title": "Container Creation Latency (P99)",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.99, rate(containerd_runtime_container_create_duration_seconds_bucket[5m]))",
            "legendFormat": "P99"
          }
        ]
      },
      {
        "title": "Active Containers",
        "type": "graph",
        "targets": [
          {
            "expr": "containerd_container_runtime_mev",
            "legendFormat": "Containers"
          }
        ]
      },
      {
        "title": "Image Pull Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(containerd_images_pulled_total[5m])",
            "legendFormat": "Pulls/sec"
          }
        ]
      }
    ]
  }
}
```

---

## 3. 分布式追踪 (OpenTelemetry)

### 3.1 追踪架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         containerd 追踪架构                                       │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  Kubelet (kubelet)                                                               │
│       │                                                                          │
│       │ CRI: RunPodSandbox, CreateContainer, StartContainer                      │
│       ▼                                                                          │
│  containerd                                                                       │
│       │                                                                          │
│       │ Trace: span_name="containerd.task.create"                                │
│       │        attributes: {container_id, image, runtime}                        │
│       ▼                                                                          │
│  containerd-shim (shim v2)                                                       │
│       │                                                                          │
│       │ Trace: span_name="shim.task.start"                                       │
│       ▼                                                                          │
│  runc                                                                             │
│       │                                                                          │
│       │ Trace: span_name="runc.create"                                           │
│       │        attributes: {rootfs, cgroups}                                     │
│       ▼                                                                          │
│  Container Process                                                                │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 OpenTelemetry 集成配置

```toml
# /etc/containerd/config.toml
[plugins]
  # OpenTelemetry 插件配置
  [plugins."io.containerd.tracing.processor.v1.otlp"]
    enabled = true
    endpoint = "http://otel-collector:4317"
    
    # 采样配置
    [plugins."io.containerd.tracing.processor.v1.otlp".sampling]
      type = "probabilistic"
      rate = 0.1  # 10% 采样率
    
    # 批量配置
    [plugins."io.containerd.tracing.processor.v1.otlp".batch]
      max_export_batch_size = 512
      schedule_delay_millis = 5000
```

### 3.3 trace 生成点

| 操作 | Span 名称 | Attributes |
|------|-----------|------------|
| **拉取镜像** | `containerd.image.pull` | image, size, duration |
| **创建容器** | `containerd.task.create` | container_id, image, config |
| **启动容器** | `containerd.task.start` | container_id, runtime |
| **停止容器** | `containerd.task.stop` | container_id, exit_code |
| **删除容器** | `containerd.task.delete` | container_id |
| **Snapshot 准备** | `containerd.snapshot.prepare` | snapshot_id, layers |

### 3.4 追踪上下文传播

```bash
# 追踪上下文通过 W3C TraceContext 传播

# containerd 生成的 traceparent
# traceparent: 00-<trace_id>-<span_id>-01
# 示例: 00-0af7651916cd43dd8448eb211c80319c-b7ad6b7169203331-01

# 传递到 kubelet
# kubelet 通过 CRI 请求传递 traceparent header
```

### 3.5 OpenTelemetry Collector 配置

```yaml
# otel-collector-config.yaml
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
    batch_size: 512
  
  memory_limiter:
    check_interval: 10s
    limit_mib: 1000
  
  transform:
    error_mode: ignore
    rules:
      - context: span
        commands:
          - set(attributes["container.runtime"], "containerd")

exporters:
  otlp:
    endpoint: "tempo:4317"
    tls:
      insecure: false
  
  prometheus:
    endpoint: "0.0.0.0:8889"

service:
  pipelines:
    traces:
      receivers: [otlp]
      processors: [batch, memory_limiter]
      exporters: [otlp]
    metrics:
      receivers: [prometheus]
      exporters: [prometheus]
```

---

## 4. 日志管理

### 4.1 containerd 日志配置

```toml
# /etc/containerd/config.toml
[debug]
  level = "info"
  format = "json"  # JSON 格式便于解析
  
[log]
  level = "info"
  format = "json"
  
  # 日志轮转
  [log.file]
    max_size = "100Mi"
    max_files = 5
    path = "/var/log/containerd/containerd.log"
```

### 4.2 容器日志收集

```yaml
# Fluent Bit 配置收集 containerd 日志
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluent-bit-config
data:
  fluent-bit.conf: |
    [INPUT]
        Name tail
        Path /var/log/containerd/*.log
        Parser json
        Tag containerd.log
    
    [FILTER]
        Name parser
        Match containerd.log
        Key_name log
        Parser json
    
    [OUTPUT]
        Name loki
        Match containerd.log
        Host loki.monitoring.svc
        Port 3100
        labels job=containerd,host=$HOSTNAME
```

### 4.3 结构化日志字段

```json
{
  "timestamp": "2026-05-19T10:30:00.000Z",
  "level": "info",
  "msg": "container task started",
  "task_id": "abc123",
  "container_id": "def456",
  "image": "nginx:latest",
  "runtime": "io.containerd.runc.v2",
  "span_id": "789abc",
  "trace_id": "xyz123"
}
```

---

## 5. 事件监控

### 5.1 Kubernetes Events 集成

```bash
# containerd 生成的事件会被 kubelet 收集并创建 Kubernetes Events

# 查看 containerd 相关事件
kubectl get events --field-selector involvedObject.name=<pod-name>

# 常见事件
# - ContainerCreated
# - ContainerStarted
# - ContainerStopped
# - ContainerUnhealthy
```

### 5.2 自定义事件收集

```yaml
# 使用 Falco 监控 containerd 事件
- rule: containerd anomaly
  desc: detect containerd anomalies
  condition: >
    proc.name = "containerd" and 
    (evt.type = "execve" and count > 10)
  output: >
    containerd exec anomaly (user=%user.name count=%count)
  priority: WARNING
```

### 5.3 AlertManager 配置

```yaml
# Prometheus AlertManager 配置
groups:
- name: containerd-alerts
  rules:
  - alert: ContainerdDown
    expr: up{job="containerd"} == 0
    for: 1m
    labels:
      severity: critical
    annotations:
      summary: "containerd is down"
      
  - alert: ContainerCreationLatencyHigh
    expr: histogram_quantile(0.99, rate(containerd_runtime_container_create_duration_seconds_bucket[5m])) > 2
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Container creation latency is high"
```

---

## 6. 综合可观测性方案

### 6.1 架构图

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         统一可观测性平台架构                                       │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                    containerd Nodes                                     │    │
│  │  ┌────────────────┐  ┌────────────────┐  ┌────────────────┐             │    │
│  │  │  ctr + metrics │  │  otlp traces   │  │  fluentd logs  │             │    │
│  │  └───────┬────────┘  └───────┬────────┘  └───────┬────────┘             │    │
│  └──────────┼───────────────────┼───────────────────┼─────────────────────┘    │
│             │                   │                   │                          │
│             ▼                   ▼                   ▼                          │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                    OpenTelemetry Collector                              │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────────┐    │    │
│  │  │ Prometheus  │ │    OTLP     │ │   Fluentd   │ │   K8s Events    │    │    │
│  │  │  Receiver   │ │  Receiver   │ │  Receiver   │ │    Receiver     │    │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────────┘    │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                    │                                            │
│                    ┌───────────────┼───────────────┐                           │
│                    ▼               ▼               ▼                           │
│           ┌────────────┐  ┌────────────┐  ┌────────────┐                       │
│           │ Prometheus │  │   Tempo   │  │    Loki    │                       │
│           │  (Metrics) │  │ (Traces)  │  │  (Logs)   │                       │
│           └──────┬─────┘  └──────┬─────┘  └──────┬─────┘                       │
│                  │               │               │                              │
│                  └───────────────┼───────────────┘                              │
│                              ▼                                                 │
│                    ┌────────────────────┐                                      │
│                    │      Grafana       │                                      │
│                    │   (统一 Dashboard) │                                      │
│                    └────────────────────┘                                      │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 6.2 一站式部署

```yaml
# docker-compose 或 Kubernetes 部署
apiVersion: apps/v1
kind: Deployment
metadata:
  name: otel-collector
spec:
  replicas: 2
  selector:
    matchLabels:
      app: otel-collector
  template:
    metadata:
      labels:
        app: otel-collector
    spec:
      containers:
      - name: otel-collector
        image: otel/opentelemetry-collector-contrib:0.90.0
        args: ["--config=/etc/otel-collector-config.yaml"]
        ports:
        - name: prometheus
          containerPort: 8889
        - name: otlp-grpc
          containerPort: 4317
        - name: otlp-http
          containerPort: 4318
        volumeMounts:
        - name: otel-config
          mountPath: /etc/otel-collector-config.yaml
          subPath: otel-collector-config.yaml
      volumes:
      - name: otel-config
        configMap:
          name: otel-collector-config
```

### 6.3 Grafana Dashboard 配置

```json
{
  "title": "containerd 统一可观测性",
  "panels": [
    {
      "title": "Runtime 概览",
      "type": "row",
      "gridPos": {"x": 0, "y": 0, "w": 24, "h": 2}
    },
    {
      "title": "容器数量",
      "type": "stat",
      "targets": [{"expr": "containerd_container_count"}],
      "gridPos": {"x": 0, "y": 2, "w": 6, "h": 4}
    },
    {
      "title": "镜像数量",
      "type": "stat", 
      "targets": [{"expr": "containerd_image_count"}],
      "gridPos": {"x": 6, "y": 2, "w": 6, "h": 4}
    },
    {
      "title": "P99 创建延迟",
      "type": "gauge",
      "targets": [{"expr": "histogram_quantile(0.99, rate(containerd_runtime_container_create_duration_seconds_bucket[5m]))"}],
      "gridPos": {"x": 12, "y": 2, "w": 6, "h": 4}
    },
    {
      "title": "gRPC 请求率",
      "type": "graph",
      "targets": [{"expr": "rate(containerd_grpc_request_total[5m])"}],
      "gridPos": {"x": 18, "y": 2, "w": 6, "h": 4}
    },
    {
      "title": "延迟分布",
      "type": "row",
      "gridPos": {"x": 0, "y": 6, "w": 24, "h": 2}
    },
    {
      "title": "容器生命周期延迟",
      "type": "timeseries",
      "targets": [
        {"expr": "histogram_quantile(0.50, rate(containerd_runtime_container_create_duration_seconds_bucket[5m]))", "legendFormat": "P50"},
        {"expr": "histogram_quantile(0.95, rate(containerd_runtime_container_create_duration_seconds_bucket[5m]))", "legendFormat": "P95"},
        {"expr": "histogram_quantile(0.99, rate(containerd_runtime_container_create_duration_seconds_bucket[5m]))", "legendFormat": "P99"}
      ],
      "gridPos": {"x": 0, "y": 8, "w": 12, "h": 8}
    },
    {
      "title": "镜像拉取延迟",
      "type": "timeseries",
      "targets": [
        {"expr": "histogram_quantile(0.99, rate(containerd_images_pulled_duration_seconds_bucket[5m]))", "legendFormat": "P99"}
      ],
      "gridPos": {"x": 12, "y": 8, "w": 12, "h": 8}
    },
    {
      "title": "分布式追踪",
      "type": "row",
      "gridPos": {"x": 0, "y": 16, "w": 24, "h": 2}
    },
    {
      "title": "追踪采样率",
      "type": "graph",
      "targets": [{"expr": "rate(containerd_traces_exported_total[5m]) / rate(containerd_traces_generated_total[5m])"}],
      "gridPos": {"x": 0, "y": 18, "w": 12, "h": 8}
    },
    {
      "title": "Span 数量",
      "type": "timeseries",
      "targets": [{"expr": "rate(containerd_spans_total[5m])"}],
      "gridPos": {"x": 12, "y": 18, "w": 12, "h": 8}
    }
  ]
}
```

---

## 7. 故障排查场景

### 7.1 容器创建慢

```bash
# 1. 检查镜像拉取延迟
histogram_quantile(0.99, rate(containerd_images_pulled_duration_seconds_bucket[5m]))

# 2. 检查 Snapshot 准备时间
histogram_quantile(0.99, rate(containerd_snapshot_prepare_duration_seconds_bucket[5m]))

# 3. 检查 gRPC 请求延迟
histogram_quantile(0.99, rate(containerd_grpc_request_duration_seconds_bucket[5m]))

# 4. 追踪分析
# 在 Jaeger 中搜索 container_id = xxx 的 trace
```

### 7.2 容器启动失败

```bash
# 1. 检查错误率
rate(containerd_runtime_container_create_duration_seconds_count{status="error"}[5m])

# 2. 检查失败原因分布
count by (error_type) (containerd_task_create_errors_total)

# 3. 查看日志
# 在 Loki 中搜索: {job="containerd"} |= "error" |= "<container_id>"
```

### 7.3 OOM 问题排查

```bash
# 1. 检查 OOM 事件
increase(containerd_tasks_oom_total[1h])

# 2. 检查内存使用趋势
containerd_container_memory_usage_bytes

# 3. 追踪内存分配
# 在 trace 中查找内存分配相关的 span
```

---

## 8. SLO/SLI 定义

### 8.1 containerd SLO

| SLO 名称 | 目标值 | SLI |
|----------|--------|-----|
| **容器创建可用性** | 99.9% | successful container creates / total container creates |
| **容器启动延迟** | P99 < 2s | histogram_quantile(0.99, container_create_duration) |
| **镜像拉取成功率** | 99.5% | successful image pulls / total image pulls |
| **gRPC 请求成功率** | 99.9% | successful requests / total requests |
| **运行时稳定性** | 99.99% | uptime / total time |

### 8.2 告警配置

```yaml
# SLO 健康检查告警
groups:
- name: containerd-slo
  rules:
  - alert: ContainerCreationSLOBreach
    expr: |
      (
        1 - 
        sum(rate(containerd_task_create_success_total[5m])) / 
        sum(rate(containerd_task_create_total[5m]))
      ) > 0.001  # 99.9% threshold
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Container creation SLO breached"
      description: "Error rate {{ $value | humanizePercentage }} exceeds 0.1%"
      
  - alert: ContainerStartLatencySLOBreach
    expr: |
      histogram_quantile(0.99, rate(containerd_runtime_container_start_duration_seconds_bucket[5m])) > 2
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Container start latency SLO breached"
      description: "P99 latency {{ $value }}s exceeds 2s threshold"
```

---

**维护者**: Kudig Team | **许可证**: MIT

## See Also

- [[domain-19-landscape-references/graduated/containerd/04-containerd-upgrade-migration.md|04-containerd-upgrade-migration]]
- [[domain-19-landscape-references/graduated/containerd/05-containerd-windows-support.md|05-containerd-windows-support]]
- [[domain-19-landscape-references/graduated/containerd/07-containerd-disaster-recovery.md|07-containerd-disaster-recovery]]
- [[domain-19-landscape-references/graduated/containerd/08-containerd-multi-tenant.md|08-containerd-multi-tenant]]

## Related

- [[entities/cncf-runtime|CNCF 容器运行时与工具链项目全景]] — Cross-reference
