---
title: Storage & Data
description: 整合原 domain-04-storage-data/16 的存储知识，涵盖 K8s 存储体系、PV/PVC/CSI 和存储基础原理。
summary: 整合原 domain-04-storage-data/16 的存储知识，涵盖 K8s 存储体系、PV/PVC/CSI 和存储基础原理。
category: domain
tags:
- storage
- pv
- pvc
- csi
- distributed-storage
- rag
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
- Storage & Data 是什么
- 如何 Storage & Data
- Kubernetes 04 storage data 最佳实践
trigger_keywords:
- Storage
- Data
- storage
- data
prerequisites:
- kubectl-basics
- storage-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Storage & Data

整合原 domain-04-storage-data/16 的存储知识，涵盖 K8s 存储体系、PV/PVC/CSI 和存储基础原理。

## 目录结构

| 子目录 | 内容 |
|---|---|
| 01-k8s-storage/ | PV、PVC、StorageClass、CSI、快照 |
| 02-storage-fundamentals/ | 块/文件/对象存储、RAID、分布式存储 |
| 03-distributed-storage/ | 分布式存储系统 |

## 与其他 Domain 的关系

- [[domain-01-cluster-fundamentals/README.md|domain-01-cluster-fundamentals]] — 集群架构
- [[domain-10-troubleshooting-diagnostics/README.md|domain-10-troubleshooting-diagnostics]] — 存储排障

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- networking|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic KUDIG Database — Global MOC — Cross-reference
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


<!-- risk-assessed -->
