---
title: PV/PVC 持久化存储管理技能
description: Kubernetes 持久化存储全生命周期管理技能，覆盖 StorageClass 配置、动态/静态供给、PVC 绑定、卷扩容、回收策略、数据保护与常见故障排查
summary: PV/PVC 存储管理操作技能，覆盖供给/绑定/扩容/回收/故障排查全流程
category: skill
tags:
- k8s
- storage
- pv
- pvc
- storageclass
- csi
- volume
- lifecycle
- troubleshooting
sources:
- 故障诊断/topic-fta/list/csi-fta.md
- 最佳实践/storage/k8s-storage-configuration-guide.md
created: '2026-05-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 平台工程师
- 技术支持
estimated_read_time: 15min
intent_queries:
- 如何配置 StorageClass 动态供给
- PVC 扩容流程是什么
- PV 回收策略怎么选择
- PVC Pending 怎么排查
- 存储卷数据如何保护
trigger_keywords:
- PV
- PVC
- StorageClass
- 动态供给
- 卷扩容
- 回收策略
- Retain
- Delete
prerequisites:
- kubectl-basics
- storage-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# PV/PVC 持久化存储管理技能

## 1. 概述

### 覆盖范围

本技能覆盖 Kubernetes 持久化存储的全生命周期管理：

- **供给（Provision）**：动态供给（StorageClass + CSI）、静态供给（手动创建 PV）
- **绑定（Bind）**：PVC 与 PV 的匹配与绑定机制
- **使用（Use）**：Pod 中引用 PVC、访问模式选择
- **扩容（Expand）**：在线/离线扩容流程与限制
- **回收（Reclaim）**：Delete/Retain/Recycle 策略选择与数据保护
- **故障排查**：PVC Pending、扩容失败、数据丢失等常见问题

### 适用场景

| 适用 | 不适用 |
|------|--------|
| PV/PVC/StorageClass 配置与管理 | CSI Driver 内部故障（→ csi-fta.md） |
| 存储卷生命周期操作 | 存储后端硬件故障 |
| 存储相关故障初步排查 | 数据库层面数据一致性 |
| 存储策略选型与最佳实践 | 备份恢复操作（→ backup-restore） |

---

## 2. 存储生命周期管理

### 2.1 动态供给（推荐）

创建 StorageClass，PVC 创建时自动触发 CSI 驱动供给 PV：

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ssd-storage
provisioner: diskplugin.csi.alibabacloud.com  # 按实际 CSI 驱动填写
volumeBindingMode: WaitForFirstConsumer  # 推荐：先调度 Pod 再绑定卷
reclaimPolicy: Retain                    # 生产推荐 Retain 防数据丢失
allowVolumeExpansion: true               # 允许扩容
parameters:
  type: cloud_essd
  performanceLevel: PL1
```

### 2.2 静态供给

管理员手动创建 PV，PVC 通过 label selector 或 storageClassName 绑定：

```yaml
apiVersion: v1
kind: PersistentVolume
metadata:
  name: manual-pv
spec:
  capacity:
    storage: 100Gi
  accessModes:
  - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  storageClassName: ""
  hostPath:
    path: /data/manual-pv
```

### 2.3 PVC 创建与绑定

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: my-pvc
spec:
  accessModes:
  - ReadWriteOnce
  resources:
    requests:
      storage: 50Gi
  storageClassName: ssd-storage
```

**绑定匹配条件**：
- StorageClass 名称一致
- 访问模式兼容
- 容量 ≥ 请求
- Label selector 匹配（若指定）

### 2.4 Pod 中使用 PVC

```yaml
volumes:
- name: data
  persistentVolumeClaim:
    claimName: my-pvc
containers:
- name: app
  volumeMounts:
  - name: data
    mountPath: /app/data
```

### 2.5 卷扩容

> ⚠️ **🟡 中风险** — 变更集群资源状态，建议先确认影响范围

```bash
# 🟡 中风险：修改 PVC 容量
kubectl patch pvc my-pvc -n ${NAMESPACE} -p '{"spec":{"resources":{"requests":{"storage":"100Gi"}}}}'
```

**扩容前提**：
1. StorageClass 设置 `allowVolumeExpansion: true`
2. CSI 驱动支持在线扩容
3. 文件系统扩展需要 Pod 重启（离线扩容）或支持在线 resize

### 2.6 回收策略

