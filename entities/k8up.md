---
title: K8up (entities)
description: '## 概述'
summary: 'K8up 是一个 Kubernetes 备份 Operator，基于 Restic 实现 PersistentVolume 的自动化备份。它通过 CRD 声明式管理备份、恢复、归档和清理策略，支持将备份存储到 S3、GCS、Azure Blob 等对象存储后端。'
category: entities
tags:
- k8s
- cncf
- storage
- k8up
- prometheus
- grafana
- ingress
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
- K8up 是什么
- 如何 K8up
trigger_keywords:
- K8up
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8up

> **CNCF 状态**: Sandbox | **类别**: Storage | **主要语言**: Go

## 概述

K8up 是一个 Kubernetes 备份 Operator，基于 Restic 实现 PersistentVolume 的自动化备份。它通过 CRD 声明式管理备份、恢复、归档和清理策略，支持将备份存储到 S3、GCS、Azure Blob 等对象存储后端。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **定期测试恢复**: 不要只测试备份，定期验证恢复流程
- **数据库钩子**: 使用 backupcommand 注解执行数据库一致性转储
- **保留策略**: 根据合规要求配置合理的保留策略
- **加密**: 使用强密码保护 Restic 仓库，密码存储在 Kubernetes Secret 中
- **完整性检查**: 定期运行 Check 验证备份数据完整性
- **监控告警**: 基于 `k8up_backup_failure_total` 配置告警

## 架构定位

在 CNCF 生态中，k8up 属于 **Storage** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[concepts/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[backstage]] — Backstage
- [[entities/emissary-ingress.md|ingress]]]] — Emissary-Ingress
- [[kubevela]] — KubeVela
- [[piraeus-datastore]] — Piraeus Datastore
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- k8up
- [[entities/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[生态参考/topic-index/backup-dr-index.md|Backup & DR 备份与灾备知识图谱索引]]
- [[生态参考/topic-index/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/topic-index/storage-index.md|Storage 存储知识图谱索引]]
- [[生态参考/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
