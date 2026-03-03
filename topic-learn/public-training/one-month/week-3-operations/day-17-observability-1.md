# Day 17: 可观测性 - 监控 + Prometheus

> **学习时间**: 4-5 小时 | **主题**: Prometheus 监控体系

---

## 今日目标

- [ ] 理解可观测性三大支柱
- [ ] 掌握 Prometheus 数据模型和 PromQL
- [ ] 部署 kube-prometheus-stack

---

## 理论学习 (2h)

### 必读文档

1. **可观测性架构总览**
   - 文件: `../../domain-8-observability/01-observability-architecture-overview.md`
   - 重点: Metrics/Logs/Traces 三大支柱

2. **监控指标系统**
   - 文件: `../../domain-8-observability/02-monitoring-metrics-system.md`
   - 重点: Prometheus 数据模型、PromQL 基础

3. **Prometheus 生产级配置**
   - 文件: `../../domain-8-observability/10-monitoring-metrics-prometheus.md`
   - 重点: 生产级配置建议

---

## 实践任务 (2.5h)

### 任务 1: 部署 kube-prometheus-stack (45min)

```bash
# 添加 Helm repo
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo update

# 安装 kube-prometheus-stack
helm install prometheus prometheus-community/kube-prometheus-stack \
  --namespace monitoring \
  --create-namespace

# 等待就绪
kubectl wait --namespace monitoring --for=condition=ready pod -l app.kubernetes.io/name=prometheus --timeout=300s

# 查看组件
kubectl get pods -n monitoring
```

### 任务 2: PromQL 查询实践 (45min)

```bash
# 访问 Prometheus UI
kubectl port-forward -n monitoring svc/prometheus-kube-prometheus-prometheus 9090:9090

# 在浏览器访问 http://localhost:9090

# 基础查询示例:
# 1. 节点 CPU 使用率
# sum(rate(node_cpu_seconds_total{mode!="idle"}[5m])) by (instance) / count(node_cpu_seconds_total{mode="idle"}) by (instance) * 100

# 2. Pod 内存使用
# container_memory_usage_bytes{container!="POD", container!=""}

# 3. Pod CPU 使用率
# rate(container_cpu_usage_seconds_total{container!="POD"}[5m])

# 4. 请求错误率
# sum(rate(http_requests_total{status=~"5.."}[5m])) / sum(rate(http_requests_total[5m])) * 100
```

### 任务 3: 告警规则配置 (45min)

```bash
# 创建 PrometheusRule
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
      expr: rate(kube_pod_container_status_restarts_total[15m]) > 0
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} is crash looping"
    - alert: PodNotReady
      expr: kube_pod_status_ready{condition="false"} == 1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} is not ready"
  - name: node-alerts
    rules:
    - alert: HighMemoryUsage
      expr: (1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) > 0.9
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Node {{ $labels.instance }} memory usage > 90%"
EOF

kubectl apply -f alert-rules.yaml
```

### 任务 4: Grafana Dashboard (30min)

```bash
# 访问 Grafana
kubectl port-forward -n monitoring svc/prometheus-grafana 3000:80

# 默认登录: admin / prom-operator

# 导入 K8s Dashboard:
# - Dashboard ID: 315 (Kubernetes cluster monitoring)
# - Dashboard ID: 6417 (Kubernetes pods monitoring)
```

---

## 费曼复述 (0.5h)

1. **Prometheus 的数据模型是什么？时序数据如何组织？**
2. **PromQL 中 rate() 和 irate() 的区别？**
3. **Alertmanager 如何实现告警分组和路由？**

---

## 今日检验

- [ ] 能够部署 Prometheus 监控栈
- [ ] 能够编写基础 PromQL 查询
- [ ] 能够配置告警规则
- [ ] 能够使用 Grafana 可视化
