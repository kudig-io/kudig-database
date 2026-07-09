---
title: 'Day 17: 可观测性 - 监控 + Prometheus'
description: 'title: Day 17: 可观测性 - 监控 + Prometheus'
summary: 'title: Day 17: 可观测性 - 监控 + Prometheus'
category: learning
tags:
- k8s
- training
- hands-on
- prometheus
- grafana
- helm
- gateway
- crd
- operator
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 17: 可观测性 - 监控 + Prometheus 是什么'
- '如何 Day 17: 可观测性 - 监控 + Prometheus'
trigger_keywords:
- Day
- '17:'
- 可观测性
- 监控
- Prometheus
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- logging-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Day 17: 可观测性 - 监控 + [[Prometheus|Prometheus]]
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - [[Kubernetes|kubernetes]] Prometheus 监控
  - K8s 可观测性架构
  - Prometheus 查询语言 PromQL
  - Grafana Dashboard 配置
trigger_keywords:
  - Prometheus
  - Grafana
  - 监控
  - 可观测性
  - Metrics
  - PromQL
  - Alertmanager
  - kube-prometheus-stack
reading_level: intermediate
audience:
  - sre-engineer
  - devops-engineer
  - platform-engineer
estimated_read_time: 240min
related_domains:
  - 可观测性
related_topics:
  - 生产运维/topic-learn/public-training/one-month/week-3-operations/day-18-observability-2
  - 生产运维/topic-learn/public-training/one-month/week-3-operations/day-21-platform-ops
  - 生产运维/topic-learn/public-training/one-month/projects/p3-observability-fault-drill
---

# Day 17: 可观测性 - 监控 + Prometheus

## 概述

今天进入可观测性体系的学习。可观测性（Observability）是现代运维的核心能力，它由三大支柱组成：**Metrics（指标）**、**Logs（日志）**和**Traces（分布式追踪）**。今天聚焦于 Metrics——如何使用 Prometheus 构建完整的监控体系。

如果说 K8s 是一辆汽车，可观测性就是仪表盘和传感器。没有监控，你不知道车速、油量、引擎温度；没有日志，你不知道引擎发生了什么异常；没有追踪，你不知道请求在系统中走了哪条路径。可观测性让你能够"看见"系统的运行状态，快速发现和定位问题。

### 学习目标

- 理解可观测性三大支柱及其在 K8s 中的应用
- 掌握 Prometheus 的数据模型和 PromQL 查询语言
- 能够部署 kube-prometheus-stack 监控栈
- 能够配置告警规则和 Grafana Dashboard
- 理解 Alertmanager 的告警路由和分组机制

---

## 核心概念详解

### 可观测性三大支柱

**Metrics（指标）** 是系统运行状态的数字化表示。指标是预定义的数值测量，适合回答"现在怎么样"的问题。例如：当前 CPU 使用率是多少？过去 5 分钟的请求错误率是多少？指标的优势是存储成本低、查询速度快，适合大规模系统和长期趋势分析。Prometheus 是 K8s 生态中最流行的指标采集和存储系统。

**Logs（日志）** 记录了系统中的离散事件。日志包含丰富的上下文信息（时间戳、级别、消息、调用栈等），适合回答"发生了什么"的问题。例如：为什么这个请求返回了 500？应用在什么时候开始报错？日志的缺点是数据量大、查询成本高。Loki 和 ELK Stack 是 K8s 中常用的日志聚合方案。

**Traces（分布式追踪）** 记录了一个请求在分布式系统中经过的完整路径。在微服务架构中，一个用户请求可能经过 API Gateway → User Service → Order Service → Payment Service → Database。分布式追踪帮你回答"问题出在哪里"：是哪个服务导致了延迟？是哪个数据库查询最慢？OpenTelemetry 是 CNCF 推荐的追踪标准。

三者之间的关系：Metrics 告诉你"有问题"（错误率升高），Logs 告诉你"什么问题"（错误信息），Traces 告诉你"问题在哪"（具体的服务调用链路）。

### Prometheus 数据模型

Prometheus 存储的是**时间序列（Time Series）**数据。每个时间序列由以下要素唯一标识：

