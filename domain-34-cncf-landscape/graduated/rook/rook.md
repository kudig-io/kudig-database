---
title: Rook
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- grafana
- rook
- ceph
- mysql
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Rook 是什么
- 如何 Rook
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Rook
- cncf
- landscape
---


# Rook

> **成熟度**: Graduated | **加入时间**: 2018-01 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://rook.io |
| **GitHub** | https://github.com/rook/rook |
| **文档** | https://rook.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Storage |

---

## 项目概述

### 简介
Rook 是云原生存储编排器，通过 Kubernetes Operator 模式将分布式存储系统(主要是 Ceph)转变为自管理、自扩展、自修复的存储服务，为 Kubernetes 应用提供块存储、文件存储和对象存储。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2016-11 | Quantum 公司发布 Rook |
| 2018-01 | 加入 CNCF Sandbox |
| 2018-09 | 晋升为 CNCF Incubating |
| 2020-10 | 晋升为 CNCF Graduated |

### 核心定位
Rook 是 Kubernetes 上运行 Ceph 存储集群的最佳方式，将复杂的 Ceph 运维自动化，使存储管理像管理无状态应用一样简单。

---

## 架构设计

### 整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                      Rook-Ceph 架构                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    Kubernetes Cluster                        ││
│  │                                                              ││
│  │  ┌─────────────────┐    ┌─────────────────┐                 ││
│  │  │  Rook Operator  │    │   CSI Driver    │                 ││
│  │  │  (控制平面)     │    │  (存储接口)     │                 ││
│  │  └────────┬────────┘    └────────┬────────┘                 ││
│  │           │                      │                           ││
│  │           ▼                      ▼                           ││
│  │  ┌─────────────────────────────────────────────────────────┐││
│  │  │                    Ceph Cluster                          │││
│  │  │                                                          │││
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐      │││
│  │  │  │  MON Pod    │  │  MON Pod    │  │  MON Pod    │      │││
│  │  │  │ (监控器)    │  │ (监控器)    │  │ (监控器)    │      │││
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘      │││
│  │  │                                                          │││
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐      │││
│  │  │  │  MGR Pod    │  │  MDS Pod    │  │  RGW Pod    │      │││
│  │  │  │ (管理器)    │  │ (元数据)    │  │ (对象网关)  │      │││
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘      │││
│  │  │                                                          │││
│  │  │  ┌─────────────────────────────────────────────────┐    │││
│  │  │  │              OSD Pods (数据存储)                 │    │││
│  │  │  │  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐      │    │││
│  │  │  │  │OSD 0│ │OSD 1│ │OSD 2│ │OSD 3│ │OSD N│      │    │││
│  │  │  │  └─────┘ └─────┘ └─────┘ └─────┘ └─────┘      │    │││
│  │  │  └─────────────────────────────────────────────────┘    │││
│  │  └─────────────────────────────────────────────────────────┘││
│  │                                                              ││
│  │  ┌─────────────────────────────────────────────────────────┐││
│  │  │                   Applications                           │││
│  │  │  ┌────────────┐ ┌────────────┐ ┌────────────┐           │││
│  │  │  │ Block (RBD)│ │Shared (CephFS)│ │ Object (S3)│         │││
│  │  │  │ PVC        │ │ PVC          │ │ Bucket     │         │││
│  │  │  └────────────┘ └────────────┘ └────────────┘           │││
│  │  └─────────────────────────────────────────────────────────┘││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### Ceph 组件说明

| 组件 | 功能 | 说明 |
|:---|:---|:---|
| **MON** | Monitor | 集群状态、配置管理 |
| **MGR** | Manager | 监控、仪表板、模块 |
| **OSD** | Object Storage Daemon | 实际数据存储 |
| **MDS** | Metadata Server | CephFS 元数据 |
| **RGW** | RADOS Gateway | S3/Swift 接口 |

---

## 安装部署

### 部署 Rook Operator

```bash
# 克隆 Rook 仓库
git clone --single-branch --branch v1.13.0 https://github.com/rook/rook.git
cd rook/deploy/examples

# 部署 CRDs 和 Operator
kubectl create -f crds.yaml -f common.yaml -f operator.yaml

# 验证 Operator 运行
kubectl -n rook-ceph get pods
```

### 创建 Ceph 集群

```yaml
# cluster.yaml
apiVersion: ceph.rook.io/v1
kind: CephCluster
metadata:
  name: rook-ceph
  namespace: rook-ceph
spec:
  cephVersion:
    image: quay.io/ceph/ceph:v18.2.0
    allowUnsupported: false
  dataDirHostPath: /var/lib/rook
  
  mon:
    count: 3
    allowMultiplePerNode: false
  
  mgr:
    count: 2
    modules:
      - name: pg_autoscaler
        enabled: true
      - name: dashboard
        enabled: true
  
  dashboard:
    enabled: true
    ssl: true
  
  storage:
    useAllNodes: true
    useAllDevices: true
    # 或指定设备
    # nodes:
    #   - name: "node1"
    #     devices:
    #       - name: "sdb"
    #       - name: "sdc"
  
  # 资源限制
  resources:
    mon:
      limits:
        cpu: "2"
        memory: "2Gi"
      requests:
        cpu: "500m"
        memory: "1Gi"
    osd:
      limits:
        cpu: "2"
        memory: "4Gi"
      requests:
        cpu: "500m"
        memory: "2Gi"
```

