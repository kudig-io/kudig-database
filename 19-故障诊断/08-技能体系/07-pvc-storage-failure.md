---
title: PVC/PV/CSI 存储故障诊断与修复 / PVC/PV/CSI Storage Troubleshooting & Remediation
description: '# PVC/PV/CSI 存储故障诊断与修复 / PVC/PV/CSI Storage Troubleshooting & Remediation'
summary: 'PVC/PV/CSI 存储问题是 [[Kubernetes|Kubernetes]] 集群中**影响数据持久化和有状态服务**的关键问题类型。当存储子系统出现问题时，Pod 无法启动（卡在 ContainerCreating）、数据无法持久化、甚至可能导致数据丢失。对于 [[StatefulSet|StatefulSet]]、数据库等有状态工作负载，'
category: storage
tags:
- k8s
- skills
- sop
- runbook
- kubelet
- ceph
- mysql
- postgresql
- statefulset
- rbac
tier: core
created: '2026-05-23'
last_updated: '2026-04-26'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- PVC/PV/CSI 存储故障诊断与修复 / PVC/PV/CSI Storage Troubleshooting & Remediation 是什么
- 如何 PVC/PV/CSI 存储故障诊断与修复 / PVC/PV/CSI Storage Troubleshooting & Remediation
trigger_keywords:
- PVC Pending
- PV bound failed
- CSI error
- volume mount failed
- storage provisioning
- disk full
- volume attach timeout
- filesystem error
- storage class not found
- volume expansion failed
- FailedMount
- FailedAttachVolume
- 存储挂载失败
- PVC无法绑定
- 磁盘挂载超时
- 存储扩容失败
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- mysql-basics
- backup-basics
skill_id: SKILL-07_PVC_STORAGE_FAILURE-001
skill_name: PVC/PV/CSI 存储故障诊断与修复 / PVC/PV/CSI Storage Troubleshooting & Remediation
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
agent_execution_mode: L2-semi-auto
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---


# PVC/PV/CSI 存储故障诊断与修复 / PVC/PV/CSI Storage Troubleshooting & Remediation

---

## 1. 概述

PVC/PV/CSI 存储问题是 [[Kubernetes|Kubernetes]] 集群中**影响数据持久化和有状态服务**的关键问题类型。当存储子系统出现问题时，Pod 无法启动（卡在 ContainerCreating）、数据无法持久化、甚至可能导致数据丢失。对于 [[StatefulSet|StatefulSet]]、数据库等有状态工作负载，存储问题往往意味着业务完全中断。

### 典型触发场景

1. **PVC 长期 Pending**: StorageClass 不存在、CSI Provisioner 异常、存储后端容量不足，导致 PVC 无法绑定 PV
2. **Volume 挂载失败**: CSI Node Driver 异常、Volume Attach 超时、文件系统损坏，Pod 卡在 ContainerCreating
3. **存储扩容失败**: Volume 扩容不支持、文件系统扩容失败、云厂商 API 限流，导致应用因空间不足而异常
4. **CSI Driver 问题**: CSI Controller/Node Pod 异常、RBAC 权限不足、存储后端连接失败
5. **Access Mode 冲突**: 多节点同时挂载 RWO Volume、错误的 Access Mode 配置导致调度失败

### 前置条件

- **RBAC 权限**:
  - 最小权限: 对 `persistentvolumeclaims`, `persistentvolumes`, `storageclasses`, `csidrivers`, `csinodes`, `volumeattachments`, `[[Pods|pods]]`, `events` 的 `get/list/watch`
  - 修复权限: `persistentvolumeclaims` 的 `patch/update/delete`, `pods` 的 `delete`
  - 验证命令: `kubectl auth can-i list persistentvolumes`
- **存储后端信息**: StorageClass 配置、CSI Driver 版本、存储后端类型（云盘/NFS/Ceph 等）
- **云厂商凭证**: 深度诊断可能需要云厂商 CLI 工具和凭证（如 aliyun cli、aws cli）
- **工具要求**:
  - `kubectl` >= v1.28（客户端版本建议与集群版本相差不超过 1 个 minor）
  - `jq` >= 1.6（可选）
  - SSH 访问（用于节点级诊断）

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| SP-01 | PVC 状态长期 Pending / PVC status stuck in Pending | `kubectl get pvc -A | grep -v Bound` 持续显示 Pending 超过 5 分钟 | 0.95 | PVC 使用 WaitForFirstConsumer 模式且尚未有 Pod 引用；新创建的 PVC 正在 Provisioning 过程中（<2 分钟） |
| SP-02 | Pod 事件出现 FailedMount / Pod events show FailedMount | `kubectl describe pod <pod> | grep -i "FailedMount|MountVolume"` 显示挂载失败事件 | 0.90 | Volume 首次挂载需要拉取远程数据（如 gitRepo 类型）；临时网络抖动导致的瞬时失败（已自动重试成功） |
| SP-03 | Volume Attach 超时 / Volume attach timeout | Pod 事件显示 `AttachVolume.Attach failed` 或 `timed out waiting for the condition` | 0.85 | 云厂商 API 正在执行中但延迟较高；节点正在启动中，kubelet 尚未就绪 |
| SP-04 | 文件系统挂载失败 / Filesystem mount failed | Pod 事件显示 `MountVolume.SetUp failed` 或 `mount: wrong fs type` | 0.90 | Volume 首次使用需要格式化（正常的 mkfs 过程）；手动指定了错误的 fsType 配置 |
| SP-05 | CSI Driver Pod CrashLoopBackOff | `kubectl get pods -n kube-system -l app.kubernetes.io/component=csi-driver` 显示 CrashLoopBackOff | 0.95 | CSI Driver 正在升级中的短暂重启；集群初始化期间的正常启动抖动 |
| SP-06 | Volume 扩容失败 / Volume expansion failed | PVC 事件显示 `VolumeResizeFailed` 或 `resize of volume failed` | 0.90 | StorageClass 不支持扩容（allowVolumeExpansion: false）；文件系统扩容需要 Pod 重启（离线扩容场景） |
| SP-07 | PV 状态 Released 无法回收 / PV stuck in Released status | `kubectl get pv | grep Released` 持续显示 Released 状态 | 0.80 | ReclaimPolicy 为 Retain 且管理员有意保留数据；正在执行数据备份后的清理操作 |
| SP-08 | 多节点同时挂载 RWO Volume 失败 / Multi-attach error for RWO volume | Pod 事件显示 `Multi-Attach error` 或调度失败原因包含 `volume node affinity conflict` | 0.95 | 使用了 RWX Volume 但配置错误；正在执行 Pod 迁移期间的短暂重叠 |
| SP-09 | 存储空间不足 / Storage disk full | Pod 日志显示 `No space left on device` 或节点 DiskPressure | 0.90 | 容器 ephemeral storage 耗尽（非 PVC 问题）；临时文件导致的短暂满盘已自动清理 |
| SP-10 | Volume detach 卡住 / Volume detach timeout | `kubectl get volumeattachment` 显示 Volume 长时间未释放；节点删除后 Volume 仍处于 Attached 状态 | 0.85 | 节点问题导致的正常 force-detach 流程（需等待 6 分钟超时）；CSI Driver 正在处理中 |
| SP-11 | StorageClass 不存在或 Provisioner 不可用 / StorageClass not found | PVC 事件显示 `storageclass.storage.k8s.io "xxx" not found` 或 `no persistent volumes available` | 0.95 | PVC 指定了静态绑定的 PV 而非动态 Provisioning；集群正在初始化 StorageClass |

### 2.2 工单关键词映射

以下为常见工单描述示例，Agent 应将其映射到本 Skill：

**中文工单描述**:
- "PVC 一直处于 Pending 状态，Pod 无法启动"
- "Pod 卡在 ContainerCreating，提示 volume 挂载失败"
- "存储扩容后 Pod 重启，但容量没有生效"
- "数据库 Pod 无法启动，提示 FailedAttachVolume"
- "CSI 驱动挂了，所有新 Pod 都无法挂载存储"
- "磁盘满了，应用写入报错 no space left"
- "PV 删不掉，一直卡在 Terminating"
- "跨节点迁移 Pod 后存储挂载失败"

**English ticket descriptions**:
- "PVC stuck in Pending status, cannot create pods"
- "Pod stuck in ContainerCreating with FailedMount event"
- "Volume expansion completed but capacity not reflected"
- "StatefulSet pods failing with volume attach timeout"
- "CSI driver pods crashing, all storage operations failing"
- "Disk full error in application logs"
- "PV stuck in Released state, cannot be reused"
- "Multi-attach error when pod rescheduled to different node"

### 2.3 排除标准

以下场景**不适用**本 Skill，Agent 应路由到其他 Skill 或手动处理：

| 排除条件 | 正确路由 | 说明 |
|---------|---------|------|
| PVC 状态 Bound，但 Pod 因其他原因 Pending | SKILL-POD-002 | 非存储问题，可能是调度约束、资源不足等 |
| Node DiskPressure 导致 Pod 被驱逐 | SKILL-NODE-001 | 节点级磁盘压力，非 PVC 存储问题 |
| Pod 内应用无法读写文件（权限问题） | 应用层问题 | SecurityContext 或应用配置问题，非存储子系统问题 |
| 使用 emptyDir/hostPath 的存储问题 | SKILL-NODE-001 | 非 CSI/PVC 相关，属于节点本地存储 |
| 存储后端本身问题（如 Ceph OSD down） | 存储团队 | 超出 Kubernetes 层面，需存储专家介入 |
| 新建集群的 StorageClass 配置问题 | 集群初始化 | 属于集群安装配置范畴 |

---

## 3. 快速分级（2 分钟内完成）

### 3.1 影响评估

按顺序执行以下命令，判断问题爆炸半径：

**Step T1**: 统计异常 PVC 数量和分布
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 获取所有非 Bound 状态的 PVC
echo "=== Non-Bound PVCs ===" && \
kubectl get pvc -A --no-headers | grep -v "Bound" | wc -l && \
kubectl get pvc -A | grep -v "Bound"
```
> **判断规则**:
> - Pending PVC 数量 > 10 且涉及多个 namespace → **P0**（大规模存储问题）
> - Pending PVC 涉及生产 namespace（如 production, prod, default）→ **P1**
> - Pending PVC 仅在测试/开发环境 → **P2**

**Step T2**: 检查 CSI Driver 健康状态
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查所有 CSI 相关 Pod 状态
kubectl get pods -A -l 'app.kubernetes.io/component in (csi-driver,csi-controller,csi-node)' --no-headers 2>/dev/null || \
kubectl get pods -n kube-system | grep -i csi
```
> **判断规则**:
> - CSI Controller Pod 不存在或 CrashLoopBackOff → **P0**（所有动态 Provisioning 失效）
> - CSI Node Pod 部分节点异常 → **P1**（部分节点无法挂载存储）
> - CSI Pod 全部 Running → 继续 T3

**Step T3**: 评估受影响的工作负载
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看因存储问题而 Pending/ContainerCreating 的 Pod
kubectl get pods -A --field-selector=status.phase!=Running,status.phase!=Succeeded --no-headers | \
  while read ns name rest; do
    kubectl get events -n $ns --field-selector involvedObject.name=$name 2>/dev/null | \
    grep -qiE "FailedMount|FailedAttachVolume|ProvisioningFailed" && echo "$ns/$name"
  done
```
> **判断规则**:
> - StatefulSet Pod 受影响（数据库、消息队列等）→ 影响关键有状态服务
> - 多个 Deployment 的 Pod 卡住 → 影响面较广
> - 仅单个 Pod 且非关键服务 → 影响有限

**Step T4**: 检查是否存在数据丢失风险
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 ReclaimPolicy 为 Delete 且 PV 状态异常的情况
kubectl get pv -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,RECLAIM:.spec.persistentVolumeReclaimPolicy | \
  grep -E "Released|Failed" | grep Delete
```
> **判断规则**:
> - 存在 ReclaimPolicy=Delete 且 PV 状态为 Released/Failed → **数据丢失风险，立即升级**
> - 未发现此类情况 → 继续常规诊断

