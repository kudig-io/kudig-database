---
title: 节点状态与健康检查 — Node Conditions 源码分析
description: 'description: ''## 概述'''
summary: 'description: ''## 概述'''
category: general
tags:
- reference
- kubelet
- scheduler
- controller-manager
- cilium
- flannel
- calico
- containerd
- daemonset
- gpu
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 节点状态与健康检查 — Node Conditions 源码分析 是什么
- 如何 节点状态与健康检查 — Node Conditions 源码分析
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 节点状态与健康检查
- Node
- Conditions
- 源码分析
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- cilium-basics
- cni-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 节点状态与健康检查 Node Conditions 源码分析
description: '## 概述'
category: functions
tags:
- k8s
- operations
- cluster-management
- kubelet
- scheduler
- controller-manager
- cilium
- flannel
- calico
- containerd
last_updated: 2026-05-18
difficulty: advanced
reading_level: advanced
audience:
- Kubernetes 运维工程师
- 平台工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes node conditions Ready MemoryPressure DiskPressure
- node condition status explanation
- kubelet node status reporting
- Node Lifecycle Controller nodeMonitorGracePeriod
- node condition impact scheduling
trigger_keywords:
- condition
- Ready
- MemoryPressure
- DiskPressure
- PIDPressure
- NetworkUnavailable
- NodeCondition
- syncNodeStatus
- nodeMonitorGracePeriod
- podEvictionTimeout
- kubelet
- PLEG
- scheduling
related_domains:
- 集群基础
- domain-9-orchestration
related_topics:
- node-create/01-overview
- node-create/02-registration
- node-create/08-troubleshooting
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 节点状态与健康检查 — Node Conditions 源码分析

## 概述

节点状态（Node Conditions）是 Kubernetes 调度器和管理控制器判断节点健康状态的核心机制。每个节点维护一组 Conditions，包括 Ready、MemoryPressure、DiskPressure、PIDPressure 和 NetworkUnavailable，它们分别反映了节点在不同维度的健康状况。

kubelet 通过定期的状态上报（Status Update）将节点的 Conditions 同步到 API Server。调度器在为 Pod 选择节点时，会检查这些 Conditions 来决定是否将 Pod 调度到该节点。Node Lifecycle Controller 盾牌持续监控节点 Conditions，当节点长时间 NotReady 时触发 Pod 驱逐。

理解 Node Conditions 的工作原理对于以下场景至关重要：

- **调度决策**：理解为什么 Pod 没有被调度到某个节点
- **故障排查**：快速定位节点不健康的根本原因
- **容量管理**：通过资源状态监控实现主动扩容
- **驱逐策略**：理解 Pod 驱逐与节点状态的关系

本文档详细分析每种 Condition 的含义、触发机制、对调度的影响以及源码实现。

---

## 源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| 节点状态上报 | `pkg/kubelet/nodestatus/` | Conditions 更新 |
| 驱逐管理 | `pkg/kubelet/eviction/` | 资源压力检测 |
| PLEG | `pkg/kubelet/pleg/` | Pod 生命周期事件 |
| 卷管理 | `pkg/kubelet/volumemanager/` | 卷状态管理 |
| 调度器 | `pkg/scheduler/` | 节点选择 |
| Node Lifecycle Controller | `pkg/controller/nodelifecycle/` | 节点健康监控 |

---

## 一、节点状态（Conditions）

### 1.1 Condition 结构

```go
// k8s.io/api/core/v1/types.go
type NodeCondition struct {
    Type               NodeConditionType    // Condition 类型
    Status             ConditionStatus      // True / False / Unknown
    LastHeartbeatTime  metav1.Time          // 最后一次上报时间
    LastTransitionTime metav1.Time          // 最后一次状态变化时间
    Reason             string               // 状态变化原因
    Message            string               // 详细描述
}
```

### 1.2 查看节点 Conditions

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有节点的 Conditions
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{range .status.conditions[*]}{.type}={.status}{" "}{end}{"\n"}{end}'

# 查看单个节点的 Conditions 详情
kubectl describe node <node-name> | grep -A 10 "Conditions"