---

## 存储类型

### 1. 块存储 (RBD)

```yaml
# 创建 CephBlockPool
apiVersion: ceph.rook.io/v1
kind: CephBlockPool
metadata:
  name: replicapool
  namespace: rook-ceph
spec:
  failureDomain: host
  replicated:
    size: 3
    requireSafeReplicaSize: true

---
# 创建 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: rook-ceph-block
provisioner: rook-ceph.rbd.csi.ceph.com
parameters:
  clusterID: rook-ceph
  pool: replicapool
  imageFormat: "2"
  imageFeatures: layering
  csi.storage.k8s.io/provisioner-secret-name: rook-csi-rbd-provisioner
  csi.storage.k8s.io/provisioner-secret-namespace: rook-ceph
  csi.storage.k8s.io/node-stage-secret-name: rook-csi-rbd-node
  csi.storage.k8s.io/node-stage-secret-namespace: rook-ceph
reclaimPolicy: Delete
allowVolumeExpansion: true

---
# 使用 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-pvc
spec:
  storageClassName: rook-ceph-block
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 10Gi
```

### 2. 共享文件系统 (CephFS)

```yaml
# 创建 CephFilesystem
apiVersion: ceph.rook.io/v1
kind: CephFilesystem
metadata:
  name: myfs
  namespace: rook-ceph
spec:
  metadataPool:
    replicated:
      size: 3
  dataPools:
    - name: data0
      replicated:
        size: 3
  metadataServer:
    activeCount: 1
    activeStandby: true
    resources:
      limits:
        cpu: "2"
        memory: "4Gi"

---
# StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: rook-cephfs
provisioner: rook-ceph.cephfs.csi.ceph.com
parameters:
  clusterID: rook-ceph
  fsName: myfs
  pool: myfs-data0
reclaimPolicy: Delete
allowVolumeExpansion: true

---
# 多 Pod 共享 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: shared-data
spec:
  storageClassName: rook-cephfs
  accessModes:
    - ReadWriteMany  # 支持多节点挂载
  resources:
    requests:
      storage: 100Gi
```

### 3. 对象存储 (S3)

```yaml
# 创建 CephObjectStore
apiVersion: ceph.rook.io/v1
kind: CephObjectStore
metadata:
  name: my-store
  namespace: rook-ceph
spec:
  metadataPool:
    replicated:
      size: 3
  dataPool:
    replicated:
      size: 3
  gateway:
    type: s3
    port: 80
    instances: 2
    resources:
      limits:
        cpu: "2"
        memory: "2Gi"

---
# 创建 ObjectBucketClaim (自动创建 Bucket)
apiVersion: objectbucket.io/v1alpha1
kind: ObjectBucketClaim
metadata:
  name: my-bucket
spec:
  generateBucketName: my-bucket
  storageClassName: rook-ceph-bucket

---
# StorageClass for Buckets
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: rook-ceph-bucket
provisioner: rook-ceph.ceph.rook.io/bucket
reclaimPolicy: Delete
parameters:
  objectStoreName: my-store
  objectStoreNamespace: rook-ceph
```

---

## 运维管理

### Dashboard 访问

```bash
# 获取 Dashboard 密码
kubectl -n rook-ceph get secret rook-ceph-dashboard-password \
  -o jsonpath="{['data']['password']}" | base64 --decode

# 端口转发
kubectl -n rook-ceph port-forward svc/rook-ceph-mgr-dashboard 8443:8443

# 访问: https://localhost:8443
# 用户名: admin
```

### Toolbox 管理

```bash
# 部署 Toolbox
kubectl apply -f toolbox.yaml

# 进入 Toolbox
kubectl -n rook-ceph exec -it deploy/rook-ceph-tools -- bash

# Ceph 命令
ceph status
ceph osd status
ceph df
rados df
ceph osd pool ls
```

### 扩容存储

```yaml
# 添加新节点的 OSD
apiVersion: ceph.rook.io/v1
kind: CephCluster
spec:
  storage:
    nodes:
      - name: "new-node"
        devices:
          - name: "sdb"
          - name: "sdc"
```

---

## 监控告警

```yaml
# 启用 Prometheus 监控
apiVersion: ceph.rook.io/v1
kind: CephCluster
spec:
  monitoring:
    enabled: true
    rulesNamespace: rook-ceph
    
# ServiceMonitor 自动创建
# Grafana Dashboard ID: 2842 (Ceph Cluster)
```

---

## 参考资源

- [官方文档](https://rook.io/docs)
- [GitHub Repo](https://github.com/rook/rook)
- [CNCF 项目页面](https://www.cncf.io/projects/rook/)
- [Ceph 文档](https://docs.ceph.com/)
- [Rook Examples](https://github.com/rook/rook/tree/master/deploy/examples)

---

**维护者**: Kudig Team | **许可证**: MIT
