---
title: OpenEBS [entities]
description: '## 概述'
summary: 'OpenEBS 是领先的容器原生存储解决方案，将存储控制器作为容器运行，实现了存储的容器化和微服务化。它提供多种存储引擎，支持本地存储 (Local PV) 和分布式复制存储 (Replicated PV)，适用于有状态应用的各种场景。'
category: entities
tags:
- k8s
- cncf
- storage
- openebs
- prometheus
- grafana
- rook
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
- OpenEBS 是什么
- 如何 OpenEBS
trigger_keywords:
- OpenEBS
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# OpenEBS

> **CNCF 状态**: Sandbox | **类别**: Storage | **主要语言**: Go

## 概述

OpenEBS 是领先的容器原生存储（Container-Native Storage）解决方案，由 MayaData 开发，2019 年加入 CNCF Sandbox。它将存储控制器作为容器运行，实现了存储的容器化和微服务化。OpenEBS 提供多种存储引擎，支持本地存储（Local PV）和分布式复制存储（Replicated PV/Mayastor），适用于有状态应用的各种场景。它是 Kubernetes 生态中使用最广泛的开源 CSI 存储项目之一。

## 核心特性

- **容器原生架构**: 存储控制器和数据平面均以 Pod 形式运行
- **多存储引擎**: Local PV（Hostpath/Device/RAM）、Mayastor（SPDK 高性能）、cStor
- **CSI 原生**: 完全基于 Container Storage Interface 实现
- **快照与克隆**: 支持 VolumeSnapshot 和 PVC Clone
- **备份恢复**: 集成 Velero 实现灾难恢复
- **监控集成**: 内置 Prometheus 指标和 Grafana 仪表盘

## 架构

OpenEBS 采用微服务存储架构。核心组件包括：MayaStor（高性能存储引擎，基于 SPDK 用户态块设备）、Local PV（直接使用节点磁盘，零开销）、cStor（基于 ZFS 的复制存储引擎）。每个存储卷对应一个 Target Pod（iSCSI/NVMe-oF Target）和多个 Replica Pod。Target Pod 接收来自 CSI Plugin 的 I/O 请求，同步写入 Replica。Provisioner 监听 PVC 创建请求，自动分配存储和创建 Target/Replica。Mayactor Operator 管理存储池和卷的生命周期。

## Kubernetes 集成

OpenEBS 通过 CSI Driver 与 Kubernetes 集成。部署为 DaemonSet（mayastor、node operator）和 Deployment（provisioner、API server）。StorageClass 定义使用哪个 OpenEBS 引擎和参数。支持标准的 PVC → PV 映射、VolumeSnapshot 和 Clone。Local PV 模式直接使用节点磁盘，无网络开销，适合需要极低延迟的数据库。Mayastor 使用 NVMe-oF 协议提供跨节点复制能力。

## 生产使用场景

1. **数据库存储**: 为 PostgreSQL、MongoDB 等 StatefulSet 提供高性能持久卷
2. **本地存储加速**: 使用 Local PV 直连 NVMe/SSD，实现最高 IOPS
3. **Dev/Test 环境**: 在共享集群上为每个团队提供隔离的存储空间
4. **Kafka/Elasticsearch**: 为分布式消息队列和搜索引擎提供复制存储

## 安装

```bash
# Helm 安装
helm repo add openebs https://openebs.github.io/openebs
helm install openebs openebs/openebs -n openebs --create-namespace
# 使用 Local PV
kubectl apply -f - <<EOF
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata: { name: openebs-local }
provisioner: openebs.io/local
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
EOF
# 或使用 Mayastor
kubectl apply -f https://raw.githubusercontent.com/openebs/mayastor/master/deploy/mayastor.yaml
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **OpenEBS** | 多引擎、CSI 原生 | 引擎选择复杂、cStor 性能一般 |
| Longhorn | 部署简单、UI 友好 | 仅块存储、性能不如 Mayastor |
| Rook/Ceph | 功能最全面 | 资源开销大、运维复杂 |
| TopoLVM | 高性能 LVM | 功能较少 |

## 架构定位

在 CNCF 生态中，OpenEBS 属于 **Storage** 类别，是容器原生存储的代表性项目。它的多引擎架构使其能适配从本地高性能到分布式复制等多种场景。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/storage-model.md|storage-model]]
- [[pod-lifecycle]]

## Related

- [[karmada]] — Karmada
- [[rook]] — Rook
- [[microcks]] — Microcks
- [[keylime]] — Keylime
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- openebs
- [[实体/cncf-storage.md|[[CNCF 存储与数据库项目全景|CNCF 存储与数据库项目全景]]]] — Cross-reference
- [[生态参考/领域索引/backup-dr-index.md|Backup & DR 备份与灾备知识图谱索引]]
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/storage-index.md|Storage 存储知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
- [[生态参考/领域索引/csi-index.md|CSI (Container Storage Interface) 知识图谱索引]]


<!-- risk-assessed -->
