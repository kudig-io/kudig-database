---
title: Carina
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- scheduler
- helm
- mysql
- statefulset
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Carina 是什么
- 如何 Carina
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Carina
- cncf
- landscape
---

# Carina

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **GitHub** | https://github.com/carina-io/carina |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Carina 是一个 Kubernetes 本地存储供应器，基于 LVM（Logical Volume Manager）管理节点上的本地磁盘，为有状态应用提供高性能的本地持久化存储。它自动发现节点上的裸盘，组建 LVM VolumeGroup，并通过 CSI 接口为 Pod 动态分配 LogicalVolume 作为 PersistentVolume，同时支持存储卷的扩容、快照和拓扑感知调度。

### 核心特性

- **自动磁盘管理**: 自动发现和管理节点本地磁盘
- **LVM 存储池**: 基于 LVM VG/LV 的动态存储分配
- **CSI 标准**: 完全兼容 Kubernetes CSI 接口
- **拓扑感知**: 调度器感知存储位置，Pod 调度到数据所在节点
- **卷扩容**: 支持在线 PVC 扩容
- **磁盘限速**: 基于 cgroup 的磁盘 IO 限速
- **RAID 支持**: 支持配置 RAID 0/1 保护数据

---

## 架构设计

```
┌──────────────────────────────────────────────────┐
│           Kubernetes Control Plane                 │
│                                                    │
│  ┌──────────────────────────────────┐             │
│  │    Carina Controller             │             │
│  │  (CSI Controller / 调度决策)      │             │
│  └──────────────┬───────────────────┘             │
└─────────────────┼─────────────────────────────────┘
                  │
    ┌─────────────▼──────────────────┐
    │          Node Agent             │
    │                                 │
    │  ┌───────────────────────┐     │
    │  │  Disk Discovery       │     │
    │  │  (自动发现裸盘)        │     │
    │  └───────────┬───────────┘     │
    │  ┌───────────▼───────────┐     │
    │  │  LVM Manager          │     │
    │  │  VG 创建 / LV 分配    │     │
    │  └───────────┬───────────┘     │
    │  ┌───────────▼───────────┐     │
    │  │  CSI Node Plugin      │     │
    │  │  挂载/卸载/扩容        │     │
    │  └───────────────────────┘     │
    │                                 │
    │  ┌─────────┐ ┌─────────┐      │
    │  │ /dev/sdb│ │ /dev/sdc│ ...  │
    │  │ (裸盘)  │ │ (裸盘)  │      │
    │  └─────────┘ └─────────┘      │
    └─────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# 安装 Carina
helm repo add carina https://carina-io.github.io/carina/
helm install carina carina/carina \
  --namespace kube-system
```

### 配置磁盘发现

```yaml
# 配置 Carina 自动发现的磁盘
apiVersion: v1
kind: ConfigMap
metadata:
  name: carina-config
  namespace: kube-system
data:
  config.json: |
    {
      "diskSelector": [
        {
          "name": "carina-vg-ssd",
          "re": ["sd[b-z]", "nvme[0-9]n[0-9]"],
          "policy": "LVM",
          "nodeLabel": "kubernetes.io/os=linux"
        }
      ],
      "diskScanInterval": "300",
      "schedulerStrategy": "spreadout"
    }
```

### 创建 StorageClass

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: carina-lvm-ssd
provisioner: carina.storage.io
parameters:
  carina.storage.io/disk-group-name: carina-vg-ssd
  carina.storage.io/filesystem-type: xfs
reclaimPolicy: Delete
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
```

### 使用本地存储

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-data
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: carina-lvm-ssd
  resources:
    requests:
      storage: 50Gi

---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
spec:
  serviceName: mysql
  replicas: 1
  selector:
    matchLabels:
      app: mysql
  template:
    metadata:
      labels:
        app: mysql
    spec:
      containers:
        - name: mysql
          image: mysql:8.0
          volumeMounts:
            - name: data
              mountPath: /var/lib/mysql
          resources:
            limits:
              carina.storage.io/disk-iops: "5000"     # IO 限速
              carina.storage.io/disk-bps: "200Mi"
      volumes:
        - name: data
          persistentVolumeClaim:
            claimName: mysql-data
```

---

## 与其他方案对比

| 特性 | Carina | OpenEBS LocalPV | TopoLVM | 原生 HostPath |
|:---|:---|:---|:---|:---|
| 磁盘管理 | LVM 自动化 | LVM/裸设备 | LVM | 无 |
| 自动发现 | 支持 | 手动 | 手动 | 无 |
| 动态分配 | CSI | CSI | CSI | 静态 |
| IO 限速 | 支持 | 不支持 | 不支持 | 不支持 |
| 卷扩容 | 在线扩容 | 支持 | 支持 | 不支持 |
| 拓扑感知 | 支持 | 支持 | 支持 | 手动 |

---

## 最佳实践

1. **磁盘规划**: 将系统盘和数据盘分开，Carina 只管理数据盘
2. **SSD/HDD 分池**: 为 SSD 和 HDD 创建不同的 VolumeGroup 和 StorageClass
3. **IO 限速**: 为共享磁盘的工作负载设置 IO 限速，避免互相影响
4. **扩容预留**: 初始分配适量空间，利用在线扩容按需增长
5. **监控**: 监控各节点 VolumeGroup 的剩余空间，及时扩容或添加磁盘

---

## 参考资源

- [Carina GitHub](https://github.com/carina-io/carina)
- [Carina 文档](https://github.com/carina-io/carina/blob/main/docs/manual/install.md)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