### 3.2 严重性分级

| 条件 | 级别 | 说明 | SLA 要求 |
|------|------|------|---------|
| CSI Controller 完全不可用 **或** 生产数据卷数据丢失风险 **或** >50% PVC Pending | **P0** | 存储子系统全局问题，影响所有新建存储和可能的数据丢失 | 立即响应，15min 内确认根因 |
| 多个 StatefulSet 存储挂载失败 **或** CSI Node 部分节点问题 | **P1** | 部分有状态服务不可用，影响业务连续性 | 15min 内响应，30min 内修复 |
| 单个 PVC Pending/挂载失败 **或** 存储扩容失败但当前容量可用 | **P2** | 单点问题，影响单个应用但不影响整体集群 | 30min 内响应，2h 内修复 |
| PV 回收问题 **或** 非生产环境存储问题 | **P3** | 非紧急问题，不影响当前业务运行 | 4h 内处理 |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过诊断流程，立即升级至人工 SRE / 值班工程师**：

- **数据丢失风险**: PV 状态 Failed 且 ReclaimPolicy=Delete，或存储后端报告数据损坏
- **CSI 全局问题**: 所有 CSI Controller Pod 均不可用，无法执行任何存储操作
- **存储后端不可达**: 存储后端（如 Ceph、云盘服务）完全不可访问
- **级联问题**: PVC Pending 数量在 5 分钟内持续增加
- **生产数据库受影响**: MySQL/PostgreSQL/MongoDB 等核心数据库的存储卷不可用

> **升级消息模板**: 参见 Section 8.2

---

## 4. 诊断工作流

### Phase 1: 快速检查（只读，零风险）

> **目标**: 通过 kubectl 远程收集存储状态信息，无需 SSH 登录节点。所有命令均为只读操作。
> **预计耗时**: 3-5 分钟

**Step D1.1**: 获取 PVC/PV 状态全景
- **命令**:
  ```bash
  # PVC 状态概览
  kubectl get pvc -A -o wide
  
  # PV 状态概览
  kubectl get pv -o wide
  ```
- **超时**: 15s
- **预期输出模式**: 表格输出包含 NAME, NAMESPACE, STATUS, VOLUME, CAPACITY, ACCESS MODES, STORAGECLASS
- **判断规则**:
  - PVC STATUS 为 `Pending` → 记录 PVC 名称和 namespace，继续 D1.2
  - PVC STATUS 为 `Bound` 但 Pod 仍有挂载问题 → 可能是 Volume Attach/Mount 问题，跳转 D1.4
  - PV STATUS 为 `Released` → 可能是 RC-009（ReclaimPolicy 问题），记录 PV 名称
  - PV STATUS 为 `Failed` → 严重问题，优先处理
- **版本差异**: 无

**Step D1.2**: 检查 StorageClass 配置
- **命令**:
  ```bash
  # 列出所有 StorageClass
  kubectl get sc -o wide
  
  # 检查默认 StorageClass
  kubectl get sc -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.annotations.storageclass\.kubernetes\.io/is-default-class}{"\n"}{end}'
  ```
- **超时**: 10s
- **预期输出模式**: StorageClass 列表，包含 PROVISIONER, RECLAIMPOLICY, VOLUMEBINDINGMODE
- **判断规则**:
  - 没有设置 `(default)` 标记且 PVC 未指定 storageClassName → RC-001
  - PVC 指定的 storageClassName 不存在 → RC-001
  - VOLUMEBINDINGMODE 为 `WaitForFirstConsumer` 且 PVC Pending → 检查是否有 Pod 正在引用（可能是正常行为）
  - PROVISIONER 不存在对应的 CSI Driver → RC-002
- **版本差异**: 无

**Step D1.3**: 检查 CSI Driver 状态
- **命令**:
  ```bash
  # 列出已注册的 CSI Driver
  kubectl get csidrivers
  
  # 检查 CSI Controller 和 Node Pod 状态
  kubectl get pods -n kube-system -l 'app in (csi-provisioner,csi-attacher,csi-snapshotter,ebs-csi-controller,disk-csi-controller)' -o wide 2>/dev/null
  kubectl get pods -n kube-system | grep -i csi
  
  # 检查 CSI Node 状态
  kubectl get csinodes -o wide
  ```
- **超时**: 15s
- **预期输出模式**: CSI Driver 列表和 Pod 状态
- **判断规则**:
  - CSI Driver 未注册（csidrivers 列表为空或缺少预期的 driver）→ RC-002
  - CSI Controller Pod 不是 Running 状态 → RC-002
  - CSI Node Pod 部分节点缺失 → RC-002（检查具体节点）
  - CSINode 对象中 allocatable 为空或 maxVolumeLimit 异常 → RC-005
- **版本差异**:
  - **[v1.31+]**: CSIStorageCapacity 默认启用，可用于检查存储容量限制

**Step D1.4**: 收集相关事件
- **命令**:
  ```bash
  # 收集 PVC 相关事件
  kubectl get events -A --sort-by=.lastTimestamp | grep -iE 'pvc|volume|mount|attach|provision|csi' | tail -50
  
  # 针对特定 PVC 的事件
  kubectl describe pvc <pvc-name> -n <namespace>
  ```
- **超时**: 15s
- **预期输出模式**: 事件列表，关注 Warning 类型
- **判断规则**:
  - 出现 `ProvisioningFailed` → 动态供应失败（RC-001/RC-002/RC-003）
  - 出现 `FailedAttachVolume` → Volume Attach 失败（RC-005/RC-006）
  - 出现 `FailedMount` → Volume Mount 失败（RC-007/RC-012）
  - 出现 `VolumeResizeFailed` → 扩容失败（RC-010）
  - 出现 `ExternalProvisioning` → 正在等待外部 Provisioner（检查 CSI 状态）
  - 出现 `WaitForFirstConsumer` → 等待 Pod 调度（正常行为，检查 Pod 状态）
- **版本差异**: 无

**Step D1.5**: 检查 Pod Volume 挂载状态
- **命令**:
  ```bash
  # 获取 Pod 的 Volume 配置和挂载状态
  kubectl describe pod <pod-name> -n <namespace> | grep -A30 "Volumes:"
  
  # 检查 Pod 的 Container 状态
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{range .status.containerStatuses[*]}{.name}: {.state}{"\n"}{end}'
  ```
- **超时**: 10s
- **预期输出模式**: Volume 配置和挂载信息
- **判断规则**:
  - Pod 卡在 `ContainerCreating` + Volume 事件有 FailedMount → 挂载问题
  - Pod 卡在 `Pending` + 事件显示调度失败 with volume constraints → RC-013（Node Affinity 冲突）
  - Container 状态显示 `Waiting` reason=ContainerCreating → 继续检查 Volume 事件
- **版本差异**: 无

**Step D1.6**: 检查 VolumeAttachment 状态
- **命令**:
  ```bash
  # 列出所有 VolumeAttachment
  kubectl get volumeattachment -o custom-columns=NAME:.metadata.name,PV:.spec.source.persistentVolumeName,NODE:.spec.nodeName,ATTACHED:.status.attached
  
  # 检查特定 VolumeAttachment 详情
  kubectl get volumeattachment -o yaml | grep -A5 "attachError|detachError"
  ```
- **超时**: 10s
- **预期输出模式**: VolumeAttachment 列表
- **判断规则**:
  - ATTACHED 为 `false` 且存在时间 > 2 分钟 → Attach 超时（RC-005）
  - 存在 `attachError` → 记录错误信息，可能是 RC-005/RC-006/RC-008
  - 同一 PV 存在多个 VolumeAttachment → Multi-Attach 问题（RC-004）
  - VolumeAttachment 长时间存在但对应的 Pod 已删除 → 可能需要清理（RC-005）
- **版本差异**: 无

---

### Phase 2: 深度诊断（只读，零风险，可能需 SSH）

> **目标**: 深入分析 CSI 组件日志、节点存储状态、存储后端连通性。
> **前提**: 部分命令需要对节点的 SSH 访问权限
> **预计耗时**: 5-15 分钟

**Step D2.1**: 分析 CSI Controller 日志
- **命令**:
  ```bash
  # 查找 CSI Controller Pod
  CSI_CONTROLLER=$(kubectl get pods -n kube-system -l 'app in (csi-provisioner,ebs-csi-controller,disk-csi-controller)' -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)
  
  # 查看 Provisioner 容器日志
  kubectl logs -n kube-system ${CSI_CONTROLLER} -c csi-provisioner --since=30m 2>/dev/null | tail -100
  
  # 查看 Attacher 容器日志
  kubectl logs -n kube-system ${CSI_CONTROLLER} -c csi-attacher --since=30m 2>/dev/null | tail -100
  
  # 查看 CSI 主容器日志
  kubectl logs -n kube-system ${CSI_CONTROLLER} --since=30m 2>/dev/null | grep -iE 'error|fail|timeout' | tail -50
  ```
- **超时**: 20s
- **预期输出模式**: CSI 组件日志
- **判断规则**:
  - 日志包含 `failed to provision volume` → 供应失败，继续查看具体错误
  - 日志包含 `volume capacity` 或 `no capacity` → RC-003（存储后端容量不足）
  - 日志包含 `CreateVolume` + `error` → CSI 后端创建卷失败
  - 日志包含 `timeout` 或 `deadline exceeded` → RC-006（网络/后端响应慢）
  - 日志包含 `authentication` 或 `permission denied` → RC-008（云厂商 API 权限问题）
  - 日志包含 `quota` 或 `limit exceeded` → RC-008（配额耗尽）
  - 日志包含 `not found` + StorageClass → RC-001
- **版本差异**: 无

**Step D2.2**: 检查 kubelet Volume 相关日志
- **命令**:
  ```bash
  # 在问题节点上检查 kubelet 日志
  ssh <node-ip> "journalctl -u kubelet --since '30 minutes ago' --no-pager | grep -iE 'volume|mount|attach|csi' | tail -100"
  ```
- **超时**: 15s
- **预期输出模式**: kubelet 日志条目
- **判断规则**:
  - 日志包含 `MountVolume.SetUp failed` → Mount 阶段失败
  - 日志包含 `MapVolume.MapPodDevice failed` → 块设备映射失败
  - 日志包含 `operation for volume timed out` → CSI Node Driver 响应超时
  - 日志包含 `volume is already exclusively attached` → RC-004（Access Mode 冲突）
  - 日志包含 `fsck` 或 `filesystem` + `error` → RC-007（文件系统问题）
  - 日志包含 `permission denied` → RC-012（SELinux/AppArmor 问题）
- **版本差异**: 无

**Step D2.3**: 检查节点磁盘和挂载状态
- **命令**:
  ```bash
  # 列出块设备
  ssh <node-ip> "lsblk -f"
  
  # 检查磁盘使用情况
  ssh <node-ip> "df -h"
  
  # 检查 Kubernetes 相关挂载
  ssh <node-ip> "mount | grep kubernetes"
  
  # 检查 iSCSI 会话（如果使用 iSCSI）
  ssh <node-ip> "iscsiadm -m session 2>/dev/null || echo 'iSCSI not in use'"
  ```
- **超时**: 15s
- **预期输出模式**: 磁盘和挂载信息
- **判断规则**:
  - 预期的块设备未出现在 lsblk 输出中 → Attach 未完成
  - 磁盘使用率 > 95% → RC-003（节点本地磁盘满影响 CSI 操作）
  - Kubernetes 挂载点显示只读（ro）→ 文件系统错误
  - iSCSI 会话缺失但应该存在 → iSCSI 连接问题
