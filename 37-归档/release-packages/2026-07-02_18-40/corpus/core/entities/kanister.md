---
title: Kanister (entities)
description: '## 概述'
summary: 'Kanister 是一个面向 Kubernetes 的应用级数据管理框架，专门用于有状态应用（数据库、消息队列等）的备份和恢复。它使用 Blueprint CRD 定义应用特定的备份/恢复操作流程，支持应用一致性的快照和备份。'
category: entities
tags:
- k8s
- cncf
- storage
- kanister
- postgresql
- job
- cronjob
- crd
- operator
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kanister 是什么
- 如何 Kanister
trigger_keywords:
- Kanister
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kanister

> **CNCF 状态**: Sandbox | **类别**: Storage | **主要语言**: Go

## 概述

Kanister 是一个面向 Kubernetes 的应用级数据管理框架，专门用于有状态应用（数据库、消息队列等）的备份和恢复。它使用 Blueprint CRD 定义应用特定的备份/恢复操作流程，支持应用一致性的快照和备份。Kanister 可以与应用的数据保护 API（如 PostgreSQL pg_dump、MongoDB mongodump）深度集成。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **Blueprint 测试**: 在非生产环境充分测试 Blueprint 的备份和恢复流程
- **定期备份**: 结合 [[CronJob|CronJob]] 或外部调度器定期创建 ActionSet 执行备份
- **恢复演练**: 定期执行恢复演练，确保备份数据可用
- **清理策略**: 设置备份保留策略，定期清理过期的备份数据
- **监控告警**: 监控 ActionSet 状态，对失败的备份/恢复操作设置告警

## 架构定位

在 CNCF 生态中，kanister 属于 **Storage** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[concepts/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-03-networking-traffic/00-core-k8s-networking/10-terway-troubleshooting-fta]]

- changelog.md|ecosystem-changelog]]

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-03-networking-traffic/00-core-k8s-networking/05-terway-usage-guide]]

- metal3-io

- inspektor-gadget

- [[kubearmor]] — KubeArmor
- [[entities/cncf-cicd.md|cncf-cicd]] — CNCF CI/CD 与发布管理项目全景
- [[entities/cncf-networking.md|cncf-networking]] — CNCF 网络与服务网格项目全景
- [[armada]] — Armada
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- digest-2026-05-21-full
- kanister
- [[entities/k8up.md|K8up]]
- [[entities/openebs.md|OpenEBS]]
- [[entities/hwameistor.md|HwameiStor]]
- [[entities/carina.md|Carina]]
- [[entities/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[domain-19-landscape-references/领域索引/backup-dr-index.md|Backup & DR 备份与灾备知识图谱索引]]
- [[domain-19-landscape-references/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
