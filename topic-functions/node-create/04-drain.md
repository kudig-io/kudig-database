# 节点维护：drain/cordon/uncordon

## 源码路径

`pkg/kubectl/cmd/drain/`
`pkg/kubectl/cmd/cordon/`

---

## drain 命令

```bash
# 安全驱逐节点上所有 Pod
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# 参数说明:
# --ignore-daemonsets: 忽略 DaemonSet Pod (不阻塞)
# --delete-emptydir-data: 删除 emptyDir 数据
# --force: 强制删除无法优雅终止的 Pod
# --grace-period: 优雅终止时间 (默认 30s)
# --timeout: 最大等待时间
```

---

## cordon (标记不可调度)

```bash
# 标记节点为不可调度
kubectl cordon <node-name>

# 效果:
# - 节点 unschedulable = true
# - 新 Pod 不会被调度到该节点
# - 已有 Pod 继续运行
```

---

## uncordon (恢复调度)

```bash
# 恢复节点调度
kubectl uncordon <node-name>

# 效果:
# - 节点 unschedulable = false
# - 新 Pod 可调度到该节点
```

---

## drain 完整流程

```
kubectl drain <node>
    ↓
1. 检查节点是否存在
    ↓
2. 标记节点为 unschedulable (cordon)
    ↓
3. 获取节点上所有 Pod (排除 DaemonSet/static Pod)
    ↓
4. 对每个 Pod:
    ↓
    a. 如果有 PDB，检查是否可以驱逐
    ↓
    b. 发送 SIGTERM (优雅终止)
    ↓
    c. 等待优雅终止完成 (--grace-period)
    ↓
    d. 超时后发送 SIGKILL
    ↓
5. 删除节点对象 (可选 --delete-emptydir-data)
```

---

## PDB (PodDisruptionBudget) 影响

```bash
# drain 受 PDB 限制
# 如果 PDB 阻止驱逐，drain 会等待或失败

# 查看 PDB
kubectl get pdb

# 示例: 保证 frontend 至少有 2 个 Pod 运行
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: frontend-pdb
spec:
  minAvailable: 2
  selector:
    matchLabels:
      app: frontend
```

---

## DaemonSet Pod 处理

```bash
# DaemonSet Pod 不受 drain 影响
# drain 默认跳过 DaemonSet Pod

# 但 --ignore-daemonsets 会:
# 1. 删除 DaemonSet Pod (空轮的)
# 2. DaemonSet controller 会在其他节点重建

# 如果想在 drain 时保留 DaemonSet:
# 使用 --ignore-daemonsets=false (但通常不要这样)
```

---

## 节点维护场景

### 场景 1: 内核升级

```bash
# 1. 标记应用不可用 (可选)
# 2. drain 节点
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data

# 3. 升级内核
sudo apt-get update && sudo apt-get upgrade -y
sudo reboot

# 4. 节点恢复后 uncordon
kubectl uncordon <node>
```

### 场景 2: 节点故障排查

```bash
# 1. 驱逐 Pod
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data --force

# 2. 排查问题

# 3. 恢复
kubectl uncordon <node>
```

---

## drain 失败处理

```bash
# drain 超时
# --timeout=5m0s 默认

# 查看失败原因
kubectl describe node <node-name> | grep -A 10 "Events"

# 手动处理无法驱逐的 Pod
kubectl delete pod <pod-name> -n <namespace> --grace-period=0 --force
```

---

## drain 与 Pod 类型

| Pod 类型 | drain 行为 |
|---------|-----------|
| Deployment/ReplicaSet | 会被驱逐，RS 会重建 |
| DaemonSet | 会被驱逐，DS 会重建 (使用 --ignore-daemonsets) |
| StatefulSet | 需要先删除 PVC 或使用 --delete-emptydir-data |
| Static Pod | 会被删除，kubelet 会重新创建 |
| 独立 Pod | 会被删除，不会重建 |

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| drain 卡住 | PDB 阻止驱逐 | 检查 PDB 状态 |
| StatefulSet Pod 无法驱逐 | PVC 保护 | 先删除 PVC 或加 --force |
| Pod 一直 Terminating | 优雅终止失败 | `kubectl delete --force` |
| 节点无法 uncordon | kubelet 未恢复 | 检查 kubelet 状态 |
