# Day 18: 可观测性 - 日志 + 分布式追踪

> **学习时间**: 4-5 小时 | **主题**: 日志聚合与链路追踪

---

## 今日目标

- [ ] 理解 K8s 日志架构
- [ ] 部署 Loki + Promtail 日志系统
- [ ] 配置 Alertmanager 告警路由

---

## 理论学习 (2h)

### 必读文档

1. **日志架构**
   - 文件: `../../domain-8-observability/03-logging-architecture.md`
   - 重点: K8s 日志架构，Sidecar vs DaemonSet

2. **分布式追踪**
   - 文件: `../../domain-8-observability/04-distributed-tracing.md`
   - 重点: OpenTelemetry、Jaeger

3. **告警管理**
   - 文件: `../../domain-8-observability/05-alerting-management.md`
   - 重点: Alertmanager 路由配置

---

## 实践任务 (2.5h)

### 任务 1: 部署 Loki (45min)

```bash
# 添加 Grafana Helm repo
helm repo add grafana https://grafana.github.io/helm-charts
helm repo update

# 安装 Loki Stack
helm install loki grafana/loki-stack \
  --namespace monitoring \
  --set promtail.enabled=true \
  --set grafana.enabled=false  # 使用已有的 Grafana

# 验证部署
kubectl get pods -n monitoring -l app=loki
kubectl get pods -n monitoring -l app=promtail
```

### 任务 2: 配置 Grafana Loki 数据源 (30min)

```bash
# 访问 Grafana
kubectl port-forward -n monitoring svc/prometheus-grafana 3000:80

# 在 Grafana 中:
# 1. Configuration -> Data Sources -> Add data source
# 2. 选择 Loki
# 3. URL: http://loki:3100
# 4. Save & Test

# LogQL 查询示例:
# {namespace="default"}
# {namespace="kube-system", container="kube-apiserver"}
# {app="nginx"} |= "error"
# rate({namespace="default"}[5m])
```

### 任务 3: Alertmanager 路由配置 (45min)

```bash
# 查看 Alertmanager 配置
kubectl get secret -n monitoring alertmanager-prometheus-kube-prometheus-alertmanager -o jsonpath='{.data.alertmanager\.yaml}' | base64 -d

# 自定义 Alertmanager 配置
cat > alertmanager-config.yaml << 'EOF'
apiVersion: v1
kind: Secret
metadata:
  name: alertmanager-prometheus-kube-prometheus-alertmanager
  namespace: monitoring
type: Opaque
stringData:
  alertmanager.yaml: |
    global:
      resolve_timeout: 5m
    route:
      group_by: ['alertname', 'severity']
      group_wait: 30s
      group_interval: 5m
      repeat_interval: 4h
      receiver: 'default'
      routes:
      - match:
          severity: critical
        receiver: 'critical'
      - match:
          severity: warning
        receiver: 'warning'
    receivers:
    - name: 'default'
      webhook_configs:
      - url: 'http://localhost:5001/'
    - name: 'critical'
      webhook_configs:
      - url: 'http://localhost:5001/critical'
    - name: 'warning'
      webhook_configs:
      - url: 'http://localhost:5001/warning'
EOF

# 注意: 生产环境替换为实际的 webhook URL (钉钉/企微/Slack)
```

### 任务 4: 日志查询实践 (30min)

```bash
# 生成测试日志
kubectl run log-test --image=busybox --restart=Never -- sh -c 'for i in $(seq 1 100); do echo "Log message $i at $(date)"; sleep 1; done'

# 在 Grafana Explore 中查询:
# {pod="log-test"}

# 过滤日志:
# {pod="log-test"} |= "message 50"

# 统计日志:
# count_over_time({namespace="default"}[5m])
```

---

## 费曼复述 (0.5h)

1. **K8s 日志的 Sidecar 和 DaemonSet 采集方式各有什么优缺点？**
2. **Loki 和 ELK 相比有什么优势？**
3. **Alertmanager 的路由规则如何工作？**

---

## 今日检验

- [ ] 能够部署 Loki 日志系统
- [ ] 能够使用 LogQL 查询日志
- [ ] 能够配置 Alertmanager 告警路由
