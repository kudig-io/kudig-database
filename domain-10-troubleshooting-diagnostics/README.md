---
title: Troubleshooting & Diagnostics
description: '- [[domain-06-observability/README.md|domain-06-observability]] — 监控与告警'
category: domain
tags:
- troubleshooting
- diagnostics
- fta
- runbook
- incident-response
- daemonset
- gpu
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
created: "2026-05-23"
---

# Troubleshooting & Diagnostics

整合原 domain-10-troubleshooting-diagnostics 的全链路排障知识，完整保留 topic-febm、topic-fta、topic-structural-trouble-shooting 和 tools 子目录结构。

## 目录结构

| 子目录 | 内容 |
|---|---|
| domain-10-troubleshooting-diagnostics/topic-febm/ | FEBM 问题事件基础模型 |
| domain-10-troubleshooting-diagnostics/topic-fta/ | FTA 故障树分析 |
| domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/ | 结构化排障框架 |
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
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index.md|Higress 知识图谱索引]]
