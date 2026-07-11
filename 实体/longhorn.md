---
title: Longhorn (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
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

## 安装

```bash
kubectl apply -f https://raw.githubusercontent.com/longhorn/longhorn/v1.7.2/deploy/longhorn.yaml
# 或使用 Helm
helm repo add longhorn https://charts.longhorn.io
helm install longhorn longhorn/longhorn --namespace longhorn-system --create-namespace
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Longhorn** | 易于部署、内置备份、UI 友好 | 性能不如 Ceph、节点规模有限 |
| Rook/Ceph | 成熟稳定、高性能、大规模支持 | 运维复杂、资源开销大 |
| OpenEBS | 多引擎选择、CSI 原生 | 功能分散、文档不够统一 |
| Linstor/DRBD | 高性能块复制 | 配置复杂、社区较小 |

## 架构定位

在 CNCF 生态中，Longhorn 属于 **Storage** 类别，是云原生块存储的代表性项目。它降低了分布式存储的运维门槛，特别适合中小规模 Kubernetes 集群和边缘场景。

## 参考链接

- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/storage-model.md|storage-model]]

## Related

- [[cozystack]] — Cozystack
- [[fluid]] — Fluid
- [[实体/cncf-storage.md|cncf-storage]] — CNCF 存储与数据库项目全景
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
- [[实体/kanister.md|Kanister]]
- [[实体/k8up.md|K8up]]
- [[实体/openebs.md|OpenEBS]]
- [[实体/hwameistor.md|HwameiStor]]
- [[实体/carina.md|Carina]]
- [[实体/release-notes-storage.md|发布说明索引 — 存储]] — Cross-reference
- [[生态参考/98-merged-indexes/index.md|发布说明阅读指南]] — Cross-reference
- [[概念/storage-tool-evolution.md|存储工具演进]] — Cross-reference
- [[生态参考/领域索引/backup-dr-index.md|Backup & DR 备份与灾备知识图谱索引]]
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/领域索引/storage-index.md|Storage 存储知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
- [[生态参考/领域索引/csi-index.md|CSI (Container Storage Interface) 知识图谱索引]]


<!-- risk-assessed -->
