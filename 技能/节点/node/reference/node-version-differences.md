---
title: Kubernetes 版本差异对比 — Node 诊断相关
description: 基于 code/ 目录源码分析的 Kubernetes 1.18~1.36 版本间 Node 生命周期管理、API 对象、CRI、调度机制等差异对比
summary: Node 诊断技能适用的版本兼容性矩阵与关键变更时间线
category: reference
tags:
- k8s
- node
- version-diff
- feature-gate
- compatibility
- lifecycle
- cri
sources:
- code/kubernetes-release-1.18/pkg/controller/nodelifecycle/
- code/kubernetes-release-1.20/pkg/features/
- code/kubernetes-release-1.28/pkg/features/
- code/kubernetes-release-1.30/pkg/features/
- code/kubernetes-release-1.32/pkg/features/
- code/kubernetes-release-1.34/pkg/features/
- code/kubernetes-1.36.2/pkg/features/
created: '2026-07-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
---

# Kubernetes 版本差异对比 — Node 诊断相关

> **数据来源说明**：本文档所有版本差异信息均基于 `code/` 目录中 Kubernetes 源码的实际代码分析，涉及版本：1.18、1.20、1.28、1.30、1.32、1.34、1.36.2。

---

## 1. 版本兼容性矩阵

### 1.1 诊断技能适用版本范围

| 技能文件 | 适用版本 | 版本注意事项 |
|---------|---------|-------------|
| 01-node-notready-diagnosis | 1.18+ | 1.34+ NodeMonitorGracePeriod 默认值变更为 50s |
| 02-node-resource-pressure | 1.22+ | NodeSwap 在 1.30 默认启用，1.34 GA |
| 03-node-component-troubleshooting | 1.26+ | EventedPLEG 从 1.26 引入（仍为 Alpha） |
| 04-node-sop-runbook | 1.20+ | GracefulNodeShutdown 从 1.20 引入 |

### 1.2 Feature Gate 版本演进矩阵

| Feature Gate | 1.18 | 1.20 | 1.28 | 1.30 | 1.32 | 1.34 | 1.36 |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| TaintBasedEvictions | GA(locked) | 已移除 | — | — | — | — | — |
| GracefulNodeShutdown | — | Alpha(默认关) | Beta(默认开) | Beta(默认开) | Beta(默认开) | Beta(默认开) | Beta(默认开) |
| GracefulNodeShutdownBasedOnPodPriority | — | — | Beta(默认开) | Beta(默认开) | Beta(默认开) | Beta(默认开) | Beta(默认开) |
| WindowsGracefulNodeShutdown | — | — | — | — | Alpha(默认关) | Beta(默认开) | Beta(默认开) |
| NodeSwap | — | — | Beta(默认关) | Beta(默认开) | Beta(默认开) | GA(locked) | GA(locked) |
| EventedPLEG | — | — | Alpha(默认关) | Alpha(默认关) | Alpha(默认关) | Alpha(默认关) | Alpha(默认关) |
| NodeLogQuery | — | — | Alpha(默认关) | Beta(默认关) | Beta(默认关) | Beta(默认关) | GA(locked) |
| UserNamespacesSupport | — | — | — | Beta(默认关) | Beta(默认关) | Beta(默认开) | GA(locked) |
| KubeletCrashLoopBackOffMax | — | — | — | — | Alpha(默认关) | Alpha(默认关) | Beta(默认开) |
| InPlacePodVerticalScaling | — | — | — | — | — | Beta(默认开) | GA(locked) |
| PodLevelResources | — | — | — | — | Alpha(默认关) | Beta(默认开) | Beta(默认开) |
| PLEGOnDemandRelist | — | — | — | — | — | — | Beta(默认开) |
| PodDisruptionConditions | — | — | Beta(默认开) | Beta(默认开) | Beta(默认开) | Beta(默认开) | Beta(默认开) |
| PodReadyToStartContainersCondition | — | — | Alpha(默认关) | Beta(默认开) | Beta(默认开) | Beta(默认开) | Beta(默认开) |
| NodeDeclaredFeatures | — | — | — | — | — | — | Beta(默认开) |
| SupplementalGroupsPolicy | — | — | — | — | — | Beta(默认开) | GA(locked) |