- **版本差异**: 无

**Step D2.4**: 检查存储后端连通性
- **命令**:
  ```bash
  # NFS 后端连通性检查
  ssh <node-ip> "showmount -e <nfs-server-ip> 2>/dev/null || echo 'NFS not available'"
  
  # iSCSI 发现
  ssh <node-ip> "iscsiadm -m discovery -t sendtargets -p <iscsi-server-ip> 2>/dev/null || echo 'iSCSI not available'"
  
  # 云厂商存储服务连通性（示例：阿里云）
  ssh <node-ip> "curl -s --max-time 5 http://100.100.100.200/latest/meta-data/ && echo 'Metadata service OK'"
  
  # 检查 DNS 解析（存储后端域名）
  ssh <node-ip> "nslookup <storage-endpoint> 2>/dev/null || host <storage-endpoint>"
  ```
- **超时**: 20s
- **预期输出模式**: 连通性测试结果
- **判断规则**:
  - NFS showmount 失败 → RC-006（NFS 服务器不可达）
  - iSCSI discovery 失败 → RC-006（iSCSI 网络问题）
  - Metadata service 不可用 → 云环境配置问题
  - DNS 解析失败 → 网络或 DNS 配置问题
- **版本差异**: 无

**Step D2.5**: 检查文件系统一致性
- **命令**:
  ```bash
  # 只读检查文件系统（不修复）
  ssh <node-ip> "fsck -n /dev/<device> 2>&1 || echo 'fsck check completed'"
  
  # 检查文件系统状态
  ssh <node-ip> "tune2fs -l /dev/<device> 2>/dev/null | grep -E 'Filesystem state|Mount count|Check interval'"
  ```
- **超时**: 30s
- **预期输出模式**: 文件系统状态信息
- **判断规则**:
  - fsck 报告需要修复 → RC-007（文件系统损坏）
  - Filesystem state 不是 `clean` → 可能需要 fsck
  - Mount count 超过 Maximum mount count → 建议执行 fsck
- **版本差异**: 无
- **风险级别**: 🟢 低（-n 参数为只读检查）

**Step D2.6**: 检查云厂商 API 状态
- **命令**:
  ```bash
  # 阿里云 ACK 场景
  aliyun ecs DescribeDisks --RegionId <region> --DiskIds '["<disk-id>"]' 2>/dev/null | jq '.Disks.Disk[0].Status'
  
  # AWS EKS 场景
  aws ec2 describe-volumes --volume-ids <volume-id> --query 'Volumes[0].State' 2>/dev/null
  
  # GCP GKE 场景
  gcloud compute disks describe <disk-name> --zone <zone> --format='value(status)' 2>/dev/null
  ```
- **超时**: 30s
- **预期输出模式**: 云盘状态
- **判断规则**:
  - 状态为 `In_use`/`in-use` 但 VolumeAttachment 显示未 attached → 状态不一致
  - 状态为 `Attaching`/`attaching` 超过 5 分钟 → Attach 超时
  - 状态为 `Available`/`available` 但应该已 attached → Attach 失败
  - API 调用失败或 AccessDenied → RC-008（权限或配额问题）
- **版本差异**: 取决于云厂商 API 版本

**Step D2.7**: 检查 VolumeAttachment 对象详情
- **命令**:
  ```bash
  # 获取问题 VolumeAttachment 的完整信息
  kubectl get volumeattachment <va-name> -o yaml
  
  # 检查所有异常的 VolumeAttachment
  kubectl get volumeattachment -o json | jq '.items[] | select(.status.attached != true) | {name: .metadata.name, pv: .spec.source.persistentVolumeName, node: .spec.nodeName, attached: .status.attached, error: .status.attachError}'
  ```
- **超时**: 10s
- **预期输出模式**: VolumeAttachment YAML
- **判断规则**:
  - `attachError.message` 包含具体错误信息 → 记录用于根因分析
  - `attached: false` + `attachTime` 存在很久 → Attach 卡住
  - 多个 VolumeAttachment 指向同一 PV → Multi-Attach 问题
- **版本差异**: 无

**Step D2.8**: 检查 CSI Node Driver 状态
- **命令**:
  ```bash
  # 获取 CSINode 对象详情
  kubectl get csinodes <node-name> -o yaml
  
  # 检查 CSI Node Pod 日志
  kubectl logs -n kube-system <csi-node-pod> -c <csi-driver-container> --since=30m | grep -iE 'error|fail' | tail -50
  ```
- **超时**: 15s
- **预期输出模式**: CSINode 配置和日志
- **判断规则**:
  - CSINode 中 `drivers` 列表为空 → CSI Node Driver 未注册
  - `allocatable` 显示 Volume 数量为 0 → RC-005（节点 Volume 配额问题）
  - 日志包含 `NodePublishVolume failed` → Mount 阶段失败
  - 日志包含 `NodeStageVolume failed` → Stage 阶段失败
- **版本差异**: 无

---

### Phase 3: 主动探测（低风险，可能需审批）

**Step D3.1**: 测试 PVC 创建（Provisioning 验证）
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 创建测试 PVC
  cat <<EOF | kubectl apply -f -
  apiVersion: v1
  kind: PersistentVolumeClaim
  metadata:
    name: test-pvc-diag-$(date +%s)
    namespace: default
  spec:
    accessModes:
      - ReadWriteOnce
    storageClassName: <storage-class-name>
    resources:
      requests:
        storage: 1Gi
  EOF
  
  # 等待并检查状态
  sleep 30
  kubectl get pvc test-pvc-diag-* -o wide
  ```
- **超时**: 60s
- **风险级别**: 🟢 低（仅创建 1Gi 测试 PVC）
- **预期输出模式**: 测试 PVC 状态
- **判断规则**:
  - 测试 PVC 成功 Bound → Provisioner 工作正常，问题在特定 PVC 配置
  - 测试 PVC 同样 Pending → Provisioner 有全局问题
  - 不同 StorageClass 测试结果不同 → 问题在特定 StorageClass 配置
- **版本差异**: 无
- **清理命令**: `kubectl delete pvc test-pvc-diag-*`

**Step D3.2**: 检查存储后端剩余容量
- **命令**:
  ```bash
  # 检查 CSIStorageCapacity（v1.24+ 支持）
  kubectl get csistoragecapacity -A
  
  # 云厂商容量检查（示例：阿里云）
  aliyun ecs DescribeAvailableResource --RegionId <region> --DestinationResource DataDisk
  ```
- **超时**: 30s
- **风险级别**: 🟢 低（只读操作）
- **预期输出模式**: 存储容量信息
- **判断规则**:
  - CSIStorageCapacity 显示 capacity: 0 → RC-003（存储后端无容量）
  - 云厂商 API 显示配额不足 → RC-008
- **版本差异**:
  - **[v1.28+]**: CSIStorageCapacity 默认启用
  - **[v1.31+]**: 支持跨节点容量感知调度

**Step D3.3**: 验证 CSI ServiceAccount 权限
- **命令**:
  ```bash
  # 检查 CSI Controller ServiceAccount 的权限
  kubectl auth can-i --list --as=system:serviceaccount:kube-system:csi-controller-sa -n kube-system | grep -E 'pv|pvc|volumeattachment|csinode|node'
  
  # 检查是否有权限创建 PV
  kubectl auth can-i create persistentvolumes --as=system:serviceaccount:kube-system:csi-controller-sa
  ```
- **超时**: 10s
- **风险级别**: 🟢 低（只读检查）
- **预期输出模式**: 权限列表
- **判断规则**:
  - 缺少必要权限（如 create persistentvolumes）→ RBAC 配置问题
  - 权限正常 → 排除 RBAC 原因
- **版本差异**: 无

**Step D3.4**: 测试 Volume Mount（需要测试 Pod）
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 创建测试 Pod 挂载 Volume
  cat <<EOF | kubectl apply -f -
  apiVersion: v1
  kind: Pod
  metadata:
    name: test-mount-diag-$(date +%s)
    namespace: default
  spec:
    containers:
    - name: test
      image: busybox
      command: ["sleep", "3600"]
      volumeMounts:
      - name: test-vol
        mountPath: /data
    volumes:
    - name: test-vol
      persistentVolumeClaim:
        claimName: <pvc-name>
    restartPolicy: Never
  EOF
  
  # 检查 Pod 事件
  sleep 60
  kubectl describe pod test-mount-diag-* | grep -A20 "Events:"
  ```
- **超时**: 120s
- **风险级别**: 🟡 中（创建测试 Pod 并挂载 Volume）
- **预期输出模式**: Pod 事件和状态
- **判断规则**:
  - Pod 成功 Running → Mount 流程正常
  - Pod 卡在 ContainerCreating + FailedMount 事件 → Mount 问题确认
- **版本差异**: 无
- **清理命令**: `kubectl delete pod test-mount-diag-*`

---

