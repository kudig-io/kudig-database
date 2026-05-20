---
title: OpenEBS
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- grafana
- helm
- mysql
- postgresql
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- OpenEBS 是什么
- 如何 OpenEBS
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- OpenEBS
- cncf
- landscape
---


# OpenEBS

> **成熟度**: Sandbox | **加入时间**: 2019-05 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://openebs.io |
| **GitHub** | https://github.com/openebs/openebs |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | Storage |
| **维护组织** | DataCore Software |

---

## 项目概述

OpenEBS 是领先的容器原生存储解决方案，将存储控制器作为容器运行，实现了存储的容器化和微服务化。它提供多种存储引擎，支持本地存储 (Local PV) 和分布式复制存储 (Replicated PV)，适用于有状态应用的各种场景。

---

## 核心特性

- **容器原生**: 存储控制器以 Pod 形式运行
- **多存储引擎**: Local PV、cStor、Jiva、Mayastor
- **声明式配置**: 使用 CRD 管理存储资源
- **快照与克隆**: 支持卷快照和克隆操作
- **备份恢复**: 集成 Velero 实现灾难恢复
- **性能调优**: 针对不同负载优化存储配置
- **监控集成**: Prometheus 指标和 Grafana 仪表板

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                     OpenEBS Architecture                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                 Application Pods                          │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐      │   │
│  │  │   MySQL     │  │  MongoDB    │  │  PostgreSQL │      │   │
│  │  │    Pod      │  │    Pod      │  │     Pod     │      │   │
│  │  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘      │   │
│  │         │ PVC            │ PVC            │ PVC          │   │
│  └─────────┼────────────────┼────────────────┼─────────────┘   │
│            │                │                │                  │
│  ┌─────────▼────────────────▼────────────────▼─────────────┐   │
│  │                 OpenEBS Data Plane                        │   │
│  │                                                           │   │
│  │  ┌─────────────────┐  ┌─────────────────────────────┐   │   │
│  │  │   Local PV      │  │     Replicated PV            │   │   │
│  │  │  ┌───────────┐  │  │  ┌─────────────────────────┐│   │   │
│  │  │  │HostPath   │  │  │  │       Mayastor          ││   │   │
│  │  │  │Device     │  │  │  │  ┌───────┐ ┌───────┐   ││   │   │
│  │  │  │LVM        │  │  │  │  │Replica│ │Replica│   ││   │   │
│  │  │  │ZFS        │  │  │  │  │  (1)  │ │  (2)  │   ││   │   │
│  │  │  └───────────┘  │  │  │  └───┬───┘ └───┬───┘   ││   │   │
│  │  └─────────────────┘  │  │      │    NVMe-oF│      ││   │   │
│  │                       │  │  ┌───▼─────────▼───┐   ││   │   │
│  │                       │  │  │   Nexus Target  │   ││   │   │
│  │                       │  │  └─────────────────┘   ││   │   │
│  │                       │  └─────────────────────────┘│   │   │
│  │                       │                              │   │   │
│  │                       │  ┌─────────────────────────┐│   │   │
│  │                       │  │       cStor / Jiva      ││   │   │
│  │                       │  │  ┌─────────┐ ┌────────┐││   │   │
│  │                       │  │  │ Target  │ │Replicas│││   │   │
│  │                       │  │  │  Pod    │ │  Pods  │││   │   │
│  │                       │  │  └─────────┘ └────────┘││   │   │
│  │                       │  └─────────────────────────┘│   │   │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                 OpenEBS Control Plane                     │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │   │
│  │  │   NDM       │  │  Operator   │  │  Provisioner    │  │   │
│  │  │ (Node Disk  │  │  (Volume    │  │  (CSI Driver)   │  │   │
│  │  │  Manager)   │  │   Mgmt)     │  │                 │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘  │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   Physical Storage                        │   │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐    │   │
│  │  │  NVMe   │  │  SSD    │  │  HDD    │  │  Cloud  │    │   │
│  │  │  Disk   │  │  Disk   │  │  Disk   │  │  Disk   │    │   │
│  │  └─────────┘  └─────────┘  └─────────┘  └─────────┘    │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

| 组件 | 说明 |
|:---|:---|
| **NDM** | Node Disk Manager，发现和管理块设备 |
| **Local PV** | 本地持久卷，最佳性能 |
| **Mayastor** | 高性能分布式存储引擎，基于 NVMe-oF |
| **cStor** | 传统分布式存储引擎，ZFS 后端 |
| **Jiva** | 轻量级复制存储引擎 |
| **CSI Driver** | Kubernetes CSI 标准接口 |

---

## 存储引擎对比

| 特性 | Local PV | Mayastor | cStor | Jiva |
|:---|:---|:---|:---|:---|
| **性能** | 最高 | 很高 | 中等 | 中等 |
| **复制** | 否 | 是 | 是 | 是 |
| **快照** | 依赖后端 | 是 | 是 | 否 |
| **适用场景** | 单节点高性能 | 生产分布式 | 传统分布式 | 开发测试 |
| **协议** | 直接访问 | NVMe-oF | iSCSI | iSCSI |

---

## 快速开始

### Helm 安装

