---
title: Pod 诊断的 Kubernetes 版本差异参考
description: 基于 code/ 目录 Kubernetes 1.18/1.20/1.28/1.30/1.32/1.34/1.36 源码对比，梳理 Pod 生命周期、PodSpec/PodStatus 字段、调度机制、容器运行时接口及诊断命令在不同版本间的差异，并给出版本兼容性矩阵
summary: 面向 Pod 异常诊断的版本差异手册，源自对 pkg/apis/core/types.go 及 kube-scheduler framework 的直接代码比对，标注了各诊断特性适用的 K8s 版本范围
category: reference
tags:
- k8s
- pod
- version-diff
- compatibility
- podspec
- podstatus
- scheduling
- cri
- troubleshooting
sources:
- code/kubernetes-release-1.18/pkg/apis/core/types.go
- code/kubernetes-release-1.20/pkg/apis/core/types.go
- code/kubernetes-release-1.28/pkg/apis/core/types.go
- code/kubernetes-release-1.30/pkg/apis/core/types.go
- code/kubernetes-release-1.32/pkg/apis/core/types.go
- code/kubernetes-release-1.34/pkg/apis/core/types.go
- code/kubernetes-1.36.2/pkg/apis/core/types.go
- code/kube-scheduler-master/framework/types.go
created: '2026-07-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
- 平台工程师
estimated_read_time: 20min
intent_queries:
- Kubernetes 不同版本 Pod 诊断有什么区别
- SchedulingGates 从哪个版本开始
- Sidecar 容器 restartPolicy 支持哪些版本
- Pod 原地扩缩容 InPlacePodVerticalScaling 版本
- PodResizeStatus 为什么没有了
- 各版本 PodSpec 字段差异
trigger_keywords:
- 版本差异
- version diff
- SchedulingGates
- SidecarContainers
- InPlacePodVerticalScaling
- ResourceClaims
- DRA
- PodResizeStatus
- 版本兼容性
- k8s 1.28
- k8s 1.32
- k8s 1.34
prerequisites:
- kubectl-basics
- pod-lifecycle
- pod-diagnosis-skills
---

> **生产环境安全提示**
>
> 本文档为版本差异参考，不含破坏性操作。所列版本引入/毕业信息以 `code/` 目录中的实际源码（`pkg/apis/core/types.go` 的 `+featureGate` 与注释）为第一依据。凡源码未能直接证实的具体毕业版本，均以 `[存疑：...]` 标注，使用前请结合目标集群 `kubectl version` 与官方 Release Notes 二次核实。

# Pod 诊断的 Kubernetes 版本差异参考

## 0. 证据来源与方法说明

本文对比的版本快照均取自本仓库 `code/` 目录，通过直接比对 `pkg/apis/core/types.go` 中 `PodSpec` / `PodStatus` / `Container` / `PodConditionType` 等类型的字段定义与 `+featureGate` 注释，以及 `kube-scheduler` 调度框架的 `ActionType` 定义得出：

| 版本快照 | 源码路径 | types.go 行数 |
|---------|---------|--------------|
| v1.18 | `code/kubernetes-release-1.18/pkg/apis/core/types.go` | 5235 |
| v1.20 | `code/kubernetes-release-1.20/pkg/apis/core/types.go` | 5442 |
| v1.28 | `code/kubernetes-release-1.28/pkg/apis/core/types.go` | 6148 |
| v1.30 | `code/kubernetes-release-1.30/pkg/apis/core/types.go` | 6434 |
| v1.32 | `code/kubernetes-release-1.32/pkg/apis/core/types.go` | 6772 |
| v1.34 | `code/kubernetes-release-1.34/pkg/apis/core/types.go` | 7102 |
| v1.36 | `code/kubernetes-1.36.2/pkg/apis/core/types.go` | 7241 |
| 调度框架 | `code/kube-scheduler-master/framework/types.go` | 707 |

> **重要**：本仓库缺少 1.29/1.31/1.33/1.35 的源码快照。凡涉及"某特性在 1.29/1.31/1.33 毕业"的表述，均无法用本地代码直接证实，已用 `[存疑]` 标注。字段的 **存在性与 alpha/beta/stable 状态** 则以上述 7 个快照的实际代码为准。

---

## 1. 版本兼容性矩阵（诊断特性 × 版本）

下表说明各项 Pod 诊断能力/字段在不同版本的可用性（✅ 稳定可用 / 🅱️ Beta 默认开启 / 🅰️ Alpha 需开 feature gate / ❌ 不存在）。状态依据源码 `+featureGate` 注释判定。