- **指标名称（Metric Name）**: 描述测量的是什么。如 `http_requests_total`（HTTP 请求总数）、`container_cpu_usage_seconds_total`（容器 CPU 使用时间）
- **标签（Labels）**: 键值对，提供维度信息。如 `method="GET"`, `status="200"`, `namespace="production"`。标签用于过滤和聚合查询

一个时间序列的完整表示：`http_requests_total{method="GET", status="200", namespace="production"}`

Prometheus 的四种指标类型：

**Counter（计数器）**: 只增不减的累计值。适用于请求数、错误数、字节数等。Counter 的绝对值通常没有意义，有意义的是它的变化率。使用 `rate()` 函数计算每秒增长率：

```
# 过去 5 分钟内 HTTP 请求的每秒增长率
rate(http_requests_total[5m])
```

**Gauge（仪表盘）**: 可增可减的当前值。适用于温度、内存使用量、当前连接数等。Gauge 的绝对值就有意义：

```
# 当前内存使用量
node_memory_MemAvailable_bytes
```

**Histogram（直方图）**: 对观测值进行采样并统计分布。将观测值放入预定义的桶（bucket）中，同时计算总和和计数。常用于延迟分析：

```
# P99 延迟
histogram_quantile(0.99, sum(rate(http_request_duration_seconds_bucket[5m])) by (le))
```

**Summary（摘要）**: 类似 Histogram，但在客户端计算分位数。适用于不需要聚合的场景（如单实例的延迟监控）。

### PromQL 查询语言

PromQL 是 Prometheus 的查询语言，支持丰富的数据操作：

**即时查询（Instant Query）**: 返回某个时间点的数据

```
# 所有 Pod 的 CPU 使用率
sum(rate(container_cpu_usage_seconds_total{container!="POD"}[5m])) by (pod)
```

**范围查询（Range Query）**: 返回一段时间范围内的数据

```
# 过去 1 小时的内存使用趋势
container_memory_usage_bytes{container!="POD"}[1h]
```

**常用函数**:

- `rate(metric[5m])`: 计算计数器的每秒增长率
- `irate(metric[5m])`: 计算最后两个数据点的瞬时增长率
- `histogram_quantile(0.99, ...)`: 计算分位数
- `sum(...) by (label)`: 按标签分组求和
- `avg(...) by (label)`: 按标签分组求平均
- `topk(10, metric)`: 取前 10 个最大的值
- `predict_linear(metric[1h], 3600)`: 线性预测未来 1 小时的值

**常用监控查询**:

```
# 节点 CPU 使用率
sum(rate(node_cpu_seconds_total{mode!="idle"}[5m])) by (instance) / sum(rate(node_cpu_seconds_total[5m])) by (instance) * 100

# 节点内存使用率
(1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) * 100

# Pod CPU 使用率
sum(rate(container_cpu_usage_seconds_total{container!="POD", container!=""}[5m])) by (namespace, pod)

# Pod 内存使用
container_memory_working_set_bytes{container!="POD", container!=""}

# 请求错误率
sum(rate(http_requests_total{status=~"5.."}[5m])) / sum(rate(http_requests_total[5m])) * 100
```

### Prometheus 在 K8s 中的部署

**kube-prometheus-stack** 是最流行的 K8s 监控方案，它包含以下组件：

- **Prometheus Operator**: 管理 Prometheus 和 Alertmanager 实例的生命周期
- **Prometheus**: 指标采集和存储
- **Alertmanager**: 告警路由、分组和通知
- **Grafana**: 数据可视化和告警
- **Node Exporter**: 节点级指标采集
- **kube-state-metrics**: K8s 对象状态指标

Prometheus Operator 通过 CRD 管理监控配置：

- **ServiceMonitor**: 定义如何发现和采集服务的指标
- **PodMonitor**: 定义如何发现和采集 Pod 的指标
- **PrometheusRule**: 定义告警规则和 Recording Rules
- **AlertmanagerConfig**: 定义告警路由和通知配置

### Alertmanager 告警管理

Alertmanager 处理 Prometheus 发送的告警，负责去重、分组、路由和通知。

