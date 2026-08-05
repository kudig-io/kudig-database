---
title: 'Day 13: K8S 集群监控'
description: 'title: Day 13: K8S 集群监控'
summary: 'title: Day 13: K8S 集群监控'
category: learning
tags:
- k8s
- training
- hands-on
- apiserver
- kubelet
- prometheus
- grafana
- daemonset
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
- 'Day 13: K8S 集群监控 是什么'
- '如何 Day 13: K8S 集群监控'
trigger_keywords:
- Day
- '13:'
- K8S
- 集群监控
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Day 13: K8S 集群监控
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - ACK ARMS [[Prometheus|Prometheus]] monitoring configuration
  - [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] PrometheusQuery PromQL queries
  - Grafana dashboard Kubernetes monitoring
  - PrometheusRule alerting rules configuration
  - kube-state-metrics cluster monitoring
trigger_keywords:
  - Prometheus
  - Grafana
  - ARMS
  - monitoring
  - alerting
  - metrics
  - PromQL
  - ServiceMonitor
  - kube-state-metrics
  - node-exporter
reading_level: intermediate
audience:
  - SRE engineers
  - ACK operators
  - Platform engineers
estimated_read_time: 45min
related_domains:
  - domain-06-observability
  - domain-10-troubleshooting-diagnostics
  - domain-12-cloud-providers
related_topics:
  - monitoring-metrics-system
  - alerting-management
  - prometheus-monitoring
---

# Day 13: K8S 集群监控

> **学习时间**: 4-5 小时 | **主题**: 监控体系搭建与告警配置

---

## 概述

监控是运维的"眼睛"，没有完善的监控体系，你就无法及时发现和定位问题。K8s 集群监控通常基于 Prometheus + Grafana 方案，在 ACK 中可以通过 ARMS Prometheus 托管服务快速接入。今天你将学习监控架构、核心指标含义、告警规则配置，以及如何编写 PromQL 查询语句来获取你关心的数据。

---

## 今日目标

- [ ] 理解 ACK 集群监控架构 (ARMS Prometheus + Grafana)
- [ ] 掌握核心监控指标的含义
- [ ] 能够查看和理解 Grafana Dashboard
- [ ] 了解告警规则配置

---

## 核心概念

### 1. K8s 监控架构

```
                  +-----------------+
                  |    Grafana      |  可视化 + 告警
                  +--------+--------+
                           |
                  +--------+--------+
                  |   Prometheus    |  采集 + 存储 + 计算
                  +--------+--------+
                           |
          +--------+-------+-------+--------+
          |        |               |        |
     +----+---+ +--+----+  +------+--+ +---+----+
     | Node   | | Pod   |  | App     | | K8s    |
     | Exporter| |cAdvisor| | Metrics | |Comp.   |
     +--------+ +-------+  +---------+ +--------+
```

### 2. Prometheus 数据模型

| 指标类型 | 说明 | 示例 | 典型函数 |
|----------|------|------|---------|
| Counter | 只增不减的累计值 | http_requests_total | rate(), increase() |
| Gauge | 可增可减的当前值 | node_memory_MemAvailable_bytes | 直接使用 |
| Histogram | 分布统计 | http_request_duration_seconds_bucket | histogram_quantile() |
| Summary | 客户端分位数 | http_request_duration_seconds_sum | 直接使用 |

### 3. 监控指标层级

| 层级 | 来源 | 关键指标 | 采集方式 |
|------|------|---------|---------|
| 节点级 | node-exporter | CPU、内存、磁盘、网络 | DaemonSet |
| 容器级 | cAdvisor (kubelet 内置) | 容器 CPU/内存/IO | kubelet /metrics/cadvisor |
| Pod 级 | kube-state-metrics | Pod 状态、重启次数、资源请求 | Deployment |
| 应用级 | 应用自身暴露 | HTTP QPS、延迟、错误率 | ServiceMonitor |
| 集群级 | 多源聚合 | 总节点数、Pod 调度率 | PromQL 聚合 |

---

## 理论学习 (2h)

### 必读文档

1. **监控指标系统**
   - 文件: `../../../domain-06-observability/02-monitoring-metrics-system.md`
   - 重点: Prometheus 数据模型、PromQL 基础

2. **告警管理**
   - 文件: `../../../domain-06-observability/05-alerting-management.md`
   - 重点: 告警规则、路由、抑制

---