> **图例**：Alpha(默认关) = 需手动开启；Beta(默认开) = 默认启用可关闭；GA(locked) = 已锁定不可关闭；— = 该版本不存在此特性

---

## 2. Node 生命周期管理变更

### 2.1 NodeMonitorGracePeriod 默认值变更

**代码位置**：`pkg/controller/nodelifecycle/config/v1alpha1/defaults.go`

| 版本范围 | 默认值 | 说明 |
|---------|--------|------|
| 1.18 ~ 1.32 | **40s** | 传统默认值 |
| 1.34+ | **50s** | 需大于 HTTP2_PING_TIMEOUT(30s) + HTTP2_READ_IDLE_TIMEOUT(15s) |

**诊断影响**：
- 1.34+ 集群中，节点从失联到被标记为 NotReady 的时间窗口增加 10s
- 排查 NotReady 时间线时需考虑此差异
- 相关命令输出中 `LastHeartbeatTime` 与 `NotReady` 转换间隔会不同

```bash
# 🟢 检查当前集群的 NodeMonitorGracePeriod 配置
kubectl get --raw /api/v1/nodes | jq '.items[0].status.conditions[] | select(.type=="Ready")'
# 对比 LastHeartbeatTime 与 LastTransitionTime 的时间差
```

### 2.2 Pod 驱逐机制演进

| 版本 | 驱逐机制 | 关键参数 |
|------|---------|---------|
| ≤1.17 | podEvictionTimeout 直接驱逐 | `--pod-eviction-timeout=5m` |
| 1.18 | TaintBasedEvictions GA | Taint + podEvictionTimeout 并存 |
| 1.20+ | 纯 Taint-based 驱逐 | `node.kubernetes.io/unreachable:NoExecute` |
| 1.32+ | TaintEviction 独立控制器 | `pkg/controller/tainteviction/` 分离 |

**代码证据**：
- 1.18: `node_lifecycle_controller.go` 中直接使用 `nc.podEvictionTimeout` 进行 Pod 驱逐
- 1.28: `node_lifecycle_controller.go` 中通过 `taintManager` 管理驱逐
- 1.32: 引入独立的 `pkg/controller/tainteviction/` 包（6 个文件）
- 1.36: `tainteviction` 包扩展至 7 个文件，增加 `namespacedobject.go`

**诊断影响**：
- 1.18 以前：检查 `--pod-eviction-timeout` 参数
- 1.18+：检查节点 Taint 和 Pod 的 `tolerations`
- 1.32+：TaintEviction 控制器日志独立，需单独查看

```bash
# 🟢 检查节点 Taint（1.18+ 驱逐机制核心）
kubectl get node <node-name> -o jsonpath='{.spec.taints}' | jq .

# 🟢 检查 Pod 的 NoExecute 容忍
kubectl get pod <pod-name> -o jsonpath='{.spec.tolerations}' | jq .
```

### 2.3 GracefulNodeShutdown 演进

| 版本 | 状态 | 关键能力 |
|------|------|---------|
| 1.20 | Alpha | 基础优雅关机（需手动开启） |
| 1.21+ | Beta(默认开) | 默认启用 |
| 1.24+ | + PodPriority | 基于 Pod 优先级的分级关机 |
| 1.32 | + Windows Alpha | Windows 节点支持（Alpha） |
| 1.34+ | + Windows Beta | Windows 节点支持默认启用 |

**诊断影响**：
- 1.20 以前：节点关机时 Pod 直接被 kill，无优雅终止
- 1.21+：kubelet 会拦截关机信号，按 `shutdownGracePeriod` 优雅终止 Pod
- 1.24+：高优先级 Pod（如 DaemonSet）获得更长终止时间
- 排查节点重启后 Pod 异常时，需确认 GracefulNodeShutdown 配置