**分组（Grouping）**: 将相关的告警合并为一个通知。配置 `group_by: ['alertname', 'namespace']`，同一命名空间中相同名称的告警会被合并。

**抑制（Inhibition）**: 当某个高级别告警触发时，自动静默相关的低级别告警。例如："集群不可达"告警触发时，抑制该集群下所有的"Pod 异常"告警。

**静默（Silencing）**: 在维护窗口或已知问题期间，暂时关闭特定告警的通知。

**路由（Routing）**: 根据告警的标签将告警发送到不同的通知渠道。例如：`severity=critical` 发送到 PagerDuty 和电话，`severity=warning` 发送到钉钉群。

---

## 实战演练

### 任务 1: 部署 kube-prometheus-stack (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 添加 Helm repo
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo update

# 安装 kube-prometheus-stack
helm install prometheus prometheus-community/kube-prometheus-stack \
  --namespace monitoring \
  --create-namespace \
  --set prometheus.prometheusSpec.retention=7d \
  --set prometheus.prometheusSpec.storageSpec.volumeClaimTemplate.spec.storageClassName=standard \
  --set prometheus.prometheusSpec.storageSpec.volumeClaimTemplate.spec.resources.requests.storage=50Gi

# 等待所有组件就绪
kubectl wait --namespace monitoring --for=condition=ready pod -l app.kubernetes.io/instance=prometheus --timeout=300s

# 查看部署的组件
kubectl get pods -n monitoring
kubectl get svc -n monitoring

# 查看 Prometheus 采集的 Target
kubectl port-forward -n monitoring svc/prometheus-operated 9090:9090
# 浏览器访问 http://localhost:9090/targets
```
### 任务 2: PromQL 查询实践 (45min)

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 访问 Prometheus UI
kubectl port-forward -n monitoring svc/prometheus-operated 9090:9090

# 在 Prometheus UI 中执行以下查询:

# 1. 节点 CPU 使用率（百分比）
# sum(rate(node_cpu_seconds_total{mode!="idle"}[5m])) by (instance) * 100

# 2. 节点内存使用率
# (1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) * 100

# 3. 磁盘使用率
# (1 - node_filesystem_avail_bytes{fstype!~"tmpfs|fuse.*"} / node_filesystem_size_bytes{fstype!~"tmpfs|fuse.*"}) * 100

# 4. Pod CPU 使用率（按 Pod 分组）
# sum(rate(container_cpu_usage_seconds_total{container!="POD", container!=""}[5m])) by (namespace, pod) * 1000

# 5. Pod 内存使用量
# container_memory_working_set_bytes{container!="POD", container!=""}

# 6. K8s 组件状态
# kube_deployment_status_replicas_unavailable > 0

# 7. 容器重启次数
# sum(kube_pod_container_status_restarts_total) by (namespace, pod)

# 8. 资源请求 vs 实际使用（集群级别）
# sum(kube_resourcequota{type="hard", resource="requests.cpu"}) - sum(kube_node_status_allocatable{resource="cpu"})
```
### 任务 3: 告警规则配置 (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建自定义告警规则
cat > alert-rules.yaml << 'EOF'
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: custom-alerts
  namespace: monitoring
  labels:
    prometheus: kube-prometheus
    role: alert-rules
spec:
  groups:
  - name: pod-alerts
    rules:
    - alert: PodCrashLooping
      expr: rate(kube_pod_container_status_restarts_total[15m]) * 60 * 5 > 0
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} is crash looping"
        description: "Pod has restarted {{ $value }} times in the last 5 minutes"
    - alert: PodNotReady
      expr: sum by (namespace, pod) (kube_pod_status_ready{condition="false"}) == 1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} is not ready"
    - alert: PodPending
      expr: kube_pod_status_phase{phase="Pending"} == 1
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} is pending for 10 minutes"
  - name: node-alerts
    rules:
    - alert: NodeNotReady
      expr: kube_node_status_condition{condition="Ready", status="unknown"} == 1
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Node {{ $labels.node }} is NotReady"
    - alert: HighMemoryUsage
      expr: (1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) > 0.9
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Node {{ $labels.instance }} memory usage > 90%"
    - alert: HighDiskUsage
      expr: (1 - node_filesystem_avail_bytes{fstype!~"tmpfs|fuse.*"} / node_filesystem_size_bytes{fstype!~"tmpfs|fuse.*"}) > 0.85
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Node {{ $labels.instance }} disk {{ $labels.mountpoint }} usage > 85%"
  - name: resource-alerts
    rules:
    - alert: DeploymentReplicasMismatch
      expr: kube_deployment_spec_replicas != kube_deployment_status_available_replicas
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "Deployment {{ $labels.namespace }}/{{ $labels.deployment }} replicas mismatch"
        description: "Expected {{ $value }} replicas but not all are available"
