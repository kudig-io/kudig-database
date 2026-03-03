# Day 21: 平台运维 + 综合实践

> **学习时间**: 4-5 小时 | **主题**: Week 3 总结与实践项目

---

## 今日目标

- [ ] 了解集群生命周期管理
- [ ] 掌握备份恢复策略
- [ ] 完成综合实践项目 P3

---

## 理论学习 (2h)

### 必读文档

1. **集群生命周期管理**
   - 文件: `../../domain-9-platform-ops/02-cluster-lifecycle-management.md`
   - 重点: 集群升级、维护窗口

2. **备份恢复策略**
   - 文件: `../../domain-9-platform-ops/12-backup-recovery-strategy.md`
   - 重点: etcd 备份、Velero

3. **监控 Playbooks**
   - 文件: `../../domain-8-observability/21-monitoring-playbooks.md`
   - 重点: 监控配置模板

---

## 综合实践项目 P3 (2.5h)

**项目: 可观测性体系搭建 + 故障演练**

详细指南见: [../projects/p3-observability-fault-drill.md](../projects/p3-observability-fault-drill.md)

### Step 1: 确认监控栈 (15min)

```bash
# 确认 Prometheus 和 Loki 已部署
kubectl get pods -n monitoring

# 确认 Grafana 可访问
kubectl port-forward -n monitoring svc/prometheus-grafana 3000:80
```

### Step 2: 配置核心告警规则 (30min)

```bash
cat > core-alerts.yaml << 'EOF'
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: core-alerts
  namespace: monitoring
spec:
  groups:
  - name: k8s-core
    rules:
    - alert: PodCrashLooping
      expr: rate(kube_pod_container_status_restarts_total[15m]) > 0
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} crash looping"
    
    - alert: NodeNotReady
      expr: kube_node_status_condition{condition="Ready",status="true"} == 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Node {{ $labels.node }} is not ready"
    
    - alert: HighCPUUsage
      expr: (1 - avg(rate(node_cpu_seconds_total{mode="idle"}[5m])) by (instance)) > 0.9
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Node {{ $labels.instance }} CPU > 90%"
    
    - alert: HighMemoryUsage
      expr: (1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) > 0.9
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Node {{ $labels.instance }} memory > 90%"
EOF

kubectl apply -f core-alerts.yaml
```

### Step 3: 配置 Grafana Dashboard (30min)

导入以下 Dashboard:
- 315: Kubernetes cluster monitoring
- 6417: Kubernetes pods monitoring
- 13639: Loki & Promtail

### Step 4: 故障注入与排查演练 (1h)

```bash
# 创建测试应用
kubectl create namespace fault-drill
kubectl create deployment app --image=nginx:alpine -n fault-drill --replicas=3
kubectl expose deployment app --port=80 -n fault-drill

# 故障 1: 模拟 OOM
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

# 在 Grafana 中观察:
# - Pod 重启次数增加
# - 告警触发

# 按 FTA 方法排查并记录

# 故障 2: 模拟 Service 不可用
kubectl delete endpoints app -n fault-drill

# 排查并修复

# 清理
kubectl delete namespace fault-drill
```

### Step 5: 产出故障排查手册 (30min)

创建 `~/troubleshooting-handbook.md`:

```markdown
# K8s 故障排查手册

## 1. Pod 问题

### 1.1 Pending
- 症状: Pod 长时间 Pending
- 排查: `kubectl describe pod <name>`
- 常见原因: 资源不足、nodeSelector、taints
- 修复: 调整资源或调度约束

### 1.2 CrashLoopBackOff
- 症状: Pod 反复重启
- 排查: `kubectl logs <name> --previous`
- 常见原因: 应用错误、配置问题
- 修复: 检查日志修复应用

## 2. Service 问题

### 2.1 无法访问
- 排查: `kubectl get endpoints <name>`
- 常见原因: selector 不匹配
- 修复: 检查 labels

## 3. 监控告警响应

### 3.1 PodCrashLooping
- 告警: Pod 重启频繁
- 响应: 检查日志和资源使用

### 3.2 NodeNotReady
- 告警: 节点不健康
- 响应: 检查 kubelet 和系统资源
```

---

## 自测检验

完成 [checkpoint.md](./checkpoint.md) 中的 Week 3 自测题。

---

## Week 3 总结

### 学习路径

```
Day 15-16: 安全体系 (RBAC, Pod Security, Secret)
Day 17-18: 可观测性 (Prometheus, Loki, Alertmanager)
Day 19-20: 故障排查 (FTA, FEBM, 实战演练)
Day 21:    平台运维 + 综合实践
```

### 关键收获

1. **安全**: RBAC 最小权限、Pod 安全标准
2. **监控**: Prometheus + Grafana + Alertmanager
3. **日志**: Loki + Promtail
4. **排障**: FTA 故障树 + FEBM 取证方法

---

恭喜完成 Week 3 的学习!
