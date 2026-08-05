---
title: Reliability Engineering
description: '| 05-chaos-engineering/ | 混沌工程原则、Chaos Mesh、Litmus、实验设计 |'
summary: '| 05-chaos-engineering/ | 混沌工程原则、Chaos Mesh、Litmus、实验设计 |'
category: domain
tags:
- sre
- slo
- disaster-recovery
- backup
- capacity-planning
- chaos-engineering
- postmortem
- performance-testing
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
- Reliability Engineering 是什么
- 如何 Reliability Engineering
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- Reliability
- Engineering
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Reliability Engineering

## 目录结构

| 子目录 | 内容 |
|---|---|
| 01-backup-recovery/ | 企业备份策略、恢复演练 |
| 02-disaster-recovery/ | 灾备演练、跨区灾备 |
| 03-capacity-planning/ | 容量规划与预测 |
| 04-slo-sli/ | SLI 定义、SLO 设定、错误预算、Burn Rate 告警 |
| 05-chaos-engineering/ | 混沌工程原则、Chaos Mesh、[[Litmus|Litmus]]、实验设计 |
| 06-postmortem/ | 无责事后复盘、复盘文化 |
| 07-sre-practices/ | 可用性计算、发布门控、事故指挥、Toil 削减 |
| 08-performance-testing/ | 负载测试、混沌与负载集成 |
| 09-disaster-recovery-playbooks/ | 灾备场景目录、恢复手册 |

## 与其他 Domain 的关系

- observability/README.md|domain-06-observability]] — SLO/SLI 监控
- [[domain-11-production-operations/README.md|domain-11-production-operations]] — 生产运维
- [[domain-08-release-change-management/README.md|domain-08-release-change-management]] — 发布管理

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
