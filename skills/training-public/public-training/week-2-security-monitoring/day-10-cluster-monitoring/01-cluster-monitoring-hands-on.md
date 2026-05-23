---
title: 'Day 10: K8s 集群监控体系搭建实操'
description: '# Day 10: K8s 集群监控体系搭建实操'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- apiserver
- kubelet
- scheduler
- prometheus
- grafana
- helm
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 10: K8s 集群监控体系搭建实操 是什么'
- '如何 Day 10: K8s 集群监控体系搭建实操'
trigger_keywords:
- Day
- '10:'
- K8s
- 集群监控体系搭建实操
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
created: "2026-05-23"
---

# Day 10: K8s 集群监控体系搭建实操

> **日期**: Week 2 Day 3 | **主题**: 监控体系搭建与告警配置 | **版本**: K8s 1.28-1.33

---

## 1. 监控架构概述

### 1.1 三层监控指标

| 层 | 指标 | 采集工具 | 说明 |
|---|------|---------|------|
| 基础设施层 | CPU/内存/磁盘/网络 | node_exporter | 节点级别资源使用 |
| [[entities/kubernetes|[[Kubernetes|kubernetes]]]] 层 | Pod/Deployment/Node 状态 | kube-state-metrics | K8s 对象状态 |
| 应用层 | 业务指标（QPS/Latency/Error） | 应用自暴露 | Pod 内应用 metrics |

### 1.2 监控组件清单

```
Prometheus Operator (监控中枢)
  ├── node_exporter (节点指标)
  ├── kube-state-metrics (K8s 对象状态)
  ├── cAdvisor (容器资源)
  ├── blackbox_exporter (探测)
  └── alertmanager (告警)
```

---

## 2. 部署 Prometheus Operator

### 2.1 使用 kube-prometheus-stack

```bash
# 添加 Prometheus Community Helm repo
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo update

# 安装 kube-prometheus-stack（生产推荐配置）
helm install prometheus prometheus-community/kube-prometheus-stack \
  --namespace monitoring \
  --create-namespace \
  --set prometheus.prometheusSpec.retention=30d \
  --set prometheus.prometheusSpec.replicas=2 \
  --set prometheus.prometheusSpec.storageSpec.volumeClaimTemplate.spec.resources.requests.storage=100Gi \
  --set alertmanager.persistentVolume.storageClass=standard \
  --set grafana.adminPassword=changeme \
  --set prometheusOperator.tls.enabled=true
```

### 2.2 关键组件验证

```bash
# 检查所有组件状态
kubectl get pods -n monitoring

# 预期输出:
# prometheus-operator-xxx        1/1 Running
# prometheus-prometheus-xxx     2/2 Running
# alertmanager-prometheus-xxx   2/2 Running
# grafana-xxx                   1/1 Running
# node-exporter-xxx             1/1 Running
# kube-state-metrics-xxx        1/1 Running

# 查看 Prometheus targets
kubectl exec -it -n monitoring prometheus-prometheus-0 -- wget -O- localhost:9090/targets | grep -i "health"
```

---

## 3. 核心监控指标解读

### 3.1 节点层指标

```promql
# CPU 使用率
100 - (avg by (instance) (rate(node_cpu_seconds_total{mode="idle"}[5m])) * 100)

# 内存使用率
100 - (avg by (instance) (node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) * 100)

# 磁盘使用率
100 - (node_filesystem_avail_bytes{mountpoint="/"} / node_filesystem_size_bytes{mountpoint="/"} * 100)

# 磁盘 I/O 延迟
rate(node_disk_io_time_seconds_total[5m]) * 100  # > 50% 说明 I/O 瓶颈
```

### 3.2 Kubernetes 层指标

```promql
# Pod CPU 使用率（相对于 limit）
rate(container_cpu_usage_seconds_total{container!=""}[5m]) / on(container,pod,namespace)
  kube_pod_container_resource_limits{resource="cpu"}

# Pod 内存使用率（相对于 limit）
container_memory_working_set_bytes / on(container,pod,namespace)
  kube_pod_container_resource_limits{resource="memory"}

# Deployment 可用副本数
kube_deployment_status_replicas_available / kube_deployment_status_replicas

# Node Ready 状态
kube_node_status_condition{condition="Ready",status="true"}

# Pending Pod 数量
kube_pod_status_phase{phase="Pending"}

# PV/PVC 使用率
kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes
```

### 3.3 应用层指标

```promql
# 请求 QPS（假设应用暴露了 http_requests_total）
sum(rate(http_requests_total[5m])) by (service)

# 请求延迟 P99
histogram_quantile(0.99, sum(rate(http_request_duration_seconds_bucket[5m])) by (le, service))

# 错误率（5xx）
sum(rate(http_requests_total{status=~"5.."}[5m])) by (service) /
sum(rate(http_requests_total[5m])) by (service)
```

---

## 4. 告警规则配置

### 4.1 节点层告警

```yaml
# PrometheusRule 示例
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: node-alerts
  namespace: monitoring
spec:
  groups:
    - name: node-resources
      rules:
        - alert: NodeCPUHigh
          expr: 100 - (avg by (instance) (rate(node_cpu_seconds_total{mode="idle"}[5m])) * 100) > 85
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "节点 CPU 使用率超过 85%"
            description: "节点 {{ $labels.instance }} CPU 使用率 {{ $value }}%"

        - alert: NodeMemoryHigh
          expr: 100 - (node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes * 100) > 90
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "节点内存使用率超过 90%"

        - alert: NodeDiskSpaceLow
          expr: (node_filesystem_avail_bytes{mountpoint="/"} / node_filesystem_size_bytes) < 0.1
          for: 10m
          labels:
            severity: critical
          annotations:
            summary: "节点磁盘空间不足 10%"
```

