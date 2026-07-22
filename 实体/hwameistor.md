---
title: HwameiStor (entities)
description: '## 概述'
summary: 'HwameiStor 是一个 Kubernetes 原生的高可用本地存储系统，能够将节点上的本地磁盘（HDD、SSD、NVMe）统一管理并提供分布式的本地存储服务。它通过 CSI 接口为有状态应用提供高性能的本地持久卷，并支持卷的高可用副本、数据迁移和自动化运维。HwameiStor 特别适合对 IOPS 和延迟敏感的工作负载，'
category: entities
tags:
- k8s
- cncf
- storage
- hwameistor
- crd
- operator
- serverless
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- HwameiStor 是什么
- 如何 HwameiStor
trigger_keywords:
- HwameiStor
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# HwameiStor

> **CNCF 状态**: Sandbox | **类别**: Storage | **主要语言**: Go

## 概述

HwameiStor 是由 Daocloud（道客）开发的开源 Kubernetes 本地存储系统，2022 年进入 CNCF Sandbox。它将集群节点上的本地磁盘（HDD、SSD、NVMe）统一池化管理，通过 CSI（Container Storage Interface）为有状态应用提供高性能的本地持久卷（Local PV）。与传统分布式存储（如 Ceph）不同，HwameiStor 将数据存储在 Pod 所在节点的本地磁盘上，避免了网络开销，提供接近裸金属的 IOPS 和延迟。

HwameiStor 通过**卷副本（Replica）**机制实现高可用：每个卷可以配置 1-3 个副本，副本分布在不同的故障域（节点/机架）。当节点故障时，卷可以自动迁移到其他节点并从副本恢复数据。它还提供磁盘热替换、卷扩容、快照等企业级存储功能。

## Key Features

- **本地磁盘池化**：自动发现和管理 HDD、SSD、NVMe 磁盘，按介质类型创建存储池
- **高可用副本**：支持 1-3 副本配置，副本跨故障域分布，单节点故障不丢数据
- **CSI 标准接口**：完全兼容 Kubernetes CSI，支持动态卷创建、扩容、快照
- **数据迁移**：节点维护时可将卷在线迁移到其他节点，业务无感知
- **磁盘热替换**：支持磁盘故障后热替换和数据重建
- **混合存储池**：按性能（SSD/HDD）划分 StorageClass，应用按需选择

## Architecture

HwameiStor 由 **LocalDiskManager**（管理节点磁盘资源的 CRD 控制器）、**LocalStorage**（管理卷和副本的核心控制器）、**Scheduler**（调度器扩展，确保 Pod 调度到有足够本地存储的节点）和 **Admission Controller**（校验 PVC 请求）组成。数据面使用 DRBD 或 LVM 实现卷管理和副本同步。每个卷对应一个 `LocalVolume` CR，副本对应 `LocalVolumeReplica` CR，控制器监控这些 CR 并协调实际的块设备操作。

## K8s 集成

HwameiStor 通过 CSI Driver 与 Kubernetes 深度集成。创建 PVC 时，StorageClass 指向 HwameiStor CSI，调度器扩展确保 Pod 调度到有对应存储池和容量的节点。卷挂载为本地块设备（`/dev/xxx`），通过文件系统（ext4/xfs）格式化后挂载到 Pod。支持 Kubernetes 原生的 VolumeSnapshot、PVC 扩容等标准操作。

## 生产部署要点

- **磁盘分类**：将 SSD 和 HDD 分配到不同存储池，按性能需求选择 StorageClass
- **副本拓扑**：配置副本拓扑约束确保副本分布在不同故障域
- **监控告警**：监控 LocalDisk 和 LocalVolume 的 conditions，及时发现问题
- **定期迁移**：在节点维护前提前进行卷迁移，减少业务影响
- **资源预留**：为每个节点预留足够的磁盘空间用于数据重建

## 生产场景

1. **数据库高性能存储**：MySQL/PostgreSQL 使用 HwameiStor 本地卷，获得接近裸盘的 IOPS
2. **AI/ML 训练数据**：GPU 节点的本地 NVMe 为训练任务提供低延迟数据读取
3. **消息队列持久化**：Kafka/RabbitMQ 使用多副本本地卷保证数据安全
4. **成本敏感的大数据**：使用 HDD 存储池替代昂贵的网络存储，降低成本

## 安装与配置

```bash
# 使用 Helm 安装 HwameiStor
helm repo add hwameistor https://hwameistor.io/storage
helm repo update
helm install hwameistor hwameistor/hwameistor -n hwameistor --create-namespace \
  --set storagePool.hdd.enabled=true \
  --set storagePool.ssd.enabled=true \
  --set scheduler.enabled=true

# 等待组件就绪
kubectl wait --for=condition=available deployment/hwameistor-local-storage -n hwameistor --timeout=120s
kubectl get pods -n hwameistor

# 查看磁盘和存储池
kubectl get localdisk
kubectl get localstoragepool
kubectl get localdisknode
```

```yaml
# StorageClass 配置（SSD 高性能池）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: hwameistor-storage-lvm-ssd
provisioner: disk.hwameistor.io
parameters:
  poolType: LocalStorage_PoolSSD
  replicaNumber: "2"
  striped: "true"
reclaimPolicy: Retain
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
---
# PVC 示例
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: data-mysql
  namespace: production
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: hwameistor-storage-lvm-ssd
  resources:
    requests:
      storage: 100Gi
---
# 卷快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: mysql-snap
  namespace: production
spec:
  volumeSnapshotClassName: hwameistor-snapshot
  source:
    persistentVolumeClaimName: data-mysql
```

