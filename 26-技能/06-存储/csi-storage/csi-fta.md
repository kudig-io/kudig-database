---
title: CSI 存储异常诊断技能
description: Kubernetes CSI 存储插件的完整故障诊断技能，覆盖控制器异常、节点插件异常、卷挂载失败、PVC 绑定失败、性能劣化、权限与密钥异常、后端存储依赖故障等场景
summary: CSI 存储故障诊断，覆盖控制器/节点插件/卷挂载/性能/权限/后端 6 大类 12+ 根因
category: skill
tags:
- k8s
- storage
- csi
- pv
- pvc
- storageclass
- troubleshooting
- fta
- volume
sources:
- 故障诊断/FTA故障树/list/csi-fta.md
- 故障诊断/高级排障/structural-04-storage-components/
- code/alibaba-cloud-csi-driver-1.36.1/
created: '2026-05-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 20min
intent_queries:
- CSI 存储挂载失败怎么排查
- PVC Pending 如何解决
- CSI Driver 崩溃如何恢复
- 卷扩容失败怎么诊断
- PV/PVC 绑定失败排查
trigger_keywords:
- CSI
- PVC
- PV
- StorageClass
- 挂载失败
- FailedMount
- 卷扩容
- 存储异常
prerequisites:
- kubectl-basics
- storage-basics
fta_id: FTA-CSI-001
component: CSI
severity: high
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# CSI 存储异常诊断技能

## 1. 概述

### 覆盖范围

本技能覆盖 CSI 存储在生产环境中的全部常见故障：

- **控制器异常**：CSI Controller 不可用、Attach/Provision 失败、扩容/快照失败
- **节点插件异常**：Node Plugin 崩溃、NodeStage/NodePublish 失败、挂载工具缺失
- **卷与挂载异常**：PVC 未绑定、卷只读/损坏、多节点挂载冲突、VolumeAttachment 残留
- **性能与容量异常**：IO 延迟/抖动、吞吐下降、容量不足
- **权限与密钥异常**：Secret 缺失、KMS 密钥过期
- **后端存储异常**：存储服务不可用、网络不可达

### 适用场景

| 适用 | 不适用 |
|------|--------|
| Pod 卷挂载失败（FailedMount/FailedAttachVolume） | 应用层文件读写错误 |
| PVC 长时间 Pending/未绑定 | emptyDir/hostPath 本地卷问题 |
| CSI Driver Pod 异常 | 存储后端内部故障（需联系存储厂商） |
| 卷扩容/快照失败 | 数据库层面的数据一致性 |

### 前置条件

- 了解集群使用的 CSI 驱动类型（云盘 CSI/NFS CSI/Ceph CSI 等）
- 具备 kube-system 命名空间 Pod 日志读取权限
- 部分诊断需要节点级 SSH 权限

---

## 2. 症状识别

| 症状 ID | 症状描述 | 工单关键词 | 确认命令 |
|---------|---------|-----------|---------|
| S1 | Pod 启动失败，Events 含 FailedMount | "挂载失败"、"卷不可用" | `kubectl describe pod <pod> -n <ns> | grep -A5 Events` |
| S2 | PVC 长时间 Pending | "PVC 未绑定"、"存储未就绪" | `kubectl get pvc -n <ns>` |
| S3 | CSI Driver Pod CrashLoopBackOff | "CSI 崩溃"、"存储插件异常" | `kubectl get pods -n kube-system -l app=csi-plugin` |
| S4 | 卷扩容后容量未生效 | "扩容失败"、"容量没变" | `kubectl get pvc -o wide` + Pod 内 `df -h` |
| S5 | 卷 IO 延迟突然升高 | "IO 慢"、"磁盘性能下降" | `kubectl exec <pod> -- iostat -x 1 5` |
| S6 | VolumeAttachment 残留导致新 Pod 无法挂载 | "卷被占用"、"Multi-Attach error" | `kubectl get volumeattachment | grep <pv-name>` |

### 排除标准

- 若 Pod 因 ImagePull 失败而非存储 → 转镜像拉取排查
- 若节点 DiskPressure 导致驱逐 → 转 [[26-技能/03-节点/node/02-node-resource-pressure.md|节点资源压力诊断]]
- 若仅应用层报 "Permission denied" → 可能是 fsGroup/SELinux 问题，非 CSI 层面

### 2.1 常见错误消息与事件日志速查