## 5. 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | 确认命令 | 风险档位 | FTA 映射 |
|--------|------|------|---------|---------|---------|---------|
| RC-001 | **StorageClass 不存在或默认 SC 未设置** — PVC 指定的 storageClassName 不存在，或 PVC 未指定 storageClassName 且集群无默认 StorageClass | ~20% | D1.2 显示 SC 不存在或无 default；D1.4 事件显示 `storageclass not found` | `kubectl get sc` 确认 SC 存在性；`kubectl get pvc <name> -o jsonpath='{.spec.storageClassName}'` | 🟢 | storage-fta: BE-sc-missing |
| RC-002 | **CSI Provisioner/Controller Pod 异常** — CSI Controller Pod 不存在、CrashLoopBackOff 或无法连接到存储后端，导致无法执行 Provisioning/Attach 操作 | ~18% | D1.3 显示 CSI Pod 非 Running；D2.1 日志有错误；D1.4 事件长时间 ExternalProvisioning | `kubectl get pods -n kube-system -l app=csi-controller`; `kubectl logs <csi-pod> -c csi-provisioner` | 🟡 | storage-fta: BE-csi-controller-failure |
| RC-003 | **存储后端容量不足** — 云盘/Ceph/NFS 等存储后端剩余容量不足，无法创建新 Volume | ~12% | D2.1 日志包含 `capacity` 或 `no space`；D3.2 显示容量不足 | 云厂商 CLI 查询剩余配额；`kubectl get csistoragecapacity -A` | 🟡 | storage-fta: BE-backend-capacity |
| RC-004 | **Volume Access Mode 不匹配** — 尝试将 RWO Volume 挂载到多个节点，或 Access Mode 配置与实际使用不符 | ~10% | D1.4 事件显示 `Multi-Attach error`；D1.6 同一 PV 多个 VolumeAttachment；D2.2 日志 `already exclusively attached` | `kubectl get volumeattachment -o custom-columns=NAME:.metadata.name,PV:.spec.source.persistentVolumeName,NODE:.spec.nodeName` | 🟢 | storage-fta: BE-access-mode |
| RC-005 | **节点 Volume Attach 达到上限** — 节点上已 Attach 的 Volume 数量达到云厂商或 CSI Driver 限制（如 AWS 最多 39 EBS volumes） | ~8% | D2.8 CSINode 的 allocatable 达到上限；D1.4 事件显示调度失败 with max volume limit | `kubectl get csinodes <node> -o jsonpath='{.spec.drivers[*].allocatable}'`; 计算节点当前 Volume 数量 | 🟡 | storage-fta: BE-attach-limit |
| RC-006 | **网络不可达导致存储后端连接失败** — 防火墙、安全组、网络分区导致节点无法连接到存储后端（NFS server、iSCSI target、云 API） | ~7% | D2.4 连通性测试失败；D2.1/D2.2 日志包含 `connection refused` 或 `timeout` | `ssh <node> "nc -zv <storage-endpoint> <port>"`; `ssh <node> "showmount -e <nfs-server>"` | 🔴 | storage-fta: BE-network-storage |
| RC-007 | **文件系统损坏** — Volume 上的文件系统损坏，无法正常挂载；可能由异常断电、强制 detach 等原因导致 | ~5% | D2.5 fsck 报告错误；D2.2 日志包含 `wrong fs type` 或 `bad superblock`；Mount 失败 | `ssh <node> "fsck -n /dev/<device>"`; `ssh <node> "dmesg | grep -i ext4|xfs"` | 🔴 | storage-fta: BE-fs-corrupt |
| RC-008 | **云厂商 API 限流/配额耗尽** — 云厂商 API 调用频率超限或存储配额用完，无法创建新 Volume 或执行 Attach 操作 | ~5% | D2.1 日志包含 `throttled`、`quota exceeded`、`limit`；D2.6 API 返回错误 | 云厂商配额页面检查；API 返回的错误码分析 | 🟡 | storage-fta: BE-cloud-quota |
| RC-009 | **ReclaimPolicy 误配导致数据残留/丢失** — PV 的 ReclaimPolicy 配置不当，Delete 导致数据丢失，Retain 导致 PV 无法重用 | ~4% | D1.1 PV 状态为 Released 且 ReclaimPolicy 为 Retain；PVC 删除后数据意外删除 | `kubectl get pv -o custom-columns=NAME:.metadata.name,RECLAIM:.spec.persistentVolumeReclaimPolicy,STATUS:.status.phase` | ⚫ | storage-fta: BE-reclaim-policy |
| RC-010 | **Volume 扩容不支持或文件系统扩容失败** — StorageClass 未开启 allowVolumeExpansion，或文件系统在线扩容失败 | ~4% | D1.4 事件显示 VolumeResizeFailed；SC 配置 allowVolumeExpansion: false；D2.2 日志 resize 错误 | `kubectl get sc <name> -o jsonpath='{.allowVolumeExpansion}'`; `kubectl describe pvc <name>` 查看扩容事件 | 🟡 | storage-fta: BE-expansion-failed |
| RC-011 | **CSI Driver 版本不兼容** — CSI Driver 版本与 Kubernetes 版本或存储后端版本不兼容，导致功能异常 | ~3% | D2.1 日志包含版本相关错误；CSI Driver sidecar 版本与 K8s 不匹配 | 对比 CSI Driver release notes 与当前 K8s 版本；检查 CSI spec version | 🟡 | storage-fta: BE-csi-version |
| RC-012 | **SELinux/AppArmor 挂载权限拒绝** — 安全模块阻止了 Volume 的挂载或访问操作 | ~2% | D2.2 日志包含 `permission denied`、`selinux`、`apparmor`；Mount 失败但 Volume 本身正常 | `ssh <node> "getenforce"` 或 `ssh <node> "aa-status"`; `ssh <node> "ausearch -m avc"` | 🟡 | storage-fta: BE-security-module |
| RC-013 | **PV Node Affinity 与 Pod 调度约束冲突** — PV 绑定到特定节点（如 local PV、Zone 限制），但 Pod 无法调度到该节点 | ~2% | D1.4 事件显示调度失败 `volume node affinity conflict`；D1.5 Pod Pending with volume constraints | `kubectl get pv <name> -o jsonpath='{.spec.nodeAffinity}'`; `kubectl describe pod <name>` 查看调度原因 | 🟢 | storage-fta: BE-node-affinity |

---

## 6. 修复操作

### 6.1 🟢 低风险（Agent 可建议自动执行）

#### REM-001: 设置默认 StorageClass
- **适用根因**: RC-001
- **前置检查**:
  ```bash
  # 确认没有默认 StorageClass
  kubectl get sc -o jsonpath='{range .items[*]}{.metadata.name}: {.metadata.annotations.storageclass\.kubernetes\.io/is-default-class}{"\n"}{end}' | grep true
  # 预期: 无输出（没有 default SC）
  
  # 确认目标 StorageClass 存在
  kubectl get sc <storage-class-name>
  # 预期: 存在
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 设置默认 StorageClass
  kubectl patch storageclass <storage-class-name> -p '{"metadata": {"annotations":{"storageclass.kubernetes.io/is-default-class":"true"}}}'
  ```
- **后置验证**:
  ```bash
  kubectl get sc
  # 预期: 目标 SC 显示 (default) 标记
  
  # 验证 Pending PVC 是否开始 Provisioning
  kubectl get pvc -A | grep Pending
  # 预期: Pending PVC 数量减少或消失
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  kubectl patch storageclass <storage-class-name> -p '{"metadata": {"annotations":{"storageclass.kubernetes.io/is-default-class":"false"}}}'
  ```

#### REM-002: 修正 PVC 的 Access Mode 配置
- **适用根因**: RC-004
- **前置检查**:
  ```bash
  # 确认当前 Access Mode
  kubectl get pvc <pvc-name> -n <namespace> -o jsonpath='{.spec.accessModes}'
  
  # 确认需要的 Access Mode（根据实际使用场景）
  # RWO: 单节点读写
  # RWX: 多节点读写
  # ROX: 多节点只读
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 注意: Access Mode 不能直接修改，需要重建 PVC
  # Step 1: 备份 PVC 配置
  kubectl get pvc <pvc-name> -n <namespace> -o yaml > pvc-backup.yaml
  
  # Step 2: 删除旧 PVC（确保 Pod 已停止引用）
  kubectl delete pvc <pvc-name> -n <namespace>
  
  # Step 3: 修改 accessModes 并重新创建
  # 编辑 pvc-backup.yaml 中的 accessModes
  kubectl apply -f pvc-backup.yaml
  ```
- **后置验证**:
  ```bash
  kubectl get pvc <pvc-name> -n <namespace> -o jsonpath='{.spec.accessModes}'
  # 预期: 显示新的 Access Mode
  
  kubectl get pvc <pvc-name> -n <namespace>
  # 预期: STATUS 为 Bound
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 恢复原始配置
  kubectl apply -f pvc-backup.yaml.original
  ```

#### REM-003: 为 PVC 指定正确的 StorageClass
- **适用根因**: RC-001
- **前置检查**:
  ```bash
  # 列出可用的 StorageClass
  kubectl get sc
  
  # 检查 PVC 当前的 storageClassName
  kubectl get pvc <pvc-name> -n <namespace> -o jsonpath='{.spec.storageClassName}'
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 导出 PVC 配置
  kubectl get pvc <pvc-name> -n <namespace> -o yaml > pvc-fix.yaml
  
  # 编辑 storageClassName（需要删除重建）
  # 1. 修改 pvc-fix.yaml 中的 spec.storageClassName
  # 2. 删除 resourceVersion, uid, status 等字段
  
  # 删除旧 PVC 并重建
  kubectl delete pvc <pvc-name> -n <namespace>
  kubectl apply -f pvc-fix.yaml
  ```
- **后置验证**:
  ```bash
  kubectl get pvc <pvc-name> -n <namespace>
  # 预期: STATUS 为 Bound
  ```
- **回滚命令**:
  ```bash
  # 恢复原始 PVC 配置
  ```

---

### 6.2 🟡 中风险（Agent 建议，人工审批后执行）

#### REM-004: 重启 CSI Controller Pod
- **适用根因**: RC-002
- **影响说明**: 重启 CSI Controller 会导致正在进行的 Provisioning/Attach 操作中断并重试。如果有大量 Pending PVC，重启后会触发批量重试。
- **审批提示**: "建议重启 CSI Controller Pod 以恢复存储操作能力。该操作会中断正在进行的存储操作，但通常会自动重试。是否批准？"
- **前置检查**:
  ```bash
  # 确认 CSI Controller Pod 状态异常
  kubectl get pods -n kube-system -l 'app in (csi-provisioner,ebs-csi-controller,disk-csi-controller,csi-controller)'
  
  # 记录当前 Pending PVC 数量
  kubectl get pvc -A | grep -c Pending
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 重启 CSI Controller Pod
  kubectl delete pod -n kube-system -l 'app in (csi-provisioner,ebs-csi-controller,disk-csi-controller,csi-controller)'
  
  # 等待新 Pod 就绪
  kubectl wait --for=condition=Ready pod -n kube-system -l 'app in (csi-provisioner,ebs-csi-controller,disk-csi-controller,csi-controller)' --timeout=120s
  ```
- **后置验证**:
  ```bash
  # 检查 CSI Controller Pod 状态
  kubectl get pods -n kube-system -l 'app in (csi-provisioner,ebs-csi-controller,disk-csi-controller,csi-controller)'
  # 预期: 所有 Pod Running
  
  # 等待 30s 后检查 PVC 状态
  sleep 30
  kubectl get pvc -A | grep Pending
  # 预期: Pending PVC 减少
  ```
- **回滚命令**:
  ```bash
  # 如果重启后问题恶化，检查 CSI Driver 配置和版本
  # Pod 重启是幂等操作，无需显式回滚
  ```

#### REM-005: 清理 Orphaned VolumeAttachment
- **适用根因**: RC-004, RC-005
- **影响说明**: 清理孤儿 VolumeAttachment 对象。这些对象的对应 Pod 已不存在，但 VolumeAttachment 仍保留，阻止 Volume 被其他 Pod 使用。
- **审批提示**: "发现孤儿 VolumeAttachment 对象，建议清理以释放 Volume。请确认对应的 Pod 已不需要该 Volume。是否批准清理？"
- **前置检查**:
  ```bash
  # 找出孤儿 VolumeAttachment
  kubectl get volumeattachment -o json | jq -r '.items[] | select(.status.attached == true) | .metadata.name' | while read va; do
    NODE=$(kubectl get volumeattachment $va -o jsonpath='{.spec.nodeName}')
    PV=$(kubectl get volumeattachment $va -o jsonpath='{.spec.source.persistentVolumeName}')
    # 检查是否有 Pod 正在使用
    PODS=$(kubectl get pods -A -o json | jq -r --arg pv "$PV" '.items[] | select(.spec.volumes[].persistentVolumeClaim.claimName != null) | select(.spec.nodeName == "'$NODE'") | .metadata.name')
    if [ -z "$PODS" ]; then
      echo "Orphaned: $va (PV: $PV, Node: $NODE)"
    fi
  done
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 删除孤儿 VolumeAttachment
  kubectl delete volumeattachment <orphaned-va-name>
  ```
- **后置验证**:
  ```bash
  # 确认 VolumeAttachment 已删除
  kubectl get volumeattachment <orphaned-va-name>
  # 预期: NotFound
  
  # 确认 PV 状态恢复
  kubectl get pv <pv-name>
  # 预期: STATUS 为 Available 或 Bound（取决于 PVC 状态）
  ```
- **回滚命令**:
  ```bash
  # VolumeAttachment 删除后无法直接恢复
  # 如需重新 Attach，创建新 Pod 引用对应 PVC 即可触发
  ```

#### REM-006: 存储后端扩容/清理
- **适用根因**: RC-003
- **影响说明**: 在存储后端执行扩容或清理操作以释放容量。具体操作取决于存储类型。
- **审批提示**: "存储后端容量不足，需要扩容或清理。请确认扩容预算或清理范围。是否批准？"
- **前置检查**:
  ```bash
  # 检查当前使用的 PV 容量分布
  kubectl get pv -o custom-columns=NAME:.metadata.name,CAPACITY:.spec.capacity.storage,SC:.spec.storageClassName --sort-by=.spec.capacity.storage
  
  # 云厂商容量检查（以阿里云为例）
  aliyun ecs DescribeDisks --RegionId <region> --Status In_use | jq '.TotalCount'
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # === 阿里云 ACK 场景 ===
  # 方案1: 扩容云盘配额（联系阿里云提工单）
  
  # 方案2: 清理未使用的 PV/PVC
  # 列出未使用的 PV
  kubectl get pv -o jsonpath='{range .items[?(@.status.phase=="Released")]}{.metadata.name}{"\n"}{end}'
  
  # 删除未使用的 PV（确认数据不需要后）
  kubectl delete pv <released-pv-name>
  
  # === NFS 场景 ===
  # 在 NFS 服务器上清理旧数据或扩容存储
  ssh <nfs-server> "df -h /nfs/data && du -sh /nfs/data/*"
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 验证容量已释放
  kubectl get csistoragecapacity -A
  # 或云厂商 API 查询
  
  # 测试创建新 PVC
  kubectl apply -f test-pvc.yaml
  kubectl get pvc test-pvc
  # 预期: Bound
  ```
