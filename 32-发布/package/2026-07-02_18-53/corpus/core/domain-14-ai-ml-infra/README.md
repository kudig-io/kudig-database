---
title: AI/ML Infrastructure
description: 整合原 domain-14-ai-ml-infra/41 的 AI 基础设施知识，涵盖 GPU 调度、分布式训练、AI Agent 和 MLOps。
summary: 整合原 domain-14-ai-ml-infra/41 的 AI 基础设施知识，涵盖 GPU 调度、分布式训练、AI Agent 和 MLOps。
category: domain
tags:
- ai
- ml
- gpu
- scheduling
- distributed-training
- agent
- rag
- daemonset
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- AI/ML Infrastructure 是什么
- 如何 AI/ML Infrastructure
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- AI
- ML
- Infrastructure
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# AI/ML Infrastructure

整合原 domain-14-ai-ml-infra/41 的 AI 基础设施知识，涵盖 GPU 调度、分布式训练、AI Agent 和 MLOps。

## 目录结构

| 子目录 | 内容 |
|---|---|
| 01-ai-infra/ | GPU 调度、DCGM、分布式训练框架 |
| 02-ai-agents/ | AI Agent 框架、RAG、工具调用、Agent Harness 工程 |
| 03-agent-runtime/ | Agent 运行时与生产部署 |
| topic-ai-coding/ | AI 编码工具（OpenRouter、OpenCode）集成 |

## 与其他 Domain 的关系

- [[domain-02-workloads-applications/README.md|domain-02-workloads-applications]] — 工作负载调度
- [[domain-07-platform-engineering/README.md|domain-07-platform-engineering]] — 平台资源管理

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- networking|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic KUDIG Database — Global MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-14-ai-ml-infra/01-ai-infra/01-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-14-ai-ml-infra/01-ai-infra/02-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-07-platform-engineering/operate/01-monitoring-alerting-system|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