| 策略 | 行为 | 适用场景 | 风险 |
|------|------|---------|------|
| **Retain**（生产推荐） | PVC 删除后 PV 保留，数据不丢失 | 生产环境、有状态服务 | 需手动清理 |
| **Delete** | PVC 删除后自动删除底层存储 | 开发/测试、临时数据 | 🔴 数据不可恢复 |
| **Recycle**（已废弃） | 清除数据后重新可用 | 不推荐使用 | — |

---

## 3. 快速分级

| 严重性 | 定义 | 响应策略 |
|--------|------|---------|
| P0 | 数据丢失（Delete 策略误触发） | 立即从备份恢复，评估影响范围 |
| P1 | PVC 扩容失败/绑定失败（业务受阻） | 15min 内检查 StorageClass 和 CSI |
| P2 | 存储性能下降 | 检查存储后端和 IOPS 限制 |
| P3 | 存储策略优化/巡检 | 审查回收策略和容量规划 |

---

## 4. 诊断工作流

### Phase 1：快速检查（< 2 分钟）

#### D1.1 检查 PVC 状态

```bash
# 🟢 低风险：只读/信息收集
kubectl get pvc -n ${NAMESPACE} -o wide
kubectl describe pvc <pvc-name> -n ${NAMESPACE}
```

**判断逻辑**：
- PVC Pending + `ProvisioningFailed` → StorageClass/CSI 问题
- PVC Pending + `WaitForFirstConsumer` → 正常，等待 Pod 调度
- PVC Lost → 底层 PV 异常

#### D1.2 检查 PV 状态

```bash
# 🟢 低风险：只读/信息收集
kubectl get pv | grep <pvc-name>
kubectl describe pv <pv-name>
```

#### D1.3 检查 Pod 挂载状态

```bash
# 🟢 低风险：只读/信息收集
kubectl get pod <pod> -n ${NAMESPACE} -o jsonpath='{.status.conditions[?(@.type=="Ready")]}'
kubectl exec <pod> -n ${NAMESPACE} -- df -h <mount-path>
```

### Phase 2：深度检查（< 10 分钟）

#### D2.1 StorageClass 配置验证

```bash
# 🟢 低风险：只读/信息收集
kubectl get sc <sc-name> -o yaml
kubectl get sc -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.provisioner}{"\t"}{.allowVolumeExpansion}{"\n"}{end}'
```

#### D2.2 CSI 驱动状态

```bash
# 🟢 低风险：只读/信息收集
kubectl get csidriver
kubectl get pods -n kube-system -l app=csi-plugin -o wide
```

#### D2.3 节点级挂载检查

```bash
# 🟢 低风险：只读（需在目标节点执行）
mount | grep <pv-name>
lsblk
df -h /var/lib/kubelet/pods/<pod-uid>/volumes/
```

---

## 5. 根因分类

| 编号 | 根因 | 概率 | 关键证据 |
|------|------|------|----------|
| RC-001 | PVC Pending — StorageClass 不存在/名称错误 | 高 | Events: "storageclass not found" |
| RC-002 | PVC Pending — Provisioner 未运行 | 高 | csi-provisioner Pod 异常 |
| RC-003 | PVC Pending — 容量/访问模式不匹配（静态） | 中 | 无匹配 PV |
| RC-004 | 扩容失败 — StorageClass 未启用 allowVolumeExpansion | 高 | `allowVolumeExpansion: false` |
| RC-005 | 扩容失败 — CSI 驱动不支持在线扩容 | 中 | 日志 "resize not supported" |
| RC-006 | 扩容后文件系统未扩展 | 高 | PVC 显示新容量，Pod 内 df 显示旧容量 |
| RC-007 | 数据丢失 — reclaimPolicy=Delete | 中 | PVC 删除后底层存储被清除 |
| RC-008 | Volume Terminating — Finalizer 阻塞 | 中 | PV/PVC 长时间 Terminating |

---

## 6. 修复操作

| 编号 | 对应根因 | 修复操作 | 风险等级 |
|------|---------|---------|:--------:|
| REM-001 | RC-001 | 创建正确的 StorageClass 或修正 PVC storageClassName | 🟡 |
| REM-002 | RC-002 | 重启 CSI Provisioner，检查 RBAC | 🟡 |
| REM-003 | RC-003 | 创建匹配 PV 或修正 PVC 请求参数 | 🟡 |
| REM-004 | RC-004 | `kubectl patch sc <name> -p '{"allowVolumeExpansion":true}'` | 🟡 |
| REM-005 | RC-005 | 升级 CSI 驱动版本 | 🟡 |
| REM-006 | RC-006 | 重启 Pod 触发文件系统扩展（resize2fs/xfs_growfs） | 🟡 |
| REM-007 | RC-007 | 🔴 无法恢复，只能从备份恢复。改为 Retain 策略 | 🔴 |
| REM-008 | RC-008 | 确认无 Pod 引用后移除 Finalizer | 🔴 |