- **回滚命令**:
  ```bash
  # 扩容操作通常无需回滚
  # 如果误删 PV，需要从备份恢复数据
  ```

#### REM-007: Volume 在线/离线扩容
- **适用根因**: RC-010
- **影响说明**: 扩展 PVC 容量。支持在线扩容的存储（如大多数云盘）无需停止 Pod；部分存储需要离线扩容（删除 Pod 后扩容）。
- **审批提示**: "建议将 PVC 容量从 X 扩容到 Y。请确认存储类型支持扩容。是否批准？"
- **前置检查**:
  ```bash
  # 检查 StorageClass 是否支持扩容
  kubectl get sc <storage-class> -o jsonpath='{.allowVolumeExpansion}'
  # 预期: true
  
  # 检查当前 PVC 容量
  kubectl get pvc <pvc-name> -n <namespace> -o jsonpath='{.spec.resources.requests.storage}'
  ```
- **执行命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl scale --replicas=0`：缩容到 0，立即停服
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 在线扩容（大多数云盘支持）
  kubectl patch pvc <pvc-name> -n <namespace> -p '{"spec":{"resources":{"requests":{"storage":"<new-size>"}}}}'
  
  # 如果需要离线扩容（文件系统不支持在线 resize）
  # Step 1: 缩容 Deployment/StatefulSet 的 replicas 为 0
  kubectl scale deployment <name> -n <namespace> --replicas=0
  
  # Step 2: 执行扩容
  kubectl patch pvc <pvc-name> -n <namespace> -p '{"spec":{"resources":{"requests":{"storage":"<new-size>"}}}}'
  
  # Step 3: 等待扩容完成
  kubectl get pvc <pvc-name> -n <namespace> -w
  
  # Step 4: 恢复 replicas
  kubectl scale deployment <name> -n <namespace> --replicas=<original-count>
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 检查 PVC 容量
  kubectl get pvc <pvc-name> -n <namespace> -o jsonpath='{.status.capacity.storage}'
  # 预期: 显示新容量
  
  # 进入 Pod 检查实际可用空间
  kubectl exec -n <namespace> <pod-name> -- df -h /data
  # 预期: 显示扩容后的容量
  ```
- **回滚命令**:
  ```bash
  # PVC 扩容后无法缩容（大多数存储不支持）
  # 如需缩容，需要创建新 PVC 并迁移数据
  ```

---

### 6.3 🔴 高风险（Agent 仅提供指导，人工执行）

#### REM-008: 文件系统修复（fsck）
- **适用根因**: RC-007
- **影响说明**: 对损坏的文件系统执行 fsck 修复。**此操作需要 Volume 处于未挂载状态**，可能导致数据丢失或修改。必须先备份数据（如可能）。
- **操作步骤**:
  1. **确保 Volume 未被挂载**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl scale --replicas=0`：缩容到 0，立即停服
> - `kubectl delete`：删除资源（可由声明式清单重建）

     ```bash
     # 停止使用该 Volume 的所有 Pod
     kubectl scale deployment <name> -n <namespace> --replicas=0
     # 或删除 Pod
     kubectl delete pod <pod-name> -n <namespace>
     
     # 等待 Pod 终止
     kubectl get pods -n <namespace> -l <label-selector>
     ```
  2. **在节点上 detach Volume（如果 VolumeAttachment 仍存在）**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

     ```bash
     # 删除 VolumeAttachment 触发 detach
     kubectl delete volumeattachment <va-name>
     ```
  3. **手动挂载 Volume 进行 fsck（需要临时 attach 到某个节点）**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

     ```bash
     # 创建临时 Pod 进行修复（使用特权模式）
     cat <<EOF | kubectl apply -f -
     apiVersion: v1
     kind: Pod
     metadata:
       name: fsck-repair-pod
       namespace: default
     spec:
       nodeName: <target-node>
       containers:
       - name: fsck
         image: alpine
         command: ["sleep", "infinity"]
         securityContext:
           privileged: true
         volumeDevices:
         - name: block-vol
           devicePath: /dev/repair-vol
       volumes:
       - name: block-vol
         persistentVolumeClaim:
           claimName: <pvc-name>
     EOF
     
     # 进入 Pod 执行 fsck
     kubectl exec -it fsck-repair-pod -- sh
     # 在容器内执行:
     # fsck -y /dev/repair-vol
     ```
  4. **删除修复 Pod 并恢复正常使用**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

     ```bash
     kubectl delete pod fsck-repair-pod
     kubectl scale deployment <name> -n <namespace> --replicas=<count>
     ```
- **安全检查**:
  - 确认有最新的数据备份
  - 记录 fsck 执行前后的文件系统状态
  - 评估数据丢失风险
- **回滚方案**:
  - 如果 fsck 导致数据丢失，从备份恢复
  - 保留 fsck 执行日志用于事后分析

#### REM-009: 强制 Detach 并重新 Attach Volume
- **适用根因**: RC-005, RC-006
- **影响说明**: 强制 detach 一个卡住的 Volume 并重新 attach。可能导致使用该 Volume 的应用数据不一致。
- **操作步骤**:
  1. **确认 Volume 需要强制 detach**:
     ```bash
     kubectl get volumeattachment -o json | jq '.items[] | select(.status.attached == true) | {name: .metadata.name, age: .metadata.creationTimestamp}'
     # 确认 VolumeAttachment 存在很久但对应 Pod 已不存在
     ```
  2. **强制删除 VolumeAttachment**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源

     ```bash
     # 添加 finalizer 清理
     kubectl patch volumeattachment <va-name> -p '{"metadata":{"finalizers":null}}' --type=merge
     
     # 删除 VolumeAttachment
     kubectl delete volumeattachment <va-name> --grace-period=0 --force
     ```
  3. **在云厂商层面确认 detach（如需要）**:
     ```bash
     # 阿里云
     aliyun ecs DetachDisk --DiskId <disk-id>
     
     # AWS
     aws ec2 detach-volume --volume-id <vol-id> --force
     ```
  4. **等待 Volume 状态恢复后重新使用**:
     ```bash
     # 检查 PV 状态
     kubectl get pv <pv-name>
     # 预期: Available
     
     # 创建新 Pod 触发重新 Attach
     ```
- **安全检查**:
  - 确认没有应用正在写入该 Volume
  - 记录强制 detach 的时间和原因
  - 评估数据一致性风险
- **回滚方案**:
  - 强制 detach 后无法回滚
  - 如数据不一致，需要从应用层面修复或恢复备份

---

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-010: PV 数据恢复（从快照/备份）
- **适用根因**: RC-007, RC-009
- **审批要求**: 需要高级 SRE + 数据负责人审批
- **数据备份**: 确认有可用的快照或备份
- **操作步骤**:
  1. **评估数据恢复方案**:
     ```bash
     # 检查是否有 VolumeSnapshot
     kubectl get volumesnapshot -A
     
     # 云厂商快照检查（以阿里云为例）
     aliyun ecs DescribeSnapshots --RegionId <region> --DiskId <disk-id>
     ```
  2. **从快照创建新 PV**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

     ```bash
     # 使用 VolumeSnapshot 恢复
     cat <<EOF | kubectl apply -f -
     apiVersion: v1
     kind: PersistentVolumeClaim
     metadata:
       name: restored-pvc
       namespace: <namespace>
     spec:
       dataSource:
         name: <snapshot-name>
         kind: VolumeSnapshot
         apiGroup: snapshot.storage.k8s.io
       accessModes:
         - ReadWriteOnce
       storageClassName: <storage-class>
       resources:
         requests:
           storage: <size>
     EOF
     ```
  3. **验证恢复的数据**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

     ```bash
     # 创建临时 Pod 挂载恢复的 PVC
     kubectl run verify-restore --image=busybox --restart=Never -- sleep 3600
     kubectl exec -it verify-restore -- ls -la /data
     ```
  4. **切换应用到恢复的 PVC**:
     ```bash
     # 更新应用配置使用新的 PVC
     # 删除旧的损坏 PVC
     ```
- **回滚方案**:
  - 保留原始损坏的 PV，不立即删除
  - 如恢复的数据不完整，可以尝试其他快照版本

#### REM-011: ReclaimPolicy 变更与数据迁移
- **适用根因**: RC-009
- **审批要求**: 需要高级 SRE + 应用负责人审批
- **数据备份**: 操作前必须完成数据备份
- **操作步骤**:
  1. **修改现有 PV 的 ReclaimPolicy**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

     ```bash
     # 将 Delete 改为 Retain（保护数据）
     kubectl patch pv <pv-name> -p '{"spec":{"persistentVolumeReclaimPolicy":"Retain"}}'
     
     # 验证修改
     kubectl get pv <pv-name> -o jsonpath='{.spec.persistentVolumeReclaimPolicy}'
     ```
  2. **处理 Released 状态的 PV**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

     ```bash
     # 方案1: 清除 claimRef 使 PV 可重新绑定
     kubectl patch pv <pv-name> --type json -p '[{"op": "remove", "path": "/spec/claimRef"}]'
     
     # 方案2: 创建新 PV 并迁移数据（如果需要更改配置）
     ```
  3. **数据迁移（如需要）**:
     ```bash
     # 使用 rsync 或 velero 进行数据迁移
     # 创建临时 Pod 同时挂载旧 PVC 和新 PVC
     # 执行数据复制
     ```
- **回滚方案**:
  - ReclaimPolicy 修改是可逆的
  - 数据迁移前确保有完整备份

---

## 7. 验证确认

### 7.1 即时验证（修复后 1-2 分钟内）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# V1: 确认 PVC 状态恢复为 Bound
kubectl get pvc <pvc-name> -n <namespace>
# 预期: STATUS 列显示 Bound

# V2: 确认 Pod 成功挂载 Volume 并运行
kubectl get pod <pod-name> -n <namespace>
# 预期: STATUS 为 Running

# V3: 确认无 FailedMount/FailedAttachVolume 事件
kubectl get events -n <namespace> --field-selector involvedObject.name=<pod-name> --sort-by=.lastTimestamp | tail -5
# 预期: 无 Warning 事件，或最新事件为 Normal 类型

# V4: 确认 VolumeAttachment 状态正常
kubectl get volumeattachment -o json | jq '.items[] | select(.spec.source.persistentVolumeName == "<pv-name>") | {name: .metadata.name, attached: .status.attached}'
# 预期: attached: true
```
### 7.2 短期监控（5-30 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| PVC 状态 | `kube_persistentvolumeclaim_status_phase{phase="Bound"}` | 所有业务 PVC 保持 Bound | 新增 Pending PVC |
| Volume 可用空间 | `kubelet_volume_stats_available_bytes` | 稳定或缓慢下降 | 可用空间 < 10% 且快速下降 |
| CSI 操作延迟 | `csi_operations_seconds_bucket` | P99 < 30s | P99 > 60s |
| CSI Pod 状态 | `kubectl get pods -n kube-system -l app=csi-controller` | 所有 Pod Running | 有 Pod 非 Running 状态 |
| Mount 事件 | `kubectl get events -A --field-selector reason=FailedMount` | 无新事件 | 新增 FailedMount 事件 |
| Pod 启动成功率 | `kubectl get pods -A --field-selector status.phase=Running` | Pod 正常启动 | 新 Pod 卡在 ContainerCreating |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认问题已解决：

