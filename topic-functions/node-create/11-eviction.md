---
title: 节点资源压力与 Eviction 源码分析
description: '## 概述'
category: functions
tags:
- k8s
- operations
- cluster-management
- kubelet
- prometheus
- containerd
- rag
last_updated: 2026-05-18
difficulty: advanced
reading_level: advanced
audience:
- Kubernetes 运维工程师
- 平台工程师
- SRE 工程师
estimated_read_time: 5min
intent_queries:
- kubelet eviction mechanism source code analysis
- Kubernetes QoS pod eviction priority
- memory.available eviction threshold configuration
- OOMKill vs Eviction kubelet difference
- evictionHard evictionSoft区别配置
trigger_keywords:
- eviction
- QoS
- OOMKill
- memory pressure
- disk pressure
- pod eviction
- kubelet eviction manager
- evictionHard
- evictionSoft
- Hard Eviction
- Soft Eviction
- OOM
- out of memory
related_domains:
- domain-3-control-plane
- domain-5-networking
- domain-12-troubleshooting
related_topics:
- node-create/04-drain
- node-create/08-troubleshooting
- node-create/12-monitoring
- cluster-create/03-certs
---


# 节点资源压力与 Eviction — 源码分析

## 概述

节点资源压力管理是 Kubernetes 保证集群稳定性的关键机制。当节点资源（内存、磁盘、PID）不足时，kubelet 会通过 Eviction（驱逐）机制主动终止低优先级的 Pod，释放资源以保护高优先级的工作负载和节点自身的稳定性。

kubelet 的驱逐管理器（Eviction Manager）是一个独立的协调循环，它定期检查节点的资源使用情况，当资源使用量超过配置的阈值时，按照 Pod 的 QoS（Quality of Service）等级和实际资源使用量来选择要驱逐的 Pod。这种机制比被动等待 Linux OOM Killer 杀死进程更加优雅和可控。

理解 Eviction 机制对于以下场景至关重要：

- **资源规划**：合理设置 Pod 的 requests 和 limits，确保 QoS 等级正确
- **稳定性保障**：配置合理的驱逐阈值，在资源紧张前主动释放
- **故障排查**：Pod 被驱逐时理解原因并采取正确的应对措施
- **成本优化**：通过 QoS 分级实现不同优先级工作负载的差异化保障

本文档从源码层面深入分析 kubelet 的驱逐管理器、QoS 分级机制、OOM Kill 行为以及资源压力状态的管理。

---

## 源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| 驱逐管理器 | `pkg/kubelet/eviction/` | 驱逐逻辑核心 |
| 驱逐辅助 | `pkg/kubelet/eviction/helpers.go` | 辅助函数 |
| QoS 管理 | `pkg/apis/core/helper/qos/` | QoS 等级判定 |
| 准入控制 | `pkg/kubelet/lifecycle/admission/` | Pod 准入 |
| cAdvisor | `pkg/kubelet/cadvisor/` | 资源监控数据源 |
| OOM 配置 | `pkg/kubelet/eviction/eviction_manager.go` | OOM 保护 |

---

## 一、kubelet Eviction 机制

### 1.1 驱逐类型

kubelet 支持两种驱逐类型：

| 类型 | 触发条件 | 行为 | 配置参数 |
|------|---------|------|---------|
| **硬驱逐（Hard Eviction）** | 资源量立即低于阈值 | 立即终止 Pod | `evictionHard` |
| **软驱逐（Soft Eviction）** | 资源量低于阈值持续一段时间 | 等待宽限期后终止 Pod | `evictionSoft` + `evictionSoftGracePeriod` |

### 1.2 驱逐信号（Eviction Signal）

kubelet 监控以下资源信号来触发驱逐：

| 信号 | 说明 | 单位 |
|------|------|------|
| `memory.available` | 节点可用内存 | bytes / Mi / Gi |
| `nodefs.available` | 节点文件系统可用空间（kubelet 卷） | bytes / % |
| `nodefs.inodesFree` | 节点文件系统可用 inode | 数量 / % |
| `imagefs.available` | 镜像文件系统可用空间 | bytes / % |
| `imagefs.inodesFree` | 镜像文件系统可用 inode | 数量 / % |
| `pid.available` | 可用 PID 数量 | 数量 |

