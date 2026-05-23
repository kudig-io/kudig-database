---
title: Platform Engineering
description: 整合原 domain-07-platform-engineering/36 的平台知识，涵盖平台构建(IDP/Backstage)、平台运维执行和平台治理。
category: domain
tags:
- platform-engineering
- idp
- backstage
- devops
- platform-ops
- daemonset
- gpu
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
created: "2026-05-23"
---

# Platform Engineering

整合原 domain-07-platform-engineering/36 的平台知识，涵盖平台构建(IDP/Backstage)、平台运维执行和平台治理。

## 目录结构

| 子目录 | 内容 |
|---|---|
| build/ | IDP 设计、[[Backstage|Backstage]]、Kratix、[[Crossplane|Crossplane]] |
| operate/ | 集群生命周期、多集群管理、监控告警、自动化 |
| governance/ | 容量规划、成本优化、多租户、安全合规 |
| developer-experience/ | DevEx 指标、团队拓扑、CLI 插件 |

## 与其他 Domain 的关系

- [[domain-08-release-change-management/README.md|domain-08-release-change-management]] — 发布管理
- observability/README.md|domain-06-observability]] — 可观测性建设

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[references/release-notes-networking|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index|Higress 知识图谱索引]]