> 以下错误消息和事件日志是 CSI 存储故障场景的高频诊断线索。Agent 在采集 Events 和 CSI 日志后可直接匹配本表快速路由。

#### 关键 Events（`kubectl get events` / `kubectl describe pod`）

| 事件 Reason | 事件 Message 模式 | 含义 | 检测命令 | 路由 |
|-------------|------------------|------|---------|------|
| `FailedMount` | `MountVolume.SetUp failed for volume "<vol>": rpc error: code = Internal desc = ...` | CSI NodePublishVolume 失败 | `kubectl get events -n <ns> --field-selector reason=FailedMount` | → RC-005 |
| `FailedMount` | `Unable to attach or mount volumes: unmounted volumes=[<vol>], unattached volumes=[...]: timed out waiting for the condition` | 卷挂载超时 | 同上 | → RC-004/RC-005 |
| `FailedMount` | `MountVolume.MountDevice failed for volume "<vol>": rpc error: code = Internal desc = ...` | CSI NodeStageVolume 失败 | 同上 | → RC-005 |
| `FailedAttachVolume` | `AttachVolume.Attach failed for volume "<vol>": rpc error: code = Internal desc = ...` | CSI ControllerPublishVolume 失败 | `kubectl get events -n <ns> --field-selector reason=FailedAttachVolume` | → RC-002 |
| `FailedAttachVolume` | `Multi-Attach error for volume "<pv>" Volume is already used by pod(s) on node <node>` | 卷未从旧节点释放 | 同上 | → RC-008 |
| `FailedAttachVolume` | `AttachVolume.Attach failed for volume "<vol>": rpc error: code = DeadlineExceeded desc = context deadline exceeded` | Attach 超时 | 同上 | → RC-002/RC-012 |
| `ProvisioningFailed` | `Failed to provision volume with StorageClass "<sc>": rpc error: code = Internal desc = ...` | 动态供给失败 | `kubectl get events --field-selector reason=ProvisioningFailed` | → RC-001 |
| `ProvisioningFailed` | `... rpc error: code = InvalidArgument desc = ...` | 参数错误（StorageClass 配置） | 同上 | → RC-007 |
| `ProvisioningFailed` | `... storageclass.storage.k8s.io "<name>" not found` | StorageClass 不存在 | 同上 | → RC-007 |
| `VolumeResizeFailed` | `Failed to resize volume "<pv>": rpc error: ...` | 卷扩容失败 | `kubectl get events --field-selector reason=VolumeResizeFailed` | → RC-003 |
| `ExternalProvisioning` | `waiting for a volume to be created, either by external provisioner "<driver>" or manually created by system administrator` | 等待外部供给器创建卷 | `kubectl describe pvc <pvc>` | → RC-001/RC-007 |
| `WaitForFirstConsumer` | `waiting for first consumer to be created before binding` | PVC 等待首个消费者（正常，非故障） | `kubectl describe pvc <pvc>` | 无需处理（正常行为） |

#### PVC/PV 状态异常与对应事件

| PVC 状态 | Events 模式 | 含义 | 检测命令 |
|----------|------------|------|----------|
| `Pending` | `provisioner not found` / `invalid storage class` | StorageClass 配置错误 | `kubectl describe pvc <pvc> -n <ns>` |
| `Pending` | `Failed to provision volume with StorageClass ...` | 动态供给失败 | 同上 |
| `Pending` | `waiting for first consumer to be created before binding` | WaitForFirstConsumer 模式（正常） | 同上 |
| `Pending` | `persistentvolume "<pv>" not found` | 静态 PV 不存在 | `kubectl get pv` |
| `Lost` | `volume "<pv>" is being deleted` | PV 被删除/回收 | `kubectl get pv <pv>` |
| `Bound` 但 Pod FailedMount | `MountVolume.SetUp failed` | 卷已绑定但挂载失败 | `kubectl describe pod <pod>` |

#### CSI Driver 日志关键错误

```bash
# 🟢 低风险：只读/信息收集
# CSI Controller 日志
kubectl logs -n kube-system -l app=csi-provisioner --tail=50 | grep -iE "error|failed|timeout"
kubectl logs -n kube-system -l app=csi-attacher --tail=50 | grep -iE "error|failed|timeout"

# CSI Node Plugin 日志
kubectl logs -n kube-system <csi-node-pod> -c csi-plugin --tail=50 | grep -iE "error|failed|timeout|not found"
```