## 实战演练 (2.5h)

### 任务 1: ACK 监控组件检查 (45min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 ARMS Prometheus 组件
kubectl get pods -n arms-prom
# NAME                                  READY   STATUS    RESTARTS   AGE
# arms-prometheus-xxxxxxxxxx-xxxxx      2/2     Running   0          30d
# arms-node-exporter-xxxxx              1/1     Running   0          30d

kubectl get svc -n arms-prom
# NAME                    TYPE        CLUSTER-IP      EXTERNAL-IP   PORT(S)
# arms-prometheus         ClusterIP   10.96.0.100     <none>        9090/TCP

# 检查 metrics-server
kubectl get pods -n kube-system -l k8s-app=metrics-server
# NAME                             READY   STATUS    RESTARTS   AGE
# metrics-server-xxxxxxxxx-xxxxx   1/1     Running   0          30d

# 查看节点资源使用
kubectl top nodes
# NAME       CPU(cores)   CPU%   MEMORY(bytes)   MEMORY%
# node-01    500m         25%    4096Mi          52%
# node-02    300m         15%    3072Mi          38%
# node-03    450m         22%    3584Mi          45%

# 查看 Pod 资源使用 Top 20
kubectl top pods -A --sort-by=cpu | head -20

# 查看 Prometheus 采集目标配置
kubectl get servicemonitors -A
# NAMESPACE    NAME                           AGE
# arms-prom    arms-prometheus                30d
# default      my-app-monitor                 10d

kubectl get podmonitors -A

# 检查 kube-state-metrics
kubectl get pods -n kube-system -l app=kube-state-metrics
```
---

### 任务 2: Grafana Dashboard 查看 (45min)

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 访问 Grafana (ACK 集成 ARMS)
# 控制台路径: 阿里云控制台 → ACK → 集群 → 运维管理 → Prometheus 监控

# 或通过 port-forward 访问自建 Grafana
kubectl port-forward -n monitoring svc/grafana 3000:80

# 核心 Dashboard 及关键指标:

# Dashboard 1: 集群概览
# - 节点数量: count(kube_node_status_condition{condition="Ready",status="true"})
# - Pod 总数: count(kube_pod_info)
# - CPU 使用率: avg(1 - rate(node_cpu_seconds_total{mode="idle"}[5m])) * 100
# - 内存使用率: (1 - sum(node_memory_MemAvailable_bytes) / sum(node_memory_MemTotal_bytes)) * 100

# Dashboard 2: 节点详情
# - CPU 使用率: (1 - rate(node_cpu_seconds_total{mode="idle"}[5m])) by (instance) * 100
# - 内存使用率: (1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) * 100
# - 磁盘使用率: (1 - node_filesystem_avail_bytes{mountpoint="/"} / node_filesystem_size_bytes{mountpoint="/"}) * 100
# - 网络流量: rate(node_network_receive_bytes_total{device="eth0"}[5m])
# - 磁盘 IO: rate(node_disk_read_bytes_total[5m])

# Dashboard 3: Pod 监控
# - CPU 使用: sum(rate(container_cpu_usage_seconds_total{container!="POD"}[5m])) by (namespace, pod)
# - 内存使用: container_memory_working_set_bytes{container!="POD"}
# - 重启次数: kube_pod_container_status_restarts_total
# - OOM 事件: kube_pod_container_status_last_terminated_reason{reason="OOMKilled"}

# Dashboard 4: API Server
# - 请求 QPS: sum(rate(apiserver_request_total[5m])) by (verb)
# - 请求延迟 P99: histogram_quantile(0.99, rate(apiserver_request_duration_seconds_bucket[5m]))
# - 错误率: sum(rate(apiserver_request_total{code=~"5.."}[5m])) / sum(rate(apiserver_request_total[5m]))
```
---

### 任务 3: 自定义告警规则 (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat > ack-alerts.yaml << 'EOF'
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ack-custom-alerts
  namespace: arms-prom
