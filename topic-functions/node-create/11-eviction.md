# 节点资源压力与 Eviction

## 源码路径

`pkg/kubelet/eviction/`
`pkg/kubelet/lifecycle/admission/`

---

## kubelet Eviction 机制

```yaml
# /var/lib/kubelet/config.yaml
evictionHard:
  memory.available: 100Mi      # 内存 < 100Mi 时驱逐
  nodefs.available: 10%        # 磁盘 < 10% 时驱逐
  imagefs.available: 15%       # 镜像盘 < 15% 时驱逐
  nodefs.inodesFree: 5%       # inode < 5% 时驱逐

evictionSoft:
  memory.available: 200Mi      # 内存 < 200Mi 时开始软驱逐
  nodefs.available: 15%

evictionSoftGracePeriod:
  memory.available: 1m30s      # 软驱逐宽限期

evictionMinimumReclaim:
  memory.available: 50Mi      # 驱逐后保留的最小内存

evictionPressureTransitionPeriod: 5m  # 状态转换延迟
```

---

## 驱逐优先级 (QoS)

```
Guaranteed (最高优先级，不易被驱逐)
  ↓
Burstable
  ↓
BestEffort (最低优先级，最先被驱逐)
```

---

## QoS 判定

```yaml
# Guaranteed: 所有容器都设置了 limits，且 requests == limits
spec:
  containers:
  - name: app
    resources:
      limits:
        cpu: "1"
        memory: "1Gi"
      requests:
        cpu: "1"
        memory: "1Gi"

# Burstable: 至少一个容器设置了 requests
spec:
  containers:
  - name: app
    resources:
      requests:
        cpu: "100m"
        memory: "128Mi"

# BestEffort: 没有任何容器设置 limits 或 requests
spec: {}
```

---

## OOM Kill

```bash
# 容器内存超限被 OOM Kill
dmesg | grep -i "oom"

# 查看 Pod OOM
kubectl get events | grep OOM

# Pod 重启原因
kubectl describe pod <pod> | grep -A 3 "Last State"

# OOM 原因:
# - 容器内存超过 limit
# - 节点内存不足，kubelet 驱逐后仍不够
```

---

## 本地临时存储

```yaml
# emptyDir 的临时存储受 eviction 影响
# ephemeral-storage:
#   - emptyDir volumes
#   - 日志文件
#   - 缓存

# 当 nodefs.available < 10% 时开始驱逐使用临时存储的 Pod
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| Pod 被 OOM Kill | 内存超 limit | 增加 limit/减少工作负载 |
| Pod 被 Eviction | 节点资源不足 | 扩容/调整 eviction 阈值 |
| 磁盘满 | 镜像/日志过多 | 清理/增加磁盘 |