| 日志模式 | 含义 | 对应根因 | 修复方向 |
|---------|------|---------|----------|
| `CreateVolume failed: ... quota exceeded` / `InsufficientCapacity` | 存储后端容量/配额不足 | RC-001 | 扩容存储配额 |
| `ControllerPublishVolume failed: ... disk is already attached` | 卷已附加到其他节点 | RC-008 | 清理残留 VolumeAttachment |
| `ControllerPublishVolume failed: ... disk not found` | 云盘不存在/已删除 | RC-012 | 检查存储后端 |
| `NodeStageVolume failed: ... mount: wrong fs type, bad option, bad superblock` | 文件系统损坏/类型不匹配 | RC-005 | 检查卷文件系统 |
| `NodeStageVolume failed: ... mount.nfs: command not found` | 挂载工具缺失 | RC-006 | 安装 nfs-utils |
| `NodePublishVolume failed: ... permission denied` | 挂载点权限不足 | RC-005/RC-011 | 检查 fsGroup/SecurityContext |
| `secret "<name>" not found` / `access denied` | Secret 缺失/权限不足 | RC-011 | 创建/修正 Secret |
| `connection refused` / `no route to host` | 存储后端网络不可达 | RC-012 | 检查网络连通性 |
| `VolumeResize failed: ... volume expansion not supported` | 存储类型不支持扩容 | RC-003 | 确认 StorageClass allowVolumeExpansion |
| `grpc: ... deadline exceeded` | CSI gRPC 调用超时 | RC-002/RC-012 | 检查 CSI Pod 状态/存储后端 |

#### 节点级存储诊断命令

```bash
# 🟢 低风险：只读/信息收集（需节点 SSH 权限）
# 检查 CSI socket 是否存在
ssh <node-ip> "ls -la /var/lib/kubelet/plugins/*/csi.sock"

# 检查实际挂载状态
ssh <node-ip> "mount | grep kubelet"
ssh <node-ip> "df -h | grep kubelet"

# 检查内核级存储错误
ssh <node-ip> "dmesg -T | grep -iE 'error|fail|readonly|I/O' | tail -20"

# 检查块设备状态
ssh <node-ip> "lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE"
```

| dmesg/内核日志模式 | 含义 | 修复 |
|----------------|------|------|
| `I/O error, dev <dev>, sector <n>` | 磁盘 IO 错误（硬件/后端故障） | 联系存储厂商（🔴 紧急） |
| `EXT4-fs error (device <dev>): ...` | 文件系统损坏 | fsck 修复（🔴 需卸载） |
| `Buffer I/O error on dev <dev>` | 缓冲 IO 错误 | 检查存储后端健康 |
| `device <dev> is write-protected` | 卷变为只读 | 检查存储后端状态 |
| `nfsv4: server <ip> not responding` | NFS 服务器无响应 | 检查 NFS 服务/网络 |
| `mount: wrong fs type, bad option, bad superblock on <dev>` | 文件系统类型错误/损坏 | 检查卷格式化状态 |

---

## 3. 快速分级

| 严重性 | 定义 | 响应策略 |
|--------|------|---------|
| P0 | 多节点卷挂载失败/数据丢失风险 | 立即检查 CSI Driver DaemonSet，5min 内响应 |
| P1 | 单 Pod 卷挂载超时（业务启动受阻） | 15min 内检查 PV/PVC 状态和 CSI 日志 |
| P2 | 卷性能下降（IO 延迟升高） | 检查存储后端健康状态和节点 IO |
| P3 | 扩容/快照非紧急操作失败 | 标准诊断流程，检查 StorageClass 配置 |

---

## 4. 诊断工作流

### Phase 1：快速检查（< 2 分钟）

#### D1.1 确认 PVC/PV 状态

```bash
# 🟢 低风险：只读/信息收集
kubectl get pvc -n ${NAMESPACE} -o wide
kubectl get pv | grep <pvc-name>
kubectl describe pvc <pvc-name> -n ${NAMESPACE} | tail -20
```

**判断逻辑**：
- PVC Pending + Events 含 `ProvisioningFailed` → 转 RC-001（控制器）
- PVC Pending + Events 含 `WaitForFirstConsumer` → 正常（等待 Pod 调度）
- PVC Bound 但 Pod FailedMount → 转节点插件子树（RC-004~006）

#### D1.2 检查 CSI Driver 状态

```bash
# 🟢 低风险：只读/信息收集
kubectl get pods -n kube-system -l app=csi-plugin -o wide
kubectl get pods -n kube-system -l app=csi-provisioner -o wide
kubectl get csidriver
```