- [ ] 所有目标 PVC 状态为 Bound，且持续 Bound 超过 5 分钟
- [ ] 使用该 PVC 的 Pod 状态为 Running
- [ ] CSI Controller 和 Node Pod 全部 Running 且无重启
- [ ] 无新增 FailedMount/FailedAttachVolume 事件
- [ ] Volume 读写测试通过（见 V5）
- [ ] 根因已明确记录并采取了预防措施

### 7.4 读写测试（建议执行）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# V5: 读写测试
kubectl exec -n <namespace> <pod-name> -- dd if=/dev/zero of=/data/test-file bs=1M count=100 conv=fsync
# 预期: 成功写入 100MB 文件

kubectl exec -n <namespace> <pod-name> -- dd if=/data/test-file of=/dev/null bs=1M
# 预期: 成功读取

# 清理测试文件
kubectl exec -n <namespace> <pod-name> -- rm /data/test-file
```
### 7.5 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| PVC 状态稳定性 | `kube_persistentvolumeclaim_status_phase` 监控 | 持续 | 如果再次 Pending → 重新进入本 Skill 诊断流程 |
| Volume 空间趋势 | `kubelet_volume_stats_available_bytes` 趋势图 | 每小时 | 空间快速下降 → 设置容量告警 |
| CSI Pod 重启 | CSI Pod 重启计数 | 每 4 小时 | 24h 内重启 >2 次 → 深度排查 CSI 问题 |
| 新 PVC Provisioning | 新建 PVC 是否正常 Bound | 每次部署 | 新 PVC Pending → 检查 CSI 状态 |
| 云厂商配额 | 云厂商配额监控 | 每日 | 配额使用 >80% → 提前扩容 |

---

## 8. 升级协议

### 8.1 自动升级条件

| 条件 | 说明 | 触发时机 |
|------|------|---------|
| **诊断超时** | 诊断工作流执行超过 **15 分钟**未能确认根因 | Phase 2 结束后仍无明确根因 |
| **修复失败** | 同一修复操作执行 **2 次**仍未通过后置验证 | REM-xxx 执行后验证失败 |
| **数据丢失风险** | 发现 ReclaimPolicy=Delete 的 PV 状态异常 | 任何诊断步骤发现此情况 |
| **CSI 全局问题** | 所有 CSI Controller Pod 不可用 | D1.3 检查发现 |
| **多个 StatefulSet 受影响** | 多个有状态服务的存储卷不可用 | T3 评估发现 |
| **未知根因** | 完成 Phase 1-3 但无法匹配任何已知根因 | 所有诊断步骤均无明确异常 |

### 8.2 升级消息模板

```
【{severity}】PVC/PV/CSI 存储问题 - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: {affected_pvc_count} 个 PVC 状态异常，{affected_pod_count} 个 Pod 无法正常运行
- 存储类型: {storage_class} (Provisioner: {provisioner})
- 影响范围:
  - 受影响 Namespace: {affected_namespaces}
  - 涉及 StatefulSet: {affected_statefulsets}
  - 数据丢失风险: {data_loss_risk}
- 已完成诊断:
  - Phase 1 快速检查: {phase1_summary}
  - Phase 2 深度诊断: {phase2_summary}
  - Phase 3 主动探测: {phase3_summary}
- 初步发现:
  - 可能根因: {suspected_root_cause} ({root_cause_id})
  - 关键证据: {key_evidence}
- 已尝试修复:
  - {attempted_remediation} → 结果: {remediation_result}
- 需要: {action_needed}
- 工单编号: {ticket_id}
- Skill 版本: SKILL-STORE-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### 8.3 交接信息包

升级时，Agent 需准备以下完整信息供人工接手：

1. **完整诊断路径**: 按时间顺序列出已执行的每个诊断步骤及输出摘要
2. **已排除的根因**: 列出已排除的根因及排除依据
3. **关键资源快照**:
   ```bash
   # PVC 详情
   kubectl describe pvc <pvc-name> -n <namespace> > pvc-describe.txt
   # PV 详情
   kubectl get pv -o yaml > pv-list.yaml
   # CSI 状态
   kubectl get csidrivers,csinodes -o yaml > csi-status.yaml
   # VolumeAttachment 状态
   kubectl get volumeattachment -o yaml > va-list.yaml
   # 相关事件
   kubectl get events -A --sort-by=.lastTimestamp | grep -iE 'volume|mount|attach|provision' > events.txt
   # CSI 日志
   kubectl logs -n kube-system <csi-controller-pod> --all-containers > csi-logs.txt
   ```
4. **事件时间线**: 关键事件按时间排列
5. **云厂商状态截图**: 如涉及云存储，附上云厂商控制台相关截图

---

## 9. K8s 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| CSI Volume Expansion | GA | GA | GA | GA | GA |
| CSI Volume Cloning | GA | GA | GA | GA | GA |
| CSI Snapshot | GA | GA | GA | GA | GA |
| CSI Storage Capacity | GA | GA | GA | GA（增强） | GA |
| ReadWriteOncePod Access Mode | beta | GA | GA | GA | GA |
| Volume Group Snapshot | - | alpha | alpha | beta | beta |
| VolumeAttributesClass | - | - | alpha | beta | beta |
| SELinux Mount Option | beta | beta | GA | GA | GA |
| Cross-namespace VolumeSnapshot | - | - | alpha | alpha | beta |
| Persistent Volume Last Phase Transition | - | beta | GA | GA | GA |
| CSI Node Expand Secret | beta | GA | GA | GA | GA |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubectl get csistoragecapacity` | 支持 | 支持 | 支持 | 增强输出 | 增强输出 |
| `kubectl get volumeattachment` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl describe pvc` (resize 状态) | 基础 | 增强 | 增强 | 增强 | 增强 |
| `kubectl get pv -o wide` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl get volumesnapshot` | 支持 | 支持 | 支持 | 支持（Group 支持） | 支持 |

### 9.3 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| PersistentVolume | v1 (core) | v1 | v1 | v1 | v1 |
| PersistentVolumeClaim | v1 (core) | v1 | v1 | v1 | v1 |
| StorageClass | storage.k8s.io/v1 | v1 | v1 | v1 | v1 |
| CSIDriver | storage.k8s.io/v1 | v1 | v1 | v1 | v1 |
| CSINode | storage.k8s.io/v1 | v1 | v1 | v1 | v1 |
| VolumeAttachment | storage.k8s.io/v1 | v1 | v1 | v1 | v1 |
| CSIStorageCapacity | storage.k8s.io/v1 | v1 | v1 | v1 | v1 |
| VolumeSnapshot | snapshot.storage.k8s.io/v1 | v1 | v1 | v1 | v1 |
| VolumeGroupSnapshot | - | snapshot.storage.k8s.io/v1alpha1 | v1alpha1 | v1beta1 | v1beta1 |
| VolumeAttributesClass | - | - | storage.k8s.io/v1alpha1 | v1beta1 | v1beta1 |

### 9.4 版本相关的诊断注意事项

- **[v1.29+]**: ReadWriteOncePod (RWOP) Access Mode GA
  - 新增的 Access Mode，允许 Volume 仅挂载到单个 Pod
  - 诊断时需检查 PVC 是否使用了 RWOP 且与调度冲突
  - 命令: `kubectl get pvc <name> -o jsonpath='{.spec.accessModes}'`

- **[v1.30+]**: SELinux Mount Option GA
  - 支持在 Volume 挂载时自动设置 SELinux 标签
  - 相关问题诊断: 检查 `securityContext.seLinuxOptions`
  - 如果 Pod 无法访问 Volume 文件，可能是 SELinux 标签问题

- **[v1.31+]**: Volume Group Snapshot Beta
  - 支持对多个 Volume 创建一致性快照
  - 新增 VolumeGroupSnapshot 资源
  - 恢复数据时可以使用 Group Snapshot 保证一致性

- **[v1.31+]**: VolumeAttributesClass Beta
  - 允许修改已创建 Volume 的属性（如 IOPS、吞吐量）
  - 诊断存储性能问题时可检查 VolumeAttributesClass 配置
  - 命令: `kubectl get volumeattributesclass`

### 9.5 云厂商特异性

#### 阿里云 ACK
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# CSI Driver 版本检查
kubectl get pods -n kube-system -l app=csi-plugin -o jsonpath='{.items[0].spec.containers[0].image}'

# 云盘状态检查
aliyun ecs DescribeDisks --RegionId <region> --DiskIds '["<disk-id>"]'

# 常见问题:
# - 云盘类型限制（ESSD 必须与 ECS 实例类型匹配）
# - 云盘挂载数量限制（基础型 16 个，企业型 64 个）
# - 跨可用区限制（云盘必须与节点在同一可用区）
```
#### AWS EKS
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# EBS CSI Driver 版本检查
kubectl get pods -n kube-system -l app=ebs-csi-controller -o jsonpath='{.items[0].spec.containers[0].image}'

# EBS Volume 状态检查
aws ec2 describe-volumes --volume-ids <vol-id>

# 常见问题:
# - EBS Volume 挂载限制（默认每实例 39 个）
# - Nitro 实例 vs 非 Nitro 实例的 device name 差异
# - gp2 vs gp3 的 IOPS 限制
```
#### GCP GKE
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# GCE PD CSI Driver 版本检查
kubectl get pods -n kube-system -l k8s-app=gce-pd-csi-driver -o jsonpath='{.items[0].spec.containers[0].image}'

# Persistent Disk 状态检查
gcloud compute disks describe <disk-name> --zone <zone>

# 常见问题:
# - Regional PD 必须在两个 Zone 有副本
# - Zonal PD 只能挂载到同一 Zone 的节点
# - 并发挂载限制
```
---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| **将 StorageClass 缺失误判为 CSI 问题** | PVC Pending，查看 CSI Pod 全部 Running | PVC 指定了不存在的 StorageClass，或未指定 SC 且无默认 SC | 先执行 D1.2 检查 StorageClass，确认 PVC 指定的 SC 存在且有 default |
| **将 Node Affinity 约束误判为容量不足** | PVC Pending，事件显示 "no persistent volumes available" | PV 的 nodeAffinity 与 Pod 调度约束冲突，不是没有容量 | 检查 PV 的 `spec.nodeAffinity`，确认 Pod 可以调度到 PV 绑定的节点 |
| **将 WaitForFirstConsumer 误判为 Provisioning 失败** | PVC 长期 Pending，事件显示 "waiting for first consumer" | 这是正常行为，StorageClass 配置了延迟绑定 | 检查是否有 Pod 引用该 PVC；如果有 Pod，再排查为什么 Pod 未被调度 |
| **将 Access Mode 冲突误判为 CSI Node 问题** | Pod 挂载失败，错误 "volume is already exclusively attached" | RWO Volume 尝试挂载到多个节点，不是 CSI 问题 | 检查 VolumeAttachment 和 Pod 分布，确认 Access Mode 与使用方式匹配 |
| **将云厂商 API 限流误判为 CSI Driver bug** | Provisioning 偶尔失败，CSI 日志显示错误 | 云厂商 API 触发限流，非 CSI 代码问题 | 检查错误信息是否包含 throttle/rate limit；分散 PVC 创建时间 |
| **将 fsType 不匹配误判为文件系统损坏** | 挂载失败，错误 "wrong fs type" | PV 指定的 fsType 与 Volume 实际格式化的 fs 不一致 | 比对 PV spec 中的 fsType 和 Volume 实际的文件系统类型 |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：

