---
title: PersistentVolume (PV)
summary: PersistentVolume (PV)：PersistentVolume（PV）是 Kubernetes 集群中的一块存储资源，由管理员预先配置或通过
  StorageClass 动态供给。PV 独立于使用它的 Pod 生命周期，用于为应用提供持久化存储能力。
category: concepts
tags:
- storage
- pv
- persistent-volume
- core
- visibility/public
tier: core
sources:
- concepts/
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---


# PersistentVolume (PV)

## 概述

PersistentVolume（PV，持久化卷）是 Kubernetes 中代表**集群一块已 provisioned 存储资源**的对象。它由管理员静态预创建，或由 StorageClass 在用户创建 PVC 时动态生成。PV 的生命周期独立于 Pod——Pod 被删除重建，PV 仍然存在并可被新 Pod 重新挂载。PV 与 PVC 解耦了"存储供应"（管理员/基础设施侧）与"存储消费"（开发者/应用侧），是 Kubernetes 存储体系的核心抽象。

## 架构与工作原理

```
┌──────── 管理员/动态供给（供给侧）─────────┐
│   静态：手工创建 PV（NFS/本地盘/云盘）      │
│   动态：StorageClass + CSI → 自动创建 PV    │
└───────────────────┬───────────────────────┘
                    │
                    ▼
            PersistentVolume (PV)
            spec.capacity / accessModes / reclaimPolicy
                    │ 绑定（1:1）
                    ▼
            PersistentVolumeClaim (PVC)  ← 用户声明需求
                    │ 挂载引用
                    ▼
              Pod (volumeMounts)
```

**PV 生命周期阶段（phase）**：
- `Available`：可用，未绑定到任何 PVC。
- `Bound`：已与某个 PVC 绑定。
- `Released`：PVC 已删除，但 PV 仍保留（reclaimPolicy=Retain），未回收。
- `Failed`：自动回收失败。

**回收策略（persistentVolumeReclaimPolicy）**：
- `Retain`：PVC 删除后 PV 保留为 Released，需管理员手工清理底层卷和 PV。**生产数据库推荐**。
- `Delete`：PVC 删除时连 PV 和底层存储卷一起删除。动态供给常用，**慎用于关键数据**。
- `Recycle`（已弃用）：旧版基本废弃，用动态供给替代。

**访问模式（accessModes）**：

| 模式 | 缩写 | 含义 |
|------|------|------|
| ReadWriteOnce | RWO | 单节点读写（云盘、本地盘） |
| ReadOnlyMany | ROX | 多节点只读 |
| ReadWriteMany | RWX | 多节点读写（NFS、CephFS、对象存储） |
| ReadWriteOncePod | RWOP | 单 Pod 读写（1.22+，比 RWO 更严） |

## 关键组件与特性

| 字段 | 作用 |
|------|------|
| `spec.capacity.storage` | 容量 |
| `spec.accessModes` | 访问模式（RWO/ROX/RWX/RWOP） |
| `spec.persistentVolumeReclaimPolicy` | Retain / Delete |
| `spec.storageClassName` | 关联 SC，空串表示静态 |
| `spec.volumeMode` | Filesystem（默认）/ Block |
| `spec.nodeAffinity` | 拓扑约束（本地盘/云盘限制节点） |
| `spec.claimRef` | 绑定的 PVC 引用 |
| `status.phase` | Available/Bound/Released/Failed |
| CSI 卷属性 | driver / volumeHandle / fsType 等 |

## 配置示例

```yaml
---
# 1. 静态 PV（NFS）
apiVersion: v1
kind: PersistentVolume
metadata:
  name: pv-nfs-data
  labels: {type: nfs}
spec:
  capacity: {storage: 100Gi}
  accessModes: [ReadWriteMany]            # NFS 支持多节点读写
  persistentVolumeReclaimPolicy: Retain
  storageClassName: nfs-static            # 空串或显式名
  nfs:
    server: nfs.example.com
    path: /export/data
    readOnly: false
---
# 2. 本地盘 PV（带 nodeAffinity）
apiVersion: v1
kind: PersistentVolume
metadata:
  name: pv-local-ssd-node1
spec:
  capacity: {storage: 500Gi}
  accessModes: [ReadWriteOnce]
  persistentVolumeReclaimPolicy: Retain
  storageClassName: local-storage
  local: {path: /mnt/ssd}
  nodeAffinity:
    required:
      nodeSelectorTerms:
      - matchExpressions:
        - {key: kubernetes.io/hostname, operator: In, values: [node-1]}
---
# 3. 用户 PVC（静态绑定 / 动态触发）
apiVersion: v1
kind: PersistentVolumeClaim
metadata: {name: data, namespace: production}
spec:
  storageClassName: fast-ssd             # 动态：SC 自动创建 PV
  accessModes: [ReadWriteOnce]
  resources: {requests: {storage: 50Gi}}
---
# 4. Pod 挂载 PVC
apiVersion: v1
kind: Pod
metadata: {name: db, namespace: production}
spec:
  containers:
  - name: db
    image: postgres:16
    volumeMounts:
    - {name: data, mountPath: /var/lib/postgresql/data}
  volumes:
  - name: data
    persistentVolumeClaim: {claimName: data}
```

