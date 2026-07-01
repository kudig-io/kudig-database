---
title: Node-pressure Eviction
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- pdb
- daemonset
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Node-pressure Eviction 是什么
- 如何 Node-pressure Eviction
trigger_keywords:
- Node-pressure
- Eviction
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---



# Node-pressure Eviction

## 概述

节点压力驱逐（Node-pressure Eviction）是 [[kubelet|kubelet]] 主动终止 Pod 以回收节点资源的过程。kubelet 监控节点的内存、磁盘空间、文件系统 inode 和 PID 等资源，当某些资源达到特定消耗水平时，kubelet 会主动使一个或多个 Pod 失败来回收资源，防止饥饿。

## 核心概念/原理

### 驱逐信号与阈值

kubelet 使用驱逐信号（eviction signals）来做出驱逐决策，将信号与驱逐阈值（eviction thresholds）进行比较。

**驱逐信号**：

| 信号 | 描述 |
|------|------|
| `memory.available` | 节点容量减去工作集内存 |
| `nodefs.available` | 节点主文件系统可用空间 |
| `nodefs.inodesFree` | 节点主文件系统可用 inode |
| `imagefs.available` | 镜像文件系统可用空间 |
| `imagefs.inodesFree` | 镜像文件系统可用 inode |
| `containerfs.available` | 容器文件系统可用空间 |
| `containerfs.inodesFree` | 容器文件系统可用 inode |
| `pid.available` | 可用进程标识符数量 |

**阈值类型**：

- **Soft eviction thresholds**：与管理员指定的宽限期配对。宽限期结束后才驱逐 Pod。使用 `eviction-soft`、`eviction-soft-grace-period` 和 `eviction-max-pod-grace-period` 配置。
- **Hard eviction thresholds**：没有宽限期，达到阈值后立即终止 Pod。默认硬阈值包括：
  - `memory.available<100Mi`（Linux）/ `<500Mi`（Windows）
  - `nodefs.available<10%`
  - `imagefs.available<15%`
  - `nodefs.inodesFree<5%`（Linux）
  - `imagefs.inodesFree<5%`（Linux）

### 节点条件

kubelet 将驱逐信号映射为节点条件：

| 节点条件 | 对应信号 |
|----------|----------|
| `MemoryPressure` | `memory.available` |
| `DiskPressure` | 各种文件系统信号 |
| `PIDPressure` | `pid.available` |

### 资源回收顺序

kubelet 在驱逐用户 Pod 之前会先尝试回收节点级资源：

- 垃圾回收死掉的 Pod 和容器。
- 删除未使用的镜像。

### Pod 选择顺序

如果节点级资源回收不足以将信号降到阈值以下，kubelet 开始驱逐用户 Pod，排序依据：

1. Pod 的资源使用是否超过请求量
2. Pod 优先级
3. 资源使用相对于请求量的多少

因此驱逐顺序大致为：
1. `BestEffort` 或资源使用超过请求的 `Burstable` Pod（按优先级和超量程度排序）
2. 资源使用未超过请求的 `Guaranteed` 和 `Burstable` Pod（按优先级排序）

## 关键机制或特性

- **最小回收量（eviction-minimum-reclaim）**：可以配置每种资源的最小回收量，防止 kubelet 反复触发多次驱逐。
- **节点条件振荡保护**：`eviction-pressure-transition-period`（默认 5 分钟）控制 kubelet 在切换节点条件状态前必须等待的时间，防止条件快速振荡导致错误的驱逐决策。
- **OOM 行为**：如果 kubelet 无法在内核 OOM killer 之前回收内存，系统会依赖 OOM killer。kubelet 根据 Pod 的 QoS 为每个容器设置 `oom_score_adj` 值。
- **MergeDefaultEvictionSettings**：kubelet 配置中的此字段设为 true 时，修改某个阈值参数后其他参数会继承默认值而不是 0。

## 使用场景

- 防止节点因内存、磁盘或 inode 耗尽而导致系统不稳定。
- 在资源紧张时自动释放节点资源，保持节点健康。
- 配合 Pod 优先级和 QoS 类，实现有策略的资源回收。

## 最佳实践/注意事项

- 节点压力驱逐与 API-initiated 驱逐不同，kubelet **不尊重** PodDisruptionBudget 和 `terminationGracePeriodSeconds`。
- 软驱逐阈值可配置最大 Pod 优雅终止期；硬驱逐阈值使用 0 秒宽限期（立即关闭）。
- 配置驱逐策略时，应确保调度器不会调度会立即触发驱逐的 Pod。
- 如果不想 [[DaemonSet|DaemonSet]] 的 Pod 被驱逐，应为它们设置足够高的优先级。
- 对于 Linux 节点，`memory.available` 的计算排除了 `inactive_file`，因为 kubelet 假设这部分内存可以在压力下回收。
- 大量使用本地存储的工作负载可能会因内核缓存被计为 `active_file` 而触发内存压力驱逐，可以通过将内存限制和请求设为相同值来缓解。

## 生产 YAML 示例

### kubelet 驱逐阈值配置

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
evictionHard:
  memory.available: "200Mi"                # 内存低于 200Mi 时硬驱逐
  nodefs.available: "10%"                  # 根分区低于 10% 时硬驱逐
  imagefs.available: "15%"                 # 镜像分区低于 15% 时硬驱逐
  nodefs.inodesFree: "5%"
  pid.available: "5%"