| 诊断特性 / 字段 | 1.18 | 1.20 | 1.28 | 1.30 | 1.32 | 1.34 | 1.36 | 关联技能 |
|---------------|:----:|:----:|:----:|:----:|:----:|:----:|:----:|---------|
| `kubectl describe pod` 基础诊断 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | 全部 |
| Exit Code / Last State 解读 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | 01 |
| Ephemeral Containers 调试 (`kubectl debug`) | 🅰️ | 🅰️ | ✅ | ✅ | ✅ | ✅ | ✅ | 01/03 |
| `RuntimeClassName`（运行时选择） | 🅱️ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | 03 |
| Sidecar 容器（init container `restartPolicy: Always`） | ❌ | ❌ | 🅰️ | 🅱️ | ✅ | ✅ | ✅ | 01/03 |
| `SchedulingGates`（调度门控） | ❌ | ❌ | 🅱️ | 🅱️ | ✅ | ✅ | ✅ | 02 |
| `SchedulingGated` Pod 状态 | ❌ | ❌ | 🅱️ | 🅱️ | ✅ | ✅ | ✅ | 02 |
| `DisruptionTarget` Pod Condition | ❌ | ❌ | ✅ | ✅ | ✅ | ✅ | ✅ | 04 |
| DRA `ResourceClaims`（动态资源分配） | ❌ | ❌ | 🅰️ | 🅰️ | 🅰️ | 🅰️ | ✅ | 02 |
| 原地扩缩容 `InPlacePodVerticalScaling` | ❌ | ❌ | 🅰️ | 🅰️ | 🅰️ | 🅰️ | 🅰️ | 01 |
| `PodResizeStatus`（`.status.resize`） | ❌ | ❌ | 🅰️ | 🅰️ | 🅰️ | ⚠️废弃 | ⚠️废弃 | 01 |
| `PodResizePending`/`PodResizeInProgress` Condition | ❌ | ❌ | ❌ | ❌ | ❌ | 🅰️ | 🅰️ | 01 |
| 容器 `StopSignal`（`.status...stopSignal`） | ❌ | ❌ | ❌ | ❌ | ❌ | 🅰️ | 🅰️ | 04 |
| `ContainerRestartRules`（按退出码重启规则） | ❌ | ❌ | ❌ | ❌ | ❌ | 🅰️ | 🅰️ | 01 |
| `RecursiveReadOnlyMounts` | ❌ | ❌ | ❌ | ❌ | 🅰️ | ✅ | ✅ | 03 |
| Pod 级资源 `PodLevelResources`（`spec.resources`） | ❌ | ❌ | ❌ | ❌ | ❌ | 🅰️ | 🅰️ | 01/02 |
| `HostnameOverride` | ❌ | ❌ | ❌ | ❌ | ❌ | 🅰️ | 🅰️ | 03 |
| Gang 调度 `SchedulingGroup`/`PodGroup` | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | 🅰️ | 02 |

> 图例：✅ Stable/GA · 🅱️ Beta（默认开启） · 🅰️ Alpha（需显式开启 feature gate） · ⚠️废弃 · ❌ 该字段不存在
>
> [存疑：矩阵中 Beta→GA 的过渡列（如 1.30 与 1.32 之间的 1.31）因缺少对应源码快照而通过相邻版本状态推断，Sidecar 容器与 SchedulingGates 的精确 GA 版本需以官方 Release Notes 核实]

---

## 2. PodSpec 字段演进（源码实证）

以下均为 `PodSpec` 结构体在各快照中的**直接代码差异**。

### 2.1 1.18/1.20 → 1.28 新增

| 字段 | 1.18/1.20 | 1.28 | 说明 |
|------|:---------:|:----:|------|
| `EphemeralContainers` | 🅰️ alpha（注释明确 "alpha-level ... EphemeralContainers feature flag"） | ✅ 无 feature gate | 支撑 `kubectl debug` 临时调试容器 |
| `SchedulingGates` | ❌ 不存在 | 🅱️ `+featureGate=PodSchedulingReadiness` | 调度门控，Pod 停留在 `SchedulingGated` |
| `PodSchedulingGate` 类型 | ❌ | ✅ 存在 | 门控条目定义 |
| 容器 `RestartPolicy *ContainerRestartPolicy` | ❌ | 🅰️ `+featureGate=SidecarContainers` | 原生 Sidecar 基础 |
| `DisruptionTarget` Condition | ❌ | ✅ 存在 | 标识 Pod 因抢占/驱逐/GC 即将终止 |
| `PodResizeStatus` 类型 | ❌ | 🅰️ 含 `Proposed`/`InProgress`/`Deferred`/`Infeasible` | 原地扩缩容状态 |

### 2.2 1.28 → 1.34 关键变更

