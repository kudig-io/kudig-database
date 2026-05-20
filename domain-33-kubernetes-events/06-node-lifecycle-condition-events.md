---
title: 06 - 节点生命周期与状态事件
description: '- [概述](#概述)'
category: kubernetes-events
tags:
- k8s
- events
- troubleshooting
- kubelet
- scheduler
- controller-manager
- prometheus
- grafana
- containerd
- docker
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- 节点生命周期与状态事件 是什么
- 如何 节点生命周期与状态事件
- Kubernetes 33 kubernetes events 最佳实践
trigger_keywords:
- 节点生命周期与状态事件
- kubernetes
- events
cross_refs:
- type: fta
  path: ../topic-fta/list/node-fta.md
  label: '故障树: node'
---


# 06 - 节点生命周期与状态事件

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **作者**: Allen Galler

---

## 📋 目录

- [概述](#概述)
- [节点生命周期状态机](#节点生命周期状态机)
- [节点状态类型详解](#节点状态类型详解)
- [Kubelet 节点状态事件](#kubelet-节点状态事件)
- [Node Controller 事件](#node-controller-事件)
- [节点驱逐机制](#节点驱逐机制)
- [生产环境监控建议](#生产环境监控建议)

---

## 概述

节点（Node）是 Kubernetes 集群中的工作机器，可以是物理机或虚拟机。节点的健康状态直接影响集群的可用性和稳定性。本文档详细记录了节点生命周期中的所有关键事件，包括：

- **kubelet 节点状态事件**：由节点上的 kubelet 组件产生，反映节点的实时健康状况
- **Node Controller 事件**：由控制平面的 node-controller 产生，负责节点的注册、监控和驱逐逻辑
- **资源压力事件**：内存、磁盘、PID 等资源不足时触发的事件
- **驱逐事件**：节点资源压力导致的 Pod 驱逐事件

---

## 节点生命周期状态机

```
┌─────────────────────────────────────────────────────────────────┐
│                    Node Lifecycle State Machine                  │
└─────────────────────────────────────────────────────────────────┘

    [New Node]
        │
        ├─> kubelet starts
        │   Event: Starting
        │
        ├─> Node registers with API Server
        │   Event: RegisteredNode (node-controller)
        │
        ├─> Initial status: NodeNotReady
        │   Event: NodeNotReady
        │
        ├─> kubelet initializes (network, runtime, etc.)
        │
        ├─> Node becomes ready
        │   Event: NodeReady
        │   Condition: Ready=True
        │
        ├─> Normal Operation
        │   ├─> Periodic heartbeat (node status updates)
        │   ├─> Resource monitoring
        │   └─> Condition updates
        │
        ├─> Resource Pressure Detected
        │   ├─> MemoryPressure: NodeHasInsufficientMemory
        │   ├─> DiskPressure: NodeHasDiskPressure
        │   └─> PIDPressure: NodeHasInsufficientPID
        │
        ├─> Eviction Threshold Met
        │   Event: EvictionThresholdMet
        │   └─> Start evicting pods
        │
        ├─> Node Becomes Unhealthy
        │   Event: NodeNotReady
        │   └─> node-controller starts monitoring
        │
        ├─> Grace Period Expired (default: 5min)
        │   Event: DeletingAllPods
        │   └─> Force terminate pods
        │
        └─> Node Removal
            Event: RemovingNode / DeletingNode

┌─────────────────────────────────────────────────────────────────┐
│                     Schedulability States                        │
└─────────────────────────────────────────────────────────────────┘

    NodeSchedulable ←──────────────→ NodeNotSchedulable
    (spec.unschedulable=false)      (spec.unschedulable=true)
                                    (kubectl cordon)
```

---

## 节点状态类型详解

Kubernetes 节点有以下五种核心 Condition 类型：

| Condition Type | 含义 | True 状态 | False 状态 | Unknown 状态 |
|:---|:---|:---|:---|:---|
| **Ready** | 节点是否健康并准备接受 Pod | 节点健康，可调度 | 节点不健康 | node-controller 无法连接 |
| **MemoryPressure** | 节点内存是否紧张 | 内存不足 | 内存充足 | 无法检测 |
| **DiskPressure** | 节点磁盘是否紧张 | 磁盘不足 | 磁盘充足 | 无法检测 |
| **PIDPressure** | 节点进程 ID 是否紧张 | PID 不足 | PID 充足 | 无法检测 |
| **NetworkUnavailable** | 节点网络是否不可用 | 网络配置错误 | 网络正常 | 无法检测 |

---

## Kubelet 节点状态事件

### `Starting` - kubelet 启动

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | kubelet |
| **关联资源** | Node |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 (仅在 kubelet 启动时) |

#### 事件含义

此事件表示节点上的 kubelet 组件正在启动。kubelet 是每个节点上的核心组件，负责管理 Pod 和容器的生命周期，与 API Server 通信，并报告节点状态。

在 kubelet 启动时，它会进行一系列初始化操作，包括加载配置、初始化容器运行时客户端、设置网络、注册节点到 API Server 等。此事件是节点生命周期的起点。

#### 典型事件消息

```yaml
Type:    Normal
Reason:  Starting
Message: Starting kubelet.
Source:  kubelet, node1.example.com
```

#### 影响面说明

- **集群影响**：单个节点的 kubelet 启动不影响其他节点
- **调度影响**：在节点变为 Ready 之前，scheduler 不会将 Pod 调度到此节点
- **现有 Pod**：如果是重启，节点上的 Pod 需要重新同步和启动

#### 排查建议

```bash
# 查看 kubelet 启动事件
kubectl get events --field-selector involvedObject.kind=Node,reason=Starting

# 检查 kubelet 服务状态
systemctl status kubelet

# 查看 kubelet 启动日志
journalctl -u kubelet -n 100 --no-pager

# 检查 kubelet 配置
kubelet --version
ps aux | grep kubelet
```

#### 解决建议

| 场景 | 建议 |
|:---|:---|
| 正常启动 | 无需处理，等待节点变为 Ready |
| 反复重启 | 检查 kubelet 日志，可能是配置错误或依赖服务未启动 |
| 启动失败 | 检查容器运行时(containerd/docker)是否正常运行 |
| 启动缓慢 | 检查网络连通性和 API Server 可达性 |

---

### `NodeReady` - 节点就绪

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | kubelet |
| **关联资源** | Node |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 (状态变化时) |

#### 事件含义

此事件表示节点已成功通过所有健康检查，变为 Ready 状态，可以接受 Pod 调度。kubelet 会定期检查节点的各项资源和组件状态，包括容器运行时、网络插件、存储插件等。

当节点的 Ready Condition 从 False/Unknown 变为 True 时，会产生此事件。这是节点生命周期中的重要里程碑，标志着节点可以开始承载工作负载。

#### 典型事件消息

```yaml
Type:    Normal
Reason:  NodeReady
Message: Node node1.example.com status is now: NodeReady
Source:  kubelet, node1.example.com
```

```bash
# kubectl describe node 输出
Conditions:
  Type             Status  LastHeartbeatTime                 LastTransitionTime                Reason                       Message
  ----             ------  -----------------                 ------------------                ------                       -------
  Ready            True    Mon, 10 Feb 2026 10:30:00 +0800  Mon, 10 Feb 2026 10:25:00 +0800  KubeletReady                 kubelet is posting ready status
```

#### 影响面说明

- **集群影响**：增加集群的可用容量
- **调度影响**：scheduler 开始可以将 Pod 调度到此节点
- **现有 Pod**：如果是从 NotReady 恢复，节点上的 Pod 会重新启动

#### 排查建议

```bash
# 查看节点状态
kubectl get nodes

# 查看节点详细信息
kubectl describe node <node-name>

# 查看节点状态历史
kubectl get events --field-selector involvedObject.kind=Node,involvedObject.name=<node-name> --sort-by='.lastTimestamp'

# 检查节点资源容量
kubectl get node <node-name> -o jsonpath='{.status.capacity}' | jq

# 检查节点可分配资源
kubectl get node <node-name> -o jsonpath='{.status.allocatable}' | jq
```

#### 解决建议

| 场景 | 建议 |
|:---|:---|
| 新节点变为 Ready | 正常情况，可以开始调度 Pod |
| 节点从 NotReady 恢复 | 检查之前失败的原因，确保根本问题已解决 |
| Ready 状态不稳定 | 检查网络、存储等底层基础设施稳定性 |
| 反复 Ready/NotReady | 可能是 kubelet 心跳超时，检查 API Server 连接和性能 |

---

### `NodeNotReady` - 节点未就绪

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet / node-controller |
| **关联资源** | Node |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 (故障时) |

#### 事件含义

此事件表示节点的健康检查失败，节点变为 NotReady 状态。这可能是由于 kubelet 本身的问题、容器运行时故障、网络问题或其他关键组件故障导致的。

当节点变为 NotReady 状态时，scheduler 将不再调度新的 Pod 到此节点。如果节点持续 NotReady 超过一定时间（默认 5 分钟），node-controller 会开始驱逐节点上的 Pod。

#### 典型事件消息

```yaml
Type:    Warning
Reason:  NodeNotReady
Message: Node node1.example.com status is now: NodeNotReady
Source:  kubelet, node1.example.com
```

```bash
# kubectl describe node 输出
Conditions:
  Type             Status  LastHeartbeatTime                 LastTransitionTime                Reason                       Message
  ----             ------  -----------------                 ------------------                ------                       -------
  Ready            False   Mon, 10 Feb 2026 10:30:00 +0800  Mon, 10 Feb 2026 10:35:00 +0800  KubeletNotReady              container runtime not responding
```

或者由 node-controller 检测到心跳超时：

```yaml
Type:    Warning
Reason:  NodeNotReady
Message: Node node1.example.com status is now: NodeNotReady (node-controller detected)
Source:  node-controller
```

#### 影响面说明

- **集群影响**：减少集群可用容量，可能触发告警
- **调度影响**：新 Pod 不会被调度到此节点
- **现有 Pod**：
  - 立即影响：Pod 状态变为 Unknown
  - 5 分钟后（默认 pod-eviction-timeout）：Pod 被驱逐，在其他节点重建
- **服务影响**：如果节点上有 Service 的 endpoints，会被标记为 NotReady

#### 排查建议

```bash
# 查看节点状态和条件
kubectl get nodes
kubectl describe node <node-name>

# 查看节点事件
kubectl get events --field-selector involvedObject.kind=Node,involvedObject.name=<node-name> --sort-by='.lastTimestamp'

# 检查 kubelet 服务状态（需要登录节点）
systemctl status kubelet
journalctl -u kubelet -n 100 --no-pager

# 检查容器运行时状态
systemctl status containerd  # 或 docker
crictl info
crictl pods

# 检查节点资源
top
free -h
df -h
iostat -x 1 5

# 检查网络连通性
ping <api-server-ip>
curl -k https://<api-server-ip>:6443/healthz

# 检查系统日志
journalctl -n 200 --no-pager
dmesg | tail -100
```

#### 解决建议

| 原因 | 解决方案 |
|:---|:---|
| kubelet 进程停止 | `systemctl restart kubelet` |
| 容器运行时故障 | 重启容器运行时: `systemctl restart containerd` |
| 网络连接中断 | 检查并修复网络配置，确保可以访问 API Server |
| 资源耗尽 (内存/磁盘) | 清理资源，释放空间，增加资源配额 |
| 系统负载过高 | 减少节点负载，迁移部分 Pod |
| 证书过期 | 更新 kubelet 证书，重启 kubelet |
| API Server 不可达 | 检查防火墙、网络策略，确保 API Server 健康 |
| CNI 插件故障 | 检查并修复 CNI 插件配置 |

---

### `NodeSchedulable` - 节点可调度

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | kubelet |
| **关联资源** | Node |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 (调度状态变化时) |

#### 事件含义

此事件表示节点的 `spec.unschedulable` 字段被设置为 `false`，节点恢复可调度状态。这通常发生在执行 `kubectl uncordon` 命令之后。

节点可调度意味着 Kubernetes scheduler 可以将新的 Pod 调度到此节点。这不影响已经运行在节点上的 Pod。

#### 典型事件消息

```yaml
Type:    Normal
Reason:  NodeSchedulable
Message: Node node1.example.com status is now: NodeSchedulable
Source:  kubelet, node1.example.com
```

#### 影响面说明

- **集群影响**：增加集群的可调度容量
- **调度影响**：scheduler 可以调度新 Pod 到此节点
- **现有 Pod**：不受影响，继续运行

#### 排查建议

```bash
# 查看节点可调度状态
kubectl get nodes
kubectl get node <node-name> -o jsonpath='{.spec.unschedulable}'

# 查看节点详细信息
kubectl describe node <node-name>

# 取消节点不可调度标记
kubectl uncordon <node-name>
```

#### 解决建议

| 场景 | 建议 |
|:---|:---|
| 维护后恢复 | 正常操作，确认节点健康后允许调度 |
| 自动恢复 | 检查是否有自动化工具误操作 |
| 意外变化 | 审计 API Server 日志，查找操作来源 |

---

### `NodeNotSchedulable` - 节点不可调度

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | kubelet |
| **关联资源** | Node |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 (调度状态变化时) |

#### 事件含义

此事件表示节点的 `spec.unschedulable` 字段被设置为 `true`，节点被标记为不可调度。这通常由管理员执行 `kubectl cordon` 命令触发，用于维护前的准备工作。

节点不可调度意味着 scheduler 不会将新的 Pod 调度到此节点，但已经运行的 Pod 不受影响，继续正常运行。这是安全排空节点（drain）的第一步。

#### 典型事件消息

```yaml
Type:    Normal
Reason:  NodeNotSchedulable
Message: Node node1.example.com status is now: NodeNotSchedulable
Source:  kubelet, node1.example.com
```

```bash
# kubectl get nodes 输出
NAME                 STATUS                     ROLES    AGE   VERSION
node1.example.com    Ready,SchedulingDisabled   worker   10d   v1.28.0
```

#### 影响面说明

- **集群影响**：减少集群的可调度容量
- **调度影响**：新 Pod 不会被调度到此节点
- **现有 Pod**：不受影响，继续运行
- **DaemonSet**：即使节点不可调度，DaemonSet Pod 仍会被调度

#### 排查建议

```bash
# 查看不可调度的节点
kubectl get nodes | grep SchedulingDisabled

# 查看节点详细信息
kubectl describe node <node-name>

# 查看节点 unschedulable 字段
kubectl get node <node-name> -o jsonpath='{.spec.unschedulable}'

# 查看相关事件
kubectl get events --field-selector involvedObject.kind=Node,involvedObject.name=<node-name>

# 标记节点为不可调度
kubectl cordon <node-name>

# 恢复节点可调度
kubectl uncordon <node-name>
```

#### 解决建议

| 场景 | 建议 |
|:---|:---|
| 计划维护 | 正常操作流程：cordon → drain → 维护 → uncordon |
| 排查问题 | 暂时隔离节点，防止新 Pod 调度到有问题的节点 |
| 意外标记 | 执行 `kubectl uncordon` 恢复调度 |
| 长期不可调度 | 考虑是否应该删除节点或解决根本问题 |

---

### `NodeHasSufficientMemory` - 节点内存充足

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | kubelet |
| **关联资源** | Node |
| **适用版本** | v1.4+ |
| **生产频率** | 低频 (MemoryPressure 状态变化时) |

#### 事件含义

此事件表示节点的可用内存恢复到正常水平，MemoryPressure condition 从 True 变为 False。kubelet 会定期检查节点的内存使用情况，当可用内存超过驱逐阈值时，会产生此事件。

这意味着节点从内存压力状态中恢复，可以继续接受新的 Pod 调度（如果之前因内存压力被标记为不可调度）。

#### 典型事件消息

```yaml
Type:    Normal
Reason:  NodeHasSufficientMemory
Message: Node node1.example.com status is now: NodeHasSufficientMemory
Source:  kubelet, node1.example.com
```

```bash
# kubectl describe node 输出
Conditions:
  Type             Status  Reason                  Message
  ----             ------  ------                  -------
  MemoryPressure   False   KubeletHasSufficientMemory   kubelet has sufficient memory available
```

#### 影响面说明

- **集群影响**：节点恢复正常服务能力
- **调度影响**：如果之前因 MemoryPressure 被 scheduler 避免，现在可以正常调度
- **驱逐影响**：停止基于内存压力的 Pod 驱逐

#### 排查建议

```bash
# 查看节点 MemoryPressure 状态
kubectl get node <node-name> -o jsonpath='{.status.conditions[?(@.type=="MemoryPressure")]}'

# 查看节点内存使用情况
kubectl describe node <node-name> | grep -A 5 "Allocated resources"

# 登录节点查看内存
free -h
vmstat 1 5

# 查看节点上 Pod 的内存使用
kubectl top pods --all-namespaces --field-selector spec.nodeName=<node-name> --sort-by=memory
```

#### 解决建议

| 场景 | 建议 |
|:---|:---|
| 正常恢复 | 无需处理，监控内存使用趋势 |
| 频繁波动 | 检查是否有内存泄漏的应用，调整 Pod 资源限制 |
| 驱逐后恢复 | 分析之前内存压力的原因，优化资源配置 |

---

### `NodeHasNoDiskPressure` - 节点无磁盘压力

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | kubelet |
| **关联资源** | Node |
| **适用版本** | v1.4+ |
| **生产频率** | 低频 (DiskPressure 状态变化时) |

#### 事件含义

此事件表示节点的磁盘空间恢复到正常水平，DiskPressure condition 从 True 变为 False。kubelet 监控节点的磁盘使用情况，包括根文件系统（nodefs）和容器镜像文件系统（imagefs）。

当磁盘可用空间超过驱逐阈值时，节点从磁盘压力状态中恢复。这意味着节点可以继续拉取镜像和创建容器。

#### 典型事件消息

```yaml
Type:    Normal
Reason:  NodeHasNoDiskPressure
Message: Node node1.example.com status is now: NodeHasNoDiskPressure
Source:  kubelet, node1.example.com
```

```bash
# kubectl describe node 输出
Conditions:
  Type             Status  Reason                  Message
  ----             ------  ------                  -------
  DiskPressure     False   KubeletHasNoDiskPressure   kubelet has no disk pressure
```

#### 影响面说明

- **集群影响**：节点恢复正常服务能力
- **调度影响**：scheduler 恢复正常调度到此节点
- **容器影响**：可以正常拉取镜像和创建容器

#### 排查建议

```bash
# 查看节点 DiskPressure 状态
kubectl get node <node-name> -o jsonpath='{.status.conditions[?(@.type=="DiskPressure")]}'

# 查看节点磁盘使用情况
kubectl describe node <node-name> | grep -A 10 "Capacity\|Allocatable"

# 登录节点查看磁盘
df -h
df -i  # 检查 inode 使用率

# 查看 kubelet 日志中的磁盘相关信息
journalctl -u kubelet | grep -i "disk\|eviction"

# 查看容器镜像占用
crictl images
du -sh /var/lib/containerd  # 或 /var/lib/docker
```

#### 解决建议

| 场景 | 建议 |
|:---|:---|
| 正常恢复 | 监控磁盘使用趋势，确保不再出现压力 |
| GC 后恢复 | 正常情况，kubelet 自动清理了未使用的镜像和容器 |
| 手动清理后恢复 | 建立定期清理机制，或调整 kubelet GC 配置 |

---

### `NodeHasSufficientPID` - 节点 PID 充足

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | kubelet |
| **关联资源** | Node |
| **适用版本** | v1.14+ |
| **生产频率** | 罕见 (PIDPressure 状态变化时) |

#### 事件含义

此事件表示节点的可用进程 ID 恢复到正常水平，PIDPressure condition 从 True 变为 False。Linux 系统对进程数量有限制（kernel.pid_max），当节点上的进程数接近此限制时会触发 PID 压力。

PID 压力的恢复意味着节点上的进程数降低到安全水平，可以继续创建新进程和容器。

#### 典型事件消息

```yaml
Type:    Normal
Reason:  NodeHasSufficientPID
Message: Node node1.example.com status is now: NodeHasSufficientPID
Source:  kubelet, node1.example.com
```

```bash
# kubectl describe node 输出
Conditions:
  Type             Status  Reason                  Message
  ----             ------  ------                  -------
  PIDPressure      False   KubeletHasSufficientPID   kubelet has sufficient PID available
```

#### 影响面说明

- **集群影响**：节点恢复正常服务能力
- **调度影响**：scheduler 恢复正常调度到此节点
- **容器影响**：可以正常创建新容器和进程

#### 排查建议

```bash
# 查看节点 PIDPressure 状态
kubectl get node <node-name> -o jsonpath='{.status.conditions[?(@.type=="PIDPressure")]}'

# 登录节点查看进程数
ps aux | wc -l
cat /proc/sys/kernel/pid_max
cat /proc/sys/kernel/threads-max

# 查看节点上的 Pod 数量
kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name> --no-headers | wc -l

# 查看 cgroup PID 使用情况
cat /sys/fs/cgroup/pids/kubepods/pids.current
cat /sys/fs/cgroup/pids/kubepods/pids.max
```

#### 解决建议

| 场景 | 建议 |
|:---|:---|
| 正常恢复 | 监控进程数趋势，确保不再出现压力 |
| Pod 驱逐后恢复 | 分析之前 PID 压力的原因，可能需要限制单个 Pod 的进程数 |
| 频繁波动 | 检查是否有异常应用创建大量进程，调整 Pod 的 PID 限制 |

---

### `NodeHasInsufficientMemory` - 节点内存不足

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Node |
| **适用版本** | v1.4+ |
| **生产频率** | 中频 (资源压力场景) |

#### 事件含义

此事件表示节点的可用内存低于 kubelet 配置的驱逐阈值，MemoryPressure condition 被设置为 True。这是节点进入资源压力状态的关键信号。

当内存不足时，kubelet 会采取以下措施：
1. 触发 MemoryPressure condition，通知 scheduler 避免调度更多 Pod
2. 如果达到硬驱逐阈值（hard eviction），立即开始驱逐 Pod
3. 如果达到软驱逐阈值（soft eviction），在宽限期后开始驱逐 Pod

kubelet 会优先驱逐 QoS 等级较低的 Pod（BestEffort > Burstable > Guaranteed）。

#### 典型事件消息

```yaml
Type:    Warning
Reason:  NodeHasInsufficientMemory
Message: Node node1.example.com status is now: NodeHasInsufficientMemory
Source:  kubelet, node1.example.com
```

```bash
# kubectl describe node 输出
Conditions:
  Type             Status  Reason                  Message
  ----             ------  ------                  -------
  MemoryPressure   True    KubeletHasInsufficientMemory   kubelet has insufficient memory available
```

#### 影响面说明

- **集群影响**：节点可用性降低，可能触发集群级别告警
- **调度影响**：scheduler 会避免调度新 Pod 到此节点
- **现有 Pod**：
  - QoS=BestEffort 的 Pod 最先被驱逐
  - QoS=Burstable 的 Pod 其次被驱逐
  - QoS=Guaranteed 的 Pod 最后被驱逐
- **服务影响**：被驱逐的 Pod 会在其他节点重建，可能导致短暂的服务中断

#### 排查建议

```bash
# 查看节点内存状态
kubectl describe node <node-name> | grep -A 5 "Conditions\|Allocated resources"

# 查看节点内存详细信息
kubectl get node <node-name> -o json | jq '.status.conditions[] | select(.type=="MemoryPressure")'

# 查看节点上 Pod 的内存使用情况（按内存排序）
kubectl top pods --all-namespaces --field-selector spec.nodeName=<node-name> --sort-by=memory

# 查看节点上 Pod 的 QoS 等级
kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name> -o custom-columns=NAME:.metadata.name,QOS:.status.qosClass,MEMORY:.spec.containers[*].resources.requests.memory

# 登录节点查看系统内存
free -h
vmstat 1 5
cat /proc/meminfo

# 查看内存最多的进程
ps aux --sort=-%mem | head -20

# 查看 kubelet 驱逐阈值配置
kubectl get --raw /api/v1/nodes/<node-name>/proxy/configz | jq '.kubeletconfig.evictionHard, .kubeletconfig.evictionSoft'

# 查看驱逐相关事件
kubectl get events --all-namespaces --field-selector reason=Evicted,involvedObject.kind=Pod
```

#### 解决建议

| 原因 | 解决方案 |
|:---|:---|
| Pod 内存使用超出预期 | 优化应用内存使用，修复内存泄漏 |
| Pod requests 配置不合理 | 调整 Pod 的 memory requests 和 limits |
| 节点内存配置不足 | 增加节点物理内存，或迁移部分 Pod 到其他节点 |
| 系统进程占用过多 | 优化系统配置，限制非必要系统进程 |
| 内存碎片化 | 重启节点进行内存整理（需要先 drain） |
| 调整驱逐阈值 | 根据实际情况调整 kubelet 的 evictionHard/evictionSoft 配置 |
| 增加节点 | 横向扩展集群，增加更多工作节点 |

---

### `NodeHasDiskPressure` - 节点磁盘压力

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Node |
| **适用版本** | v1.4+ |
| **生产频率** | 中频 (资源压力场景) |

#### 事件含义

此事件表示节点的磁盘空间或 inode 低于 kubelet 配置的驱逐阈值，DiskPressure condition 被设置为 True。kubelet 监控两个文件系统：

1. **nodefs**：节点根文件系统，存储 Pod 日志、EmptyDir 卷、writable layers 等
2. **imagefs**：容器镜像文件系统（如果单独分区），存储容器镜像和容器可写层

当磁盘压力发生时，kubelet 会：
1. 触发 DiskPressure condition，通知 scheduler 避免调度
2. 执行镜像垃圾回收（Image GC），删除未使用的镜像
3. 执行容器垃圾回收（Container GC），删除已停止的容器
4. 如果仍然不足，开始驱逐 Pod（优先驱逐占用磁盘最多的 Pod）

#### 典型事件消息

```yaml
Type:    Warning
Reason:  NodeHasDiskPressure
Message: Node node1.example.com status is now: NodeHasDiskPressure
Source:  kubelet, node1.example.com
```

```bash
# kubectl describe node 输出
Conditions:
  Type             Status  Reason                  Message
  ----             ------  ------                  -------
  DiskPressure     True    KubeletHasDiskPressure   kubelet has disk pressure
```

#### 影响面说明

- **集群影响**：节点可用性降低，可能触发集群级别告警
- **调度影响**：scheduler 会避免调度新 Pod 到此节点
- **镜像拉取**：可能无法拉取新镜像
- **容器创建**：可能无法创建新容器
- **现有 Pod**：占用磁盘最多的 Pod 会被优先驱逐
- **日志收集**：Pod 日志可能无法正常写入

#### 排查建议

```bash
# 查看节点磁盘状态
kubectl describe node <node-name> | grep -A 5 "Conditions\|Capacity\|Allocatable"

# 查看 DiskPressure 详细信息
kubectl get node <node-name> -o json | jq '.status.conditions[] | select(.type=="DiskPressure")'

# 登录节点查看磁盘使用情况
df -h
df -i  # 查看 inode 使用情况
du -sh /* | sort -rh | head -20

# 查看 kubelet/containerd 数据目录占用
du -sh /var/lib/kubelet
du -sh /var/lib/containerd
du -sh /var/log/pods

# 查看容器镜像占用
crictl images
crictl images -v | awk '{sum+=$5} END {print sum/1024/1024/1024 " GB"}'

# 查看未使用的容器
crictl ps -a | grep Exited

# 查看 Pod 磁盘使用（需要 metrics-server）
kubectl top pods --all-namespaces --field-selector spec.nodeName=<node-name>

# 查看 kubelet GC 配置
kubectl get --raw /api/v1/nodes/<node-name>/proxy/configz | jq '.kubeletconfig.imageGCHighThresholdPercent, .kubeletconfig.imageGCLowThresholdPercent'

# 查看驱逐阈值
kubectl get --raw /api/v1/nodes/<node-name>/proxy/configz | jq '.kubeletconfig.evictionHard, .kubeletconfig.evictionSoft'
```

#### 解决建议

| 原因 | 解决方案 |
|:---|:---|
| 容器镜像过多 | 手动清理未使用镜像: `crictl rmi --prune` |
| 容器日志过大 | 配置日志轮转，限制容器日志大小（--log-max-size, --log-max-files） |
| 已停止容器未清理 | 清理停止的容器: `crictl rm $(crictl ps -a -q --state Exited)` |
| EmptyDir 卷占用过大 | 检查 Pod 的 EmptyDir 使用，清理或限制大小 |
| 系统日志过大 | 清理系统日志: `journalctl --vacuum-time=7d` |
| 磁盘配置不足 | 扩容磁盘，或为 imagefs 单独分配磁盘 |
| 调整 GC 阈值 | 降低 imageGCHighThresholdPercent，更积极地回收镜像 |
| 调整驱逐阈值 | 根据实际情况调整 evictionHard/evictionSoft 配置 |
| Pod 数据持久化问题 | 使用 PV 而不是 EmptyDir 存储大量数据 |

---

### `NodeHasInsufficientPID` - 节点 PID 不足

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Node |
| **适用版本** | v1.14+ |
| **生产频率** | 罕见 (特殊场景) |

#### 事件含义

此事件表示节点的可用进程 ID 低于 kubelet 配置的驱逐阈值，PIDPressure condition 被设置为 True。这是一种相对罕见但严重的资源压力状态。

Linux 系统对进程数量有限制（由 `kernel.pid_max` 控制，通常为 32768 或更高）。当节点上运行大量容器，且每个容器启动多个进程时，可能会耗尽 PID。

当 PID 不足时，kubelet 会：
1. 触发 PIDPressure condition，通知 scheduler 避免调度
2. 无法创建新进程和容器
3. 如果达到驱逐阈值，开始驱逐 Pod

#### 典型事件消息

```yaml
Type:    Warning
Reason:  NodeHasInsufficientPID
Message: Node node1.example.com status is now: NodeHasInsufficientPID
Source:  kubelet, node1.example.com
```

```bash
# kubectl describe node 输出
Conditions:
  Type             Status  Reason                  Message
  ----             ------  ------                  -------
  PIDPressure      True    KubeletHasInsufficientPID   kubelet has insufficient PID available
```

#### 影响面说明

- **集群影响**：节点无法创建新进程，严重影响可用性
- **调度影响**：scheduler 会避免调度新 Pod 到此节点
- **容器影响**：无法创建新容器，现有容器也可能无法 fork 新进程
- **现有 Pod**：部分 Pod 会被驱逐以释放 PID
- **系统影响**：系统命令可能无法执行，SSH 登录可能受阻

#### 排查建议

```bash
# 查看节点 PID 压力状态
kubectl describe node <node-name> | grep -A 2 PIDPressure

# 查看详细信息
kubectl get node <node-name> -o json | jq '.status.conditions[] | select(.type=="PIDPressure")'

# 登录节点查看进程数
ps aux | wc -l
ps -eLf | wc -l  # 包含线程

# 查看系统 PID 限制
cat /proc/sys/kernel/pid_max
cat /proc/sys/kernel/threads-max

# 查看当前 PID 使用情况
cat /proc/sys/kernel/pid_max
cat /sys/fs/cgroup/pids/kubepods/pids.current
cat /sys/fs/cgroup/pids/kubepods/pids.max

# 查找进程数最多的容器
for pod in $(crictl pods -q); do
  echo "Pod: $pod"
  crictl ps | grep $pod | wc -l
done

# 查看节点上的 Pod 数量
kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name> --no-headers | wc -l

# 查看 kubelet PID 驱逐配置
kubectl get --raw /api/v1/nodes/<node-name>/proxy/configz | jq '.kubeletconfig.evictionHard, .kubeletconfig.evictionSoft'

# 查找创建进程最多的 Pod
kubectl top pods --all-namespaces --field-selector spec.nodeName=<node-name>
```

#### 解决建议

| 原因 | 解决方案 |
|:---|:---|
| 单个容器进程数过多 | 限制容器的 PID 数量，设置 `--pids-limit` |
| Pod 数量过多 | 减少节点上的 Pod 数量，迁移部分 Pod |
| 应用异常创建进程 | 修复应用 bug，避免进程泄漏 |
| 系统 PID 限制过低 | 提高 `kernel.pid_max`: `sysctl -w kernel.pid_max=65535` |
| Kubernetes 配置不当 | 调整 kubelet 的 `podPidsLimit` 和 `evictionHard` 配置 |
| 僵尸进程过多 | 查找并清理僵尸进程，修复父进程的信号处理 |
| 驱逐 Pod | 手动驱逐部分 Pod 以释放 PID: `kubectl delete pod <pod-name>` |

---

### `Rebooted` - 节点重启

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Node |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 (节点重启后) |

#### 事件含义

此事件表示 kubelet 检测到节点已重启（通过 boot ID 变化检测）。节点重启可能是计划内的维护操作，也可能是意外的系统崩溃、断电或内核 panic。

当 kubelet 检测到节点重启后，会执行以下操作：
1. 产生 Rebooted 事件
2. 检查挂载的卷，确保卷状态正确
3. 重新同步所有 Pod，重新创建容器
4. 重新配置网络和存储

节点重启会导致所有非持久化数据丢失，包括 EmptyDir 卷、容器缓存等。

#### 典型事件消息

```yaml
Type:    Warning
Reason:  Rebooted
Message: Node node1.example.com has been rebooted, boot id: 12345678-1234-1234-1234-123456789abc
Source:  kubelet, node1.example.com
```

或者：

```yaml
Type:    Warning
Reason:  Rebooted
Message: Node rebooted, boot id changed
Source:  kubelet, node1.example.com
```

#### 影响面说明

- **集群影响**：节点在重启期间不可用，降低集群容量
- **Pod 影响**：
  - 所有 Pod 需要重新启动
  - EmptyDir 卷的数据丢失
  - 容器的 writable layer 丢失
  - 启动顺序可能与之前不同
- **存储影响**：
  - PV 挂载需要重新挂载
  - 本地存储（hostPath, local PV）需要重新检查
- **网络影响**：
  - Pod IP 可能变化
  - CNI 需要重新初始化
  - iptables 规则需要重建
- **服务影响**：节点重启导致的服务中断时间取决于重启速度和 Pod 启动时间

#### 排查建议

```bash
# 查看节点重启事件
kubectl get events --field-selector involvedObject.kind=Node,reason=Rebooted --all-namespaces

# 查看节点启动时间
kubectl get node <node-name> -o json | jq '.status.nodeInfo.bootID'

# 登录节点查看系统启动时间
uptime
who -b
last reboot | head

# 查看系统日志，找出重启原因
journalctl --since "2 hours ago" | grep -i "reboot\|shutdown\|panic"
dmesg | grep -i "reboot\|panic"

# 查看内核日志
journalctl -k --since "2 hours ago"

# 查看系统崩溃报告（如果有）
ls -lh /var/crash/

# 查看节点上的 Pod 状态
kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name>

# 查看 Pod 重启情况
kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name> -o custom-columns=NAME:.metadata.name,RESTARTS:.status.containerStatuses[*].restartCount

# 检查卷挂载状态
mount | grep kubelet
kubectl get volumeattachments | grep <node-name>
```

#### 解决建议

| 重启原因 | 解决方案 |
|:---|:---|
| 计划内维护 | 正常情况，确认所有 Pod 已恢复，检查服务健康 |
| 系统更新/补丁 | 验证更新成功，监控节点稳定性 |
| 内核 panic | 分析 panic 日志，可能是内核 bug 或硬件问题 |
| OOM killer | 调整系统内存配置，优化应用内存使用 |
| 断电/硬件故障 | 检查硬件状态，修复或更换故障硬件 |
| 人为误操作 | 审计操作记录，加强权限管理和操作规范 |
| 自动重启（看门狗） | 检查触发自动重启的条件，解决根本问题 |
| 卷挂载失败 | 检查存储系统，修复卷挂载问题 |
| Pod 无法启动 | 查看 Pod 事件和日志，解决启动问题 |

---

### `NodeAllocatableEnforced` - 节点可分配资源限制已更新

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | kubelet |
| **关联资源** | Node |
| **适用版本** | v1.6+ |
| **生产频率** | 低频 (kubelet 启动或配置更改时) |

#### 事件含义

此事件表示 kubelet 已应用或更新节点的 Allocatable 资源限制。Allocatable 是节点上可供 Pod 使用的资源量，计算公式为：

```
Allocatable = Capacity - Reserved(System) - Reserved(Kubernetes) - Eviction Threshold
```

其中：
- **Capacity**：节点的总资源容量（CPU、内存、磁盘等）
- **System Reserved**：为系统进程（sshd, systemd 等）保留的资源
- **Kubernetes Reserved**：为 Kubernetes 组件（kubelet, container runtime 等）保留的资源
- **Eviction Threshold**：驱逐阈值，触发 Pod 驱逐前保留的资源

kubelet 使用 cgroup 强制执行这些限制，确保系统和 Kubernetes 组件不会被 Pod 耗尽资源。

#### 典型事件消息

```yaml
Type:    Normal
Reason:  NodeAllocatableEnforced
Message: Updated Node Allocatable limit across pods
Source:  kubelet, node1.example.com
```

或者更详细的消息：

```yaml
Type:    Normal
Reason:  NodeAllocatableEnforced
Message: Updated limits for pod cgroup: memory=16Gi, cpu=8
Source:  kubelet, node1.example.com
```

#### 影响面说明

- **资源管理**：确保系统和 Kubernetes 组件有足够资源运行
- **调度影响**：scheduler 使用 Allocatable 值进行调度决策
- **Pod 限制**：所有 Pod 的资源使用总和不能超过 Allocatable
- **防止节点不稳定**：通过保留资源防止 OOM 和系统进程被杀

#### 排查建议

```bash
# 查看节点的 Capacity 和 Allocatable
kubectl describe node <node-name> | grep -A 10 "Capacity\|Allocatable"

# 查看详细的资源信息
kubectl get node <node-name> -o json | jq '.status.capacity, .status.allocatable'

# 查看资源预留配置
kubectl get node <node-name> -o json | jq '.metadata.annotations'

# 查看 kubelet 配置
kubectl get --raw /api/v1/nodes/<node-name>/proxy/configz | jq '.kubeletconfig | {systemReserved, kubeReserved, evictionHard}'

# 登录节点查看 cgroup 限制
cat /sys/fs/cgroup/memory/kubepods/memory.limit_in_bytes
cat /sys/fs/cgroup/cpu/kubepods/cpu.cfs_quota_us

# 查看节点资源使用情况
kubectl top node <node-name>

# 查看节点上所有 Pod 的资源 requests 总和
kubectl describe node <node-name> | grep -A 15 "Allocated resources"
```

#### 解决建议

| 场景 | 建议 |
|:---|:---|
| 首次启动 | 正常情况，验证 Allocatable 配置是否符合预期 |
| 配置更新 | 确认配置更改是否符合规划，监控节点和 Pod 的行为 |
| Allocatable 过小 | 调整 kubelet 的 systemReserved 和 kubeReserved 配置 |
| Allocatable 过大 | 增加资源预留，防止系统进程被饿死 |
| cgroup 限制失效 | 检查 cgroup 配置和 kubelet 日志 |
| 资源超额分配 | 调整 Pod 的 requests 和 limits |

**推荐配置示例**：

```yaml
# kubelet 配置文件
systemReserved:
  cpu: "500m"
  memory: "1Gi"
  ephemeral-storage: "10Gi"
kubeReserved:
  cpu: "500m"
  memory: "1Gi"
  ephemeral-storage: "10Gi"
evictionHard:
  memory.available: "500Mi"
  nodefs.available: "10%"
  imagefs.available: "15%"
enforceNodeAllocatable:
  - pods
  - kube-reserved
  - system-reserved
```

---

### `InvalidDiskCapacity` - 磁盘容量无效

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Node |
| **适用版本** | v1.0+ |
| **生产频率** | 罕见 (配置错误时) |

#### 事件含义

此事件表示 kubelet 检测到文件系统的容量为 0 或无效。这通常是由于文件系统挂载失败、磁盘故障或 cadvisor 获取磁盘信息失败导致的。

当磁盘容量无效时，kubelet 无法准确监控磁盘使用情况，可能导致：
1. 无法正确执行磁盘压力检测
2. 镜像垃圾回收（Image GC）无法正常工作
3. 容器垃圾回收（Container GC）无法正常工作
4. 驱逐机制可能失效

#### 典型事件消息

```yaml
Type:    Warning
Reason:  InvalidDiskCapacity
Message: invalid capacity 0 on image filesystem
Source:  kubelet, node1.example.com
```

或者：

```yaml
Type:    Warning
Reason:  InvalidDiskCapacity
Message: failed to get fs info for "imagefs": unable to find data in memory cache
Source:  kubelet, node1.example.com
```

#### 影响面说明

- **监控影响**：无法准确监控磁盘使用情况
- **GC 影响**：垃圾回收机制可能失效
- **驱逐影响**：基于磁盘压力的驱逐可能不工作
- **调度影响**：可能导致 DiskPressure condition 不准确

#### 排查建议

```bash
# 查看节点磁盘相关事件
kubectl get events --field-selector involvedObject.kind=Node,reason=InvalidDiskCapacity

# 查看节点详细信息
kubectl describe node <node-name>

# 登录节点查看文件系统挂载
df -h
mount | grep kubelet
mount | grep containerd

# 检查 kubelet 使用的路径
ls -la /var/lib/kubelet
ls -la /var/lib/containerd

# 查看 kubelet 日志
journalctl -u kubelet -n 200 --no-pager | grep -i "disk\|filesystem\|capacity"

# 检查 cadvisor 是否正常工作
curl http://localhost:4194/api/v1.3/machine

# 查看文件系统状态
stat -f /var/lib/kubelet
stat -f /var/lib/containerd

# 检查磁盘错误
dmesg | grep -i "disk\|error\|fail"
smartctl -a /dev/sda  # 需要安装 smartmontools
```

#### 解决建议

| 原因 | 解决方案 |
|:---|:---|
| 文件系统未挂载 | 挂载文件系统，确保 kubelet 数据目录正常 |
| 磁盘故障 | 修复或更换故障磁盘 |
| cadvisor 故障 | 重启 kubelet 以重启 cadvisor |
| 权限问题 | 检查 kubelet 对文件系统的访问权限 |
| 容器运行时问题 | 检查并修复容器运行时配置 |
| tmpfs 配置错误 | 检查 tmpfs 挂载配置，确保大小正确 |
| 重启 kubelet | `systemctl restart kubelet` |

---

### `FreeDiskSpaceFailed` - 磁盘空间清理失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Node |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 (磁盘压力时) |

#### 事件含义

此事件表示 kubelet 尝试通过垃圾回收（GC）释放磁盘空间，但未能释放足够的空间以满足要求。这通常发生在节点磁盘使用率很高，且镜像 GC 和容器 GC 都无法释放足够空间的情况下。

当磁盘空间清理失败时，kubelet 会：
1. 产生 FreeDiskSpaceFailed 事件
2. 继续处于 DiskPressure 状态
3. 可能开始驱逐 Pod 以释放磁盘空间

这是磁盘压力升级的信号，表明常规的垃圾回收机制已经不够用了。

#### 典型事件消息

```yaml
Type:    Warning
Reason:  FreeDiskSpaceFailed
Message: failed to garbage collect required amount of images. Wanted to free 5242880000 bytes, but freed 524288000 bytes
Source:  kubelet, node1.example.com
```

或者：

```yaml
Type:    Warning
Reason:  FreeDiskSpaceFailed
Message: failed to free disk space: failed to garbage collect required amount of images
Source:  kubelet, node1.example.com
```

#### 影响面说明

- **磁盘状态**：节点继续处于 DiskPressure 状态
- **镜像拉取**：可能无法拉取新镜像
- **容器创建**：可能无法创建新容器
- **Pod 驱逐**：kubelet 可能开始驱逐 Pod
- **服务影响**：可能导致服务中断和 Pod 迁移

#### 排查建议

```bash
# 查看磁盘空间清理失败事件
kubectl get events --field-selector reason=FreeDiskSpaceFailed --all-namespaces

# 查看节点磁盘状态
kubectl describe node <node-name> | grep -A 5 DiskPressure

# 登录节点查看磁盘使用情况
df -h
df -i

# 查看哪些目录占用最多
du -sh /* | sort -rh | head -20
du -sh /var/lib/kubelet/* | sort -rh | head -20
du -sh /var/lib/containerd/* | sort -rh | head -20

# 查看容器镜像占用
crictl images
crictl images -v | awk '{print $3}' | awk '{sum+=$1} END {print "Total:", sum/1024/1024/1024 "GB"}'

# 查看已停止的容器
crictl ps -a | grep Exited | wc -l

# 查看 Pod 日志占用
du -sh /var/log/pods/* | sort -rh | head -20

# 查看 kubelet GC 日志
journalctl -u kubelet | grep -i "garbage collect\|image gc\|container gc"

# 查看 GC 配置
kubectl get --raw /api/v1/nodes/<node-name>/proxy/configz | jq '.kubeletconfig | {imageGCHighThresholdPercent, imageGCLowThresholdPercent, imageMinimumGCAge}'
```

#### 解决建议

| 原因 | 解决方案 |
|:---|:---|
| 所有镜像都在使用 | 手动删除未使用的镜像，或增加磁盘空间 |
| 容器日志过大 | 配置日志轮转，清理旧日志 |
| 大文件占用 | 查找并删除不必要的大文件 |
| GC 阈值过高 | 降低 imageGCHighThresholdPercent，更积极地回收 |
| 磁盘真的满了 | 扩容磁盘，或迁移部分 Pod 到其他节点 |
| 僵尸容器过多 | 手动清理: `crictl rm $(crictl ps -a -q --state Exited)` |
| 手动清理镜像 | `crictl rmi --prune` 或 `crictl rmi <image-id>` |
| 清理 Pod 日志 | 删除旧的 Pod 日志目录 |
| 驱逐 Pod | 手动驱逐占用磁盘多的 Pod |

**手动清理步骤**：

```bash
# 1. 清理未使用的镜像（谨慎操作）
crictl rmi --prune

# 2. 清理已停止的容器
crictl rm $(crictl ps -a -q --state Exited)

# 3. 清理旧的 Pod 日志（谨慎操作，可能影响问题排查）
find /var/log/pods -type d -mtime +7 -exec rm -rf {} \;

# 4. 清理 systemd journal 日志
journalctl --vacuum-time=7d

# 5. 清理 apt/yum 缓存
apt-get clean  # Debian/Ubuntu
yum clean all  # RHEL/CentOS
```

---

### `EvictionThresholdMet` - 驱逐阈值已达到

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Node |
| **适用版本** | v1.4+ |
| **生产频率** | 中频 (资源压力时) |

#### 事件含义

此事件表示节点的资源使用已经达到 kubelet 配置的驱逐阈值（eviction threshold），kubelet 将开始驱逐 Pod 以回收资源。驱逐阈值分为两类：

1. **硬驱逐阈值（Hard Eviction）**：立即驱逐，没有宽限期
   - 例如：`memory.available<100Mi`, `nodefs.available<10%`
   - 达到阈值后，kubelet 立即选择 Pod 进行驱逐

2. **软驱逐阈值（Soft Eviction）**：有宽限期，超时后才驱逐
   - 例如：`memory.available<1Gi,eviction-soft-grace-period.memory.available=1m30s`
   - 达到阈值后等待宽限期，期间如果资源恢复则不驱逐

kubelet 驱逐 Pod 的优先级顺序：
1. **BestEffort** Pods（没有 requests 和 limits）
2. **Burstable** Pods（使用量超过 requests 的）
3. **Burstable** Pods（使用量未超过 requests 的）
4. **Guaranteed** Pods（requests == limits）

#### 典型事件消息

```yaml
Type:    Warning
Reason:  EvictionThresholdMet
Message: Attempting to reclaim memory
Source:  kubelet, node1.example.com
```

```yaml
Type:    Warning
Reason:  EvictionThresholdMet
Message: Attempting to reclaim nodefs
Source:  kubelet, node1.example.com
```

```yaml
Type:    Warning
Reason:  EvictionThresholdMet
Message: Attempting to reclaim ephemeral-storage
Source:  kubelet, node1.example.com
```

#### 影响面说明

- **Pod 驱逐**：符合条件的 Pod 会被终止
- **服务中断**：被驱逐的 Pod 需要在其他节点重建，可能短暂中断服务
- **调度影响**：节点被标记为压力状态，新 Pod 不会调度到此节点
- **级联效应**：驱逐的 Pod 迁移到其他节点，可能导致其他节点也产生压力

#### 排查建议

```bash
# 查看驱逐阈值事件
kubectl get events --field-selector reason=EvictionThresholdMet --all-namespaces --sort-by='.lastTimestamp'

# 查看节点条件
kubectl describe node <node-name> | grep -A 10 Conditions

# 查看被驱逐的 Pod
kubectl get events --field-selector reason=Evicted --all-namespaces

# 查看节点资源使用情况
kubectl top node <node-name>
kubectl top pods --all-namespaces --field-selector spec.nodeName=<node-name> --sort-by=memory

# 查看驱逐阈值配置
kubectl get --raw /api/v1/nodes/<node-name>/proxy/configz | jq '.kubeletconfig | {evictionHard, evictionSoft, evictionSoftGracePeriod, evictionMaxPodGracePeriod}'

# 登录节点查看资源
free -h
df -h
cat /proc/meminfo | grep -i available

# 查看 kubelet 驱逐日志
journalctl -u kubelet | grep -i "evict\|threshold"

# 查看哪些 Pod 被驱逐了
kubectl get pods --all-namespaces -o json | jq '.items[] | select(.status.reason=="Evicted") | {name: .metadata.name, namespace: .metadata.namespace, reason: .status.reason, message: .status.message}'
```

#### 解决建议

| 场景 | 解决方案 |
|:---|:---|
| 内存驱逐 | 优化应用内存使用，调整 Pod memory requests/limits |
| 磁盘驱逐 | 清理磁盘空间，扩容磁盘，限制容器日志大小 |
| PID 驱逐 | 限制容器进程数，修复进程泄漏问题 |
| 频繁驱逐 | 增加节点资源，或调整驱逐阈值 |
| 阈值配置不当 | 根据实际情况调整 evictionHard/evictionSoft 配置 |
| 应用资源配置不当 | 为 Pod 设置合理的 requests 和 limits |
| 集群容量不足 | 增加更多工作节点 |
| 防止特定 Pod 被驱逐 | 使用 Guaranteed QoS，设置 PriorityClass |

**推荐驱逐阈值配置**：

```yaml
# kubelet 配置
evictionHard:
  memory.available: "500Mi"
  nodefs.available: "10%"
  imagefs.available: "15%"
  nodefs.inodesFree: "5%"
  pid.available: "5%"

evictionSoft:
  memory.available: "1Gi"
  nodefs.available: "15%"
  imagefs.available: "20%"

evictionSoftGracePeriod:
  memory.available: "1m30s"
  nodefs.available: "2m"
  imagefs.available: "2m"

evictionMaxPodGracePeriod: 60
```

---

### `ContainerGCFailed` - 容器垃圾回收失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Node |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 (GC 失败时) |

#### 事件含义

此事件表示 kubelet 尝试清理已停止的容器（Container Garbage Collection）时失败。Container GC 负责删除已经退出的容器，释放磁盘空间。

kubelet 的 Container GC 策略：
- **MinAge**：容器至少存在多久后才能被回收（默认 0，立即回收）
- **MaxPerPodContainer**：每个 Pod 保留的死亡容器数（默认 1）
- **MaxContainers**：节点上保留的死亡容器总数（默认 -1，无限制）

GC 失败可能由以下原因导致：
1. 容器运行时（containerd/docker）API 调用失败
2. 容器卷无法卸载
3. 容器文件系统无法删除
4. 权限问题

#### 典型事件消息

```yaml
Type:    Warning
Reason:  ContainerGCFailed
Message: rpc error: code = Unknown desc = failed to remove container "abc123": device or resource busy
Source:  kubelet, node1.example.com
```

```yaml
Type:    Warning
Reason:  ContainerGCFailed
Message: failed to garbage collect containers: rpc error accessing container runtime
Source:  kubelet, node1.example.com
```

#### 影响面说明

- **磁盘空间**：无法通过 Container GC 释放磁盘空间
- **容器数量**：死亡容器堆积，占用资源
- **性能影响**：过多容器可能影响容器运行时性能
- **磁盘压力**：可能加剧 DiskPressure 状态

#### 排查建议

```bash
# 查看 Container GC 失败事件
kubectl get events --field-selector reason=ContainerGCFailed --all-namespaces

# 查看节点事件详情
kubectl describe node <node-name>

# 登录节点查看已停止的容器
crictl ps -a | grep Exited | wc -l
crictl ps -a | grep Exited | head -20

# 查看容器运行时状态
systemctl status containerd
crictl info

# 尝试手动删除容器
crictl rm <container-id>

# 查看容器文件系统
ls -la /run/containerd/io.containerd.runtime.v2.task/k8s.io/
ls -la /var/lib/containerd/io.containerd.snapshotter.v1.overlayfs/snapshots/

# 查看挂载点
mount | grep kubelet
mount | grep overlay

# 查看 kubelet 日志
journalctl -u kubelet | grep -i "container gc\|garbage collect"

# 查看容器运行时日志
journalctl -u containerd | grep -i "remove\|delete\|error"

# 检查磁盘 I/O
iostat -x 1 5
```

#### 解决建议

| 原因 | 解决方案 |
|:---|:---|
| 容器运行时故障 | 重启容器运行时: `systemctl restart containerd` |
| 卷卸载失败 | 手动卸载: `umount <mount-point>` |
| 文件系统繁忙 | 查找占用进程: `lsof <file>`, `fuser -m <mount-point>` |
| 权限问题 | 检查 kubelet 和容器运行时的权限 |
| overlayfs 问题 | 手动清理 overlay 层 |
| 手动清理容器 | `crictl rm $(crictl ps -a -q --state Exited)` |
| 重启 kubelet | `systemctl restart kubelet` |
| 文件系统损坏 | 运行 `fsck` 修复文件系统（需要先 drain 节点） |

---

### `ImageGCFailed` - 镜像垃圾回收失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Node |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 (GC 失败时) |

#### 事件含义

此事件表示 kubelet 尝试清理未使用的容器镜像（Image Garbage Collection）时失败。Image GC 负责删除不再使用的镜像，释放磁盘空间。

kubelet 的 Image GC 策略：
- **HighThresholdPercent**：磁盘使用率达到此值时触发 GC（默认 85%）
- **LowThresholdPercent**：GC 会删除镜像直到磁盘使用率降到此值（默认 80%）
- **MinAge**：镜像至少存在多久后才能被回收（默认 2 分钟）

Image GC 会根据镜像的最后使用时间（LRU）来决定删除顺序，正在使用的镜像不会被删除。

GC 失败可能由以下原因导致：
1. 容器运行时 API 调用失败
2. 所有镜像都在使用中
3. 镜像文件系统损坏
4. 权限问题

#### 典型事件消息

```yaml
Type:    Warning
Reason:  ImageGCFailed
Message: failed to garbage collect required amount of images. Attempted to free 10737418240 bytes, but only freed 1073741824 bytes
Source:  kubelet, node1.example.com
```

```yaml
Type:    Warning
Reason:  ImageGCFailed
Message: failed to get ImageFs info: unable to find data for container /
Source:  kubelet, node1.example.com
```

```yaml
Type:    Warning
Reason:  ImageGCFailed
Message: rpc error: code = Unknown desc = failed to remove image: image is being used by containers
Source:  kubelet, node1.example.com
```

#### 影响面说明

- **磁盘空间**：无法通过 Image GC 释放磁盘空间
- **镜像堆积**：未使用的镜像堆积，占用磁盘
- **磁盘压力**：可能导致或加剧 DiskPressure 状态
- **镜像拉取**：可能无法拉取新镜像

#### 排查建议

```bash
# 查看 Image GC 失败事件
kubectl get events --field-selector reason=ImageGCFailed --all-namespaces

# 查看节点事件详情
kubectl describe node <node-name>

# 登录节点查看镜像占用
crictl images
crictl images -v

# 查看镜像占用空间总和
crictl images -v | awk 'NR>1 {sum+=$5} END {print "Total:", sum/1024/1024/1024 "GB"}'

# 查看哪些镜像占用最多
crictl images -v | sort -k5 -rh | head -20

# 查看镜像使用情况
for img in $(crictl images -q); do
  echo "Image: $img"
  crictl ps -a | grep $img | wc -l
done

# 查看磁盘使用情况
df -h
du -sh /var/lib/containerd/io.containerd.content.v1.content

# 查看 kubelet GC 配置
kubectl get --raw /api/v1/nodes/<node-name>/proxy/configz | jq '.kubeletconfig | {imageGCHighThresholdPercent, imageGCLowThresholdPercent, imageMinimumGCAge}'

# 查看 kubelet 日志
journalctl -u kubelet | grep -i "image gc\|garbage collect"

# 查看容器运行时日志
journalctl -u containerd | grep -i "image\|remove\|delete"

# 尝试手动删除未使用的镜像
crictl rmi --prune
```

#### 解决建议

| 原因 | 解决方案 |
|:---|:---|
| 所有镜像都在使用 | 删除不需要的 Pod，释放镜像引用 |
| 容器运行时故障 | 重启容器运行时: `systemctl restart containerd` |
| GC 阈值过高 | 降低 imageGCHighThresholdPercent，更早触发 GC |
| 镜像真的太多 | 手动删除未使用的镜像: `crictl rmi <image-id>` |
| 磁盘真的满了 | 扩容磁盘，或为 imagefs 单独分配磁盘 |
| 镜像拉取策略问题 | 调整 Pod 的 imagePullPolicy，避免堆积多个版本 |
| 手动清理 | `crictl rmi --prune` 删除所有未使用的镜像 |
| 重启 kubelet | `systemctl restart kubelet` |

**手动清理步骤**：

```bash
# 1. 查看镜像列表
crictl images

# 2. 删除未使用的镜像（谨慎操作）
crictl rmi --prune

# 3. 手动删除特定镜像
crictl rmi <image-id>

# 4. 查看清理效果
df -h
crictl images
```

---

## Node Controller 事件

### `RegisteredNode` - 节点已注册

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | node-controller |
| **关联资源** | Node |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 (新节点加入时) |

#### 事件含义

此事件表示新节点已成功注册到 Kubernetes 集群的控制平面。当 kubelet 首次启动时，它会向 API Server 注册节点对象，node-controller 检测到新节点后会产生此事件。

节点注册是节点加入集群的第一步，之后 node-controller 会持续监控节点的健康状态。

#### 典型事件消息

```yaml
Type:    Normal
Reason:  RegisteredNode
Message: Node node1.example.com event: Registered Node node1.example.com in Controller
Source:  node-controller
```

或者：

```yaml
Type:    Normal
Reason:  RegisteredNode
Message: Registered node node1.example.com
Source:  controllermanager
```

#### 影响面说明

- **集群影响**：集群增加一个新的工作节点
- **容量影响**：集群总容量增加
- **监控影响**：node-controller 开始监控此节点

#### 排查建议

```bash
# 查看节点注册事件
kubectl get events --field-selector reason=RegisteredNode --all-namespaces

# 查看节点信息
kubectl get nodes
kubectl describe node <node-name>

# 查看节点注册时间
kubectl get node <node-name> -o jsonpath='{.metadata.creationTimestamp}'

# 查看节点标签和注解
kubectl get node <node-name> -o json | jq '.metadata.labels, .metadata.annotations'

# 查看 node-controller 日志（需要访问控制平面）
kubectl logs -n kube-system <controller-manager-pod> | grep -i "register\|node"
```

#### 解决建议

| 场景 | 建议 |
|:---|:---|
| 新节点加入 | 正常情况，验证节点配置和资源 |
| 意外注册 | 检查是否有未授权的节点加入集群 |
| 重复注册 | 检查 kubelet 配置，可能是节点名称冲突 |

---

### `RemovingNode` - 从控制器移除节点

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | node-controller |
| **关联资源** | Node |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 (节点删除时) |

#### 事件含义

此事件表示 node-controller 正在将节点从其管理列表中移除。这通常发生在执行 `kubectl delete node` 命令后，或者节点对象被 API Server 删除时。

移除节点后，node-controller 将不再监控此节点的健康状态。

#### 典型事件消息

```yaml
Type:    Normal
Reason:  RemovingNode
Message: Removing Node node1.example.com from Controller
Source:  node-controller
```

#### 影响面说明

- **集群影响**：集群容量减少
- **监控影响**：node-controller 停止监控此节点
- **Pod 影响**：节点上的 Pod 已经或将要被删除

#### 排查建议

```bash
# 查看节点删除事件
kubectl get events --field-selector reason=RemovingNode --all-namespaces

# 查看节点列表
kubectl get nodes

# 查看 node-controller 日志
kubectl logs -n kube-system <controller-manager-pod> | grep -i "remove\|delete"
```

#### 解决建议

| 场景 | 建议 |
|:---|:---|
| 计划内下线 | 正常操作，确认节点已 drain |
| 意外删除 | 检查操作审计日志，找出删除来源 |
| 节点故障后清理 | 正常情况，清理失败节点 |

---

### `DeletingNode` - 删除节点

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | node-controller |
| **关联资源** | Node |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 (节点删除时) |

#### 事件含义

此事件表示 node-controller 正在删除节点对象。这是节点生命周期的终点。

#### 典型事件消息

```yaml
Type:    Normal
Reason:  DeletingNode
Message: Deleting Node node1.example.com
Source:  node-controller
```

#### 影响面说明

- **节点对象**：节点对象将从 API Server 删除
- **Pod 影响**：节点上的 Pod 应该已经被删除
- **资源影响**：节点的资源不再计入集群容量

#### 排查建议

```bash
# 查看节点删除事件
kubectl get events --field-selector reason=DeletingNode --all-namespaces

# 检查节点是否还存在
kubectl get nodes | grep <node-name>

# 查看所有节点
kubectl get nodes
```

#### 解决建议

| 场景 | 建议 |
|:---|:---|
| 正常下线 | 无需处理，节点已成功移除 |
| 确认 Pod 迁移 | 检查应用是否已在其他节点运行 |
| 更新监控和告警 | 移除对此节点的监控配置 |

---

### `DeletingAllPods` - 删除节点上的所有 Pod

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | node-controller |
| **关联资源** | Node |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 (节点失败时) |

#### 事件含义

此事件表示 node-controller 检测到节点持续 NotReady 超过 pod-eviction-timeout（默认 5 分钟），开始强制删除节点上的所有 Pod。这是节点失败后的自动恢复机制。

当节点长时间 NotReady 时，node-controller 会：
1. 产生 DeletingAllPods 事件
2. 删除节点上所有 Pod 的 API 对象
3. 触发 Pod 在其他节点重建（如果是 Deployment, StatefulSet 等管理的）

注意：这只是删除 API 对象，实际的容器进程可能仍在故障节点上运行（如果节点只是网络隔离而非真正宕机）。

#### 典型事件消息

```yaml
Type:    Normal
Reason:  DeletingAllPods
Message: Deleting all Pods from Node node1.example.com
Source:  node-controller
```

```yaml
Type:    Normal
Reason:  DeletingAllPods
Message: Node node1.example.com event: Deleting all Pods because of NodeNotReady condition
Source:  node-controller
```

#### 影响面说明

- **Pod 生命周期**：节点上所有 Pod 的 API 对象被删除
- **服务影响**：
  - Deployment/ReplicaSet：Pod 会在其他节点重建
  - StatefulSet：Pod 会按顺序在其他节点重建
  - DaemonSet：Pod 不会重建（因为是按节点调度的）
  - 裸 Pod（无控制器）：Pod 丢失，不会重建
- **数据影响**：
  - EmptyDir 卷的数据丢失
  - hostPath 卷的数据保留在故障节点
  - PV 卷需要重新挂载（可能受 VolumeAttachment 限制）
- **恢复时间**：取决于 Pod 重新调度和启动的时间

#### 排查建议

```bash
# 查看 DeletingAllPods 事件
kubectl get events --field-selector reason=DeletingAllPods --all-namespaces

# 查看节点状态
kubectl get nodes
kubectl describe node <node-name>

# 查看节点上的 Pod（可能已经不存在）
kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name>

# 查看 Pod 删除和重建事件
kubectl get events --all-namespaces --sort-by='.lastTimestamp' | grep <node-name>

# 查看 Pod 在新节点上的状态
kubectl get pods --all-namespaces -o wide | grep -v <node-name>

# 查看 node-controller 配置
kubectl describe cm kube-controller-manager -n kube-system | grep pod-eviction-timeout

# 查看 node-controller 日志
kubectl logs -n kube-system <controller-manager-pod> | grep -i "deleting.*pod\|evict"

# 检查存储卷的挂载状态
kubectl get volumeattachments | grep <node-name>

# 查看受影响的服务
kubectl get svc --all-namespaces
kubectl get endpoints --all-namespaces
```

#### 解决建议

| 场景 | 解决方案 |
|:---|:---|
| 节点真的宕机 | 确认 Pod 已在其他节点重建，修复或更换故障节点 |
| 网络隔离 | 修复网络问题，可能需要手动清理故障节点上的容器 |
| 节点维护 | 如果是计划内维护，应该先 drain 而不是等待自动驱逐 |
| Pod 未重建 | 检查控制器状态（Deployment, StatefulSet 等） |
| 存储卷无法挂载 | 手动释放 VolumeAttachment: `kubectl delete volumeattachment <name>` |
| 裸 Pod 丢失 | 使用控制器管理 Pod，避免使用裸 Pod |
| 数据丢失 | 使用 PV 持久化数据，不要依赖 EmptyDir |
| 服务中断时间过长 | 调整 pod-eviction-timeout（需权衡利弊）|
| 防止数据损坏 | 对于有状态应用，考虑使用 fencing 机制 |

**pod-eviction-timeout 配置建议**：

```yaml
# kube-controller-manager 配置
# 默认值：5m（5分钟）
# 较短的超时：快速恢复，但可能误判
# 较长的超时：减少误判，但恢复慢
--pod-eviction-timeout=5m

# 对于网络不稳定的环境，可以适当延长：
--pod-eviction-timeout=10m
```

---

### `TerminatingEvictedPod` - 终止被驱逐的 Pod

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | node-controller |
| **关联资源** | Node |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 (节点失败时) |

#### 事件含义

此事件表示 node-controller 正在标记特定的 Pod 为删除状态。这通常发生在节点持续 NotReady，node-controller 开始驱逐节点上的 Pod 时。

每个被驱逐的 Pod 都会产生一个 TerminatingEvictedPod 事件。

#### 典型事件消息

```yaml
Type:    Normal
Reason:  TerminatingEvictedPod
Message: Pod my-app-xxx from Namespace default has been marked for deletion. Pod will be deleted if it is not being updated or node will not be ready before Sun, 10 Feb 2026 11:00:00 +0800
Source:  node-controller
```

或者：

```yaml
Type:    Normal
Reason:  TerminatingEvictedPod
Message: Marking for deletion Pod my-app-xxx
Source:  node-controller
```

#### 影响面说明

- **Pod 生命周期**：Pod 被标记为 Terminating 状态
- **服务影响**：Pod 对应的 Endpoint 被移除，流量不再路由到此 Pod
- **重建**：如果 Pod 有控制器（Deployment 等），会在其他节点重建

#### 排查建议

```bash
# 查看 TerminatingEvictedPod 事件
kubectl get events --field-selector reason=TerminatingEvictedPod --all-namespaces

# 查看处于 Terminating 状态的 Pod
kubectl get pods --all-namespaces | grep Terminating

# 查看特定 Pod 的事件
kubectl describe pod <pod-name> -n <namespace>

# 查看 Pod 的状态和原因
kubectl get pod <pod-name> -n <namespace> -o json | jq '.status.reason, .status.message'

# 查看 Pod 所在节点
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.nodeName}'

# 查看节点状态
kubectl describe node <node-name>
```

#### 解决建议

| 场景 | 解决方案 |
|:---|:---|
| 节点 NotReady | 修复节点，或确认 Pod 已在其他节点重建 |
| Pod 长时间 Terminating | 检查节点连通性，可能需要强制删除: `kubectl delete pod <pod> --grace-period=0 --force` |
| 频繁驱逐 | 调查节点不稳定的根本原因，修复基础设施问题 |
| 服务影响 | 确保有足够的副本数，避免单点故障 |

---

## 节点驱逐机制

### kubelet 驱逐（Node-pressure Eviction）

kubelet 会主动监控节点资源，当资源不足时驱逐 Pod。

#### 驱逐信号（Eviction Signals）

| 信号 | 含义 | 描述 |
|:---|:---|:---|
| `memory.available` | 可用内存 | `memory.available := node.status.capacity[memory] - workingSet` |
| `nodefs.available` | 节点根文件系统可用空间 | `nodefs.available := node.stats.fs.available` |
| `nodefs.inodesFree` | 节点根文件系统可用 inode | `nodefs.inodesFree := node.stats.fs.inodesFree` |
| `imagefs.available` | 镜像文件系统可用空间 | `imagefs.available := node.stats.runtime.imagefs.available` |
| `imagefs.inodesFree` | 镜像文件系统可用 inode | `imagefs.inodesFree := node.stats.runtime.imagefs.inodesFree` |
| `pid.available` | 可用进程 ID | `pid.available := node.stats.rlimit.maxpid - node.stats.rlimit.curproc` |

#### 驱逐阈值类型

**硬驱逐阈值（Hard Eviction Thresholds）**：
- 达到阈值立即驱逐，没有宽限期
- 不遵守 Pod 的 terminationGracePeriodSeconds
- 默认配置：
  ```yaml
  evictionHard:
    memory.available: "100Mi"
    nodefs.available: "10%"
    nodefs.inodesFree: "5%"
    imagefs.available: "15%"
  ```

**软驱逐阈值（Soft Eviction Thresholds）**：
- 达到阈值后等待宽限期，期间资源恢复则不驱逐
- 遵守 Pod 的 terminationGracePeriodSeconds（但不超过 evictionMaxPodGracePeriod）
- 默认配置：
  ```yaml
  evictionSoft:
    memory.available: "1.5Gi"
    nodefs.available: "15%"
  evictionSoftGracePeriod:
    memory.available: "1m30s"
    nodefs.available: "2m"
  evictionMaxPodGracePeriod: 90
  ```

#### 驱逐策略

**1. 驱逐顺序（按优先级）**：

1. **BestEffort or Burstable** Pods 且使用量超过 requests
2. **Burstable** Pods 且使用量低于 requests
3. **Guaranteed** Pods 和 **Burstable** Pods 且使用量等于 requests

在同一优先级内，按以下顺序驱逐：
1. Pod Priority 较低的（PriorityClass）
2. 资源使用量超过 requests 更多的
3. Pod Priority 和资源使用相同时，随机选择

**2. 驱逐行为**：

- **内存压力**：kubelet 不等待宽限期，立即杀掉 Pod（类似 OOM killer）
- **磁盘压力**：kubelet 先执行 Image GC 和 Container GC，失败后才驱逐 Pod
- **PID 压力**：kubelet 驱逐 Pod 以释放 PID

**3. 最小回收量（Minimum Eviction Reclaim）**：

kubelet 可以配置最小回收量，确保驱逐后资源充足：

```yaml
evictionMinimumReclaim:
  memory.available: "500Mi"
  nodefs.available: "1Gi"
  imagefs.available: "2Gi"
```

### node-controller 驱逐（Taint-based Eviction）

node-controller 通过 taint 机制驱逐 Pod。

#### Taint 类型

当节点出现问题时，node-controller 会自动添加 taint：

| Taint Key | Effect | 条件 | 含义 |
|:---|:---|:---|:---|
| `node.kubernetes.io/not-ready` | NoExecute | Ready=False | 节点未就绪 |
| `node.kubernetes.io/unreachable` | NoExecute | Ready=Unknown | 节点不可达 |
| `node.kubernetes.io/memory-pressure` | NoSchedule | MemoryPressure=True | 内存压力 |
| `node.kubernetes.io/disk-pressure` | NoSchedule | DiskPressure=True | 磁盘压力 |
| `node.kubernetes.io/pid-pressure` | NoSchedule | PIDPressure=True | PID 压力 |
| `node.kubernetes.io/network-unavailable` | NoSchedule | NetworkUnavailable=True | 网络不可用 |
| `node.kubernetes.io/unschedulable` | NoSchedule | spec.unschedulable=true | 节点不可调度 |

#### 容忍时间（Toleration Seconds）

Pod 可以通过 toleration 设置对 taint 的容忍时间：

```yaml
tolerations:
- key: "node.kubernetes.io/not-ready"
  operator: "Exists"
  effect: "NoExecute"
  tolerationSeconds: 300  # 5分钟
- key: "node.kubernetes.io/unreachable"
  operator: "Exists"
  effect: "NoExecute"
  tolerationSeconds: 300  # 5分钟
```

默认情况下，Kubernetes 会自动为所有 Pod 添加以下 toleration：
- `node.kubernetes.io/not-ready:NoExecute` for 300s
- `node.kubernetes.io/unreachable:NoExecute` for 300s

DaemonSet 的 Pod 会有特殊的 toleration，不会因为节点问题被驱逐。

---

## 生产环境监控建议

### 监控指标

**节点级别指标**：

| 指标 | 含义 | 告警阈值建议 |
|:---|:---|:---|
| `kube_node_status_condition{condition="Ready",status="true"}` | 节点 Ready 状态 | == 0 持续 1 分钟 |
| `kube_node_status_condition{condition="MemoryPressure",status="true"}` | 节点内存压力 | == 1 |
| `kube_node_status_condition{condition="DiskPressure",status="true"}` | 节点磁盘压力 | == 1 |
| `kube_node_status_condition{condition="PIDPressure",status="true"}` | 节点 PID 压力 | == 1 |
| `kube_node_spec_unschedulable` | 节点不可调度 | == 1 持续 10 分钟 |
| `node_memory_MemAvailable_bytes` | 节点可用内存 | < 500Mi |
| `node_filesystem_avail_bytes{mountpoint="/"}` | 节点磁盘可用空间 | < 10% |
| `node_filesystem_files_free{mountpoint="/"}` | 节点可用 inode | < 10% |

**事件级别监控**：

```promql
# 节点 NotReady 事件
kube_event_count{reason="NodeNotReady"} > 0

# 驱逐阈值事件
kube_event_count{reason="EvictionThresholdMet"} > 0

# Pod 驱逐事件
kube_event_count{reason="Evicted"} > 0

# 节点重启事件
kube_event_count{reason="Rebooted"} > 0

# GC 失败事件
kube_event_count{reason=~"ImageGCFailed|ContainerGCFailed"} > 0
```

### 告警规则

**Prometheus AlertManager 规则示例**：

```yaml
groups:
- name: node-health
  interval: 30s
  rules:
  # 节点 NotReady
  - alert: NodeNotReady
    expr: kube_node_status_condition{condition="Ready",status="true"} == 0
    for: 1m
    labels:
      severity: critical
    annotations:
      summary: "Node {{ $labels.node }} is not ready"
      description: "Node {{ $labels.node }} has been NotReady for more than 1 minute."

  # 节点内存压力
  - alert: NodeMemoryPressure
    expr: kube_node_status_condition{condition="MemoryPressure",status="true"} == 1
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Node {{ $labels.node }} has memory pressure"
      description: "Node {{ $labels.node }} has been under memory pressure for more than 5 minutes."

  # 节点磁盘压力
  - alert: NodeDiskPressure
    expr: kube_node_status_condition{condition="DiskPressure",status="true"} == 1
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Node {{ $labels.node }} has disk pressure"
      description: "Node {{ $labels.node }} has been under disk pressure for more than 5 minutes."

  # 节点 PID 压力
  - alert: NodePIDPressure
    expr: kube_node_status_condition{condition="PIDPressure",status="true"} == 1
    for: 2m
    labels:
      severity: warning
    annotations:
      summary: "Node {{ $labels.node }} has PID pressure"
      description: "Node {{ $labels.node }} has been under PID pressure for more than 2 minutes."

  # 节点可用内存低
  - alert: NodeLowMemory
    expr: node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes * 100 < 10
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Node {{ $labels.node }} has low available memory"
      description: "Node {{ $labels.node }} has less than 10% memory available for more than 5 minutes."

  # 节点磁盘空间低
  - alert: NodeLowDiskSpace
    expr: node_filesystem_avail_bytes{mountpoint="/"} / node_filesystem_size_bytes{mountpoint="/"} * 100 < 15
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Node {{ $labels.node }} has low disk space"
      description: "Node {{ $labels.node }} has less than 15% disk space available for more than 10 minutes."

  # 节点驱逐 Pod
  - alert: NodeEvictingPods
    expr: rate(kube_event_count{reason="Evicted"}[5m]) > 0
    for: 2m
    labels:
      severity: warning
    annotations:
      summary: "Node is evicting pods due to resource pressure"
      description: "Eviction events detected in the cluster."

  # 节点 GC 失败
  - alert: NodeGCFailed
    expr: rate(kube_event_count{reason=~"ImageGCFailed|ContainerGCFailed"}[10m]) > 0
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Node {{ $labels.node }} GC failed"
      description: "Node {{ $labels.node }} has failed to garbage collect resources."
```

### 仪表板（Dashboard）

**Grafana 仪表板关键面板**：

1. **集群节点概览**
   - 总节点数
   - Ready 节点数
   - NotReady 节点数
   - 不可调度节点数

2. **节点状态时间线**
   - 节点 Ready 状态历史
   - 节点 Condition 变化历史

3. **节点资源压力**
   - MemoryPressure 节点列表
   - DiskPressure 节点列表
   - PIDPressure 节点列表

4. **节点资源使用**
   - CPU 使用率（按节点）
   - 内存使用率（按节点）
   - 磁盘使用率（按节点）
   - 网络流量（按节点）

5. **节点事件统计**
   - 最近 1 小时事件统计（按 Reason）
   - NotReady 事件时间线
   - 驱逐事件时间线
   - GC 失败事件时间线

6. **节点容量和分配**
   - 节点 Capacity vs Allocatable
   - 节点资源请求 vs 使用
   - 节点 Pod 数量

### 最佳实践

1. **资源预留配置**
   ```yaml
   # 为系统进程预留资源
   systemReserved:
     cpu: "500m"
     memory: "1Gi"
     ephemeral-storage: "10Gi"
   
   # 为 Kubernetes 组件预留资源
   kubeReserved:
     cpu: "500m"
     memory: "1Gi"
     ephemeral-storage: "10Gi"
   ```

2. **驱逐阈值配置**
   ```yaml
   # 硬驱逐阈值（立即驱逐）
   evictionHard:
     memory.available: "500Mi"
     nodefs.available: "10%"
     nodefs.inodesFree: "5%"
     imagefs.available: "15%"
     pid.available: "5%"
   
   # 软驱逐阈值（有宽限期）
   evictionSoft:
     memory.available: "1Gi"
     nodefs.available: "15%"
     imagefs.available: "20%"
   
   evictionSoftGracePeriod:
     memory.available: "1m30s"
     nodefs.available: "2m"
     imagefs.available: "2m"
   ```

3. **镜像垃圾回收配置**
   ```yaml
   # 磁盘使用率达到 85% 时触发 Image GC
   imageGCHighThresholdPercent: 85
   
   # Image GC 会删除镜像直到磁盘使用率降到 80%
   imageGCLowThresholdPercent: 80
   
   # 镜像至少存在 2 分钟后才能被回收
   imageMinimumGCAge: 2m
   ```

4. **容器日志轮转**
   ```yaml
   # 单个容器日志文件最大大小
   containerLogMaxSize: 10Mi
   
   # 每个容器保留的日志文件数量
   containerLogMaxFiles: 5
   ```

5. **Pod 资源配置最佳实践**
   - 为所有 Pod 设置 requests 和 limits
   - 使用 QoS Guaranteed 保护关键应用
   - 为不重要的应用使用 QoS BestEffort
   - 设置合理的 PriorityClass

6. **节点维护流程**
   ```bash
   # 1. 标记节点不可调度
   kubectl cordon <node-name>
   
   # 2. 驱逐节点上的 Pod
   kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data
   
   # 3. 执行维护操作
   
   # 4. 恢复节点可调度
   kubectl uncordon <node-name>
   ```

7. **自动化响应**
   - 设置自动告警通知（PagerDuty, Slack, Email）
   - 对于非关键节点，配置自动修复（自动重启、自动替换）
   - 配置 Cluster Autoscaler 自动扩缩容

8. **日志收集和分析**
   - 收集 kubelet 日志到中心化日志系统
   - 收集节点系统日志（journalctl, dmesg）
   - 收集节点事件到日志系统
   - 定期分析日志，发现潜在问题

---

> **KUDIG-DATABASE** | Domain-33: Kubernetes Events 全域事件大全 | 文档 06/15