spec:
  groups:
  - name: node-alerts
    rules:
    - alert: NodeHighCPU
      expr: (1 - avg(rate(node_cpu_seconds_total{mode="idle"}[5m])) by (instance)) > 0.85
      for: 5m
      labels:
        severity: warning
        team: sre
      annotations:
        summary: "节点 {{ $labels.instance }} CPU 使用率超过 85%"
        description: "节点 {{ $labels.instance }} CPU 使用率已达 {{ $value | humanizePercentage }}，持续超过 5 分钟"

    - alert: NodeHighMemory
      expr: (1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) > 0.9
      for: 5m
      labels:
        severity: warning
        team: sre
      annotations:
        summary: "节点 {{ $labels.instance }} 内存使用率超过 90%"

    - alert: NodeDiskPressure
      expr: node_filesystem_avail_bytes{mountpoint="/"} / node_filesystem_size_bytes{mountpoint="/"} < 0.1
      for: 5m
      labels:
        severity: critical
        team: sre
      annotations:
        summary: "节点 {{ $labels.instance }} 磁盘空间不足 10%"

    - alert: NodeNotReady
      expr: kube_node_status_condition{condition="Ready",status="true"} == 0
      for: 3m
      labels:
        severity: critical
        team: sre
      annotations:
        summary: "节点 {{ $labels.node }} 不可达，持续超过 3 分钟"

  - name: pod-alerts
    rules:
    - alert: PodCrashLooping
      expr: rate(kube_pod_container_status_restarts_total[15m]) > 0
      for: 5m
      labels:
        severity: critical
        team: sre
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 持续重启"
        runbook: "检查日志: kubectl logs {{ $labels.pod }} -n {{ $labels.namespace }} --previous"

    - alert: PodOOMKilled
      expr: kube_pod_container_status_last_terminated_reason{reason="OOMKilled"} == 1
      for: 1m
      labels:
        severity: warning
        team: sre
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 被 OOMKilled"
        runbook: "增大 limits.memory 或检查内存泄漏"

    - alert: PodPending
      expr: kube_pod_status_phase{phase="Pending"} == 1
      for: 10m
      labels:
        severity: warning
        team: sre
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 处于 Pending 超过 10 分钟"

    - alert: DeploymentReplicasMismatch
      expr: kube_deployment_status_replicas_available != kube_deployment_spec_replicas
      for: 15m
      labels:
        severity: warning
        team: sre
      annotations:
        summary: "Deployment {{ $labels.namespace }}/{{ $labels.deployment }} 可用副本数不匹配"

  - name: cluster-alerts
    rules:
    - alert: APIServerHighLatency
      expr: histogram_quantile(0.99, rate(apiserver_request_duration_seconds_bucket[5m])) > 1
      for: 5m
      labels:
        severity: warning
        team: sre
      annotations:
        summary: "API Server P99 延迟超过 1 秒"

    - alert: HighErrorRate
      expr: sum(rate(apiserver_request_total{code=~"5.."}[5m])) / sum(rate(apiserver_request_total[5m])) > 0.05
      for: 5m
      labels:
        severity: critical
        team: sre
      annotations:
        summary: "API Server 错误率超过 5%"
EOF

kubectl apply -f ack-alerts.yaml

# 验证规则已加载
kubectl get prometheusrules -n arms-prom
kubectl describe prometheusrule ack-custom-alerts -n arms-prom
```
---

### 任务 4: 常用 PromQL 查询 (30min)

```bash
# === 集群级指标 ===

# 集群 CPU 使用率 (%)
avg(1 - rate(node_cpu_seconds_total{mode="idle"}[5m])) * 100

# 集群内存使用率 (%)
(1 - sum(node_memory_MemAvailable_bytes) / sum(node_memory_MemTotal_bytes)) * 100

# 集群 Pod 总数
count(kube_pod_info)

# 集群 Running Pod 数
count(kube_pod_status_phase{phase="Running"})

# === 节点级指标 ===

# 各节点 CPU 使用率 (%)
(1 - rate(node_cpu_seconds_total{mode="idle"}[5m])) by (instance) * 100

# 各节点内存使用率 (%)
(1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) * 100

# 各节点磁盘使用率 (%)
(1 - node_filesystem_avail_bytes{mountpoint="/"} / node_filesystem_size_bytes{mountpoint="/"}) * 100

# 各节点网络流入 (bytes/s)
rate(node_network_receive_bytes_total{device="eth0"}[5m]) by (instance)

# === Pod 级指标 ===

# Pod 重启次数 Top 10
topk(10, sum(kube_pod_container_status_restarts_total) by (namespace, pod))

# Pod CPU 使用 Top 10
topk(10, sum(rate(container_cpu_usage_seconds_total{container!="POD",container!=""}[5m])) by (namespace, pod))