#### D1.3 检查 Pod 事件

```bash
# 🟢 低风险：只读/信息收集
kubectl get events -n ${NAMESPACE} --field-selector reason=FailedMount --sort-by='.lastTimestamp' | tail -10
kubectl get events -n ${NAMESPACE} --field-selector reason=FailedAttachVolume --sort-by='.lastTimestamp' | tail -10
```

### Phase 2：深度检查（< 10 分钟）

#### D2.1 CSI Controller 日志

```bash
# 🟢 低风险：只读/信息收集
kubectl logs -n kube-system -l app=csi-provisioner --tail=50 | grep -E "error|failed"
kubectl logs -n kube-system -l app=csi-attacher --tail=50 | grep -E "error|failed"
```

#### D2.2 CSI Node Plugin 日志

```bash
# 🟢 低风险：只读/信息收集
kubectl logs -n kube-system <csi-node-pod-on-target-node> -c csi-plugin --tail=50
```

#### D2.3 VolumeAttachment 检查

```bash
# 🟢 低风险：只读/信息收集
kubectl get volumeattachment | grep <pv-name>
kubectl describe volumeattachment <va-name>
```

**判断逻辑**：
- VolumeAttachment 状态 attached=false 且持续 → 转 RC-002
- 存在多个 VolumeAttachment 指向同一 PV → 转 RC-008（残留）

#### D2.4 节点级检查

```bash
# 🟢 低风险：只读（需在目标节点执行）
ls /var/lib/kubelet/plugins/           # CSI socket 目录
ls /var/lib/kubelet/pods/<pod-uid>/volumes/  # 挂载点
mount | grep <pv-name>                  # 实际挂载状态
dmesg | tail -20                        # 内核级存储错误
```

### Phase 3：主动探测（需审批）

#### D3.1 存储后端连通性测试

```bash
# 🟢 低风险：只读
# 云盘：检查 ECS 与云盘状态
aliyun ecs DescribeDisks --DiskIds '["<disk-id>"]' | jq '.Disks.Disk[0].Status'
# NFS：测试挂载
showmount -e <nfs-server>
mount -t nfs <nfs-server>:/export /mnt/test
```

#### D3.2 清理残留 VolumeAttachment

```bash
# 🔴 高风险：可能导致数据不一致
kubectl delete volumeattachment <va-name>
```

---

## 5. 根因分类

| 编号 | 根因 | 概率 | 关键证据 | FTA 映射 |
|------|------|------|----------|---------|
| RC-001 | CSI Controller 不可用/Provision 失败 | 高 | csi-provisioner CrashLoop/日志错误 | TE→IE-1→BE-1.1 |
| RC-002 | CSI Attacher 失败（Attach 超时） | 中 | VolumeAttachment attached=false | TE→IE-1→BE-1.2 |
| RC-003 | 快照/扩容操作失败 | 中 | VolumeResizeFailed 事件 | TE→IE-1→BE-1.3 |
| RC-004 | CSI Node Plugin 崩溃 | 高 | csi-plugin Pod CrashLoopBackOff | TE→IE-2→BE-2.1 |
| RC-005 | NodeStageVolume/NodePublishVolume 失败 | 高 | kubelet 日志 "MountVolume.SetUp failed" | TE→IE-2→BE-2.2 |
| RC-006 | 节点挂载工具缺失 | 中 | 日志 "mount.nfs: command not found" | TE→IE-2→BE-2.3 |
| RC-007 | PVC 未绑定（StorageClass/Provisioner 配置错误） | 高 | PVC Events "provisioner not found" | TE→IE-3→BE-3.1 |
| RC-008 | VolumeAttachment 残留（旧节点未清理） | 中 | Multi-Attach error for volume | TE→IE-3→BE-3.2 |
| RC-009 | IO 延迟/性能劣化 | 中 | iostat await > 100ms | TE→IE-4→BE-4.1 |
| RC-010 | 容量不足（卷/文件系统满） | 中 | kubelet_volume_stats_used_bytes 接近上限 | TE→IE-4→BE-4.2 |
| RC-011 | Secret 缺失/权限不足 | 中 | 日志 "secret not found" / "access denied" | TE→IE-5→BE-5.1 |
| RC-012 | 后端存储服务异常/网络不可达 | 中 | 云盘状态异常/NFS 不可达 | TE→IE-6→BE-6.1 |