```bash
# 🟢 检查 kubelet 优雅关机配置
cat /var/lib/kubelet/config.yaml | grep -A 5 "shutdownGrace"
# 或
ps aux | grep kubelet | grep -o "shutdown-grace-period=[^ ]*"
```

---

## 3. Node API 对象字段变更

### 3.1 NodeStatus 结构演进

**代码位置**：`staging/src/k8s.io/api/core/v1/types.go`

| 字段 | 引入版本 | 说明 |
|------|---------|------|
| `capacity` / `allocatable` | 1.0+ | 基础资源容量 |
| `conditions` | 1.0+ | 节点状态条件 |
| `addresses` | 1.0+ | 节点地址 |
| `daemonEndpoints` | 1.0+ | 守护进程端点 |
| `nodeInfo` | 1.0+ | 系统信息 |
| `images` | 1.0+ | 镜像列表 |
| `volumesInUse` / `volumesAttached` | 1.0+ | 卷信息 |
| `config` | 1.11+ | 动态 Kubelet 配置状态 |
| `runtimeHandlers` | **1.32+** | 可用运行时处理器列表 |
| `features` | **1.32+** | CRI 实现的特性集（SupplementalGroupsPolicy） |
| `declaredFeatures` | **1.36+** | 节点声明的 Feature Gate 列表 |

### 3.2 NodeCondition 结构（稳定）

```go
// 所有版本（1.18~1.36）结构一致
type NodeCondition struct {
    Type               NodeConditionType
    Status             ConditionStatus
    LastHeartbeatTime  metav1.Time
    LastTransitionTime metav1.Time
    Reason             string
    Message            string
}
```

**诊断影响**：`kubectl describe node` 输出的 Conditions 部分在所有版本中格式一致，诊断命令通用。

### 3.3 NodeSpec 变更

| 字段 | 版本 | 说明 |
|------|------|------|
| `taints` | 1.6+ | 节点污点 |
| `configSource` | 1.11+（已废弃） | 动态配置源 |
| `podCIDRs` | 1.16+ | 双栈 Pod CIDR |

[存疑：此处关于 1.32 版本引入 `runtimeHandlers` 字段的具体小版本号可能存在不准确之处，代码中 1.32 已存在该字段，但精确引入版本需进一步核实 KEP-3673 的毕业时间线]

---

## 4. 容器运行时接口 (CRI) 差异

### 4.1 CRI API 接口稳定性

**代码位置**：`staging/src/k8s.io/cri-api/pkg/apis/services.go`

| 接口 | 1.28 | 1.36 | 变更说明 |
|------|------|------|---------|
| `RuntimeService` | 稳定 | 稳定 | 接口签名无变化 |
| `PodSandboxManager` | 稳定 | 稳定 | 修复参数命名 typo（`odSandboxID` → `podSandboxID`） |
| `ContainerManager` | 稳定 | 稳定 | 无变化 |
| `ContainerStatsManager` | 稳定 | 稳定 | 无变化 |

### 4.2 PLEG 实现架构变更

**代码位置**：`pkg/kubelet/pleg/generic.go`

| 版本 | 架构模式 | 关键特征 |
|------|---------|----------|
| 1.28~1.34 | `wait.Until(g.Relist, period)` | 简单定时器驱动全局 relist |
| 1.36+ | `workerLoopIteration()` 优先级循环 | 支持全局 relist + 单 Pod 按需 relist |

**1.36 PLEG 架构重大变更**：

```go
// 1.36 新增：优先级工作分发循环
// 优先级：1.stop > 2.全局relist > 3.单Pod relist
func (g *GenericPLEG) workerLoopIteration() bool {
    // First priority: stopCh
    // Second priority: global Relist (globalRelistTimer)
    // Third priority: single pod relist (relistRequests channel)
}

// 1.36 新增：按需单 Pod relist（PLEGOnDemandRelist Feature Gate）
func (g *GenericPLEG) RequestRelist(podUID types.UID) {
    // 通过 relistRequests channel 提交单 Pod relist 请求
}

// 1.36 新增：请求重新检查
func (g *GenericPLEG) RequestReinspect(podUID types.UID) {
    // 标记 Pod 在下次 Relist 时重新检查
}
```

