---
title: Cloud Providers
description: 整合原 domain-12-cloud-providers/27 的云厂商知识，涵盖主流云服务商和多云混合部署。
category: domain
tags:
- cloud
- aws
- gcp
- azure
- alicloud
- multicloud
- hybrid
- daemonset
- gpu
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cloud Providers 是什么
- 如何 Cloud Providers
- Kubernetes 12 cloud providers 最佳实践
trigger_keywords:
- Cloud
- Providers
- cloud
- providers
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- gpu-scheduling-basics
---

# Cloud Providers

整合原 domain-12-cloud-providers/27 的云厂商知识，涵盖主流云服务商和多云混合部署。

## 目录结构

| 子目录 | 内容 |
|---|---|
| 00-aws/ | AWS EKS |
| 01-gcp/ | Google GKE |
| 02-azure/ | Azure AKS |
| 03-alicloud/ | 阿里云 ACK |
| 04-tencent/ | 腾讯云 TKE |
| 05-huawei/ | 华为云 CCE |
| 06-multi-cloud/ | 多云混合部署策略 |

## 与其他 Domain 的关系

- [[domain-01-cluster-fundamentals/README.md|domain-01-cluster-fundamentals]] — 集群架构
- [[domain-07-platform-engineering/README.md|domain-07-platform-engineering]] — 平台运维

## Related

- [[domain-19-landscape-references/98-merged-indexes/README-from-domain-19-landscape-references|Domain-34: CNCF Landscape 开源项目]] — Cross-reference
- [[references/release-notes-networking|发布说明索引 — 网络]] — Cross-reference
- [[domain-03-networking-traffic/98-merged-indexes/MOC-from-domain-03-networking-traffic|domain-03-networking-traffic MOC]] — Cross-reference
- [[domain-20-application-patterns/98-merged-indexes/README-from-domain-20-application-patterns|Topic 应用层架构设计最佳实践]] — Cross-reference
- [[domain-20-application-patterns/98-merged-indexes/MOC-from-domain-20-application-patterns|topic-application-architecture MOC]] — Cross-reference
- [[concepts/bp-common-best-practices|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- [[domain-08-release-change-management/98-merged-indexes/MOC-from-domain-08-release-change-management|domain-08-release-change-management MOC]] — Cross-reference
- [[skills/learn-decision-tree-mermaid|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system|监控告警体系]] — Cross-reference
- [[domain-09-reliability-engineering/98-merged-indexes/README-from-domain-09-reliability-engineering|Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity)]] — Cross-reference
- [[entities/ecosystem-changelog|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index|Higress 知识图谱索引]]
