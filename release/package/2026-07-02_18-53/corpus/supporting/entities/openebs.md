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
last_updated: 2026-05
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

OpenEBS 是领先的容器原生存储解决方案，将存储控制器作为容器运行，实现了存储的容器化和微服务化。它提供多种存储引擎，支持本地存储 (Local PV) 和分布式复制存储 (Replicated PV)，适用于有状态应用的各种场景。

## 核心能力

- **容器原生**: 存储控制器以 Pod 形式运行
- **多存储引擎**: Local PV、cStor、Jiva、Mayastor
- **声明式配置**: 使用 CRD 管理存储资源
- **快照与克隆**: 支持卷快照和克隆操作
- **备份恢复**: 集成 Velero 实现灾难恢复
- **性能调优**: 针对不同负载优化存储配置

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **引擎选择**: 高性能场景用 Mayastor，简单场景用 Local PV
- **磁盘规划**: 使用专用磁盘，避免与系统盘混用
- **副本策略**: 生产环境至少 3 副本
- **备份策略**: 结合 Velero 实现定期备份
- **资源限制**: 为存储组件设置合理的资源限制
- **监控告警**: 监控存储池容量和 I/O 性能

## 架构定位

在 CNCF 生态中，openebs 属于 **Storage** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[concepts/storage-model.md|storage-model]]
- [[pod-lifecycle]]

## Related

- [[karmada]] — Karmada
- [[rook]] — Rook
- [[microcks]] — Microcks
- [[keylime]] — Keylime
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- openebs
- [[entities/cncf-storage.md|[[CNCF 存储与数据库项目全景|CNCF 存储与数据库项目全景]]]] — Cross-reference
- [[domain-19-landscape-references/领域索引/backup-dr-index.md|Backup & DR 备份与灾备知识图谱索引]]
- [[domain-19-landscape-references/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/storage-index.md|Storage 存储知识图谱索引]]
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/领域索引/csi-index.md|CSI (Container Storage Interface) 知识图谱索引]]


<!-- risk-assessed -->