EOF

kubectl apply -f alert-rules.yaml

# 验证规则已加载
kubectl get prometheusrule -n monitoring
```
### 任务 4: Grafana Dashboard (30min)

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 访问 Grafana
kubectl port-forward -n monitoring svc/prometheus-grafana 3000:80

# 默认登录: admin / prom-operator
# 首次登录后建议修改密码

# 导入社区 Dashboard:
# 1. 左侧菜单 → Dashboards → Import
# 2. 输入 Dashboard ID:
#    - 315: Kubernetes cluster monitoring (经典集群监控)
#    - 6417: Kubernetes pods monitoring (Pod 级别监控)
#    - 1860: Node Exporter Full (节点详细监控)
#    - 7249: Kubernetes Deployment (Deployment 监控)
#    - 15760: Kubernetes Views Pods (Pod 概览)
# 3. 选择 Prometheus 数据源 → Import
```
---

## 常见问题

### Q1: Prometheus 的数据保留期应该设置多长？

取决于你的存储容量和需求。建议：开发环境 7 天，预发环境 15 天，生产环境 30 天。如果需要更长的保留期，使用 Thanos 将数据上传到对象存储。Prometheus 本地存储（TSDB）不适合长期存储，每个 Prometheus 实例建议不超过 50GB 数据。

### Q2: rate() 和 irate() 的区别？

`rate()` 计算指定时间窗口内的平均增长率，结果平滑，适合告警（避免瞬时的抖动触发告警）。`irate()` 只使用最后两个数据点计算瞬时增长率，结果灵敏，适合图表展示（能反映瞬时的变化）。建议告警规则使用 `rate()`，图表展示使用 `irate()`。

### Q3: Prometheus 的 Target 显示"down"怎么排查？

Target down 意味着 Prometheus 无法从目标拉取指标。排查步骤：1) 点击 Target URL 检查是否可以手动访问；2) 检查网络策略是否阻止了 Prometheus 到目标的通信；3) 检查目标 Pod 是否在运行且端口正确；4) 检查 ServiceMonitor/PodMonitor 的标签选择器是否匹配。

### Q4: 如何监控自定义的业务指标？

应用需要通过 `/metrics` 端点暴露 Prometheus 格式的指标。然后创建 ServiceMonitor 让 Prometheus 自动发现和采集。大多数编程语言都有 Prometheus 客户端库（如 Python 的 prometheus_client、Java 的 micrometer、Go 的 prometheus/client_golang）。

---

## 要点总结

| 知识点 | 要点 |
|--------|------|
| 可观测性三大支柱 | Metrics（指标）、Logs（日志）、Traces（追踪） |
| Prometheus 数据模型 | 时间序列 = 指标名称 + 标签 + 时间戳 + 值 |
| 四种指标类型 | Counter、Gauge、Histogram、Summary |
| PromQL | rate/irate/histogram_quantile/sum by/avg by |
| 告警管理 | Alertmanager 分组、抑制、静默、路由 |

---

## 延伸阅读

- [可观测性架构总览](../../可观测性/01-observability-architecture-overview.md)
- [监控指标系统](../../可观测性/02-monitoring-metrics-system.md)
- [Prometheus 生产级配置](../../可观测性/10-monitoring-metrics-prometheus.md)
- [告警管理](../../可观测性/05-alerting-management.md)
- [SLO/SLI 体系](../../可观测性/18-slo-sli-system.md)

## Related

- [[生态参考/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