```bash
# 添加 Helm 仓库
helm repo add openebs https://openebs.github.io/charts
helm repo update

# 安装 OpenEBS (包含 Local PV 和 NDM)
helm install openebs openebs/openebs \
  --namespace openebs \
  --create-namespace

# 验证安装
kubectl get pods -n openebs
kubectl get sc
```

### 安装 Mayastor (高性能引擎)

```bash
# 前置条件：启用 HugePages
echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

# 加载内核模块
sudo modprobe nvme_tcp

# 安装 Mayastor
helm install mayastor openebs/mayastor \
  --namespace openebs \
  --set mayastor.cpuCount=2
```

---

## Local PV 配置

### HostPath Local PV

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: openebs-hostpath
  annotations:
    openebs.io/cas-type: local
    cas.openebs.io/config: |
      - name: StorageType
        value: "hostpath"
      - name: BasePath
        value: "/var/openebs/local"
provisioner: openebs.io/local
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer

---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: local-hostpath-pvc
spec:
  storageClassName: openebs-hostpath
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 10Gi
```

### Device Local PV

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: openebs-device
provisioner: openebs.io/local
parameters:
  openebs.io/cas-type: local
  cas.openebs.io/config: |
    - name: StorageType
      value: device
    - name: FSType
      value: ext4
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
```

### LVM Local PV

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: openebs-lvm
parameters:
  storage: "lvm"
  volgroup: "lvmvg"
  fsType: "ext4"
provisioner: local.csi.openebs.io
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
```

---

## Mayastor 配置

### DiskPool 定义

```yaml
apiVersion: openebs.io/v1alpha1
kind: DiskPool
metadata:
  name: pool-on-node-1
  namespace: openebs
spec:
  node: worker-node-1
  disks:
    - /dev/nvme0n1
    - /dev/nvme1n1

---
apiVersion: openebs.io/v1alpha1
kind: DiskPool
metadata:
  name: pool-on-node-2
  namespace: openebs
spec:
  node: worker-node-2
  disks:
    - /dev/nvme0n1
```

### Mayastor StorageClass

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: mayastor-3
provisioner: io.openebs.csi-mayastor
parameters:
  ioTimeout: "30"
  protocol: nvmf
  repl: "3"  # 3 副本
  thin: "false"
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true

---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mayastor-pvc
spec:
  storageClassName: mayastor-3
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 100Gi
```

---

## cStor 配置 (传统引擎)

### CStorPoolCluster

```yaml
apiVersion: cstor.openebs.io/v1
kind: CStorPoolCluster
metadata:
  name: cstor-pool
  namespace: openebs
spec:
  pools:
    - nodeSelector:
        kubernetes.io/hostname: "worker-1"
      dataRaidGroups:
        - blockDevices:
            - blockDeviceName: "blockdevice-xxx-1"
            - blockDeviceName: "blockdevice-xxx-2"
      poolConfig:
        dataRaidGroupType: "stripe"
        
    - nodeSelector:
        kubernetes.io/hostname: "worker-2"
      dataRaidGroups:
        - blockDevices:
            - blockDeviceName: "blockdevice-yyy-1"
            - blockDeviceName: "blockdevice-yyy-2"
      poolConfig:
        dataRaidGroupType: "mirror"
```

### cStor StorageClass

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: cstor-csi-disk
provisioner: cstor.csi.openebs.io
parameters:
  cas-type: cstor
  cstorPoolCluster: cstor-pool
  replicaCount: "3"
allowVolumeExpansion: true
```

---

## 快照与克隆

### 创建快照

```yaml
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: mysql-snapshot
spec:
  volumeSnapshotClassName: csi-mayastor-snapshotclass
  source:
    persistentVolumeClaimName: mysql-pvc
```

### 从快照恢复

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-pvc-restore
spec:
  storageClassName: mayastor-3
  dataSource:
    name: mysql-snapshot
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 100Gi
```

---

## 监控

### Prometheus 指标

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: openebs-ndm
  namespace: openebs
spec:
  selector:
    matchLabels:
      app: openebs-ndm
  endpoints:
    - port: metrics
      interval: 30s
```

### 关键指标

| 指标 | 说明 |
|:---|:---|
| `openebs_volume_read_iops` | 卷读 IOPS |
| `openebs_volume_write_iops` | 卷写 IOPS |
| `openebs_volume_read_latency` | 读延迟 |
| `openebs_pool_used_capacity` | 存储池使用量 |

---

## 最佳实践

1. **引擎选择**: 高性能场景用 Mayastor，简单场景用 Local PV
2. **磁盘规划**: 使用专用磁盘，避免与系统盘混用
3. **副本策略**: 生产环境至少 3 副本
4. **备份策略**: 结合 Velero 实现定期备份
5. **资源限制**: 为存储组件设置合理的资源限制
6. **监控告警**: 监控存储池容量和 I/O 性能

---

## 参考资源

- [官方文档](https://openebs.io/docs)
- [GitHub Repo](https://github.com/openebs/openebs)
- [Mayastor 文档](https://mayastor.gitbook.io)
- [用户案例](https://openebs.io/adopters)
- [Slack 社区](https://kubernetes.slack.com/messages/openebs)

---

**维护者**: Kudig Team | **许可证**: MIT
