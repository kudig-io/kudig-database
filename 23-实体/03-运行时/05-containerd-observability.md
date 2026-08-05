---
title: containerd 可观测性
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- 06-containerd-observability
- etcd
- prometheus
- grafana
- containerd
- falco
- crd
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- containerd 可观测性 是什么
- 如何 containerd 可观测性
trigger_keywords:
- containerd
- 可观测性
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# containerd 可观测性

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

Containerd 可观测性是关于监控和诊断 containerd 容器运行时行为的实践方法论。它涵盖了容器生命周期事件追踪、镜像拉取性能监控、运行时资源使用追踪、容器日志采集和 CRI gRPC 调用审计等多个维度。通过系统性的可观测性配置，运维团队可以快速定位容器启动失败、镜像拉取超时、运行时资源竞争等常见问题。

## Key Features（核心能力）

- **Native Metrics**：containerd 内置 Prometheus metrics 端点暴露运行时指标
- **CRI Metrics**：kubelet 通过 CRI 暴露容器操作延迟和错误率指标
- **事件日志**：containerd 通过 CRI 的 container log 接口输出容器 stdout/stderr
- **调试工具**：ctr、crictl、nerdctl 等命令行工具用于运行时调试
- **分布式追踪**：通过 OpenTelemetry 追踪容器镜像拉取链路
- **健康检查**：containerd binary 内置健康检查端点

## 架构与工作原理

可观测性数据分三层采集：Metrics 层通过 containerd metrics_v2 端点暴露镜像拉取计数、容器操作延迟、运行时 GC 统计等指标；Logs 层通过 CRI 接口将容器 stdout/stderr 重定向到 JSON 文件，由 Fluentd/Fluent Bit 采集；Events 层通过 K8s Event API 记录容器生命周期事件。关键指标包括 container_image_pull_duration_seconds、container_runtime_operations_seconds 等。

## K8s 集成

在 K8s 中，containerd 指标通过 kubelet 的 /metrics/cadvisor 和 /metrics/probes 端点暴露。cAdvisor 提供容器级别的 CPU、内存、网络、IO 指标。CRI 通过 kubelet 暴露镜像操作统计。通过 DaemonSet 部署 node-exporter 获取节点级指标。日志通过 DaemonSet 部署 Fluent Bit/Fluentd 自动采集所有节点的容器日志。

## 生产用例

- **容器启动排障**：通过事件和指标快速定位 Pod 启动失败原因
- **镜像拉取优化**：监控镜像拉取延迟和带宽使用，优化 Registry 配置
- **运行时资源监控**：追踪容器运行时的 CPU、内存、IO 使用情况
- **性能基线建立**：建立正常运行基线，支持异常检测和容量规划

## 安装与配置

### containerd Metrics 配置

```toml
# /etc/containerd/config.toml - 启用 metrics
version = 2

[metrics]
  address = "0.0.0.0:1338"  # metrics 端点
  grpc_histogram = true

[plugins."io.containerd.grpc.v1.cri"]
  # 启用 CRI 指标
  [plugins."io.containerd.grpc.v1.cri".containerd]
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
        SystemdCgroup = true
```

### Prometheus 采集配置

```yaml
# ServiceMonitor 采集 containerd metrics
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: containerd-metrics
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: containerd-exporter
  endpoints:
  - port: metrics
    interval: 15s
    path: /v1/metrics
---
# DaemonSet 暴露 containerd metrics
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: containerd-exporter
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: containerd-exporter
  template:
    metadata:
      labels:
        app: containerd-exporter
    spec:
      hostPID: true
      containers:
      - name: exporter
        image: registry.example.com/containerd-exporter:latest
        ports:
        - containerPort: 1338
          name: metrics
        volumeMounts:
        - name: containerd-sock
          mountPath: /run/containerd/containerd.sock
          readOnly: true
      volumes:
      - name: containerd-sock
        hostPath:
          path: /run/containerd/containerd.sock
```

### 关键指标说明

| 指标名称 | 类型 | 含义 | 告警阈值 |
|----------|------|------|----------|
| `engine_daemon_container_states_containers` | Gauge | 各状态容器数 | stopped > 10 |
| `containerd_container_count` | Gauge | 容器总数 | - |
| `containerd_task_count` | Gauge | 运行中任务数 | - |
| `containerd_snapshot_count` | Gauge | 快照数量 | > 500 |
| `grpc_server_handled_total` | Counter | gRPC 调用总数 | - |
| `grpc_server_handling_seconds` | Histogram | gRPC 调用延迟 | P99 > 5s |
| `image_pull_duration_seconds` | Histogram | 镜像拉取耗时 | P95 > 60s |
| `container_create_duration_seconds` | Histogram | 容器创建耗时 | P99 > 10s |
| `container_start_duration_seconds` | Histogram | 容器启动耗时 | P99 > 30s |

### Grafana Dashboard 配置

