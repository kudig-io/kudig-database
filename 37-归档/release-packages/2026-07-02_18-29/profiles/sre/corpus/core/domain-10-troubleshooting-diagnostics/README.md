---
title: Troubleshooting & Diagnostics
description: '- [[domain-06-observability/README.md|domain-06-observability]] — 监控与告警'
summary: '- [[domain-06-observability/README.md|domain-06-observability]] — 监控与告警'
category: domain
tags:
- troubleshooting
- diagnostics
- fta
- runbook
- incident-response
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
- Troubleshooting & Diagnostics 是什么
- 如何 Troubleshooting & Diagnostics
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- Troubleshooting & Diagnostics 故障排查
- Troubleshooting & Diagnostics 排障步骤
trigger_keywords:
- Troubleshooting
- Diagnostics
- troubleshooting
- diagnostics
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Troubleshooting & Diagnostics

整合原 domain-10-troubleshooting-diagnostics 的全链路排障知识，完整保留 topic-febm、topic-fta、topic-structural-trouble-shooting 和 tools 子目录结构。

## 目录结构

| 子目录 | 内容 |
|---|---|
| 00-core-troubleshooting/ | 核心排障方法论与通用流程 |
| 01-resource-troubleshooting/ | 资源层排障（Pod/Node/存储/CronJob） |
| 02-infrastructure-troubleshooting/ | 基础设施排障（网络/DNS/控制面） |
| 03-advanced-troubleshooting/ | 高级排障（症状映射、版本问题、云厂商） |
| 04-jvm-tuning/ | JVM 调优与 Java 应用排障 |
| topic-febm/ | FEBM 问题事件基础模型 |
| topic-fta/ | FTA 故障树分析（44 组件故障树，推理骨架） |
| topic-skills/ | 诊断-修复 Skill 集（17 个场景化诊断流程） |
| topic-structural-trouble-shooting/ | 结构化排障框架（按组件域分类） |
| topic-multi-fault-scenarios/ | 多故障并发场景 |
| topic-qa-corpus/ | 问答语料库 |
| tools/ | 排障工具集合 |

## 与其他 Domain 的关系

- observability/README.md|domain-06-observability]] — 监控与告警
- [[domain-05-security-compliance/README.md|domain-05-security-compliance]] — 安全事件响应

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


<!-- risk-assessed -->