evictionSoft:
  memory.available: "500Mi"                # 内存低于 500Mi 时触发软驱逐
  nodefs.available: "15%"
evictionSoftGracePeriod:
  memory.available: "1m30s"                # 软驱逐等待 90 秒
  nodefs.available: "2m"
evictionMaxPodGracePeriod: 60              # 软驱逐最大优雅终止期 60 秒
evictionMinimumReclaim:
  memory.available: "256Mi"                # 驱逐后至少回收 256Mi
  nodefs.available: "1Gi"
evictionPressureTransitionPeriod: 5m0s     # 节点条件状态切换等待时间
```

### 确保关键 Pod 不被驱逐（Guaranteed QoS）

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: critical-service
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: critical-service
  template:
    metadata:
      labels:
        app: critical-service
    spec:
      priorityClassName: system-critical   # 高优先级 — 最后被驱逐
      containers:
        - name: app
          image: registry.example.com/critical:v2.0
          resources:
            requests:
              cpu: "500m"                  # requests == limits → Guaranteed QoS
              memory: 1Gi
            limits:
              cpu: "500m"
              memory: 1Gi                  # 防止内存突增触发 OOM
```

## 驱逐顺序快速参考

```
节点资源压力触发
    ↓
1. 垃圾回收：清理死掉的 Pod/容器 + 未使用的镜像
    ↓（不足以缓解压力）
2. 驱逐用户 Pod，排序规则：
   ├─ BestEffort Pod（无 requests/limits）         → 最先被驱逐
   ├─ Burstable Pod（资源使用超过 requests）        → 其次
   ├─ Burstable Pod（资源使用未超过 requests）      → 再次
   └─ Guaranteed Pod（requests == limits）          → 最后被驱逐
   
   同 QoS 类内按 Pod Priority 排序：低优先级先驱逐
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| 节点频繁出现 MemoryPressure | 驱逐阈值设置过高或 Pod 内存请求不准确 | `kubectl describe node` 查看 Conditions；检查 `evictionHard.memory.available` |
| Pod 被 OOM Killed 而非被 kubelet 驱逐 | 内存增长过快，kubelet 来不及驱逐 | 检查 Pod 的 OOMKilled 事件；调高硬驱逐阈值 |
| 节点条件频繁在 True/False 间振荡 | evictionPressureTransitionPeriod 过短 | 增大 `evictionPressureTransitionPeriod`（默认 5 分钟） |
| 驱逐后立即又触发新的驱逐 | evictionMinimumReclaim 未设置或过小 | 配置 `evictionMinimumReclaim` 确保每次驱逐回收足够资源 |
| DaemonSet Pod 被驱逐 | DaemonSet Pod 优先级不够高 | 为 DaemonSet 设置高 PriorityClass |
| 大量使用本地存储的 Pod 触发 DiskPressure | 容器日志或临时文件占用过多 | 配置日志轮转；设置 Pod 的 `ephemeral-storage` limits |

## 生产检查清单

- [ ] 为所有节点配置合理的硬驱逐阈值（memory / disk / inode / pid）
- [ ] 配置软驱逐阈值 + 宽限期，给 Pod 优雅终止时间
- [ ] 设置 `evictionMinimumReclaim` 避免反复驱逐
- [ ] 关键服务使用 Guaranteed QoS（requests == limits）+ 高 PriorityClass
- [ ] DaemonSet Pod 设置足够高的优先级避免被驱逐
- [ ] 监控节点条件：MemoryPressure / DiskPressure / PIDPressure
- [ ] 为 Pod 设置合理的 `ephemeral-storage` requests 和 limits
- [ ] 配置日志轮转和镜像清理策略减少磁盘压力

## 命令快速参考

```bash
# 查看节点压力条件
kubectl get nodes -o custom-columns='NAME:.metadata.name,MEM_PRESSURE:.status.conditions[?(@.type=="MemoryPressure")].status,DISK_PRESSURE:.status.conditions[?(@.type=="DiskPressure")].status'

# 查看节点详细条件
kubectl describe node <node-name> | grep -A 20 Conditions

# 查看因压力驱逐的 Pod
kubectl get events --field-selector reason=Evicted --all-namespaces

# 查看节点资源使用
kubectl top nodes

# 查看 kubelet 驱逐配置
ssh <node> cat /var/lib/kubelet/config.yaml | grep -A 20 eviction

# 查看 Pod QoS 等级
kubectl get pods -o custom-columns='NAME:.metadata.name,QOS:.status.qosClass'

# 查看节点内存使用详情
kubectl describe node <node-name> | grep -A 5 "Allocated resources"
```

## 交叉引用

- [API 发起驱逐](./api-initiated-eviction.md) — API 驱逐尊重 PDB，节点压力驱逐不尊重
- [Pod 优先级与抢占](./pod-priority-and-preemption.md) — 驱逐排序中 Pod Priority 的作用
- [污点与容忍度](./taints-and-tolerations.md) — kubelet 自动添加 memory-pressure / disk-pressure 污点
- [Pod Overhead](./pod-overhead.md) — kubelet 驱逐排序包含 Pod overhead

## 参考链接

- [Kubernetes 官方文档 - Node-pressure Eviction](https://kubernetes.io/docs/concepts/scheduling-eviction/node-pressure-eviction/)

## Related

- [[domain-19-landscape-references/topic-index/scheduler-index.md|Scheduler 调度与弹性伸缩知识图谱索引]]