| 字段 / 类型 | 1.28 | 1.34 | 诊断影响 |
|------------|:----:|:----:|---------|
| `SchedulingGates` feature gate | 🅱️ `PodSchedulingReadiness` | ✅ 无 gate（已 GA） | 1.34 上 `SchedulingGated` 为默认行为，诊断 Pending 必查 |
| `PodResizeStatus`（`.status.resize`） | 🅰️ 含 `Proposed` 值 | ⚠️ 注释标 `Deprecated`，且 **`Proposed` 值被移除** | 1.34 改用 `PodResizePending`/`PodResizeInProgress` 两个 Condition |
| `ContainerRestartRules` / `RestartPolicyRules` | ❌ | 🅰️ `+featureGate=ContainerRestartRules` | 可按退出码决定是否重启，影响 CrashLoop 判断 |
| 容器 `StopSignal *Signal` | ❌ | 🅰️ `+featureGate=ContainerStopSignals` | 可自定义停止信号，影响 Terminating 分析 |
| `PodLevelResources`（`spec.resources`） | ❌ | 🅰️ `+featureGate=PodLevelResources` | Pod 级 requests/limits，影响 OOM 与调度诊断 |
| `HostnameOverride` | ❌ | 🅰️ `+featureGate=HostnameOverride` | 覆盖 Pod hostname |
| `AllocatedResourcesStatus` | ❌ | 🅰️ `+featureGate=ResourceHealthStatus` | 设备健康状态，诊断 GPU/DRA 资源 |
| `NodeName` 字段注释 | 简短 | 明确"设置后 kubelet 接管生命周期，不应用于表达调度意愿" | 语义澄清，非行为变更 |

### 2.3 1.34 → 1.36 关键变更

| 字段 / 类型 | 1.34 | 1.36 | 诊断影响 |
|------------|:----:|:----:|---------|
| DRA `ResourceClaims` | 🅰️ "alpha field ... DynamicResourceAllocation feature gate" | ✅ "stable field ... requires DRA gate enabled" | 1.36 DRA 转 stable，`kubectl describe` 可稳定看到 ResourceClaim 绑定 |
| `SchedulingGroup *PodSchedulingGroup` | ❌ | 🅰️ `+featureGate=GenericWorkload` | Gang/组调度，新增 Pod 归属调度组，影响批调度诊断 |
| `PodSchedulingGroup` / `PodGroupName` 类型 | ❌ | ✅ 存在 | 组调度 API 对象 |

---

## 3. PodStatus / PodCondition 演进

### 3.1 PodPhase（跨版本稳定）

`PodPhase` 五值（`Pending`/`Running`/`Succeeded`/`Failed`/`Unknown`）自 1.18 至 1.36 **保持不变**。其中 `PodUnknown` 注释在新版本明确标注"Deprecated in v1.21: 自 2015 年起即不再被设置"——诊断时不应期待遇到 `Unknown` phase。

### 3.2 PodConditionType 增量

| Condition | 引入快照 | 用途 |
|-----------|:-------:|------|
| `PodScheduled` / `Ready` / `Initialized` / `ContainersReady` | 1.18 起 | 基础生命周期 |
| `DisruptionTarget` | 1.28 快照已存在 | 抢占/驱逐/GC 即将终止的信号 |
| `PodResizePending` | 1.34 快照新增 | 原地扩缩容：spec 已改但 kubelet 未分配资源 |
| `PodResizeInProgress` | 1.34 快照新增 | 原地扩缩容：kubelet 已分配、尚未全部生效 |

> **诊断提示**：在 1.34+ 排查"Pod 卡在扩缩容"时，应查 `PodResizePending`/`PodResizeInProgress` 两个 Condition，而非旧的 `.status.resize` 字段（后者已废弃且 `Proposed` 值被移除）。

---

## 4. 调度机制演进（kube-scheduler framework）

依据 `code/kube-scheduler-master/framework/types.go` 中的 `ActionType` 与资源事件定义：

| 机制 | 证据 | 诊断影响 |
|------|------|---------|
| `UpdatePodSchedulingGatesEliminated` 事件 | framework/types.go 定义了该细粒度事件，门控清空后触发重新入队 | 排查"门控已删仍不调度"时确认调度器版本支持该事件 |
| `UpdatePodGeneratedResourceClaim` 事件 | 依赖 DynamicResourceAllocation | DRA 场景 Pending 诊断 |
| `PodGroup` 资源事件（`scheduling.k8s.io/PodGroup`） | framework 定义了 `PodGroupInfo`/`PodGroupAssignments` | 组/Gang 调度诊断（对应 1.36 `SchedulingGroup`） |
| `schedulingv1alpha3.PodGroup` 引用 | framework/types.go import | 组调度处于 v1alpha3，需开 `GenericWorkload` |

> [存疑：`kube-scheduler-master` 为主干快照，其调度框架能力（尤其 PodGroup/Gang 调度）领先于 1.36 稳定版，实际集群是否可用取决于所部署 kube-scheduler 的具体版本，需以 `kubectl get pods -n kube-system -l component=kube-scheduler -o yaml` 确认镜像 tag]