**新增 Feature Gate**：
- `PLEGOnDemandRelist`：1.36 Beta（默认开）— 启用按需单 Pod relist，减少不必要的全局 relist 开销

**诊断影响**：
- 1.36+ 的 PLEG 不健康诊断需考虑新的 `relistRequests` 队列是否满（容量 200）
- PLEG 健康检查逻辑不变：`elapsed > RelistThreshold` 则不健康
- 1.36+ 日志中可能出现 "Relist request channel full; dropping relist request" 错误
- EventedPLEG（仍为 Alpha）在 1.36 中委托 `RequestRelist` 给 GenericPLEG

```bash
# 🟢 检查 PLEG 健康状态（所有版本）
crictl info | jq '.conditions[] | select(.type=="RuntimeReady")'
# 🟢 检查 kubelet 日志中 PLEG 相关信息
journalctl -u kubelet --since "5 min ago" | grep -i "pleg\|relist"
```

### 4.3 NodeRuntimeHandler（1.32+ 新增）

```go
// 1.32+ 新增：NodeStatus.RuntimeHandlers
type NodeRuntimeHandler struct {
    Name     string                      // 运行时名称（空字符串=默认）
    Features *NodeRuntimeHandlerFeatures // 支持的特性
}

type NodeRuntimeHandlerFeatures struct {
    RecursiveReadOnlyMounts *bool  // 递归只读挂载支持
    UserNamespaces          *bool  // 用户命名空间支持（1.36+）
}
```

### 4.4 NodeFeatures（1.32+ 新增）

```go
// 1.32+ 新增：NodeStatus.Features
type NodeFeatures struct {
    SupplementalGroupsPolicy *bool  // 运行时是否支持 SupplementalGroupsPolicy
}
```

**诊断影响**：
- 1.32+ 可通过 `kubectl get node -o jsonpath='{.status.runtimeHandlers}'` 查看运行时能力
- 排查容器启动失败时，可确认运行时是否支持所需特性
- 1.28 及以前版本无此信息，需通过 `crictl info` 直接查询运行时

```bash
# 🟢 查看节点运行时处理器（1.32+）
kubectl get node <node-name> -o jsonpath='{.status.runtimeHandlers}' | jq .

# 🟢 查看节点 CRI 特性（1.32+）
kubectl get node <node-name> -o jsonpath='{.status.features}' | jq .

# 🟢 通用：直接查询 containerd 运行时信息（所有版本）
crictl info | jq '.config'
```

---

## 5. 调度机制与 Node 相关演进

### 5.1 InPlacePodVerticalScaling（原地资源调整）

| 版本 | 状态 | 诊断影响 |
|------|------|---------|
| 1.27~1.32 | Alpha(默认关) | 无影响 |
| 1.33~1.34 | Beta(默认开) | Pod 资源可原地调整，不触发重新调度 |
| 1.35+ | GA(locked) | 始终启用 |

**诊断影响**：
- 1.33+ 集群中，Pod 资源变更不再触发重新调度，`kubectl describe pod` 中不会出现 `Resizing` 相关的调度事件
- 节点资源压力诊断时，需考虑原地调整可能导致的资源碎片化
- 相关 Feature：`PodLevelResources`（1.32 Alpha → 1.34 Beta）

### 5.2 NodeSwap 对调度的影响

| 版本 | 状态 | 调度行为 |
|------|------|---------|
| ≤1.27 | Alpha(默认关) | kubelet 要求禁用 swap |
| 1.28~1.29 | Beta(默认关) | 可选启用 swap |
| 1.30~1.33 | Beta(默认开) | 默认允许 swap |
| 1.34+ | GA(locked) | 始终允许 swap |

