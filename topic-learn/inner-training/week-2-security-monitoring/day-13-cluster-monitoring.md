# Day 13: K8S 集群监控

> **学习时间**: 4-5 小时 | **主题**: 监控体系搭建与告警配置

---

## 今日目标

- [ ] 理解 ACK 集群监控架构 (ARMS Prometheus + Grafana)
- [ ] 掌握核心监控指标的含义
- [ ] 能够查看和理解 Grafana Dashboard
- [ ] 了解告警规则配置

---

## 理论学习 (2h)

### 必读文档

1. **监控指标系统**
   - 文件: `../../../domain-8-observability/02-monitoring-metrics-system.md`
   - 重点: Prometheus 数据模型、PromQL 基础

2. **告警管理**
   - 文件: `../../../domain-8-observability/05-alerting-management.md`
   - 重点: 告警规则、路由、抑制

---

## 实践任务 (2.5h)

### 任务 1: ACK 监控组件检查 (45min)

```bash
# 检查 ARMS Prometheus 组件
kubectl get pods -n arms-prom
kubectl get svc -n arms-prom

# 检查 metrics-server
kubectl get pods -n kube-system -l k8s-app=metrics-server
kubectl top nodes
kubectl top pods -A --sort-by=cpu | head -20

# 查看 Prometheus 采集的指标
kubectl get servicemonitors -A
kubectl get podmonitors -A
```

### 任务 2: Grafana Dashboard 查看 (45min)

```bash
# 访问 Grafana (ACK 集成 ARMS)
# 控制台 -> ACK -> 运维管理 -> Prometheus 监控

# 核心 Dashboard:
# 1. 集群概览: 节点数、Pod 数、CPU/内存使用率
# 2. 节点监控: 各节点 CPU、内存、磁盘、网络
# 3. Pod 监控: Pod CPU/内存使用、重启次数
# 4. API Server: 请求延迟、QPS、错误率

# 关键指标:
# - node_cpu_seconds_total: 节点 CPU 使用
# - node_memory_MemAvailable_bytes: 节点可用内存
# - kube_pod_container_status_restarts_total: Pod 重启次数
# - apiserver_request_duration_seconds: API 请求延迟
```

### 任务 3: 自定义告警规则 (45min)

```bash
# 创建 PrometheusRule
cat > ack-alerts.yaml << 'EOF'
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ack-custom-alerts
  namespace: arms-prom
spec:
  groups:
  - name: ack-alerts
    rules:
    - alert: NodeHighCPU
      expr: (1 - avg(rate(node_cpu_seconds_total{mode="idle"}[5m])) by (instance)) > 0.85
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "节点 {{ $labels.instance }} CPU 使用率超过 85%"
    - alert: PodCrashLooping
      expr: rate(kube_pod_container_status_restarts_total[15m]) > 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 持续重启"
    - alert: NodeDiskPressure
      expr: node_filesystem_avail_bytes{mountpoint="/"} / node_filesystem_size_bytes{mountpoint="/"} < 0.1
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "节点 {{ $labels.instance }} 磁盘空间不足 10%"
EOF
kubectl apply -f ack-alerts.yaml
```

### 任务 4: 常用 PromQL 查询 (30min)

```bash
# 在 Grafana 或 Prometheus 中执行以下查询:

# 集群 CPU 使用率
# avg(1 - rate(node_cpu_seconds_total{mode="idle"}[5m])) * 100

# 集群内存使用率
# (1 - sum(node_memory_MemAvailable_bytes) / sum(node_memory_MemTotal_bytes)) * 100

# Pod 重启次数 (Top 10)
# topk(10, sum(kube_pod_container_status_restarts_total) by (namespace, pod))

# API Server 请求延迟 P99
# histogram_quantile(0.99, rate(apiserver_request_duration_seconds_bucket[5m]))

# 节点磁盘使用率
# (1 - node_filesystem_avail_bytes{mountpoint="/"} / node_filesystem_size_bytes{mountpoint="/"}) * 100
```

---

## 费曼复述 (0.5h)

1. **ACK 集群监控的架构是什么？各组件的作用是什么？**
2. **哪些监控指标是集群运维最关键的？为什么？**
3. **如何配置一个"节点 CPU 过高"的告警规则？**

---

## 今日检验

- [ ] 能查看 ACK 集群的监控数据
- [ ] 理解核心监控指标的含义
- [ ] 能创建自定义告警规则
- [ ] 能编写基础 PromQL 查询

---

## 核心概念总结

| 监控维度 | 关键指标 | 告警阈值 (参考) |
|----------|---------|----------------|
| 节点 CPU | node_cpu_seconds_total | > 85% 持续 5min |
| 节点内存 | node_memory_MemAvailable_bytes | > 90% 持续 5min |
| 节点磁盘 | node_filesystem_avail_bytes | < 10% 可用 |
| Pod 重启 | kube_pod_container_status_restarts_total | > 0 次/15min |
| API 延迟 | apiserver_request_duration_seconds | P99 > 1s |

---

## 明日预告

Day 14 将学习集群资源配额与 License 管理。
