# 节点监控

## 源码路径

`pkg/kubelet/metrics/`
`pkg/kubelet/server/stats/`

---

## kubelet Metrics

```bash
# kubelet metrics 端点
curl -k https://localhost:10250/metrics

# 常用 metrics:
# kubelet_runtime_operations_total
# kubelet_runtime_operations_errors_total
# kubelet_pod_worker_duration_seconds
# kubelet_volume_stats_capacity_bytes
# kubelet_volume_stats_used_bytes
# kubelet_node_controller_evictions_total
```

---

## kubectl top

```bash
# 查看节点资源使用
kubectl top nodes

# 查看 Pod 资源使用
kubectl top pods

# 查看特定 Pod
kubectl top pod <pod-name> -n <namespace>
```

---

## node-exporter

```bash
# node-exporter 部署 (Prometheus 组件)
kubectl apply -f https://raw.githubusercontent.com/prometheus/node_exporter/master/examples/kube-system/node-exporter.yaml

# 暴露端口
kubectl port-forward -n kube-system node-exporter:9100
```

---

## 关键指标

```bash
# CPU 使用
kubectl top node

# 内存使用
kubectl top node

# Pod 数量
kubectl get pods -o wide --all-namespaces | grep <node>

# 磁盘 I/O
# 通过 node-exporter 获取
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| kubectl top 不工作 | metrics-server 未安装 | 安装 metrics-server |
| node-exporter 无法采集 | RBAC 问题 | 配置 ClusterRole |