# JSON 格式查看
kubectl get node <node> -o jsonpath='{.status.conditions}' | jq .
```
### 1.3 Conditions 类型总览

| Condition | 说明 | 正常值 | 异常影响 |
|-----------|------|--------|---------|
| `Ready` | 节点是否就绪 | `True` | False: 不调度新 Pod; Unknown: 触发驱逐 |
| `MemoryPressure` | 内存是否不足 | `False` | True: 仅调度 Guaranteed Pod |
| `DiskPressure` | 磁盘是否不足 | `False` | True: 仅调度 Guaranteed Pod |
| `PIDPressure` | PID 是否不足 | `False` | True: 仅调度 Guaranteed Pod |
| `NetworkUnavailable` | 网络是否不可用 | `False` | True: 不调度新 Pod |

---

## 二、Ready 状态

### 2.1 Ready 状态详解

```bash
# Ready = True: 节点健康，可以调度 Pod
# Ready = False: 节点异常（kubelet 报告），不调度新 Pod
# Ready = Unknown: kubelet 超时未上报，控制器可能驱逐 Pod
```

### 2.2 Ready 状态判定源码

```go
// pkg/kubelet/nodestatus/setters.go
func NodeReadyCondition(now metav1.Time, ...) []v1.NodeCondition {
    // Ready 状态判定逻辑:
    // 1. 检查容器运行时是否健康 (runtime OK)
    // 2. 检查 PLEG 是否正常 (PLEG OK)
    // 3. 检查网络是否配置完成
    // 4. 检查 volumes 是否挂载正常
    // 5. 所有检查通过 → Ready=True
    //    任一检查失败 → Ready=False
    
    if runtimeOK && plegOK && networkOK {
        return v1.NodeCondition{
            Type:   v1.NodeReady,
            Status: v1.ConditionTrue,
            Reason: "KubeletReady",
        }
    }
}
```

### 2.3 Ready=False 的常见原因

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# kubelet 进程异常
systemctl status kubelet

# 容器运行时异常
crictl info

# PLEG 异常 (Pod Lifecycle Event Generator)
# PLEG 负责检测容器状态变化
# 如果 PLEG 超时，kubelet 会报告 NotReady
journalctl -u kubelet | grep pleg

# 网络插件异常
ls /etc/cni/net.d/

# 证书过期
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates
```
### 2.4 Ready=Unknown 的处理

```bash
# Ready=Unknown 表示 kubelet 超时未上报状态
# 超时时间由以下参数控制:
# - node-monitor-period: 5s (kubelet 上报间隔)
# - node-monitor-grace-period: 40s (容忍无响应时间)
# - pod-eviction-timeout: 5m (Pod 驱逐超时)

# kube-controller-manager 参数:
--node-monitor-period=5s
--node-monitor-grace-period=40s
--pod-eviction-timeout=5m0s
```

---

## 三、MemoryPressure

### 3.1 内存压力检测

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 当节点可用内存低于驱逐阈值时，MemoryPressure=True
# 默认阈值: memory.available < 100Mi (硬驱逐)

# 检查内存压力状态
kubectl get node <node> -o jsonpath='{.status.conditions[?(@.type=="MemoryPressure")].status}'

# 查看可用内存
kubectl top node <node>
free -h
cat /proc/meminfo | grep -i "memavailable"
```
### 3.2 MemoryPressure 对调度的影响

```go
// pkg/scheduler/framework/plugins/noderesources/fit.go
// 当 MemoryPressure=True 时:
// - BestEffort Pod: 不调度
// - Burstable Pod: 不调度
// - Guaranteed Pod: 可以调度
```

### 3.3 源码分析：内存压力检测

```go
// pkg/kubelet/eviction/eviction_manager.go
func (m *managerImpl) synchronize(...) error {
    // 1. 获取当前可用内存
    memoryAvailable := memInfo.available
    
    // 2. 对比硬驱逐阈值
    if memoryAvailable < threshold.MemoryAvailable {
        // 设置 MemoryPressure=True
        // 开始驱逐 BestEffort Pod
    }
    
    // 3. 更新节点 Condition
    nodeConditions = append(nodeConditions, v1.NodeCondition{
        Type:   v1.NodeMemoryPressure,
        Status: v1.ConditionTrue,
    })
}
```

---

## 四、DiskPressure

### 4.1 磁盘压力检测

```bash
# 触发条件:
# imagefs.available < 15% (默认) → 镜像文件系统空间不足
# nodefs.available < 10% (默认)  → 节点文件系统空间不足

# 检查磁盘使用
df -h /var/lib/kubelet      # nodefs
df -h /var/lib/containerd   # imagefs

# 检查 inode 使用
df -i