## 常用操作与命令

```bash
# 查看 PV/PVC 与绑定状态
kubectl get pv,pvc -n production
kubectl get pv -o custom-columns=NAME:.metadata.name,CAP:.spec.capacity.storage,MODE:.spec.accessModes,POLICY:.spec.persistentVolumeReclaimPolicy,STATUS:.status.phase,CLAIM:.spec.claimRef.name

# 详细诊断（PVC Pending 看这里）
kubectl describe pv <pv-name>
kubectl describe pvc <pvc-name> -n production

# 保留策略修改（动态 PV 改 Retain 防误删）
kubectl patch pv pvc-xxx -p '{"spec":{"persistentVolumeReclaimPolicy":"Retain"}}'

# 清理 Released PV（重新变 Available）
kubectl patch pv pvc-xxx --type=json -p='[{"op":"remove","path":"/spec/claimRef"}]'

# 在线扩容 PVC（要求 SC allowVolumeExpansion=true）
kubectl patch pvc data -p '{"spec":{"resources":{"requests":{"storage":"100Gi"}}}}' -n production

# PV 快照（需 VolumeSnapshotClass）
kubectl apply -f - <<EOF
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata: {name: db-snap, namespace: production}
spec:
  volumeSnapshotClassName: csi-snap
  source: {persistentVolumeClaimName: data}
EOF
kubectl get volumesnapshot -n production
```

## 最佳实践

1. **数据库用 Retain**：reclaimPolicy=Retain 防止误删 PVC 导致数据丢失，关键数据保险绳。
2. **优先动态供给**：除非有静态资源（NFS/本地盘），否则一律走 StorageClass 动态，减少人工。
3. **跨 AZ 用 WaitForFirstConsumer**：避免卷与 Pod 跨 AZ，挂载失败。
4. **RWX 仅共享存储可用**：云盘只支持 RWO；多 Pod 共享必须用 NFS/CephFS/对象存储。
5. **PV 快照做备份**：用 VolumeSnapshot 定期快照，配合 Velero 做集群级备份恢复。
6. **容量预留与扩容**：初始给够并开启 allowVolumeExpansion，避免停机扩容。
7. **本地盘配 nodeAffinity**：本地卷强绑定节点，Pod 必须有 nodeAffinity 防漂移失败。
8. **监控 PV 容量与 IOPS**：磁盘满/限速是常见故障源，纳入 Prometheus 告警。

## 远程顾问诊断要点

- 询问用户 PV 的供给方式（静态/动态）
- 检查 StorageClass 配置和 provisioner 状态
- 确认 PV 的 ReclaimPolicy（Retain/Recycle/Delete）

## 常见陷阱

- **PVC 一直 Pending（无绑定）**：静态 PV 不匹配（accessModes/capacity/storageClass/selector），或动态 provisioner 异常。
- **删除 PVC 后 PV 卡 Released**：reclaimPolicy=Retain，需手工清 claimRef 或删除 PV。
- **Pod 挂载失败（Multi-Attach）**：RWO 卷被多个节点上的 Pod 同时引用；改用 RWX 或确保单 Pod。
- **本地盘节点变更**：nodeAffinity 锁的节点被移除，PV 永久不可用。
- **跨 AZ 挂载失败**：动态卷 Immediate 绑定到 AZ-a，Pod 调度到 AZ-b；改 WaitForFirstConsumer。
- **扩容不支持**：SC 未开 allowVolumeExpansion，只能新建卷迁移。
- **Delete 策略误删数据**：测试环境用 Delete 清理方便，生产误用导致数据丢失。
- **in-tree volume 弃用**：老 PV 用 kubernetes.io/aws-ebs 等 in-tree 插件，迁移 CSI 后无法管理。

## 相关链接

- [[概念/persistent-volume-claim.md|PersistentVolumeClaim]] — PVC 声明与绑定
- [[概念/storageclass.md|StorageClass]] — 存储类动态供给
- [[概念/kubernetes.md|Kubernetes]] — 核心概念
- [[概念/statefulset.md|StatefulSet]] — 每副本独立 PVC
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