**诊断影响**：
- 1.30+ 集群中，节点有 swap 不再导致 kubelet 启动失败
- MemoryPressure 诊断时需区分物理内存压力与 swap 使用情况
- `kubectl describe node` 中 1.36+ 会显示 `NodeSwapStatus`（swap 容量信息）

```bash
# 🟢 检查节点 swap 状态
free -h
# 🟢 检查 kubelet swap 配置
grep -i swap /var/lib/kubelet/config.yaml
```

### 5.3 NodeDeclaredFeatures（1.35+ 引入）

1.36 版本引入 `NodeDeclaredFeatures` 机制，节点可在 `NodeStatus.DeclaredFeatures` 中声明其支持的 Feature Gate 列表。

**诊断影响**：
- 1.36+ 可通过 API 直接查看节点支持的特性
- 调度器可基于节点声明的特性进行更精确的调度决策
- 排查调度失败时，可检查节点是否声明了所需特性

```bash
# 🟢 查看节点声明的特性（1.36+）
kubectl get node <node-name> -o jsonpath='{.status.declaredFeatures}' | jq .
```

---

## 6. 诊断命令版本兼容性

### 6.1 通用命令（所有版本适用）

| 命令 | 适用版本 | 说明 |
|------|---------|------|
| `kubectl get nodes` | 所有 | 节点列表 |
| `kubectl describe node` | 所有 | 节点详情 |
| `kubectl get node -o yaml` | 所有 | 完整 YAML |
| `systemctl status kubelet` | 所有 | kubelet 服务状态 |
| `journalctl -u kubelet` | 所有 | kubelet 日志 |
| `crictl ps` / `crictl pods` | 1.11+ | CRI 容器/Pod 列表 |
| `crictl info` | 1.11+ | 运行时信息 |

### 6.2 版本特定命令

| 命令/能力 | 最低版本 | 说明 |
|----------|---------|------|
| `kubectl get node -o jsonpath='{.status.runtimeHandlers}'` | 1.32 | 运行时处理器列表 |
| `kubectl get node -o jsonpath='{.status.features}'` | 1.32 | CRI 特性 |
| `kubectl get node -o jsonpath='{.status.declaredFeatures}'` | 1.36 | 节点声明特性 |
| `kubectl get --raw /api/v1/nodes/<name>/log` (查询模式) | 1.30 | NodeLogQuery Beta |
| kubelet `/logs/?query=` HTTP 端点 | 1.27(Alpha) | 结构化日志查询 |

### 6.3 输出格式差异

| 输出项 | 1.28 及以前 | 1.32+ | 1.36+ |
|--------|-----------|-------|-------|
| `kubectl describe node` Conditions | 标准 5 条件 | 同左 | 同左 |
| `kubectl get node -o wide` | 标准列 | 同左 | 同左 |
| NodeStatus JSON | 无 runtimeHandlers | +runtimeHandlers, +features | +declaredFeatures |
| kubelet 日志查询 | 仅文件路径 | +结构化查询(Beta) | 结构化查询(GA) |

---

## 7. Kubelet 关键参数版本变更

### 7.1 已废弃/移除参数

| 参数 | 废弃版本 | 移除版本 | 替代方案 |
|------|---------|---------|---------|
| `--pod-eviction-timeout` | 1.18 | — | Taint-based eviction |
| `--dynamic-config-dir` | 1.24 | — | 不再推荐动态配置 |

[存疑：此处关于 `--pod-eviction-timeout` 参数的移除版本可能存在不准确之处。代码中 1.28~1.36 的 config 转换文件均标注 "WARNING: in.PodEvictionTimeout requires manual conversion: does not exist in peer-type"，表明该字段在内部配置中已不存在，但命令行参数可能仍被接受（被忽略），需进一步核实]

### 7.2 新增关键参数

