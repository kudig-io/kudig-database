---
title: Longhorn (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- storage
- longhorn
- crd
- operator
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
- Longhorn 是什么
- 如何 Longhorn
trigger_keywords:
- Longhorn
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Longhorn

> **CNCF 状态**: Incubating | **类别**: Storage | **主要语言**: Go

## 概述

Longhorn 是由 Rancher Labs（现 SUSE）开源的云原生分布式块存储系统，2019 年加入 CNCF Sandbox，后晋升为 Incubating 项目。Longhorn 利用容器和微服务架构将存储控制器和数据平面容器化，为 Kubernetes 提供企业级持久化存储。它通过跨节点数据复制、快照、备份和灾难恢复功能，使有状态应用在 Kubernetes 上运行更加可靠和简单。

## 核心特性

- **分布式块存储**: 为 Kubernetes Pod 提供高可用的 PersistentVolume
- **同步复制**: 跨节点数据复制，默认 3 副本，可自定义副本数
- **快照与备份**: 支持定时快照，备份到 NFS/S3 兼容存储
- **跨集群灾难恢复**: 利用备份在另一个集群恢复数据卷
- **精简置备**: 按需分配存储空间，提高利用率
- **内置 UI**: 提供直观的 Web 管理界面管理卷和快照

## 架构

Longhorn 采用完全分布式架构，核心组件包括：Longhorn Manager（DaemonSet，管理卷生命周期）、Longhorn Engine（每个卷一个实例，负责数据复制和快照）、Longhorn UI（管理界面）、CSI Driver（实现 Kubernetes CSI 接口）。数据以多个 Replica 的形式分布在集群节点上，每个 Replica 是一个 Linux 进程。Longhorn Engine 接收来自 CSI 的 I/O 请求，同步写入所有 Replica，确保数据一致性。引擎本身也是容器化的，通过 Kubernetes 进行编排和管理。

## Kubernetes 集成

Longhorn 通过 CSI（Container Storage Interface）与 Kubernetes 集成，自动配置和挂载 PersistentVolume。它部署为 DaemonSet 在每个节点运行 Longhorn Manager，通过 Longhorn CSI Plugin 暴露存储能力。支持动态置备（Dynamic Provisioning）、StorageClass、Volume Snapshot 和 PVC 克隆等标准 K8s 存储 API。通过 Helm Chart 一键安装，无需修改节点配置。

## 生产使用场景

1. **数据库持久化**: 为 PostgreSQL、MySQL 等 StatefulSet 提供可靠的块存储
2. **Dev/Test 环境**: 在裸金属集群上替代云厂商 EBS/GPD，降低成本
3. **跨集群 DR**: 利用 S3 备份实现跨集群数据恢复，构建灾备方案
4. **边缘计算**: 轻量级部署，为边缘集群提供持久化能力

## 安装与配置

```bash
# Helm 安装 Longhorn
helm repo add longhorn https://charts.longhorn.io
helm repo update
helm install longhorn longhorn/longhorn --namespace longhorn-system --create-namespace \
  --set defaultSettings.defaultReplicaCount=3 \
  --set defaultSettings.backupTarget=s3://backup-bucket@us-east-1/ \
  --set persistence.defaultClassReplicaCount=3

# 等待组件就绪
kubectl wait --for=condition=available deployment/longhorn-driver-deployer -n longhorn-system --timeout=180s
kubectl get pods -n longhorn-system

# 访问 Longhorn UI
kubectl port-forward svc/longhorn-frontend 8080:80 -n longhorn-system
# 打开 http://localhost:8080
```

```yaml
# StorageClass 配置
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: longhorn-encrypted
provisioner: driver.longhorn.io
parameters:
  numberOfReplicas: "3"
  staleReplicaTimeout: "2880"
  fromBackup: ""
  encrypted: "true"
  fsType: "ext4"
reclaimPolicy: Retain
volumeBindingMode: Immediate
allowVolumeExpansion: true
---
# 定时快照和备份
apiVersion: longhorn.io/v1beta2
kind: RecurringJob
metadata:
  name: daily-backup
  namespace: longhorn-system
spec:
  cron: "0 2 * * *"  # 每天凌晨2点
  task: "backup"
  groups:
  - default
  retain: 7  # 保留7天
  concurrency: 2
---
# PVC 示例
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: postgres-data
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: longhorn-encrypted
  resources:
    requests:
      storage: 50Gi
```

## 运维操作

```bash
# 🟢 查看卷状态
kubectl get volumes.longhorn.io -n longhorn-system
kubectl get replicas.longhorn.io -n longhorn-system

# 🟢 查看节点磁盘状态
kubectl get nodes.longhorn.io -n longhorn-system -o yaml

# 🟡 在线扩容 PVC
kubectl patch pvc postgres-data -p '{"spec":{"resources":{"requests":{"storage":"100Gi"}}}}'

# 🟡 手动创建快照
kubectl apply -f - <<EOF
apiVersion: longhorn.io/v1beta2
kind: Snapshot
metadata:
  name: pre-upgrade-snap
  namespace: longhorn-system
spec:
  volume: pvc-xxxx
EOF

# 🟡 从备份恢复卷
kubectl apply -f - <<EOF
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: restored-data
spec:
  storageClassName: longhorn
  dataSource:
    name: backup-xxxx
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
  accessModes: ["ReadWriteOnce"]
  resources:
    requests:
      storage: 50Gi
EOF

# 🔴 删除卷（数据不可恢复）
kubectl delete volumes.longhorn.io pvc-xxxx -n longhorn-system
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 卷 Degraded | 副本数不足（节点故障） | `kubectl get volumes.longhorn.io -o yaml` | 等待自动重建或添加节点 |
| 卷 Detached | Engine Pod 崩溃 | `kubectl get pods -n longhorn-system` | 重启 Engine Pod |
| 备份失败 | S3/NFS 不可达或凭据过期 | `kubectl logs -n longhorn-system -l app=longhorn-manager` | 检查备份目标和凭据 |
| IO 性能下降 | 节点磁盘 IO 瓶颈或副本重建中 | `kubectl top nodes` + `iostat -x 1` | 等待重建完成或添加磁盘 |
| PVC 扩容失败 | 文件系统不支持或卷状态异常 | `kubectl describe pvc` | 确认卷 Attached 且 fsType 支持扩容 |

```
排查流程：
├── 卷状态异常
│   ├── kubectl get volumes.longhorn.io 查看状态
│   ├── 检查 Engine 和 Replica Pod 是否运行
│   ├── 查看 Longhorn UI 中的卷详情
│   └── 检查节点磁盘剩余空间
├── 副本重建失败
│   ├── 确认目标节点有足够磁盘空间
│   ├── 检查节点间网络连通性
│   ├── 查看 Replica 重建进度
│   └── 调整 replicaReplenishmentWaitInterval
└── 性能问题
    ├── iostat -x 1 检查磁盘 IO 利用率
    ├── 确认副本分布在不同节点
    ├── 检查是否正在重建（占用 IO）
    └── 考虑使用 SSD 节点
```

## 生产案例

### 案例 1：中小集群数据库持久化

- **场景**：10 节点 K8s 集群运行 PostgreSQL、Redis、RabbitMQ，需要可靠的持久化存储，无专业存储团队
- **排查**：之前使用 hostPath，节点故障后数据丢失，Pod 无法调度到其他节点
- **方案**：部署 Longhorn 3 副本，定时 S3 备份，StatefulSet 使用 Longhorn PVC
- **效果**：节点故障后数据零丢失，Pod 自动迁移恢复 < 2 分钟，运维复杂度极低

### 案例 2：跨集群灾难恢复

- **场景**：生产集群和 DR 集群，需要实现数据库级别的跨集群恢复，RPO < 1 小时
- **排查**：之前无 DR 方案，集群级故障将导致所有数据永久丢失
- **方案**：Longhorn 定时备份到 S3，DR 集群从 S3 恢复卷，定期演练恢复流程
- **效果**：RPO < 1 小时，RTO < 30 分钟，年度 DR 演练成功率 100%

## 替代方案

| 项目 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| **Longhorn** | 易部署、内置备份、UI 友好 | 性能不如 Ceph、规模有限 | 中小集群/边缘 |
| Rook/Ceph | 成熟稳定、高性能、大规模 | 运维复杂、资源开销大 | 大规模生产 |
| OpenEBS | 多引擎选择、CSI 原生 | 功能分散、文档不统一 | 灵活场景 |
| Linstor/DRBD | 高性能块复制 | 配置复杂、社区较小 | 高性能复制 |

## 架构定位

在 CNCF 生态中，Longhorn 属于 **Storage** 类别，是云原生块存储的代表性项目。它降低了分布式存储的运维门槛，特别适合中小规模 Kubernetes 集群和边缘场景。

## 参考链接

- [[22-概念/01-核心架构/controller-pattern.md|controller-pattern]]
- [[22-概念/04-存储/storage-model.md|storage-model]]

## Related

- [[cozystack]] — Cozystack
- [[fluid]] — Fluid
- [[23-实体/15-参考与索引/cncf-storage.md|cncf-storage]] — CNCF 存储与数据库项目全景
- [[kuasar]] — Kuasar
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- longhorn
- RELEASE-NOTES-1.9
- RELEASE-NOTES-0.8
- RELEASE-NOTES-1.8
- RELEASE-NOTES-1.3
- RELEASE-NOTES-0.2
- RELEASE-NOTES-1.7
- RELEASE-NOTES-0.6
- RELEASE-NOTES-1.6
- RELEASE-NOTES-0.7
- RELEASE-NOTES-1.2
- RELEASE-NOTES-0.3
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.10
- RELEASE-NOTES-0.4
- RELEASE-NOTES-1.1
- RELEASE-NOTES-1.0
- RELEASE-NOTES-1.4
- RELEASE-NOTES-1.11
- RELEASE-NOTES-0.5
- [[23-实体/05-存储/kanister.md|Kanister]]
- [[23-实体/05-存储/k8up.md|K8up]]
- [[23-实体/05-存储/openebs.md|OpenEBS]]
- [[23-实体/05-存储/hwameistor.md|HwameiStor]]
- [[23-实体/05-存储/carina.md|Carina]]
- [[23-实体/15-参考与索引/release-notes-storage.md|发布说明索引 — 存储]] — Cross-reference
- [[21-生态参考/98-merged-indexes/index.md|发布说明阅读指南]] — Cross-reference
- [[22-概念/12-研究/storage-tool-evolution.md|存储工具演进]] — Cross-reference
- [[21-生态参考/03-领域索引/backup-dr-index.md|Backup & DR 备份与灾备知识图谱索引]]
- [[21-生态参考/03-领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[21-生态参考/03-领域索引/storage-index.md|Storage 存储知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
- [[21-生态参考/03-领域索引/csi-index.md|CSI (Container Storage Interface) 知识图谱索引]]


<!-- risk-assessed -->