```json
{
  "dashboard": {
    "title": "Containerd Runtime Overview",
    "panels": [
      {
        "title": "Container States",
        "type": "stat",
        "targets": [{"expr": "engine_daemon_container_states_containers"}]
      },
      {
        "title": "Image Pull Duration P95",
        "type": "timeseries",
        "targets": [{"expr": "histogram_quantile(0.95, rate(image_pull_duration_seconds_bucket[5m]))"}]
      },
      {
        "title": "Container Create Rate",
        "type": "timeseries",
        "targets": [{"expr": "rate(container_create_duration_seconds_count[5m])"}]
      },
      {
        "title": "gRPC Error Rate",
        "type": "timeseries",
        "targets": [{"expr": "rate(grpc_server_handled_total{grpc_code!=\"OK\"}[5m]) / rate(grpc_server_handled_total[5m])"}]
      }
    ]
  }
}
```

## 运维操作

```bash
# 🟢 查看 containerd metrics
curl -s http://localhost:1338/v1/metrics | grep containerd | head -30

# 🟢 使用 crictl 查看运行时状态
crictl info | jq '.config'
crictl stats  # 容器资源使用
crictl statsp  # Pod 资源使用

# 🟢 查看容器日志
crictl logs <container-id> --tail 50
crictl logs <container-id> --since 1h

# 🟢 查看镜像列表和大小
crictl images
crictl rmi --prune  # 清理未使用镜像

# 🟢 检查 containerd 健康状态
systemctl status containerd
ctr version
ctr plugins ls | grep -E "cri|metrics"

# 🟢 查看容器事件
journalctl -u containerd --since "10 min ago" | grep -E "create|start|stop|delete"

# 🟢 cAdvisor 指标（通过 kubelet）
curl -sk https://localhost:10250/metrics/cadvisor | grep container_ | head -20
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| metrics 端点无响应 | metrics 未启用/端口冲突 | `curl localhost:1338/v1/metrics` | 检查 config.toml metrics 配置 |
| 镜像拉取延迟高 | Registry 慢/网络问题 | `crictl pull <image>`; 查看指标 | 配置镜像缓存/检查网络 |
| 容器启动慢 | 存储 IO 慢/镜像层多 | `container_start_duration_seconds` | 优化镜像/检查存储 |
| 容器数量异常增长 | 容器泄漏/未清理 | `crictl ps -a | wc -l` | 检查 GC 配置/清理停止容器 |
| gRPC 错误率高 | 资源不足/插件异常 | `grpc_server_handled_total` | 检查 containerd 日志/资源 |

### 排查流程

```
containerd 可观测性异常
├── 指标缺失？
│   ├── metrics 端点可达？→ curl localhost:1338/v1/metrics
│   ├── Prometheus 采集正常？→ 检查 ServiceMonitor
│   └── containerd 服务运行？→ systemctl status containerd
├── 容器启动慢？
│   ├── 镜像拉取慢 → 检查 Registry/网络/镜像大小
│   ├── 存储挂载慢 → 检查 CSI/磁盘 IO
│   └── CNI 配置慢 → 检查网络插件
└── 资源异常？
    ├── 容器泄漏 → crictl ps -a 检查停止容器
    ├── 快照累积 → ctr snapshots ls | wc -l
    └── 内存增长 → 检查 containerd 进程 RSS
```

## 生产案例

### 案例1：镜像拉取延迟导致 Pod 启动超时

- **场景**：新节点加入集群后，Pod 启动时间超过 5 分钟，触发调度超时
- **排查**：`image_pull_duration_seconds` P95 = 180s；镜像大小 2GB；Registry 带宽受限
- **方案**：部署本地 Registry 镜像缓存（registry:2 proxy）；启用 image pre-pull DaemonSet；使用 Stargz lazy pulling
- **效果**：镜像拉取时间降至 15s，Pod 启动时间 < 30s

### 案例2：容器泄漏导致节点资源耗尽

- **场景**：节点上停止的容器累积到 500+，占用大量磁盘和 inotify 资源
- **排查**：`crictl ps -a | grep Exited | wc -l` = 523；containerd GC 未触发
- **方案**：配置 kubelet `--maximum-dead-containers=50`；设置 containerd `max_container_log_line_size`；添加容器数量告警
- **效果**：停止容器自动清理，节点资源稳定

## 对比替代方案

| 方案 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| containerd native metrics | 原生、低开销、实时 | 指标有限、无历史 | 运行时监控 |
| cAdvisor (kubelet) | 容器级详细指标、K8s集成 | 仅运行时指标、无应用层 | K8s 标准监控 |
| node-exporter | 节点级全面指标 | 无容器级别细节 | 节点资源监控 |
| eBPF (Tetragon/Cilium) | 内核级深度可见性 | 复杂、性能开销 | 安全/深度诊断 |
| OpenTelemetry | 统一遥测、分布式追踪 | 配置复杂 | 全链路可观测 |

## 检查清单

- [ ] containerd metrics 端点已启用且可访问
- [ ] Prometheus ServiceMonitor 已配置
- [ ] Grafana Dashboard 已部署
- [ ] 关键指标告警已配置（镜像拉取、容器启动、gRPC 错误）
- [ ] 容器日志采集已配置（Fluent Bit/Fluentd）
- [ ] 容器 GC 策略已配置
- [ ] 节点级监控（node-exporter）已部署
- [ ] 历史指标存储已配置（Thanos/VictoriaMetrics）

## Related

- [[spiderpool]] — Spiderpool
- [[ratify]] — Ratify
- [[container2wasm]] — container2wasm
- [[containerd]] — containerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 06-containerd-observability
- [[23-实体/15-参考与索引/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference


<!-- risk-assessed -->
