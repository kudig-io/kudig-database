# Piraeus Datastore

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://piraeus.io/ |
| **GitHub** | https://github.com/piraeusdatastore/piraeus-operator |
| **许可证** | Apache-2.0 |
| **开发语言** | Go, Java |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Piraeus Datastore 是基于 LINSTOR 和 DRBD 技术的 Kubernetes 高可用存储解决方案。它提供高性能的块存储，支持同步复制、快照、加密和灾难恢复。Piraeus 将成熟的 Linux 存储技术（DRBD 同步复制已有 20+ 年历史）与 Kubernetes 原生体验结合，为有状态应用提供企业级存储。

### 核心特性

- **同步复制**: 基于 DRBD 的同步数据复制，零数据丢失
- **高性能**: 接近裸盘的 IOPS 和延迟，支持 NVMe 和 SSD
- **快照和克隆**: 支持卷快照和即时克隆
- **加密**: 内置卷加密支持
- **CSI 驱动**: 完整的 Kubernetes CSI 实现
- **Operator 管理**: Kubernetes Operator 自动化部署和运维

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                  Piraeus Datastore                    │
│                                                       │
│  ┌──────────────────────────────────────────────┐    │
│  │            Piraeus Operator                    │    │
│  │  (部署和管理 LINSTOR 组件)                    │    │
│  └─────────────────────┬────────────────────────┘    │
│                        │                              │
│  ┌─────────────────────▼────────────────────────┐    │
│  │          LINSTOR Controller (HA)              │    │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐   │    │
│  │  │Controller│  │Controller│  │Controller│   │    │
│  │  │  Node 1  │  │  Node 2  │  │  Node 3  │   │    │
│  │  └──────────┘  └──────────┘  └──────────┘   │    │
│  └─────────────────────┬────────────────────────┘    │
│                        │                              │
│  ┌─────────────────────▼────────────────────────┐    │
│  │              LINSTOR Satellites               │    │
│  │  ┌──────────────────────────────────────────┐│    │
│  │  │               Node 1                      ││    │
│  │  │  ┌──────────┐  ┌──────────────────────┐ ││    │
│  │  │  │ LINSTOR  │  │  DRBD Resources       │ ││    │
│  │  │  │ Satellite│  │  ┌─────┐ ┌─────┐     │ ││    │
│  │  │  └──────────┘  │  │Vol A│ │Vol B│     │ ││    │
│  │  │                │  └──┬──┘ └──┬──┘     │ ││    │
│  │  └────────────────┼─────┼───────┼────────┘ ││    │
│  │                   │     │       │           ││    │
│  │  ┌────────────────┼─────┼───────┼──────────┐│    │
│  │  │     Storage    │  ┌──▼──┐ ┌──▼──┐       ││    │
│  │  │     Pool       │  │ LVM │ │ LVM │       ││    │
│  │  │                │  │Pool │ │Pool │       ││    │
│  │  │                │  └─────┘ └─────┘       ││    │
│  │  └────────────────┴────────────────────────┘│    │
│  └──────────────────────────────────────────────┘    │
│                                                       │
│         DRBD 同步复制 (Node 1 <-> Node 2)            │
└──────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装 Piraeus Operator

```bash
# 添加 Helm 仓库
helm repo add piraeus-charts https://piraeus.io/helm-charts/

# 安装 Operator
helm install piraeus-operator piraeus-charts/piraeus-operator \
  --namespace piraeus-datastore \
  --create-namespace
```

### 部署 LINSTOR 集群

```yaml
# linstorcluster.yaml
apiVersion: piraeus.io/v1
kind: LinstorCluster
metadata:
  name: linstorcluster
spec:
  # 存储池配置
  storagePools:
    lvmThinPools:
      - name: lvm-thin
        volumeGroup: drbdpool
        thinPool: thinpool
  
  # 节点选择
  nodeAffinity:
    nodeSelectorTerms:
      - matchExpressions:
          - key: piraeus.io/storage-node
            operator: Exists
```

```bash
kubectl apply -f linstorcluster.yaml
```

### 创建 StorageClass

```yaml
# storageclass.yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: piraeus-replicated
provisioner: linstor.csi.linbit.com
parameters:
  # 2 副本
  placementCount: "2"
  storagePool: lvm-thin
  resourceGroup: ssd-storage
  
  # 文件系统类型
  fsType: ext4
  
  # 允许扩容
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
```

### 创建 PVC

```yaml
# pvc.yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: database-storage
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: piraeus-replicated
  resources:
    requests:
      storage: 100Gi
```

---

## 高级功能

### 快照

```yaml
# 创建 VolumeSnapshotClass
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: piraeus-snapshot
driver: linstor.csi.linbit.com
deletionPolicy: Delete
---
# 创建快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: database-snapshot
spec:
  volumeSnapshotClassName: piraeus-snapshot
  source:
    persistentVolumeClaimName: database-storage
```

### 卷加密

```yaml
# 加密 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: piraeus-encrypted
provisioner: linstor.csi.linbit.com
parameters:
  placementCount: "2"
  storagePool: lvm-thin
  
  # 启用加密
  encryption: "true"
  encryptionPassphrase: "${ENCRYPTION_KEY}"
```

### 跨区域复制

```yaml
# 配置跨区域的副本放置
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: piraeus-ha
provisioner: linstor.csi.linbit.com
parameters:
  placementCount: "3"
  storagePool: lvm-thin
  
  # 在不同可用区放置副本
  replicasOnSame:
    - kubernetes.io/hostname
  replicasOnDifferent:
    - topology.kubernetes.io/zone
```

### LINSTOR CLI 操作

```bash
# 进入 LINSTOR 控制器 Pod
kubectl exec -it linstor-controller-0 -n piraeus-datastore -- bash

# 查看节点
linstor node list

# 查看存储池
linstor storage-pool list

# 查看资源
linstor resource list

# 查看卷状态
linstor volume list
```

---

## 与其他方案对比

| 特性 | Piraeus | Longhorn | OpenEBS | Rook/Ceph |
|:---|:---|:---|:---|:---|
| 复制技术 | DRBD 同步 | 异步 | 取决于引擎 | Ceph |
| 数据一致性 | 强一致 | 最终一致 | 引擎相关 | 强一致 |
| 性能 | 接近裸盘 | 中等 | 引擎相关 | 中等 |
| 快照 | 支持 | 支持 | 支持 | 支持 |
| 加密 | 内置 | 需配置 | 引擎相关 | 支持 |
| 运维复杂度 | 中等 | 低 | 中等 | 高 |

---

## 最佳实践

1. **专用存储节点**: 为存储工作负载配置专用节点，避免与计算混部
2. **SSD/NVMe**: 使用 SSD 或 NVMe 获得最佳性能
3. **副本规划**: 生产环境至少 2 副本，跨可用区部署 3 副本
4. **监控 DRBD**: 监控 DRBD 同步状态，及时发现脑裂或同步延迟
5. **定期快照**: 配置定期快照策略，保护关键数据

---

## 参考资源

- [Piraeus 官方文档](https://piraeus.io/docs/)
- [Piraeus Operator GitHub](https://github.com/piraeusdatastore/piraeus-operator)
- [LINSTOR 文档](https://linbit.com/drbd-user-guide/linstor-guide-1_0-en/)
- [DRBD 项目](https://linbit.com/drbd/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