### 1.3 驱逐配置详解

```yaml
# /var/lib/kubelet/config.yaml

# 硬驱逐阈值 — 超过立即驱逐，无宽限期
evictionHard:
  memory.available: "100Mi"       # 可用内存 < 100Mi
  nodefs.available: "10%"         # 节点磁盘 < 10%
  imagefs.available: "15%"        # 镜像盘 < 15%
  nodefs.inodesFree: "5%"         # inode < 5%

# 软驱逐阈值 — 超过并持续宽限期后驱逐
evictionSoft:
  memory.available: "200Mi"       # 可用内存 < 200Mi
  nodefs.available: "15%"         # 节点磁盘 < 15%

# 软驱逐宽限期
evictionSoftGracePeriod:
  memory.available: "1m30s"       # 内存压力持续 1 分 30 秒后驱逐
  nodefs.available: "2m"          # 磁盘压力持续 2 分钟后驱逐

# 驱逐后最小回收量 — 驱逐后至少回收这么多资源
evictionMinimumReclaim:
  memory.available: "50Mi"        # 至少回收 50Mi 内存
  nodefs.available: "500Mi"       # 至少回收 500Mi 磁盘

# 驱逐状态转换延迟 — 防止状态抖动
evictionPressureTransitionPeriod: 5m
```

### 1.4 源码分析：驱逐管理器

```go
// pkg/kubelet/eviction/eviction_manager.go
type managerImpl struct {
    // 驱逐配置
    thresholdManager   thresholdManager
    // 信号监控
    signalThresholds   map[evictionapi.Signal]evictionapi.Threshold
    // 最后一次驱逐时间
    lastEviction       time.Time
}

func (m *managerImpl) synchronize(diskInfo resourceInfo, memInfo resourceInfo) error {
    // 1. 获取当前资源使用量 (通过 cAdvisor)
    // 2. 对比每个信号的阈值
    // 3. 如果超过硬驱逐阈值，立即驱逐
    // 4. 如果超过软驱逐阈值，检查宽限期
    // 5. 按 QoS 等级选择要驱逐的 Pod
    // 6. 终止选中的 Pod
    // 7. 更新节点 Conditions
}
```

---

## 二、驱逐优先级（QoS）

### 2.1 QoS 等级

Kubernetes 根据 Pod 的资源 requests 和 limits 配置，将 Pod 分为三个 QoS 等级：

```
驱逐优先级 (从高到低):
  ┌─────────────────────────────────────────────────────────────┐
  │  Guaranteed (最高优先级，最后被驱逐)                         │
  │  条件: 所有容器都设置了 limits，且 requests == limits        │
  ├─────────────────────────────────────────────────────────────┤
  │  Burstable (中等优先级)                                      │
  │  条件: 至少一个容器设置了 requests 或 limits                 │
  ├─────────────────────────────────────────────────────────────┤
  │  BestEffort (最低优先级，最先被驱逐)                         │
  │  条件: 所有容器都没有设置 requests 和 limits                 │
  └─────────────────────────────────────────────────────────────┘
```

### 2.2 QoS 判定源码

```go
// pkg/apis/core/helper/qos/qos.go
func GetPodQOS(pod *v1.Pod) v1.PodQOSClass {
    requests := v1.ResourceList{}
    limits := v1.ResourceList{}
    
    for _, container := range pod.Spec.Containers {
        // 收集所有容器的 requests 和 limits
        for name, quantity := range container.Resources.Requests {
            requests[name] = quantity
        }
        for name, quantity := range container.Resources.Limits {
            limits[name] = quantity
        }
    }
    
    // 判定逻辑:
    // 1. 如果所有容器都设置了 CPU+memory 的 limits → Guaranteed
    // 2. 如果至少设置了 requests → Burstable
    // 3. 否则 → BestEffort
}
```

### 2.3 QoS 配置示例