---

## 7. 验证确认

### 即时验证

```bash
# 🟢 低风险
kubectl get pvc -n ${NAMESPACE}              # Bound
kubectl exec <pod> -- df -h <mount-path>     # 容量正确
kubectl exec <pod> -- touch <mount-path>/test && kubectl exec <pod> -- rm <mount-path>/test  # 可读写
```

### 解决标准

| 条件 | 判定 |
|------|------|
| PVC 状态 Bound | ✅ |
| Pod 内挂载点容量与 PVC 请求一致 | ✅ |
| 挂载点可正常读写 | ✅ |
| 回收策略为 Retain（生产） | ✅ |

---

## 8. 升级协议

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 数据丢失 | 立即从备份恢复，评估影响 |
| P1 | 扩容/绑定失败 | 检查 StorageClass 和 CSI |
| P2 | 存储优化 | 审查回收策略和容量规划 |

---

## 9. 版本兼容矩阵

| K8s 版本 | 存储关键变化 |
|---------|------------|
| 1.20-1.23 | CSI 在线扩容 GA；`CSIVolumeFSResize` 无需重启 Pod |
| 1.24-1.25 | in-tree 存储驱动 deprecated；`ReadWriteOncePod` Beta |
| 1.26-1.28 | `VolumeAttributesClass` Alpha；结构化参数 |
| 1.29-1.32 | `RecoverVolumeExpansionFailure` Beta |
| 1.34-1.36 | `VolumeAttributesClass` Beta→GA |

> [存疑：`ReadWriteOncePod` 在 1.29 的精确状态需确认 KEP-2477]

---

## 10. 知识进化

### 常见误诊模式

| 误诊模式 | 表现 | 正确做法 |
|---------|------|---------|
| 将 WaitForFirstConsumer 误判为故障 | PVC Pending 无错误事件 | 确认 volumeBindingMode，Pod 调度后自动绑定 |
| 扩容后以为立即生效 | PVC 显示新容量但 Pod 内未变 | 需重启 Pod 或等待在线 resize |
| 删除 PVC 前未确认回收策略 | 数据被自动删除 | 先 `kubectl get pv -o yaml` 确认 reclaimPolicy |

### 变更记录

| 版本 | 日期 | 变更内容 | 触发原因 |
|------|------|---------|---------|
| 1.0.0 | 2026-05-23 | 初版存储管理操作文档 | 技能库初始化 |
| 2.0.0 | 2026-07-23 | 重构为标准技能结构，补全根因/修复/验证/版本矩阵 | 技能建设最佳实践对标 |

---

## 生产案例

### 案例 1: PVC 扩容后文件系统未扩展

| 时间 | 事件 |
|------|------|
| 10:00 | 编辑 PVC 从 10Gi 扩容到 50Gi |
| 10:05 | `kubectl get pvc` 显示 50Gi，但 Pod 内 df 仍显示 10Gi |
| 10:08 | 需要重启 Pod 触发文件系统扩展 |
| 10:10 | 🟡 REM-006 重启 Pod，文件系统自动扩展 |

**根因**: RC-006。块设备扩容后需要文件系统 resize2fs/xfs_growfs，CSI 在 Pod 重启时执行。

### 案例 2: PV 回收策略 Delete 导致数据永久丢失

**现象**: 删除 PVC 后底层云盘被自动删除，数据无法恢复。

**诊断**: StorageClass reclaimPolicy=Delete

**修复**: 🔴 REM-007 无法恢复，只能从备份恢复。改为 Retain 策略。

### 案例 3: PV 长时间 Terminating — Finalizer 阻塞

**现象**: 删除 PVC 后 PV 状态 Terminating 超过 30min

**诊断**: `kubectl get pv <name> -o yaml` 显示 `kubernetes.io/pv-protection` finalizer

**修复**: 🔴 REM-008 确认无 Pod 引用后 `kubectl patch pv <name> -p '{"metadata":{"finalizers":null}}'`

---

## 相关链接

- [[技能/故障诊断-存储/csi-storage/csi-fta.md|CSI 存储异常诊断]] — 同域技能（CSI 层面故障）
- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]] — 方法论基础
- [[技能/故障诊断-控制面/etcd/backup-restore-etcd.md|etcd 备份恢复]] — 数据保护
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]] — 知识索引
- [[生态参考/领域索引/storage-index.md|Storage 存储知识图谱索引]] — 知识索引

<!-- risk-assessed -->