# Pod 内存使用 Top 10
topk(10, sum(container_memory_working_set_bytes{container!="POD",container!=""}) by (namespace, pod))

# === API Server 指标 ===

# API Server 请求延迟 P99
histogram_quantile(0.99, sum(rate(apiserver_request_duration_seconds_bucket[5m])) by (le, verb))

# API Server 请求 QPS 按 verb 分类
sum(rate(apiserver_request_total[5m])) by (verb)

# API Server 错误率 (%)
sum(rate(apiserver_request_total{code=~"5.."}[5m])) / sum(rate(apiserver_request_total[5m])) * 100

# === 资源配额指标 ===

# Namespace CPU 请求使用率
sum(kube_pod_container_resource_requests{resource="cpu"}) by (namespace) / sum(kube_node_status_allocatable{resource="cpu"}) by (namespace) * 100

# 命名空间内存请求量
sum(kube_pod_container_resource_requests{resource="memory"}) by (namespace)
```

---

## 费曼复述 (0.5h)

1. **ACK 集群监控的架构是什么？各组件的作用是什么？**
2. **哪些监控指标是集群运维最关键的？为什么？**
3. **如何配置一个"节点 CPU 过高"的告警规则？**
4. **rate() 和 irate() 函数有什么区别？**

---

## 今日检验

- [ ] 能查看 ACK 集群的监控数据
- [ ] 理解核心监控指标的含义
- [ ] 能创建自定义告警规则
- [ ] 能编写基础 PromQL 查询

---

## 配置参考

### 告警规则配置模板

```yaml
- alert: <AlertName>
  expr: <PromQL Expression>
  for: <Duration>
  labels:
    severity: <critical|warning|info>
    team: <team-name>
  annotations:
    summary: "<简短描述>"
    description: "<详细描述>"
    runbook: "<修复步骤>"
```

### 告警严重级别定义

| 级别 | 含义 | 响应时间 | 通知方式 |
|------|------|---------|---------|
| critical | 服务中断或即将中断 | 5 分钟内 | 电话 + IM + 短信 |
| warning | 潜在风险 | 30 分钟内 | IM + 邮件 |
| info | 信息性告警 | 下一个工作日 | 邮件 |

---

## 常见问题

### Q1: Prometheus 数据保留多久？

ARMS Prometheus 默认数据保留 15 天，可通过控制台调整（最长 180 天）。自建 Prometheus 通过 `--storage.tsdb.retention.time` 参数设置。

### Q2: 如何为应用添加自定义监控指标？

1. 应用暴露 `/metrics` 端点 (Prometheus 格式)
2. 创建 ServiceMonitor 资源:

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: my-app-monitor
spec:
  selector:
    matchLabels:
      app: my-app
  endpoints:
  - port: http
    path: /metrics
    interval: 30s
```

### Q3: 告警太多导致告警疲劳怎么办？

- 配置告警抑制规则 (inhibit_rules)，相关告警只触发最高级别
- 设置合理的 `for` 持续时间，避免瞬时波动触发
- 使用 recording rules 预聚合，减少重复计算
- 定期审查告警规则，删除无效告警

---

## 要点总结

| 监控维度 | 关键指标 | 告警阈值 (参考) | 采集来源 |
|----------|---------|----------------|---------|
| 节点 CPU | node_cpu_seconds_total | > 85% 持续 5min | node-exporter |
| 节点内存 | node_memory_MemAvailable_bytes | > 90% 持续 5min | node-exporter |
| 节点磁盘 | node_filesystem_avail_bytes | < 10% 可用 | node-exporter |
| Pod 重启 | kube_pod_container_status_restarts_total | > 0 次/15min | kube-state-metrics |
| API 延迟 | apiserver_request_duration_seconds | P99 > 1s | kube-apiserver |
| Pod 状态 | kube_pod_status_phase | Pending > 10min | kube-state-metrics |

---

## 明日预告

Day 14 将学习集群资源配额与 License 管理。

---

## 延伸阅读

- [监控指标系统](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-06-observability/02-metrics/01-monitoring-metrics-system.md)
- [告警管理](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-06-observability/05-alerting/04-alerting-management.md)
- [Prometheus 监控](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-06-observability/02-metrics/04-monitoring-metrics-prometheus.md)
- [可观测性架构总览](../../domain-06-observability/01-observability-architecture-overview.md)

```

<!-- risk-assessed -->