```yaml
# Guaranteed: requests == limits
spec:
  containers:
  - name: app
    resources:
      requests:
        cpu: "1"
        memory: "1Gi"
      limits:
        cpu: "1"
        memory: "1Gi"

# Burstable: 设置了 requests 但不等于 limits
spec:
  containers:
  - name: app
    resources:
      requests:
        cpu: "500m"
        memory: "512Mi"
      limits:
        cpu: "2"
        memory: "2Gi"

# BestEffort: 没有设置任何 requests/limits
spec:
  containers:
  - name: app
    image: nginx
```

### 2.4 同一 QoS 等级内的驱逐顺序

在同一个 QoS 等级内，kubelet 按照以下规则选择要驱逐的 Pod：

1. **实际资源使用量超过请求量最多的 Pod 优先被驱逐**
2. **使用量与请求量的比值最大的 Pod 优先被驱逐**

```go
// pkg/kubelet/eviction/helpers.go
func sortPods(pods []*v1.Pod, sorter func(*v1.Pod) float64) {
    // 按资源使用量排序:
    // 1. BestEffort Pod 先于 Burstable Pod
    // 2. Burstable Pod 中，(usage - request) 最大的先驱逐
    // 3. Guaranteed Pod 通常不会被驱逐 (除非内存压力极端)
}
```

---

## 三、OOM Kill

### 3.1 OOM Kill 与 Eviction 的区别

| 维度 | Eviction (kubelet) | OOM Kill (内核) |
|------|-------------------|-----------------|
| 触发者 | kubelet | Linux 内核 OOM Killer |
| 时机 | 资源接近耗尽时主动触发 | 资源已耗尽时被动触发 |
| 选择逻辑 | 按 QoS 等级和资源使用量 | 按 oom_score_adj 和内存使用量 |
| 优雅性 | 发送 SIGTERM，等待优雅终止 | 直接发送 SIGKILL，无法捕获 |
| Pod 状态 | Evicted 状态 | OOMKilled 状态 |

### 3.2 OOM Kill 排查

```bash
# 查看 OOM 事件
dmesg | grep -i "oom"
dmesg | grep -i "out of memory"
dmesg | grep -i "killed process"

# 查看 Pod OOM 事件
kubectl get events --all-namespaces | grep OOM
kubectl describe pod <pod> | grep -A 5 "Last State"
# Last State:     Terminated
#   Reason:       OOMKilled
#   Exit Code:    137

# 查看 cgroup OOM (cgroup v2)
cat /sys/fs/cgroup/kubepods/memory.events
# oom_kill 3

# 查看进程 oom_score_adj
cat /proc/<pid>/oom_score_adj
# Guaranteed: -997 (不容易被 OOM)
# Burstable: varies (基于内存使用)
# BestEffort: 1000 (最容易被 OOM)
```

### 3.3 OOM 保护配置

```bash
# kubelet 会为不同 QoS 的 Pod 设置不同的 oom_score_adj:
# Guaranteed:   -997  (最低，最后被 OOM Kill)
# Burstable:    min(max(0, 1000 - (1000 * memoryRequest) / memoryCapacity), 999)
# BestEffort:   1000  (最高，最先被 OOM Kill)

# 查看容器 oom_score_adj
kubectl exec -it <pod> -- cat /proc/1/oom_score_adj
```

---

## 四、本地临时存储

### 4.1 临时存储来源

```yaml
# 临时存储 (ephemeral-storage) 包含:
# 1. emptyDir 卷
# 2. 容器可写层 (container writable layer)
# 3. 容器日志
# 4. tmpfs (如果使用 emptyDir.medium=Memory)

# 当 nodefs.available < 10% (默认) 时:
# - kubelet 开始驱逐使用最多临时存储的 Pod
```

### 4.2 临时存储限制

```yaml
# 设置 Pod 临时存储限制
spec:
  containers:
  - name: app
    resources:
      limits:
        ephemeral-storage: "1Gi"      # 限制临时存储使用量
      requests:
        ephemeral-storage: "500Mi"
```

---

## 五、资源压力状态管理

### 5.1 节点 Conditions

kubelet 根据资源使用情况更新节点 Conditions：

| Condition | 触发条件 | 影响 |
|-----------|---------|------|
| `MemoryPressure` | 可用内存 < 驱逐阈值 | 仅允许调度 Guaranteed Pod |
| `DiskPressure` | 磁盘空间 < 驱逐阈值 | 仅允许调度 Guaranteed Pod |
| `PIDPressure` | PID 使用量 > 阈值 | 仅允许调度 Guaranteed Pod |