---

## 6. 修复操作

| 编号 | 对应根因 | 修复操作 | 风险等级 | 审批要求 |
|------|---------|---------|:--------:|---------|
| REM-001 | RC-001 | 重启 CSI Controller Deployment，检查 RBAC 和镜像 | 🟡 | 无需 |
| REM-002 | RC-002 | 检查云盘状态和 CSI attacher 日志，必要时重启 attacher | 🟡 | 无需 |
| REM-003 | RC-003 | 确认 StorageClass `allowVolumeExpansion: true`，检查 CSI 驱动版本 | 🟡 | 变更审批 |
| REM-004 | RC-004 | 重启 CSI Node DaemonSet Pod，检查 /var/lib/kubelet/plugins 目录 | 🟡 | 无需 |
| REM-005 | RC-005 | 检查 CSI socket 文件存在性，重启 CSI Node Pod | 🟡 | 无需 |
| REM-006 | RC-006 | 安装缺失的挂载工具（nfs-utils/iscsi-initiator-utils/ceph-common） | 🟡 | 变更审批 |
| REM-007 | RC-007 | 修正 StorageClass provisioner 名称或创建对应 StorageClass | 🟡 | 变更审批 |
| REM-008 | RC-008 | 删除残留 VolumeAttachment（确认旧 Pod 已终止） | 🔴 | 高级审批 |
| REM-009 | RC-009 | 检查存储后端 QoS/IOPS 限制，升级存储规格 | 🟡 | 变更审批 |
| REM-010 | RC-010 | 扩容 PVC 或清理数据释放空间 | 🟡 | 变更审批 |
| REM-011 | RC-011 | 创建/修正 Secret，检查 RBAC 权限 | 🟡 | 变更审批 |
| REM-012 | RC-012 | 检查存储后端服务状态和网络连通性，联系存储厂商 | 🟢 | 无需 |

---

## 7. 验证确认

### 即时验证（修复后 1 分钟）

```bash
# 🟢 低风险
kubectl get pvc -n ${NAMESPACE}          # PVC Bound
kubectl get pod <pod> -n ${NAMESPACE}    # Pod Running
kubectl exec <pod> -n ${NAMESPACE} -- df -h  # 挂载点可见
```

### 短期监控（15-30 分钟）

- CSI Driver Pod 无重启
- 无新增 FailedMount/FailedAttachVolume 事件
- IO 延迟恢复正常水平

### 解决标准

| 条件 | 判定 |
|------|------|
| PVC 状态 Bound | ✅ |
| Pod Running 且挂载点可读写 | ✅ |
| CSI Driver 全部 READY | ✅ |
| 30 分钟内无新增存储相关事件 | ✅ |

---

## 8. 升级协议

| 级别 | 自动升级条件 | 消息模板 | 交接信息 |
|------|------------|---------|---------|
| P0→专家 | 多节点卷挂载失败 > 5min | "【P0】CSI 多节点挂载失败，影响 {N} Pod" | CSI Driver 状态 + 事件 + 后端存储状态 |
| P1→SME | 单 Pod 挂载超时 > 15min | "【P1】Pod {pod} 卷挂载失败" | PVC/PV 状态 + CSI 日志 + VolumeAttachment |
| P2→二线 | 性能问题持续 > 1h | "【P2】卷 IO 性能劣化" | iostat 输出 + 存储后端指标 |

---

## 9. 版本兼容矩阵

| K8s 版本 | CSI 关键变化 |
|---------|------------|
| 1.20-1.23 | CSI 1.5 规范；`CSIMigration` 逐步 GA（in-tree → CSI） |
| 1.24-1.25 | in-tree 存储驱动标记 deprecated；`CSIMigration` 全量 GA |
| 1.26-1.28 | `ReadWriteOncePod` 访问模式 Beta；CSI 节点扩容 GA |
| 1.29-1.32 | `VolumeAttributesClass` Alpha→Beta；结构化参数 |
| 1.34-1.36 | `CSIVolumeHealth` Beta；卷健康监控 |

> [存疑：`ReadWriteOncePod` 在 1.29 是否已 GA，需确认 KEP 状态]

**通用提示**：排障前先确认 CSI 驱动版本和 StorageClass 配置：
```bash
# 🟢 低风险
kubectl get csidriver -o wide
kubectl get sc -o wide
```

---

## 10. 知识进化

### 常见误诊模式