| 主题 | 引用路径 | 适用场景 |
|------|---------|---------|
| CSI 架构与工作原理 | `存储/` | 理解 Provisioner、Attacher、Mounter 的分工 |
| 存储类故障排查 | `故障诊断/04-storage-csi-troubleshooting.md` | CSI 驱动级别的深度排查 |
| PVC 生命周期详解 | `故障诊断/14-pvc-storage-troubleshooting.md` | PVC 各状态转换的详细说明 |
| 云厂商存储集成 | `云厂商/` | ACK/EKS/GKE 特定的存储配置 |
| Volume Snapshot 使用 | `存储/` | 快照创建和恢复的详细步骤 |
| StatefulSet 存储最佳实践 | `工作负载/` | 有状态应用的存储配置建议 |

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-04 | v1.0 | 初始版本发布。覆盖 K8s v1.28-v1.32，包含 13 个根因、11 个修复操作 | 首批 Skill 库建设，基于存储相关工单分析确定 PVC/PV/CSI 为高优先级场景 |

### 10.4 待补充的知识空白

以下领域在当前版本中覆盖有限，后续版本将增强：

1. **块存储 vs 文件存储 vs 对象存储**: 不同存储类型的特殊诊断方法
2. **Ceph/GlusterFS 特定问题**: 分布式存储的特定故障模式
3. **Local PV 和 TopoLVM**: 本地存储的诊断差异
4. **存储加密问题**: LUKS/KMS 加密卷的诊断
5. **跨区域存储复制**: DR 场景下的存储问题
6. **存储性能问题**: IOPS/吞吐量不足的诊断与调优

---

## 附录 A：自动化诊断脚本

### A.1 PVC/PV 快速诊断脚本 (diagnose-pvc-quick.sh)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# =============================================================================
# PVC/PV 快速诊断脚本 - 诊断 Kubernetes 存储问题
# Usage: bash diagnose-pvc-quick.sh --namespace <ns> [--pvc <pvc-name>]
# Risk: NONE (read-only kubectl operations)
# Source: SKILL-STORE-001
# =============================================================================
set -euo pipefail

# --- 颜色定义 ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# --- 输出函数 ---
info()    { echo -e "${BLUE}[INFO]${NC} $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC} $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*"; }
success() { echo -e "${GREEN}[OK]${NC} $*"; }

# --- 帮助信息 ---
usage() {
    cat <<EOF
Usage: $(basename "$0") --namespace <namespace> [--pvc <pvc-name>]

PVC/PV 快速诊断脚本 - 检查存储问题并输出诊断摘要

Options:
    --namespace, -n    指定 Kubernetes 命名空间 (必需)
    --pvc, -p          指定要诊断的 PVC 名称 (可选，不指定则检查所有)
    --help, -h         显示帮助信息

Examples:
    $(basename "$0") --namespace default
    $(basename "$0") -n production -p mysql-data
EOF
    exit 0
}

# --- 参数解析 ---
NAMESPACE=""
PVC_NAME=""

while $# -gt 0; do
    case "$1" in
        --namespace|-n) NAMESPACE="$2"; shift 2 ;;
        --pvc|-p)       PVC_NAME="$2"; shift 2 ;;
        --help|-h)      usage ;;
        *)              error "未知参数: $1"; usage ;;
    esac
done

# --- 参数验证 ---
if -z "$NAMESPACE"; then
    error "必须指定 --namespace 参数"
    usage
fi

# --- 前置检查 ---
if ! command -v kubectl &>/dev/null; then
    error "kubectl 未安装或不在 PATH 中"
    exit 1
fi

if ! kubectl cluster-info &>/dev/null; then
    error "无法连接到 Kubernetes 集群，请检查 kubeconfig"
    exit 1
fi

if ! kubectl get namespace "$NAMESPACE" &>/dev/null; then
    error "命名空间 '$NAMESPACE' 不存在"
    exit 1
fi

# --- 诊断结果收集 ---
declare -A DIAGNOSIS
DIAGNOSIS["timestamp"]=$(date -u '+%Y-%m-%dT%H:%M:%SZ')
DIAGNOSIS["namespace"]=$NAMESPACE

echo -e "\n${BLUE}${BOLD}════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}${BOLD}  PVC/PV 快速诊断 - $NAMESPACE${NC}"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════${NC}"
echo -e "  时间: $(date -u '+%Y-%m-%d %H:%M:%S UTC')\n"

# --- Step 1: 检查非 Bound 状态的 PVC ---
info "[1/5] 检查非 Bound 状态的 PVC..."
if -n "$PVC_NAME"; then
    PENDING_PVCS=$(kubectl get pvc -n "$NAMESPACE" "$PVC_NAME" -o jsonpath='{.status.phase}' 2>/dev/null || echo "NotFound")
    if "$PENDING_PVCS" != "Bound"; then
        warn "PVC $PVC_NAME 状态: $PENDING_PVCS"
    else
        success "PVC $PVC_NAME 状态: Bound"
    fi
else
    PENDING_PVCS=$(kubectl get pvc -n "$NAMESPACE" --no-headers 2>/dev/null | grep -v "Bound" || true)
    if -n "$PENDING_PVCS"; then
        warn "发现非 Bound 状态的 PVC:"
        echo "$PENDING_PVCS" | while read line; do echo "    $line"; done
    else
        success "所有 PVC 均为 Bound 状态"
    fi
fi

# --- Step 2: 检查 StorageClass ---
info "[2/5] 检查 StorageClass 配置..."
SC_LIST=$(kubectl get sc -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.annotations.storageclass\.kubernetes\.io/is-default-class}{"\n"}{end}' 2>/dev/null)
DEFAULT_SC=$(echo "$SC_LIST" | grep "true" | awk '{print $1}' || true)
if -n "$DEFAULT_SC"; then
    success "默认 StorageClass: $DEFAULT_SC"
else
    warn "未设置默认 StorageClass"
fi

# --- Step 3: 检查 CSI Driver Pod 状态 ---
info "[3/5] 检查 CSI Driver Pod 状态..."
CSI_PODS=$(kubectl get pods -n kube-system -l 'app.kubernetes.io/component in (csi-driver,csi-controller,csi-node)' --no-headers 2>/dev/null || \
           kubectl get pods -n kube-system 2>/dev/null | grep -i csi || true)
CSI_NOT_RUNNING=$(echo "$CSI_PODS" | grep -v "Running" | grep -v "^$" || true)
if -n "$CSI_NOT_RUNNING"; then
    warn "CSI Driver Pod 异常:"
    echo "$CSI_NOT_RUNNING" | while read line; do echo "    $line"; done
else
    success "CSI Driver Pod 状态正常"
fi

# --- Step 4: 收集 FailedMount/FailedAttachVolume 事件 ---
info "[4/5] 检查存储相关事件..."
STORAGE_EVENTS=$(kubectl get events -n "$NAMESPACE" --sort-by=.lastTimestamp 2>/dev/null | \
                 grep -iE 'FailedMount|FailedAttachVolume|ProvisioningFailed|VolumeResizeFailed' | tail -10 || true)
if -n "$STORAGE_EVENTS"; then
    warn "发现存储相关警告事件:"
    echo "$STORAGE_EVENTS" | while read line; do echo "    $line"; done
else
    success "无存储相关警告事件"
fi

# --- Step 5: 检查 VolumeAttachment 状态 ---
info "[5/5] 检查 VolumeAttachment 状态..."
VA_NOT_ATTACHED=$(kubectl get volumeattachment -o json 2>/dev/null | \
                  jq -r '.items[] | select(.status.attached != true) | .metadata.name' 2>/dev/null || true)
if -n "$VA_NOT_ATTACHED"; then
    warn "VolumeAttachment 未完成:"
    echo "$VA_NOT_ATTACHED" | while read line; do echo "    $line"; done
else
    success "所有 VolumeAttachment 状态正常"
fi

# --- 输出诊断摘要 (JSON) ---
echo -e "\n${CYAN}${BOLD}── 诊断摘要 (JSON) ──${NC}"
cat <<EOF
{
  "timestamp": "$(date -u '+%Y-%m-%dT%H:%M:%SZ')",
  "namespace": "$NAMESPACE",
  "pvc_filter": "${PVC_NAME:-all}",
  "default_storageclass": "${DEFAULT_SC:-none}",
  "csi_pods_healthy": $([ -z "$CSI_NOT_RUNNING" ] && echo "true" || echo "false"),
  "storage_events_found": $([ -n "$STORAGE_EVENTS" ] && echo "true" || echo "false"),
  "va_issues_found": $([ -n "$VA_NOT_ATTACHED" ] && echo "true" || echo "false")
}
EOF

echo -e "\n${GREEN}诊断完成${NC}"
```
### A.2 CSI Driver 健康检查脚本 (check-csi-health.sh)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# =============================================================================
# CSI Driver 健康检查脚本
# Usage: bash check-csi-health.sh
# Risk: NONE (read-only operations)
# Source: SKILL-STORE-001
# =============================================================================
set -euo pipefail

# --- 颜色定义 ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# --- 输出函数 ---
info()    { echo -e "${BLUE}[INFO]${NC} $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC} $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*"; }
success() { echo -e "${GREEN}[OK]${NC} $*"; }

# --- 帮助信息 ---
usage() {
    cat <<EOF
Usage: $(basename "$0") [--help]

CSI Driver 健康检查脚本 - 检查所有 CSI Driver 组件状态

Options:
    --help, -h    显示帮助信息
EOF
    exit 0
}

"${1:-}" == "--help" && usage

# --- 前置检查 ---
if ! command -v kubectl &>/dev/null; then
    error "kubectl 未安装或不在 PATH 中"
    exit 1
fi

if ! kubectl cluster-info &>/dev/null; then
    error "无法连接到 Kubernetes 集群"
    exit 1
fi

echo -e "\n${BLUE}${BOLD}════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}${BOLD}  CSI Driver 健康检查${NC}"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════${NC}"
echo -e "  时间: $(date -u '+%Y-%m-%d %H:%M:%S UTC')\n"

# --- Step 1: 检查已注册的 CSI Driver ---
info "[1/4] 检查已注册的 CSI Driver..."
CSI_DRIVERS=$(kubectl get csidrivers -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' 2>/dev/null || true)
if -n "$CSI_DRIVERS"; then
    success "已注册的 CSI Driver:"
    echo "$CSI_DRIVERS" | while read driver; do echo "    - $driver"; done
else
    warn "未发现已注册的 CSI Driver"
fi

# --- Step 2: 检查 CSI Controller Pod 状态 ---
info "[2/4] 检查 CSI Controller Pod 状态..."
CONTROLLER_PODS=$(kubectl get pods -n kube-system -o wide 2>/dev/null | \
                  grep -iE 'csi.*controller|csi.*provisioner|ebs-csi|disk-csi' || true)
if -n "$CONTROLLER_PODS"; then
    NOT_RUNNING=$(echo "$CONTROLLER_PODS" | grep -v "Running" || true)
    if -n "$NOT_RUNNING"; then
        warn "CSI Controller Pod 异常:"
        echo "$NOT_RUNNING" | while read line; do echo "    $line"; done
    else
        success "CSI Controller Pod 全部 Running"
    fi
else
    warn "未发现 CSI Controller Pod"
fi

# --- Step 3: 检查 CSI Node Plugin Pod 状态 ---
info "[3/4] 检查 CSI Node Plugin Pod 状态..."
NODE_PODS=$(kubectl get pods -n kube-system -o wide 2>/dev/null | \
            grep -iE 'csi.*node|csi.*plugin' || true)
if -n "$NODE_PODS"; then
    NOT_RUNNING=$(echo "$NODE_PODS" | grep -v "Running" || true)
    if -n "$NOT_RUNNING"; then
        warn "CSI Node Plugin Pod 异常:"
        echo "$NOT_RUNNING" | while read line; do echo "    $line"; done
    else
        success "CSI Node Plugin Pod 全部 Running"
    fi
else
    warn "未发现 CSI Node Plugin Pod"
fi

# --- Step 4: 检查 CSI Node Driver 注册状态 ---
info "[4/4] 检查 CSI Node Driver 注册状态..."
CSI_NODES=$(kubectl get csinodes -o json 2>/dev/null || echo '{"items":[]}')
NODE_COUNT=$(echo "$CSI_NODES" | jq '.items | length')
if "$NODE_COUNT" -gt 0; then
    success "CSI Node 注册数量: $NODE_COUNT"
    # 检查每个节点的 driver 注册
    EMPTY_DRIVERS=$(echo "$CSI_NODES" | jq -r '.items[] | select(.spec.drivers == null or .spec.drivers == []) | .metadata.name' 2>/dev/null || true)
    if -n "$EMPTY_DRIVERS"; then
        warn "以下节点无 CSI Driver 注册:"
        echo "$EMPTY_DRIVERS" | while read node; do echo "    - $node"; done
    fi
else
    warn "未发现 CSI Node 注册信息"
fi

echo -e "\n${GREEN}CSI 健康检查完成${NC}"
```
### A.3 修复后验证脚本 (verify-storage.sh)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# =============================================================================
# PVC/PV 修复后验证脚本
# Usage: bash verify-storage.sh --namespace <ns> --pvc <pvc-name>
# Risk: LOW (creates temporary test pod)
# Source: SKILL-STORE-001
# =============================================================================
set -euo pipefail

