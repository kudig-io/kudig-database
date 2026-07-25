---
title: Production Operations
description: 原 domain-11-production-operations 精简后的生产运维知识，聚焦 FinOps、治理、事件响应和绿色计算。
summary: 原 domain-11-production-operations 精简后的生产运维知识，聚焦 FinOps、治理、事件响应和绿色计算。
category: domain
tags:
- finops
- cost-optimization
- incident-response
- governance
- green-computing
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
- Production Operations 是什么
- 如何 Production Operations
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- Production
- Operations
- production
- operations
prerequisites:
- kubectl-basics
- gpu-ml-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Production Operations

原 domain-11-production-operations 精简后的生产运维知识，聚焦 FinOps、治理、事件响应和绿色计算。

## 目录结构

| 子目录 | 内容 |
|---|---|
| 01-finops/ | [[Kubernetes|Kubernetes]] 成本治理、FinOps 优化 |
| 02-governance/ | 资源配额、多租户治理 |
| 03-incident-response/ | 事件响应流程 |
| 04-green-computing/ | 绿色计算、可持续运维 |
| ticket-cases/ | 运维工单闭环样本（工单→诊断→修复→验证） |
| reply-templates/ | 工单回复模板 |

## 与其他 Domain 的关系

- [[domain-09-reliability-engineering/README.md|domain-09-reliability-engineering]] — 可靠性工程
- [[domain-07-platform-engineering/README.md|domain-07-platform-engineering]] — 平台治理

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- networking|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic KUDIG Database — Global MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/运维/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
