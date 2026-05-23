---
title: 集群操作函数库
description: '## 概述'
category: general
tags:
- k8s
- daemonset
- gpu
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 集群操作函数库 是什么
- 如何 集群操作函数库
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 集群操作函数库
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

# 集群操作函数库

> 领域: topic-functions
> 创建时间: 2026-05-15
> 最后更新: 2026-05-21

## 概述

本主题包含 Kubernetes 集群常见操作函数和流程，提供标准化的操作模板。

## 内容索引

| 文件 | 说明 | 文档数 |
|------|------|--------|
| cluster-cert | 集群证书管理 | 17 |
| cluster-create | 集群创建流程 | 25 |
| cluster-delete | 集群删除流程 | 13 |
| deployment-create | 应用部署流程 | 10 |
| node-create | 节点添加流程 | 17 |

## 相关主题

- 控制平面
- 生命周期管理
- 场景导航

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
- [[concepts/platform-engineering-idp.md|Platform Engineering and Internal Developer Platforms]]
