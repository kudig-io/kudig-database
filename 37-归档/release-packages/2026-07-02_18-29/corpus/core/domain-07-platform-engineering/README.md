---
title: Platform Engineering
description: 整合原 domain-07-platform-engineering/36 的平台知识，涵盖平台构建(IDP/Backstage)、平台运维执行和平台治理。
summary: 整合原 domain-07-platform-engineering/36 的平台知识，涵盖平台构建(IDP/Backstage)、平台运维执行和平台治理。
category: domain
tags:
- platform-engineering
- idp
- backstage
- devops
- platform-ops
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
- Platform Engineering 是什么
- 如何 Platform Engineering
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- Platform
- Engineering
- platform
- engineering
prerequisites:
- kubectl-basics
- platform-engineering-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Platform Engineering

整合原 domain-07-platform-engineering/36 的平台知识，涵盖平台构建(IDP/Backstage)、平台运维执行和平台治理。

## 目录结构

| 子目录 | 内容 |
|---|---|
| build/ | IDP 设计、[[Backstage|Backstage]]、Kratix、[[Crossplane|Crossplane]] |
| operate/ | 集群生命周期、多集群管理、监控告警、自动化 |
| governance/ | 容量规划、成本优化、多租户、安全合规 |
| developer-experience/ | DevEx 指标、团队拓扑、CLI 插件 |
| topic-code-analysis/ | 代码分析与质量治理 |

## 与其他 Domain 的关系

- [[domain-08-release-change-management/README.md|domain-08-release-change-management]] — 发布管理
- observability/README.md|domain-06-observability]] — 可观测性建设

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
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


<!-- risk-assessed -->