## 运维操作

```bash
# 🟢 查看卷状态和副本分布
kubectl get localvolume -A
kubectl get localvolumereplica -A -o wide

# 🟢 查看磁盘健康状态
kubectl get localdisk -o custom-columns=NAME:.metadata.name,NODE:.spec.nodeName,TYPE:.spec.diskType,STATE:.status.state

# 🟡 在线扩容 PVC
kubectl patch pvc data-mysql -n production -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'

# 🟡 节点维护前迁移卷
kubectl apply -f - <<EOF
apiVersion: hwameistor.io/v1alpha1
kind: LocalVolumeMigrate
metadata:
  name: migrate-to-node2
spec:
  volumeName: pvc-xxxx
  sourceNode: node-1
  targetNode: node-2
  abortOnFailure: true
EOF

# 🔴 删除故障磁盘（数据将重建）
kubectl patch localdisk disk-node1-sdb -p '{"spec":{"state":"Inactive"}}'

# 🟢 查看存储池容量使用率
kubectl get localstoragepool -o custom-columns=NAME:.metadata.name,TOTAL:.spec.totalCapacity,FREE:.status.freeCapacity
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| PVC Pending | 目标节点存储池容量不足 | `kubectl describe pvc <name>` | 扩容磁盘或更换 StorageClass |
| 副本同步失败 | 节点间网络不通或磁盘故障 | `kubectl get localvolumereplica` | 检查节点网络，替换故障磁盘 |
| Pod 调度失败 | 调度器扩展未运行或节点无匹配存储池 | `kubectl logs -n hwameistor -l app=hwameistor-scheduler` | 重启 scheduler 组件 |
| 卷挂载超时 | 节点上 CSI 插件异常 | `kubectl logs -n hwameistor -l app=hwameistor-csi-node` | 重启 CSI DaemonSet Pod |
| 数据重建慢 | 磁盘 IO 瓶颈或副本数不足 | `kubectl get localvolume -o yaml` | 等待重建完成或添加磁盘 |

```
排查流程：
├── PVC Pending
│   ├── kubectl describe pvc 查看事件
│   ├── 确认 StorageClass 存在且 provisioner 正确
│   ├── 检查目标节点存储池剩余容量
│   └── 确认 WaitForFirstConsumer 模式下 Pod 已调度
├── 卷副本异常
│   ├── kubectl get localvolumereplica 查看副本状态
│   ├── 检查副本所在节点磁盘健康
│   ├── 检查节点间网络连通性
│   └── 查看 LocalStorage 控制器日志
└── 性能问题
    ├── iostat -x 1 检查磁盘 IO 利用率
    ├── 确认 SSD/HDD 存储池分类正确
    └── 检查 striped 参数是否启用
```

## 生产案例

### 案例 1：MySQL 集群本地存储高性能方案

- **场景**：生产 MySQL 集群需要高 IOPS 低延迟存储，网络存储（Ceph）延迟 2-5ms 无法满足 P99 < 1ms 要求
- **排查**：Ceph RBD 网络往返 + OSD 处理延迟导致 MySQL 慢查询增多，业务超时告警频繁
- **方案**：迁移到 HwameiStor SSD 存储池，2 副本配置，WaitForFirstConsumer 绑定模式，Pod 反亲和确保副本跨节点
- **效果**：IO 延迟从 2-5ms 降至 0.1-0.3ms，MySQL P99 查询时间从 50ms 降至 5ms，存储成本降低 40%

### 案例 2：节点故障自动卷迁移

- **场景**：Kafka 集群使用 HwameiStor 2 副本本地卷，一台节点磁盘故障导致 Pod 无法启动
- **排查**：节点磁盘 SMART 报警，LocalVolumeReplica 状态变为 Degraded，Pod 处于 Pending
- **方案**：HwameiStor 自动从健康副本恢复数据到新节点，LocalVolumeMigrate CR 触发卷迁移，Kafka Pod 自动重调度
- **效果**：从磁盘故障到 Kafka 完全恢复仅 8 分钟（含数据重建），无数据丢失，业务无感知

## 对比

| 特性 | HwameiStor | Longhorn | OpenEBS (cStor) | Rook/Ceph | 适用场景 |
|------|-----------|----------|-----------------|-----------|----------|
| 存储类型 | 本地卷 | 分布式块 | 分布式块 | 分布式块/文件 | 性能 vs 灵活性 |
| 性能 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ | 数据库/AI 训练 |
| 高可用 | 副本+迁移 | 副本 | 副本 | 副本 | 生产可靠性 |
| 网络依赖 | 低（本地） | 高 | 高 | 高 | 边缘/离线场景 |
| 运维复杂度 | 低 | 低 | 中 | 高 | 小团队运维 |

## 参考链接

- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/storage-model.md|storage-model]]
- [[实体/csi-drivers.md|csi-drivers]]

## Related

- [[bootc]] — bootc
- [[serverless-workflow]] — Serverless Workflow
- [[cloudnativepg]] — CloudNativePG
- [[strimzi]] — Strimzi
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- hwameistor
- [[实体/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/领域索引/storage-index.md|Storage 存储知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