---

## 5. 容器运行时接口（CRI）相关差异

> **说明**：本仓库 `code/` 目录中的 Kubernetes 快照主要包含 API 类型定义（`pkg/apis/core`），未包含完整的 kubelet CRI 客户端实现，以下 CRI 相关结论部分依据 API 侧可观测字段推断。

| 差异点 | 版本影响 | 诊断影响 |
|-------|---------|---------|
| Dockershim 移除 | Kubernetes 1.24 起 kubelet 不再内置 dockershim | 1.28+ 集群节点运行时为 containerd/CRI-O，`docker ps` 不再适用，应使用 `crictl ps` |
| `RuntimeClassName` GA | 1.20 快照已无 feature gate（1.18 注释标 "beta as of v1.14"） | 多运行时（如 runc/gVisor/Kata）诊断需结合 RuntimeClass |
| 容器 `StopSignal` 上报 | 1.34 快照新增 `.status...stopSignal` | 排查 Terminating 时可确认实际停止信号来源 |

> [存疑：Dockershim 于 1.24 移除属官方公开信息，但本仓库无 1.24 及 kubelet CRI 层源码，"docker/crictl 命令适用性"结论基于运行时生态常识而非本地代码，生产环境请以节点 `crictl info` 与 `kubectl get node -o wide` 的 CONTAINER-RUNTIME 列为准]

---

## 6. 诊断命令与输出格式差异

| 命令 / 行为 | 低版本（≤1.20） | 高版本（≥1.28） | 备注 |
|------------|----------------|----------------|------|
| `kubectl debug`（临时容器） | ❌ 需 alpha gate，多数环境不可用 | ✅ 直接可用 | Ephemeral Containers 1.25 GA |
| `kubectl get pod` STATUS 列出现 `SchedulingGated` | ❌ | ✅（1.28 beta 起） | 诊断 Pending 的新分支 |
| `.status.resize` 字段 | ❌ | 1.28~1.32 可见，1.34 废弃 | 输出解析需按版本分支 |
| `kubectl describe pod` 显示 Resource Claims | ❌ | 1.36 稳定显示 | DRA GA |
| Init 容器显示 `restartPolicy: Always`（Sidecar） | ❌ | 1.28+ 逐步可见 | describe 输出中 init 容器带重启策略 |

> **一致性提示**：技能集 01–04 中的核心诊断命令（`kubectl get/describe/logs/get events`）在 1.18–1.36 全版本通用，输出**主体结构一致**；版本差异主要体现在**新增字段的有无**，不影响既有命令的基本用法。

---

## 7. 面向诊断的版本适配清单

排障前先执行以下命令确定版本，再对照本文选择适配的诊断路径：

```bash
# 🟢 低风险：只读/信息收集
kubectl version -o json | grep -E '"gitVersion"'
kubectl get nodes -o wide          # 查看 CONTAINER-RUNTIME 列
kubectl get pods -n kube-system -l component=kube-scheduler -o jsonpath='{.items[0].spec.containers[0].image}'
```

| 目标集群版本 | 诊断适配要点 |
|------------|------------|
| ≤ 1.24 | 无 `SchedulingGated`；`kubectl debug` 多不可用；节点可能仍为 dockershim |
| 1.25–1.27 | Ephemeral Containers 已 GA；Sidecar/SchedulingGates 处于 alpha/beta，[存疑：具体状态需核实] |
| 1.28–1.32 | `SchedulingGated`、DisruptionTarget、`.status.resize` 可用；DRA/原地扩缩容多为 alpha |
| ≥ 1.33 | Sidecar 容器 GA[存疑：精确 GA 版本待核实]；关注 `.status.resize` 废弃迁移 |
| ≥ 1.34 | 用 `PodResizePending`/`PodResizeInProgress` 替代 `.status.resize`；新增 StopSignal/ContainerRestartRules |
| ≥ 1.36 | DRA `ResourceClaims` 稳定；出现 Gang 调度 `SchedulingGroup`/`PodGroup` |

---

## 相关链接

- [[技能/pod/README.md|Pod 异常诊断技能集]]
- [[技能/pod/01-pod-crashloop-oomkilled.md|CrashLoopBackOff & OOMKilled 诊断]]
- [[技能/pod/02-pod-pending-scheduling.md|Pod Pending 与调度失败诊断]]
- [[技能/pod/03-pod-imagepull-container.md|镜像拉取与容器创建诊断]]
- [[技能/pod/04-pod-sop-runbook.md|Pod 诊断 SOP/Runbook]]
- [[技能/pod/reference/pod-exit-codes.md|容器退出码参考]]
