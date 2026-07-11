---
title: CubeFS (entities)
description: '## 概述'
summary: 'CubeFS 是一个云原生存储平台，提供多协议兼容（POSIX/S3/HDFS）的分布式文件和对象存储。'
category: entities
tags:
- k8s
- cncf
- storage
- cubefs
- prometheus
- grafana
- crd
- operator
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CubeFS 是什么
- 如何 CubeFS
trigger_keywords:
- CubeFS
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- logging-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# CubeFS

> **CNCF 状态**: Graduated | **类别**: Storage | **主要语言**: Go

## 概述

CubeFS（原 ChubaoFS）是一个云原生存储平台，由京东开发，2019 年加入 CNCF 孵化，2024 年正式毕业（Graduated）。它提供多协议兼容的分布式文件和对象存储，同时支持 POSIX 文件接口、S3 对象存储接口和 Hadoop HDFS 接口。CubeFS 的多协议支持使得同一份数据可以被容器化应用（POSIX mount）、AI/ML 训练（S3 SDK）和大数据分析（Hadoop HDFS）同时访问。它采用元数据与数据分离的架构，支持纠删码（Erasure Coding）实现高存储效率，支持多副本实现高可用。CubeFS 特别适合 AI/ML 大规模数据集存储、大数据分析和容器化应用的持久化存储场景。

## 核心能力

- **多协议支持**: POSIX（FUSE mount）、S3（对象存储）、HDFS（Hadoop 兼容）三种接口统一访问
- **弹性扩展**: 元数据节点（MetaNode）和数据节点（DataNode）独立水平扩展
- **多租户**: Volume 级别资源隔离和配额管理
- **纠删码**: 高效存储空间利用，支持 EC 模式减少存储成本
- **多级缓存**: 本地 SSD 缓存加速热数据访问
- **AI/ML 优化**: 大规模数据集顺序读取优化

## 架构

CubeFS 采用元数据与数据分离的分布式架构：

- **Master**: 元数据管理节点（Raft 共识集群），管理 Volume、分区、节点分配
- **MetaNode**: 元数据存储节点，管理文件 inode 和目录树
- **DataNode**: 数据存储节点，管理实际数据块（Data Partition）
- **Volume**: 逻辑存储单元，类似 LVM Volume，可配置副本数或纠删码
- **Object Node**: S3 协议网关，将 S3 请求转换为内部数据访问
- **FUSE Client**: POSIX 挂载客户端，将 CubeFS Volume 挂载到 Pod

数据流：`应用 → POSIX/S3/HDFS → CubeFS Master (元数据路由) → MetaNode (元数据) → DataNode (数据)`

## K8s 集成

CubeFS 通过 CSI Driver（CubeFS CSI）与 Kubernetes 集成。StorageClass 定义 CubeFS Volume 配置（容量、副本数、Owner），PVC 创建时自动创建 CubeFS Volume。Pod 通过标准 PV/PVC 机制挂载 CubeFS Volume（FUSE 挂载）。CSI Driver 以 DaemonSet 运行在每个节点上，负责 FUSE 挂载/卸载。CubeFS Operator（cubefs-operator）管理 Master/MetaNode/DataNode 的生命周期。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 PV/PVC/StorageClass 标准机制完全兼容。

## 生产场景

1. **AI/ML 训练数据**: GPU 训练 Pod 通过 POSIX 挂载大规模训练数据集
2. **大数据分析**: Spark/Flink 通过 HDFS 接口访问 CubeFS 上的数据
3. **容器持久化**: 数据库和消息队列通过 PVC 使用 CubeFS 块存储
4. **对象存储替代**: 应用通过 S3 SDK 访问 CubeFS，替代 AWS S3

## 安装

```bash
# Helm 安装 CubeFS
helm repo add cubefs http://cubefs.io/charts/
helm install cubefs cubefs/cubefs -n cubefs --create-namespace \
  --set master.replicas=3 \
  --set metanode.replicas=3 \
  --set datanode.replicas=5

# 安装 CubeFS CSI Driver
kubectl apply -f https://github.com/cubefs/cubefs-csi/releases/latest/download/csi-driver.yaml

# 创建 StorageClass
kubectl apply -f - <<EOF
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: cubefs-sc
provisioner: cubefs.csi.driver
parameters:
  masterAddr: "cubefs-master.cubefs.svc:17010"
  ownerName: "k8s"
  volumeType: "replicate"
  capacity: "100GB"
  replicaNum: "3"
EOF

# 创建 PVC
kubectl apply -f - <<EOF
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: cubefs-pvc
spec:
  accessModes: ["ReadWriteMany"]
  storageClassName: cubefs-sc
  resources:
    requests:
      storage: 100Gi
EOF
```

## 对比

| 特性 | CubeFS | Ceph | JuiceFS | MinIO |
|------|--------|------|---------|-------|
| POSIX | ✅ | ✅ CephFS | ✅ | ❌ |
| S3 | ✅ | ✅ RGW | ⚠️ | ✅ |
| HDFS | ✅ | ❌ | ❌ | ❌ |
| CNCF 状态 | Graduated | Graduated | 非 CNCF | 非 CNCF |

## 架构定位

在 CNCF 生态中，CubeFS 属于 **Storage** 类别，为云原生应用提供多协议统一存储能力。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[概念/storage-model.md|storage-model]]
- [[实体/csi-drivers.md|csi-drivers]]

## Related

- [[stacker]] — Stacker
- [[opentelemetry]] — OpenTelemetry
- [[kusionstack]] — KusionStack
- [[fluentd]] — Fluentd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cubefs
- [[实体/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[生态参考/领域索引/storage-index.md|Storage 存储知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
- [[生态参考/领域索引/csi-index.md|CSI (Container Storage Interface) 知识图谱索引]]


<!-- risk-assessed -->