### 4.2 控制平面告警

```yaml
    - name: control-plane
      rules:
        - alert: APIServerDown
          expr: up{job="apiserver"} == 0
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "API Server 不可用"

        - alert: KubeSchedulerUnhealthy
          expr: up{job="kube-scheduler"} == 0
          for: 3m
          labels:
            severity: critical
          annotations:
            summary: "Scheduler 不可用"

        - alert: EtcdLeaderChanges
          expr: rate(etcd_server_leader_changes_seen_total[5m]) > 0.5
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "etcd leader 频繁切换"
```

### 4.3 应用层告警

```yaml
        - alert: PodMemoryUsageHigh
          expr: |
            kube_pod_container_resource_usage{resource="memory"} > 0.9
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Pod 内存使用率超过 90%"

        - alert: HPAAtMaxReplicas
          expr: kube_hpa_status_condition{condition="ScalingActive",status="false"} == 1
          for: 2m
          labels:
            severity: warning
          annotations:
            summary: "HPA 无法扩容"

        - alert: DeploymentReplicasMismatch
          expr: |
            kube_deployment_status_replicas_available !=
            kube_deployment_status_replicas
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "Deployment 副本数不匹配"
```

---

## 5. Grafana 仪表盘配置

### 5.1 导入官方仪表盘

```bash
# 常用仪表盘 ID（可直接在 Grafana UI 导入）
# Kubernetes / Nodes - 15764
# Kubernetes / Pods - 15765
# Kubernetes / Deployment - 15762
# etcd - 12130
# API Server - 15424

# 使用 kubectl 导入
kubectl apply -f - <<'EOF'
apiVersion: v1
kind: ConfigMap
metadata:
  name: grafana-import-dashboard
  namespace: monitoring
data:
  dashboards.yaml: |
    {"default": {"path": "/var/lib/grafana/dashboards"}}
EOF
```

### 5.2 自定义仪表盘指标

| 视图 | 核心指标 | PromQL |
|------|---------|--------|
| 集群总览 | CPU/内存使用率 | `cluster:node_cpu_usage:ratio` |
| 命名空间 | NS 级资源使用 | `namespace:container_memory_usage:sum` |
| Pod 详情 | 单 Pod QPS/Latency | `rate(http_requests_total{service="$service"}[5m])` |
| 有状态应用 | PVC 使用率 | `kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes` |

---

## 6. AlertManager 告警路由

### 6.1 配置路由规则

```yaml
# alertmanager-config.yaml
global:
  resolve_timeout: 5m

route:
  receiver: "default"
  group_wait: 30s
  group_interval: 5m
  repeat_interval: 4h
  routes:
    - match:
        severity: critical
      receiver: "critical-alert"
      group_wait: 10s
      continue: true
    - match:
        severity: warning
      receiver: "warning-alert"
    - match:
        alertname: "Watchdog"
      receiver: "watchdog"

receivers:
  - name: "default"
    email_configs:
      - to: "ops-team@example.com"
        send_resolved: true
  - name: "critical-alert"
    pagerduty_configs:
      - service_key: "YOUR_PD_KEY"
        severity: critical
    webhook_configs:
      - url: "https://oapi.dingtalk.com/robot/send?access_token=YOUR_TOKEN"
  - name: "warning-alert"
    email_configs:
      - to: "ops-team@example.com"
  - name: "watchdog"
    webhook_configs:
      - url: "https://oapi.dingtalk.com/robot/send?access_token=YOUR_TOKEN"
```

---

## 7. 监控故障排查

| 问题 | 诊断 | 修复 |
|------|------|------|
| Prometheus 收集不到数据 | 检查 ServiceMonitor 是否正确配置 | 确认 label 和 namespace selector |
| 指标缺失 | 检查 endpoint 是否 up | `kubectl get endpoints -n monitoring` |
| Grafana 无数据 | 检查数据源配置 | 确认 Prometheus URL 可达 |
| 告警未触发 | 检查 PrometheusRule 配置 | `kubectl get prometheusrules -n monitoring` |

---

## 8. 实战练习

**练习 1**: 使用 helm 安装 kube-prometheus-stack，配置 30 天数据保留

**练习 2**: 配置节点 CPU > 85% 持续 5 分钟触发 critical 告警

**练习 3**: 配置 AlertManager 将 critical 告警发送到钉钉，warning 告警发送到邮件

**练习 4**: 导入 etcd 和 API Server 官方 Grafana 仪表盘，验证数据正确性

---

```yaml
---
id: LEARN-WEEK2-DAY10
title: Day 10 - K8s 集群监控体系搭建实操
topic: security-monitoring
type: hands-on-guide
tags: [monitoring, prometheus, grafana, alertmanager, metrics, hands-on, k8s-1.28-1.33]
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - "Prometheus Operator 怎么部署"
  - "K8s 监控告警规则怎么配"
  - "Grafana 仪表盘怎么导入"
  - "AlertManager 路由规则怎么写"
  - "kube-prometheus-stack 部署"
trigger_keywords:
  - Prometheus
  - Grafana
  - AlertManager
  - kube-state-metrics
  - node-exporter
  - ServiceMonitor
  - PrometheusRule
  - 告警规则
  - 监控指标
  - 自定义仪表盘
  - 告警路由
reading_level: advanced
audience:
  - sre
  - ops-engineer
estimated_read_time: 50min
related_domains:
  - domain-06-observability
  - domain-20-enterprise-monitoring-alerting
related_topics:
  - monitoring
  - prometheus
  - grafana
  - alertmanager
  - observability
related:
  - domain-06-observability/01-prometheus-operator-deep-dive.md
  - domain-06-observability/05-alert-manager-configuration.md
---
```