| 参数/配置 | 引入版本 | 说明 |
|----------|---------|------|
| `shutdownGracePeriod` | 1.20 | 优雅关机总时长 |
| `shutdownGracePeriodCriticalPods` | 1.20 | 关键 Pod 关机时长 |
| `shutdownGracePeriodByPodPriority` | 1.23 | 按优先级分级关机 |
| `enableSystemLogQuery` | 1.27 | 结构化日志查询 |
| `memorySwap.swapBehavior` | 1.22 | Swap 行为配置 |
| `crashLoopBackOffMax` | 1.32 | CrashLoop 最大退避时间 |

---

## 8. 版本升级诊断注意事项

### 8.1 升级检查清单

| 升级路径 | 关键检查项 |
|---------|-----------|
| 1.28 → 1.30 | NodeSwap 默认启用，检查 swap 配置；NodeLogQuery 升级为 Beta |
| 1.30 → 1.32 | KubeletCrashLoopBackOffMax 引入；RuntimeHandlers API 可用 |
| 1.32 → 1.34 | NodeMonitorGracePeriod 40s→50s；NodeSwap GA；WindowsGracefulNodeShutdown Beta |
| 1.34 → 1.36 | NodeLogQuery GA；UserNamespacesSupport GA；NodeDeclaredFeatures Beta；InPlacePodVerticalScaling GA；**PLEGOnDemandRelist Beta**（PLEG 架构重构） |

### 8.2 版本特定故障模式

| 版本 | 特有故障模式 | 诊断要点 |
|------|------------|---------|
| 1.30+ | Swap 相关 OOM 行为变化 | 检查 `memorySwap.swapBehavior` 配置 |
| 1.32+ | CrashLoopBackOff 退避策略变化 | 检查 `crashLoopBackOffMax` 配置 |
| 1.34+ | NotReady 判定延迟增加 10s | 调整监控告警阈值 |
| 1.36+ | 节点特性声明不匹配 | 检查 `declaredFeatures` 与实际能力 |
| 1.36+ | PLEG relistRequests 队列满 | 检查日志 "Relist request channel full" |

---

## 8.3 Node Lease 与心跳机制稳定性

**代码位置**：`pkg/kubelet/apis/config/v1beta1/defaults.go`、`pkg/kubelet/kubelet.go`

| 参数 | 1.28 | 1.36 | 变更 |
|------|------|------|------|
| `NodeLeaseDurationSeconds` | 40s | 40s | 无变化 |
| `NodeStatusUpdateFrequency` | 10s | 10s | 无变化 |
| `NodeStatusReportFrequency` | 5min | 5min | 无变化 |
| Lease 续租间隔 | leaseDuration × 0.25 | leaseDuration × 0.25 | 无变化 |

**结论**：Node Lease 机制在 1.28~1.36 间完全稳定，诊断命令通用：

```bash
# 🟢 检查节点 Lease 续租状态（所有版本通用）
kubectl get lease -n kube-node-lease <node-name> -o jsonpath='{.spec.renewTime}'
kubectl get lease -n kube-node-lease <node-name> -o jsonpath='{.spec.leaseDurationSeconds}'
```

### 8.4 PodDisruptionConditions（1.28+ Beta）

`PodDisruptionConditions` 在 1.28 即为 Beta（默认开），为被驱逐的 Pod 添加 `DisruptionTarget` 条件。

**诊断影响**：
- 1.28+ 集群中，被节点驱逐的 Pod 会在 `status.conditions` 中显示 `DisruptionTarget=True`
- 可通过此条件区分「节点驱逐」与「其他原因导致的 Pod 终止」

```bash
# 🟢 检查 Pod 是否因节点驱逐而终止（1.28+）
kubectl get pod <pod-name> -o jsonpath='{.status.conditions[?(@.type=="DisruptionTarget")]}'
```

---

## 9. 诊断命令版本差异

> 以下内容整合自 `故障诊断/技能体系/skill-set/k8s-node-notready/reference/version-matrix.md`

