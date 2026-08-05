---
title: 生产场景导航
description: '## 概述'
summary: '本主题将 [[Kubernetes|Kubernetes]] 运维知识按实际生产场景组织，覆盖 20 个常见运维场景。每个场景包含决策树、关联文档、FTA 故障树和快速操作手册。'
category: general
tags:
- k8s
- rag
- daemonset
- gpu
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 生产场景导航 是什么
- 如何 生产场景导航
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- 生产场景导航
- production
- operations
- best
- practices
prerequisites:
- kubectl-basics
- gpu-ml-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 生产场景导航

> 领域: topic-scenarios
> 创建时间: 2026-05-20

## 概述

本主题将 [[Kubernetes|Kubernetes]] 运维知识按实际生产场景组织，覆盖 20 个常见运维场景。每个场景包含决策树、关联文档、FTA 故障树和快速操作手册。

## 场景分类

### 日常运维
- [[daily-ops]] - 日常运维操作
- [[capacity-planning]] - 容量规划
- [[monitoring-alerting]] - 监控与告警
- [[cost-optimization]] - 成本优化

### 部署与发布
- deployment]] - 集群部署
- [[app-deployment]] - 应用部署
- [[gitops-workflow]] - GitOps 工作流
- [[edge-ops]] - 边缘运维

### 故障排查
- [[troubleshooting]] - 通用故障排查
- [[network-diagnosis]] - 网络诊断
- [[storage-issues]] - 存储问题
- [[performance-tuning]] - 性能调优

### 安全与合规
- [[security-hardening]] - 安全加固
- [[security-incident]] - 安全事件响应
- [[compliance-audit]] - 合规审计

### 专项运维
- [[backup-restore]] - 备份与恢复
- [[multi-cluster]] - 多集群管理
- [[mesh-ops]] - 服务网格运维
- [[ai-infra-ops]] - AI 基础设施运维

## 使用方式

1. 选择对应场景
2. 阅读场景决策树
3. 查看关联文档和 FTA
4. 执行推荐操作

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- networking|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-14-ai-ml-infra/01-ai-infra/01-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-14-ai-ml-infra/01-ai-infra/02-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-07-platform-engineering/operate/01-monitoring-alerting-system|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/higress-index.md|Higress 知识图谱索引]]

## 场景案例
- [[skills/best-practices/scenarios/cluster-deployment.md|Cluster Deployment]]
- [[skills/best-practices/scenarios/upgrade-migration.md|Upgrade Migration]]


<!-- risk-assessed -->