### 5.2 状态转换延迟

```yaml
# evictionPressureTransitionPeriod: 5m
# 状态转换延迟 5 分钟，防止状态抖动:
# - 从正常 → 压力状态: 立即转换
# - 从压力 → 正常状态: 需要持续 5 分钟无压力
```

---

## 六、监控与告警

### 6.1 驱逐相关指标

```bash
# kubelet 驱逐指标
kubelet_evictions_total{signal="memory.available"}          # 驱逐次数
kubelet_node_controller_evictions_total                     # 节点控制器驱逐次数

# 资源使用指标
node_memory_MemAvailable_bytes                             # 可用内存
node_filesystem_avail_bytes{mountpoint="/"}                # 可用磁盘
kube_node_status_condition{condition="MemoryPressure"}    # 内存压力状态
kube_node_status_condition{condition="DiskPressure"}      # 磁盘压力状态
```

### 6.2 推荐告警规则

```yaml
# Prometheus 告警
groups:
- name: eviction-alerts
  rules:
  - alert: PodEvicted
    expr: increase(kubelet_evictions_total[5m]) > 0
    labels:
      severity: warning
    annotations:
      summary: "Pod was evicted on {{ $labels.node }}"

  - alert: NodeMemoryPressure
    expr: kube_node_status_condition{condition="MemoryPressure",status="true"} == 1
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Node {{ $labels.node }} is under memory pressure"
```

---

## 七、常见错误与排查

| 错误 | 原因 | 排查命令 | 解决方案 |
|------|------|---------|---------|
| Pod 被 OOMKilled | 容器内存超过 limit | `kubectl describe pod <pod> \| grep OOMKilled` | 增加 memory limit 或优化内存使用 |
| Pod 被 Evicted | 节点资源不足 | `kubectl describe pod <pod> \| grep -A 5 Status` | 扩容节点或调整驱逐阈值 |
| 磁盘满导致节点不可用 | 镜像/日志占满磁盘 | `df -h; du -sh /var/lib/containerd/*` | 清理镜像/日志，增加磁盘容量 |
| BestEffort Pod 频繁被驱逐 | 节点资源长期紧张 | `kubectl get events \| grep Evicted` | 为 Pod 设置 requests/limits |
| 软驱逐不触发 | 宽限期配置错误 | `cat /var/lib/kubelet/config.yaml \| grep evictionSoft` | 检查 evictionSoftGracePeriod 配置 |
| Guaranteed Pod 被驱逐 | 系统级内存耗尽 | `dmesg \| grep oom` | 增加 --kube-reserved 和 --system-reserved |
| 驱逐后状态不恢复 | evictionPressureTransitionPeriod 太长 | `kubectl describe node <node> \| grep Conditions` | 减少延迟时间 |

### 调试命令

```bash
# 查看被驱逐的 Pod
kubectl get pods --all-namespaces | grep Evicted

# 查看驱逐事件
kubectl get events --all-namespaces --sort-by='.lastTimestamp' | grep -i evict

# 查看节点资源压力
kubectl describe node <node> | grep -A 10 Conditions

# 查看 kubelet 驱逐日志
journalctl -u kubelet | grep -i eviction

# 查看内存使用详情
kubectl top nodes
kubectl top pods --all-namespaces --sort-by=memory
```

---

## 相关函数

| 函数 | 源码位置 | 说明 |
|------|---------|------|
| `managerImpl.synchronize` | `pkg/kubelet/eviction/eviction_manager.go` | 驱逐主循环 |
| `sortPods` | `pkg/kubelet/eviction/helpers.go` | Pod 排序（按 QoS） |
| `GetPodQOS` | `pkg/apis/core/helper/qos/qos.go` | QoS 等级判定 |
| `evictPod` | `pkg/kubelet/eviction/eviction_manager.go` | 执行驱逐 |
| `memoryThreshold` | `pkg/kubelet/eviction/threshold.go` | 内存阈值判断 |
| `diskThreshold` | `pkg/kubelet/eviction/threshold.go` | 磁盘阈值判断 |