### 9.1 诊断命令差异表（v1.28-v1.32）

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubectl debug node/<name>` | 支持，使用 `--image` 指定调试镜像 | 同左 | 新增 `--profile` 参数（GA） | 同左 | 同左 |
| `kubectl get --raw /api/v1/nodes/<name>/proxy/healthz` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl get --raw /api/v1/nodes/<name>/proxy/configz` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl top node` (metrics-server) | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl get lease -n kube-node-lease` | 支持（v1.17+ GA） | 同左 | 同左 | 同左 | 同左 |
| `crictl` 版本要求 | >=1.28 | >=1.29 | >=1.30 | >=1.31 | >=1.32 |

### 9.2 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| Node | v1 (core) | v1 | v1 | v1 | v1 |
| Lease | coordination.k8s.io/v1 | v1 | v1 | v1 | v1 |
| Event | events.k8s.io/v1 | v1 | v1 | v1 | v1 |
| CSR (CertificateSigningRequest) | certificates.k8s.io/v1 | v1 | v1 | v1 | v1 |
| RuntimeClass | node.k8s.io/v1 | v1 | v1 | v1 | v1 |

### 9.3 版本相关诊断注意事项

#### [v1.28+]: GracefulNodeShutdown 默认启用

当节点正在关机时，kubelet 会尝试优雅终止 Pod。在诊断时需注意区分计划关机和异常关机：

- 检查 `shutdownGracePeriod` 和 `shutdownGracePeriodCriticalPods` 配置
- 日志中出现 `shutting down gracefully` 不一定是问题
- **诊断影响**: 看到 `shutting down gracefully` 日志时，需确认是否为计划内操作

#### [v1.30+]: Node swap support (beta)

可能影响内存压力的判断：

- 如果 `NodeSwap` feature gate 启用且 `swapBehavior: LimitedSwap`，需同时检查 swap 使用情况
- `free -m` 输出中的 Swap 行不再是"异常"信号
- kubelet 的 `--fail-swap-on` 标志在启用 swap 时为 `false`
- **诊断影响**: MemoryPressure 的计算可能包含 swap 使用量；`swap is enabled` 日志属于正常信息

#### [v1.31+]: EventedPLEG 默认启用

- 传统 GenericPLEG 的 relist 操作频率降低，`PLEG is not healthy` 误报减少
- 但如果 EventedPLEG 本身异常，可能出现新的问题模式
- 诊断时需检查 `--feature-gates=EventedPLEG=true` 是否生效
- **诊断影响**: PLEG 相关日志的解读需考虑 EventedPLEG 的行为差异

#### [v1.32+]: nftables kube-proxy 模式 GA

- 使用 nftables 模式时，`iptables -L` 不再显示 kube-proxy 规则
- 需使用 `nft list ruleset` 检查规则
- **诊断影响**: 检查 kube-proxy 规则的命令需根据模式调整：
  ```bash
  # iptables 模式（传统）
  iptables -t nat -L KUBE-SERVICES 2>/dev/null | head -5
  
  # ipvs 模式
  ipvsadm -Ln 2>/dev/null | head -10
  
  # nftables 模式（v1.32+ GA）
  nft list ruleset 2>/dev/null | grep -A5 "KUBE-SERVICES"
  ```

---

## 10. 常见误诊模式（版本相关）

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|----------|
| **网络抖动误判为 kubelet 崩溃** | Ready=Unknown，看似 kubelet 停止发送心跳 | 网络链路不稳定（交换机端口 flapping、MTU 问题、云网络限流） | 先 SSH 确认 kubelet 进程状态，再测试网络连通性 |
| **DiskPressure 归因于镜像过多** | DiskPressure=True，磁盘使用率高 | 容器日志未正确配置轮转，单个 Pod 日志占用几十 GB | 检查 `/var/log/pods/` 下的大文件：`du -sh /var/log/pods/* \| sort -rh \| head -10` |
| **PLEG 不健康误判为容器运行时问题** | `PLEG is not healthy` 日志 | 某个 Pod 的 container 处于 D 状态（不可中断 I/O 等待），阻塞了 CRI 调用 | 检查 D 状态进程：`ps aux \| awk '$8=="D"'` |
| **证书过期误判为网络问题** | `connection refused` 或 TLS 错误 | kubelet 客户端证书已过期，TLS 握手失败被解读为网络问题 | 在排查网络前先检查证书有效期。TLS 握手失败和 TCP 连接失败有本质区别 |
| **cordon 操作误判为节点问题** | Pod 无法调度到某节点 | 运维人员之前执行了 `kubectl cordon` 但未记录 | 仔细区分 `NotReady` 和 `Ready,SchedulingDisabled` |
| **时间偏差导致间歇性问题** | 节点状态时好时坏，难以找到明确根因 | 节点 NTP 未同步，时钟偏差导致 TLS 证书间歇性验证失败和 Lease 续租异常 | 在诊断早期就检查时间同步。时间偏差是最容易被忽视但影响广泛的根因 |