# 检查大文件
du -sh /var/lib/containerd/*
du -sh /var/log/*
du -sh /var/lib/kubelet/*
```

### 4.2 磁盘压力的后果

```bash
# 当 DiskPressure=True 时:
# 1. 不调度新 Pod (除 Guaranteed)
# 2. kubelet 开始驱逐 Pod 释放磁盘空间
# 3. kubelet 加速镜像垃圾回收

# 镜像垃圾回收参数:
# --image-gc-high-threshold=85%  (磁盘使用 > 85% 时触发)
# --image-gc-low-threshold=80%   (清理到 80% 以下)
```

---

## 五、PIDPressure

### 5.1 PID 压力检测

```bash
# 当节点可用 PID 数量低于阈值时触发
# 默认阈值: pid.available < 32768 - 1000 = 31768

# 查看系统 PID 上限
cat /proc/sys/kernel/pid_max

# 查看当前 PID 使用量
ps -eLf | wc -l

# 查看 PID 使用详情
cat /proc/sys/kernel/pid_max
cat /sys/fs/cgroup/pids/pids.current     # cgroup v1
cat /sys/fs/cgroup/pids.max              # cgroup v2
```

### 5.2 PID 压力防护

```yaml
# 配置 Pod PID 限制
# /var/lib/kubelet/config.yaml
podPidsLimit: 4096    # 每个 Pod 最多 4096 个 PID

# 防止 fork bomb 攻击
# 单个 Pod 不能耗尽节点所有 PID
```

---

## 六、NetworkUnavailable

### 6.1 网络不可用检测

```bash
# 当 CNI 插件未正确配置时，NetworkUnavailable=True
# kubelet 根据以下条件判定:
# 1. CNI 配置文件是否存在 (/etc/cni/net.d/)
# 2. 网络接口是否配置完成
# 3. 节点 IP 是否可用

# 检查 CNI 状态
ls /etc/cni/net.d/
ip link show
ip addr show
ip route
```

### 6.2 网络状态清除

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# CNI 插件负责将 NetworkUnavailable 设置为 False
# 不同的 CNI 插件有不同的实现:
# - Calico: calico-node DaemonSet
# - Cilium: cilium-agent DaemonSet
# - Flannel: kube-flannel DaemonSet

# 手动清除 NetworkUnavailable (调试用)
kubectl patch node <node> -p '{"status":{"conditions":[{"type":"NetworkUnavailable","status":"False","reason":"NetworkConfigured"}]}}'
```
---

## 七、节点就绪调度流程

### 7.1 调度器检查逻辑

```
API Server 收到 Pod 调度请求
        │
        ▼
调度器遍历所有节点:
        │
        ├── 检查 NodeAffinity / NodeSelector
        │       └── 不匹配 → 跳过
        │
        ├── 检查 Taints / Tolerations
        │       └── 不容忍 → 跳过
        │
        ├── 检查 Ready == True?
        │       └── False/Unknown → 不调度
        │
        ├── 检查 MemoryPressure?
        │       └── True → 仅调度 Guaranteed Pod
        │
        ├── 检查 DiskPressure?
        │       └── True → 仅调度 Guaranteed Pod
        │
        ├── 检查 PIDPressure?
        │       └── True → 仅调度 Guaranteed Pod
        │
        ├── 检查 NetworkUnavailable?
        │       └── True → 不调度
        │
        ├── 检查资源是否充足 (CPU/Memory/GPU)
        │       └── 不足 → 不调度
        │
        └── 通过所有检查 → 加入候选节点
                │
                ▼
        评分并选择最优节点
```

---

## 八、节点资源状态

### 8.1 Capacity 与 Allocatable

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点资源
kubectl get node <node> -o jsonpath='{
  "Capacity CPU: "}{.status.capacity.cpu}{"\n"
  "Allocatable CPU: "}{.status.allocatable.cpu}{"\n"
  "Capacity Memory: "}{.status.capacity.memory}{"\n"
  "Allocatable Memory: "}{.status.allocatable.memory}{"\n"
  "Capacity Pods: "}{.status.capacity.pods}{"\n"
  "Allocatable Pods: "}{.status.allocatable.pods}
}'

# 计算公式:
# Allocatable = Capacity - KubeReserved - SystemReserved - EvictionHard
```
### 8.2 资源使用查看

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 节点资源使用概览
kubectl top nodes

# 详细资源分配
kubectl describe node <node> | grep -A 20 "Allocated resources"

# 查看请求量 vs 限制量
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.allocatable.cpu}{"\t"}{.status.allocatable.memory}{"\n"}{end}'
```
---

## 九、常见错误与排查

| Condition 异常 | 可能原因 | 排查命令 | 解决方案 |
|---------------|---------|---------|---------|
| Ready=False | kubelet 异常 | `systemctl status kubelet; journalctl -u kubelet` | 重启 kubelet，修复配置 |
| Ready=Unknown | kubelet 超时 | `curl -k https://localhost:10250/healthz` | 检查网络，重启 kubelet |
| MemoryPressure=True | 内存不足 | `free -h; kubectl top node` | 扩容/驱逐 Pod/增加内存 |
| DiskPressure=True | 磁盘不足 | `df -h; du -sh /var/lib/*` | 清理镜像/日志/增加磁盘 |
| PIDPressure=True | PID 不足 | `cat /proc/sys/kernel/pid_max; ps -eLf | wc -l` | 增加 pid_max，设置 podPidsLimit |
| NetworkUnavailable=True | CNI 未配置 | `ls /etc/cni/net.d/; ip link` | 安装 CNI 插件 |

---

## 相关函数

| 函数 | 源码位置 | 说明 |
|------|---------|------|
| `NodeReadyCondition` | `pkg/kubelet/nodestatus/setters.go` | Ready 状态判定 |
| `setNodeCondition` | `pkg/kubelet/nodestatus/setters.go` | 设置 Condition |
| `syncNodeStatus` | `pkg/kubelet/kubelet.go` | 节点状态同步 |
| `synchronize` | `pkg/kubelet/eviction/eviction_manager.go` | 驱逐管理主循环 |
| `NodeExists` | `pkg/controller/nodelifecycle/` | 节点存在性检查 |
| `CheckNodeReady` | `pkg/scheduler/framework/plugins/` | 调度器就绪检查 |

## Related

- [[reference|#reference Hub]] — tag hub

- [[系统基础/topic-cheat-sheet/go.md|go]]
- [[系统基础/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/cni.md|cni]]
- [[entities/containerd.md|containerd]]


<!-- risk-assessed -->
