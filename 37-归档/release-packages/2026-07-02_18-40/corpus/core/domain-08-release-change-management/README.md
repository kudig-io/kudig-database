---
title: Release & Change Management
description: 整合原 domain-08-release-change-management/24/29 和 domain-11-production-operations
  部分内容的发布与变更管理知识。
summary: 整合原 domain-08-release-change-management/24/29 和 domain-11-production-operations
  部分内容的发布与变更管理知识。
category: domain
tags:
- gitops
- cicd
- iac
- terraform
- change-management
- testing
- argocd
- flux
- daemonset
- gpu
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Release & Change Management 是什么
- 如何 Release & Change Management
- Kubernetes 08 release change management 最佳实践
trigger_keywords:
- Release
- Change
- Management
- release
- change
- management
prerequisites:
- kubectl-basics
- gitops-basics
- iac-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Release & Change Management

整合原 domain-08-release-change-management/24/29 和 domain-11-production-operations 部分内容的发布与变更管理知识。

## 目录结构

| 子目录 | 内容 |
|---|---|
| 01-gitops/ | [[ArgoCD|ArgoCD]]、[[Flux|Flux]]、GitOps 实践 |
| 02-iac/ | Terraform、Ansible、Pulumi |
| 03-change-management/ | 变更管理流程、发布策略 |
| 04-testing-quality/ | 自动化测试、质量保障 |
| topic-deployment/ | 部署策略与决策树 |
| topic-migration/ | 迁移评估、规划与实战案例 |

## 与其他 Domain 的关系

- [[domain-07-platform-engineering/README.md|domain-07-platform-engineering]] — 平台工程
- [[domain-09-reliability-engineering/README.md|domain-09-reliability-engineering]] — 可靠性工程

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- networking|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-14-ai-ml-infra/01-ai-infra/01-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-14-ai-ml-infra/01-ai-infra/02-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-07-platform-engineering/operate/01-monitoring-alerting-system|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