# --- 颜色定义 ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m'

# --- 输出函数 ---
info()    { echo -e "${BLUE}[INFO]${NC} $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC} $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*"; }
success() { echo -e "${GREEN}[PASS]${NC} $*"; }
fail()    { echo -e "${RED}[FAIL]${NC} $*"; }

# --- 统计 ---
PASS_COUNT=0
FAIL_COUNT=0

# --- 帮助信息 ---
usage() {
    cat <<EOF
Usage: $(basename "$0") --namespace <namespace> --pvc <pvc-name>

PVC/PV 修复后验证脚本 - 验证存储功能恢复正常

Options:
    --namespace, -n    指定 Kubernetes 命名空间 (必需)
    --pvc, -p          指定要验证的 PVC 名称 (必需)
    --help, -h         显示帮助信息

Examples:
    $(basename "$0") --namespace default --pvc mysql-data
EOF
    exit 0
}

# --- 参数解析 ---
NAMESPACE=""
PVC_NAME=""

while $# -gt 0; do
    case "$1" in
        --namespace|-n) NAMESPACE="$2"; shift 2 ;;
        --pvc|-p)       PVC_NAME="$2"; shift 2 ;;
        --help|-h)      usage ;;
        *)              error "未知参数: $1"; usage ;;
    esac
done

if -z "$NAMESPACE"; then
    error "必须指定 --namespace 和 --pvc 参数"
    usage
fi

# --- 前置检查 ---
if ! command -v kubectl &>/dev/null; then
    error "kubectl 未安装"
    exit 1
fi

TEST_POD="storage-verify-$(date +%s)"

echo -e "\n${BLUE}${BOLD}════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}${BOLD}  存储修复后验证 - $NAMESPACE/$PVC_NAME${NC}"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════${NC}\n"

# --- V1: 验证 PVC Bound 状态 ---
info "[V1] 验证 PVC Bound 状态..."
PVC_STATUS=$(kubectl get pvc -n "$NAMESPACE" "$PVC_NAME" -o jsonpath='{.status.phase}' 2>/dev/null || echo "NotFound")
if "$PVC_STATUS" == "Bound"; then
    success "PVC 状态: Bound"
    ((PASS_COUNT++))
else
    fail "PVC 状态: $PVC_STATUS (期望: Bound)"
    ((FAIL_COUNT++))
fi

# --- V2: 创建临时 Pod 执行读写测试 ---
info "[V2] 创建临时 Pod 执行读写测试..."
cat <<EOF | kubectl apply -f - 2>/dev/null
apiVersion: v1
kind: Pod
metadata:
  name: $TEST_POD
  namespace: $NAMESPACE
spec:
  restartPolicy: Never
  containers:
  - name: test
    image: busybox:1.36
    command: ["sh", "-c", "echo 'storage-test-ok' > /data/test.txt && cat /data/test.txt && rm /data/test.txt"]
    volumeMounts:
    - name: test-vol
      mountPath: /data
  volumes:
  - name: test-vol
    persistentVolumeClaim:
      claimName: $PVC_NAME
EOF

info "等待测试 Pod 完成 (最多 60s)..."
if kubectl wait --for=condition=Ready pod/$TEST_POD -n "$NAMESPACE" --timeout=60s 2>/dev/null || \
   kubectl wait --for=jsonpath='{.status.phase}'=Succeeded pod/$TEST_POD -n "$NAMESPACE" --timeout=60s 2>/dev/null; then
    POD_OUTPUT=$(kubectl logs -n "$NAMESPACE" "$TEST_POD" 2>/dev/null || true)
    if "$POD_OUTPUT" == *"storage-test-ok"*; then
        success "读写测试通过"
        ((PASS_COUNT++))
    else
        fail "读写测试失败: $POD_OUTPUT"
        ((FAIL_COUNT++))
    fi
else
    fail "测试 Pod 执行超时"
    ((FAIL_COUNT++))
fi

# --- V3: 检查 Volume 指标 ---
info "[V3] 检查 Volume 相关指标..."
PV_NAME=$(kubectl get pvc -n "$NAMESPACE" "$PVC_NAME" -o jsonpath='{.spec.volumeName}' 2>/dev/null || true)
if -n "$PV_NAME"; then
    PV_STATUS=$(kubectl get pv "$PV_NAME" -o jsonpath='{.status.phase}' 2>/dev/null || echo "Unknown")
    if "$PV_STATUS" == "Bound"; then
        success "PV $PV_NAME 状态: Bound"
        ((PASS_COUNT++))
    else
        fail "PV 状态异常: $PV_STATUS"
        ((FAIL_COUNT++))
    fi
else
    fail "无法获取 PV 名称"
    ((FAIL_COUNT++))
fi

# --- 清理临时资源 ---
info "清理临时资源..."
kubectl delete pod -n "$NAMESPACE" "$TEST_POD" --ignore-not-found=true &>/dev/null || true

# --- 输出验证结果 ---
echo -e "\n${BOLD}════════════════════════════════════════════════════════${NC}"
TOTAL=$((PASS_COUNT + FAIL_COUNT))
if $FAIL_COUNT -eq 0; then
    echo -e "${GREEN}${BOLD}验证结果: 全部通过 ($PASS_COUNT/$TOTAL)${NC}"
    exit 0
else
    echo -e "${RED}${BOLD}验证结果: 存在失败 (通过: $PASS_COUNT, 失败: $FAIL_COUNT)${NC}"
    exit 1
fi
```
## 修复动作

> **本章定位**: 基于 Section 6 修复操作的快速决策摘要，供 Agent 在 QA 语料和运行时直接引用。

### 修复动作速查表

| 根因 | 修复动作 | 风险 | 验证命令 |
|------|---------|------|---------|
| RC-001 缺少默认 StorageClass | `kubectl patch storageclass <sc> -p '{"metadata":{"annotations":{"storageclass.kubernetes.io/is-default-class":"true"}}}'` | 🟢 低风险 | `kubectl get sc` |
| RC-001 PVC StorageClass 错误 | 导出 PVC YAML → 修正 storageClassName → 删除重建 | 🟢 低风险（数据保留在原 PV，前提是 ReclaimPolicy=Retain） | `kubectl get pvc <pvc> -n <ns>` |
| RC-004 Access Mode 冲突 | 停止旧 Pod 后重新调度，或改用 RWX StorageClass | 🟢 低风险 | `kubectl get volumeattachment | grep <pv>` |
| RC-002 CSI Controller 异常 | `kubectl delete pod -n kube-system -l 'app in (csi-provisioner,ebs-csi-controller,disk-csi-controller,csi-controller)'` | 🟡 中风险（正在进行的存储操作中断并重试） | `kubectl get pods -n kube-system -l 'app in (csi-provisioner,ebs-csi-controller,disk-csi-controller,csi-controller)'` |
| RC-005 孤儿 VolumeAttachment | `kubectl delete volumeattachment <va-name>` | 🟡 中风险（确认对应 Pod 已停止，避免数据损坏） | `kubectl get volumeattachment <va-name>` |
| RC-010 扩容失败 | `kubectl patch pvc <pvc> -n <ns> -p '{"spec":{"resources":{"requests":{"storage":"<new-size>"}}}}'` | 🟡 中风险（离线扩容需停止 Pod） | `kubectl get pvc <pvc> -n <ns> -o jsonpath='{.status.capacity.storage}'` |
| RC-003 存储后端容量不足 | 清理未使用的 Released PV 或联系存储团队扩容后端 | 🟡 中风险（清理 PV 可能导致数据丢失） | `kubectl get pv` |

### danger_operations 高风险操作标注

```yaml
danger_operations:
  - operation: "删除 VolumeAttachment"
    risk: "在 Volume 仍被 Pod 使用时强制删除 VolumeAttachment 可能导致文件系统损坏或数据不一致"
    prerequisite:
      - "确认对应 Pod 已完全终止: kubectl get pods -A | grep <pod-name>"
      - "确认无正在进行的写入操作"
    rollback: "重新创建 Pod 引用对应 PVC 即可触发重新 Attach"

  - operation: "fsck 文件系统修复"
    risk: "fsck 可能修改或删除损坏的文件，导致数据丢失；必须在 Volume 未挂载时执行"
    prerequisite:
      - "停止所有使用该 Volume 的 Pod"
      - "确认有最新的数据备份"
      - "使用 fsck -n 先做只读检查"
    rollback: "从备份恢复数据"

  - operation: "删除 Released PV（ReclaimPolicy=Delete）"
    risk: "直接删除 PV 会触发存储后端卷删除，数据永久丢失"
    prerequisite:
      - "确认 PV 上的数据已备份或不再需要"
      - "确认 PVC 已删除且没有 Pod 引用"
    mitigation: "如需保留数据，先 patch PV 的 ReclaimPolicy 为 Retain: kubectl patch pv <pv> -p '{\"spec\":{\"persistentVolumeReclaimPolicy\":\"Retain\"}}'"

  - operation: "修改 PVC 的 storageClassName / AccessMode（删除重建）"
    risk: "删除 PVC 后，如果 PV 的 ReclaimPolicy=Delete，存储后端卷会被删除，数据永久丢失"
    prerequisite:
      - "操作前确认 PV 的 ReclaimPolicy: kubectl get pv <pv> -o jsonpath='{.spec.persistentVolumeReclaimPolicy}'"
      - "如为 Delete，先 patch 为 Retain 并备份数据"
```

### 通用验证步骤

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 确认 PVC 状态为 Bound
kubectl get pvc <pvc> -n <ns>

# 2. 确认 PV 状态正常
kubectl get pv <pv>

# 3. 确认 Pod 成功挂载并运行
kubectl get pod <pod> -n <ns>

# 4. 进入 Pod 验证文件系统可读写
kubectl exec <pod> -n <ns> -- df -h <mount-path>
kubectl exec <pod> -n <ns> -- touch <mount-path>/healthcheck.txt

# 5. 确认 CSI Driver 健康
kubectl get pods -n kube-system -l 'app in (csi-provisioner,ebs-csi-controller,disk-csi-controller,csi-controller)'
```
## Related

- [[21-生态参考/03-领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[21-生态参考/03-领域索引/storage-index.md|Storage 存储知识图谱索引]]
- [[21-生态参考/03-领域索引/csi-index.md|CSI (Container Storage Interface) 知识图谱索引]]

```

<!-- risk-assessed -->
