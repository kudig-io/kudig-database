# 项目 P3: 可观测性体系搭建 + 故障演练

> **所属周**: Week 3 | **预计时间**: 2.5 小时

---

## 项目目标

搭建完整的可观测性体系，并进行故障注入演练:
- 监控: Prometheus + Grafana
- 日志: Loki + Promtail
- 告警: Alertmanager
- 故障演练: 注入故障并按 FTA 方法排查

---

## 前置条件

- 已完成 Week 3 Day 15-20 的学习
- 已部署 kube-prometheus-stack
- 已部署 Loki

---

## 项目步骤

### Step 1: 确认监控组件 (15min)

```bash
# 检查 Prometheus
kubectl get pods -n monitoring -l app.kubernetes.io/name=prometheus

# 检查 Grafana
kubectl get pods -n monitoring -l app.kubernetes.io/name=grafana

# 检查 Loki
kubectl get pods -n monitoring -l app=loki

# 访问 Grafana
kubectl port-forward -n monitoring svc/prometheus-grafana 3000:80
```

### Step 2: 配置告警规则 (30min)

```bash
cat > core-alerts.yaml << 'EOF'
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: core-alerts
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
    - alert: HighCPUUsage
      expr: (1 - avg(rate(node_cpu_seconds_total{mode="idle"}[5m])) by (instance)) > 0.9
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Node {{ $labels.instance }} CPU usage > 90%"
    
    - alert: HighMemoryUsage
      expr: (1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) > 0.9
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Node {{ $labels.instance }} memory usage > 90%"
EOF

kubectl apply -f core-alerts.yaml

# 验证规则已加载
kubectl get prometheusrule -n monitoring
```

### Step 3: 配置 Grafana Dashboard (30min)

在 Grafana 中导入以下 Dashboard:

1. **K8s 集群监控** (ID: 315)
   - 节点资源使用
   - Pod 数量统计
   - 网络流量

2. **K8s Pod 监控** (ID: 6417)
   - Pod CPU/内存使用
   - 容器重启次数
   - 网络 IO

3. **Loki 日志** (ID: 13639)
   - 日志查询
   - 日志统计

### Step 4: 创建故障演练环境 (15min)

```bash
# 创建测试 namespace
kubectl create namespace fault-drill

# 部署测试应用
kubectl create deployment app --image=nginx:alpine -n fault-drill --replicas=3
kubectl expose deployment app --port=80 -n fault-drill
```

### Step 5: 故障注入与排查 (45min)

#### 故障 1: OOMKilled

```bash
# 注入故障
cat > oom-inject.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: oom-inject
  namespace: fault-drill
spec:
  containers:
  - name: stress
    image: polinux/stress
    command: ["stress"]
    args: ["--vm", "1", "--vm-bytes", "200M"]
    resources:
      limits:
        memory: 100Mi
EOF

kubectl apply -f oom-inject.yaml

# 排查步骤:
# 1. 查看 Pod 状态
kubectl get pod oom-inject -n fault-drill

# 2. 查看详情
kubectl describe pod oom-inject -n fault-drill

# 3. 在 Grafana 中查看:
#    - Container memory usage
#    - OOMKilled 事件

# 4. 在 Loki 中查询:
#    {namespace="fault-drill", pod="oom-inject"}
```

#### 故障 2: CrashLoopBackOff

```bash
# 注入故障
kubectl run crash-app --image=busybox -n fault-drill -- /bin/sh -c "exit 1"

# 排查步骤:
kubectl get pod crash-app -n fault-drill
kubectl logs crash-app -n fault-drill --previous
kubectl describe pod crash-app -n fault-drill
```

#### 故障 3: Service 不可访问

```bash
# 注入故障: 删除 Endpoints
kubectl delete endpoints app -n fault-drill

# 排查步骤:
kubectl get endpoints app -n fault-drill
kubectl describe svc app -n fault-drill
kubectl get pods -n fault-drill -l app=app --show-labels
```

### Step 6: 编写故障排查报告 (15min)

使用 FEBM 方法记录排查过程:

```markdown
## 故障报告: OOMKilled

### 1. 现象
- Pod 状态: CrashLoopBackOff
- 重启原因: OOMKilled

### 2. 证据收集
- kubectl describe pod: 显示 OOMKilled
- Grafana: 内存使用达到 limit
- Loki: 无应用错误日志

### 3. 根因分析
- 应用内存需求 200M
- limits 设置 100M
- 触发 OOM Killer

### 4. 修复方案
- 增加 memory limits 到 256Mi

### 5. 预防措施
- 添加内存使用告警
- 压测确定合理 limits
```

---

## 验收清单

- [ ] 告警规则配置成功
- [ ] Grafana Dashboard 可以展示数据
- [ ] Loki 可以查询日志
- [ ] 成功注入 3 类故障
- [ ] 按 FTA/FEBM 方法完成排查
- [ ] 完成故障排查报告

---

## 清理资源

```bash
kubectl delete namespace fault-drill
```