| 误诊模式 | 表现 | 正确做法 |
|---------|------|---------|
| 将 WaitForFirstConsumer 误判为故障 | PVC Pending 但无错误事件 | 确认 volumeBindingMode，Pod 调度后自动绑定 |
| 将应用权限问题误判为 CSI 故障 | "Permission denied" 但挂载成功 | 检查 fsGroup/runAsUser/SELinux 上下文 |
| 将节点磁盘满误判为 CSI 异常 | 多 Pod 同时 FailedMount | 先 `df -h` 检查节点磁盘空间 |

### 变更记录

| 版本 | 日期 | 变更内容 | 触发原因 |
|------|------|---------|---------|
| 1.0.0 | 2026-05-23 | 初版 FTA 故障树 | 技能库初始化 |
| 2.0.0 | 2026-07-23 | 重构为 12 章节标准结构，补全根因/修复/验证 | 技能建设最佳实践对标 |

---

## 生产级观测与证据

### 关键事件

| 事件 | 含义 | 对应根因 |
|------|------|---------|
| `FailedMount` | 卷挂载到 Pod 失败 | RC-004~006 |
| `FailedAttachVolume` | 卷 Attach 到节点失败 | RC-002 |
| `ProvisioningFailed` | 动态供给失败 | RC-001/007 |
| `VolumeResizeFailed` | 扩容失败 | RC-003 |
| `ExternalExpanding` | 正在扩容（正常中间态） | — |

### 关键指标

| 指标 | 用途 |
|------|------|
| `kubelet_volume_stats_used_bytes` | 卷容量使用率 |
| `kubelet_volume_stats_available_bytes` | 卷剩余空间 |
| `csi_operations_seconds` | CSI 操作耗时 |
| `kube_persistentvolumeclaim_status_phase` | PVC 状态 |
| `node_disk_io_time_seconds_total` | 磁盘 IO 时间 |

### 关键日志来源

| 组件 | 日志获取方式 |
|------|------------|
| CSI Controller | `kubectl logs -n kube-system -l app=csi-provisioner` |
| CSI Attacher | `kubectl logs -n kube-system -l app=csi-attacher` |
| CSI Node Plugin | `kubectl logs -n kube-system <csi-node-pod> -c csi-plugin` |
| kubelet 卷管理 | `journalctl -u kubelet | grep -i "volume\|mount"` |

---

## 生产案例

### 案例 1: CSI Driver Pod 崩溃导致 PVC 挂载失败

| 时间 | 事件 |
|------|------|
| 09:00 | 新 Pod 启动失败，Events: "FailedMount: timeout waiting for volume" |
| 09:05 | `kubectl get pods -n kube-system -l app=csi-plugin` 显示 CrashLoopBackOff |
| 09:10 | 日志: "failed to connect to CSI socket: no such file" |
| 09:15 | 🟡 REM-004 重启 CSI DaemonSet，检查 /var/lib/kubelet/plugins 目录 |
| 09:20 | 卷挂载恢复 |

**根因**: RC-004。节点重启后 CSI socket 文件未重新创建，kubelet 与 CSI driver 通信失败。

### 案例 2: PV 容量扩展失败——StorageClass 不支持

**现象**: `kubectl edit pvc` 增大容量后，PVC 状态仍为原始大小。

**诊断**: `kubectl get sc -o jsonpath='{.items[*].allowVolumeExpansion}'` → false

**修复**: 🟡 REM-003 修改 StorageClass `allowVolumeExpansion: true`，重新编辑 PVC

### 案例 3: VolumeAttachment 残留导致 Multi-Attach Error

**现象**: Pod 重新调度到新节点后 FailedAttachVolume: "Multi-Attach error for volume"

**诊断**: `kubectl get volumeattachment | grep <pv>` 显示旧节点的 VA 仍存在

**修复**: 🔴 REM-008 确认旧 Pod 已终止后删除残留 VolumeAttachment

---

## 相关链接

- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]] — 方法论基础
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]] — 执行引擎
- [[26-技能/06-存储/csi-storage/manage-persistent-storage.md|PV/PVC 存储管理]] — 同域技能
- [[26-技能/03-节点/node/02-node-resource-pressure.md|节点资源压力诊断]] — 跨域关联
- [[21-生态参考/03-领域索引/csi-index.md|CSI 知识图谱索引]] — 知识索引
- [[21-生态参考/03-领域索引/pvc-index.md|PVC 知识图谱索引]] — 知识索引

<!-- risk-assessed -->