---

## 11. 待补充的知识空白

以下领域在当前版本中覆盖有限，后续版本将增强：

1. **GPU 节点 NotReady**: GPU 驱动异常导致的节点 NotReady 场景（NVIDIA device plugin crash, GPU memory error）
2. **Windows 节点**: Windows 容器节点的 NotReady 诊断差异（kubelet on Windows, containerd on Windows）
3. **ARM 架构节点**: ARM 节点的特定问题模式
4. **边缘节点**: 使用 KubeEdge / OpenYurt 等边缘方案的节点 NotReady 诊断差异（弱网环境、离线容忍）
5. **虚拟节点**: Virtual Kubelet 实现的虚拟节点 NotReady 诊断

---

## 12. 存疑信息汇总

以下信息通过代码分析发现但存在不确定性，需进一步核实：

1. [存疑：此处关于 1.32 版本引入 `runtimeHandlers` 字段的具体小版本号可能存在不准确之处，代码中 1.32 已存在该字段，但精确引入版本（1.31 或 1.32）需进一步核实 KEP-3673 的毕业时间线]

2. [存疑：此处关于 `--pod-eviction-timeout` 参数的移除版本可能存在不准确之处。代码中 1.28~1.36 的 config 转换文件均标注该字段不存在于对等类型中，但命令行参数可能仍被接受（被忽略），需进一步核实]

3. [存疑：此处关于 EventedPLEG 在 1.36 版本仍为 Alpha 的状态可能存在不准确之处。代码中确认 1.36 仍为 Alpha，但考虑到该特性自 1.26 引入已跨越多个版本，后续版本可能快速推进，需关注 KEP-3386 最新进展]

4. [存疑：此处关于 GracefulNodeShutdown 在 1.36 仍为 Beta 而非 GA 的状态需进一步核实。代码中确认 1.36 仍为 Beta（无 GA 条目），但作为 1.21 即进入 Beta 的特性，长期未 GA 较为罕见]

---

## 13. 代码分析来源索引

| 分析维度 | 代码路径 | 涉及版本 |
|---------|---------|---------|
| Node 生命周期控制器默认值 | `pkg/controller/nodelifecycle/config/v1alpha1/defaults.go` | 1.18, 1.28, 1.30, 1.32, 1.34, 1.36 |
| Feature Gate 定义 | `pkg/features/kube_features.go` / `versioned_kube_features.go` | 1.18, 1.20, 1.28, 1.30, 1.32, 1.34, 1.36 |
| Node API 类型 | `staging/src/k8s.io/api/core/v1/types.go` | 1.28, 1.32, 1.34, 1.36 |
| CRI API 接口 | `staging/src/k8s.io/cri-api/pkg/apis/services.go` | 1.28, 1.36 |
| TaintEviction 控制器 | `pkg/controller/tainteviction/` | 1.32, 1.36 |
| Kubelet 优雅关机 | `pkg/kubelet/nodeshutdown/` | 1.28, 1.36 |
| PLEG 实现 | `pkg/kubelet/pleg/` | 1.28, 1.36 |
| Kubelet 配置验证 | `pkg/kubelet/apis/config/validation/validation.go` | 1.28, 1.36 |
