# 节点状态与健康检查

## 源码路径

`pkg/kubelet/lifecycle/`
`pkg/kubelet/nodestatus/`
`pkg/kubelet/volumemanager/`

---

## 节点状态 (Conditions)

```bash
# 查看节点条件
kubectl get nodes -o jsonpath='{range .items[*]} {.metadata.name}: {.status.conditions[*].type}={.status.conditions[*].status}{"\n"}{end}'

# 完整条件详情
kubectl describe node <node-name> | grep -A 5 "Conditions"
```

---

## Ready 状态

```bash
# Ready = kubelet 运行中 + 节点资源充足 + 网络正常
# Status: True/False/Unknown

# kubelet 报告节点为 NotReady 时:
# - 节点不可调度新 Pod
# - 已有 Pod 继续运行
# - 等待 kubelet 恢复
```

---

## MemoryPressure

```bash
# 节点内存不足
kubectl get nodes -o jsonpath='{range .items[*]} {.metadata.name}: MemoryPressure={.status.conditions[?(@.type=="MemoryPressure")].status}{"\n"}{end}'

# kubelet 根据节点可用内存设置此条件
# 当 nodefs.available < 阈值时，kubelet 开始驱逐 Pod
```

---

## DiskPressure

```bash
# 节点磁盘不足
# 检查:
# - /var/lib/kubelet (容器镜像、日志)
# - /var/lib/containerd (containerd 存储)
# - /var/log (系统日志)

# 触发条件:
# imagefs.available < 15% (默认)
# nodefs.available < 10% (默认)
```

---

## PIDPressure

```bash
# 进程 ID 不足 (PIDs)
# 默认限制: 32768

# 查看 PIDs 使用
cat /proc/sys/kernel/pid_max

# kubelet PIDPressure 触发阈值:
# PIDs >= pid_max - 1000 时
```

---

## NetworkUnavailable

```bash
# 节点网络不可用
# CNI 未正确配置时为 True

# kubelet 根据 CNI 配置状态设置此条件
# kube-router/Calico/Cilium 负责正确配置网络
```

---

## kubelet 健康检查

```bash
# kubelet API 健康检查
curl -k https://localhost:10250/healthz

# kubelet metrics
curl -k https://localhost:10250/metrics

# kubelet PLEG (Pod Lifecycle Event Generator)
curl -k https://localhost:10250/stats/summary
```

---

## 节点资源状态

```bash
# 查看节点 allocatable
kubectl get node <node> -o jsonpath='{
  "CPU: "}{.status.allocatable.cpu}{"\n"}
  "Memory: "}{.status.allocatable.memory}{"\n"}
  "Ephemeral: "}{.status.allocatable.ephemeral-storage}{"\n"}
  "Pods: "}{.status.allocatable.pods}
'

# 对比 capacity vs allocatable
kubectl get node <node> -o jsonpath='{
  "Capacity CPU: "}{.status.capacity.cpu}{"\n"}
  "Allocatable CPU: "}{.status.allocatable.cpu}
'
```

---

## 节点就绪调度流程

```
API Server 决定是否调度 Pod 到节点:
    ↓
检查节点 Conditions:
    ↓
Ready == True?
    ↓
No: 不调度
    ↓
Yes:
    ↓
MemoryPressure == True?
    ↓
Yes: 仅调度 BestEffort Pod (如果有的话)
    ↓
No:
    ↓
DiskPressure == True?
    ↓
Yes: 仅调度 BestEffort Pod
    ↓
No:
    ↓
PIDPressure == True?
    ↓
Yes: 仅调度 BestEffort Pod
    ↓
No:
    ↓
NetworkUnavailable == True?
    ↓
Yes: 不调度
    ↓
No:
    ↓
节点污点与 Pod 容忍匹配?
    ↓
No: 不调度
    ↓
Yes:
    ↓
调度 Pod 到该节点
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| MemoryPressure True | 节点内存不足 | 添加内存/驱逐 Pod |
| DiskPressure True | 磁盘空间不足 | 清理镜像/日志 |
| NetworkUnavailable True | CNI 配置错误 | 检查 CNI 插件 |
| Ready Unknown | kubelet 无响应 | 重启 kubelet |
