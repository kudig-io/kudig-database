---
title: Cloud Providers
description: 整合原 domain-12-cloud-providers/27 的云厂商知识，涵盖主流云服务商和多云混合部署。
summary: 整合原 domain-12-cloud-providers/27 的云厂商知识，涵盖主流云服务商和多云混合部署。
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
tier: core
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Cloud Providers

整合原 domain-12-cloud-providers/27 的云厂商知识，涵盖主流云服务商和多云混合部署。

## 目录结构

| 子目录 | 内容 |
|---|---|
| 01-alibaba-cloud/ | 阿里云通用（RAM、存储、网络、安全） |
| 02-aws-eks/ | AWS EKS 生产运维（含生产 Runbook） |
| 03-google-cloud-gke/ | Google GKE 生产运维（含生产 Runbook） |
| 04-azure-aks/ | Azure AKS 生产运维（含生产 Runbook） |
| 05-alicloud-ack/ | 阿里云 ACK 生产运维（含生产 Runbook） |
| 06-tencent-tke/ | 腾讯云 TKE 生产运维（含生产 Runbook） |
| 07-huawei-cce/ | 华为云 CCE 生产运维（含生产 Runbook） |
| 08-multi-cloud/ | 多云/混合云部署策略 |
| 09-ucloud-uk8s/ | UCloud UK8S |
| 10-ibm-iks/ | IBM Cloud Kubernetes Service |
| 11-oracle-oke/ | Oracle OKE |
| 12-volcengine-vek/ | 火山引擎 VKE |
| 13-ctyun-tke/ | 天翼云 TKE |
| 14-ecloud-cke/ | 移动云 CKE |
| 15-alicloud-apsara-ack/ | 阿里云专有云 ACK（Apsara Stack） |

## 与其他 Domain 的关系

- [[domain-01-cluster-fundamentals/README.md|domain-01-cluster-fundamentals]] — 集群架构
- [[domain-07-platform-engineering/README.md|domain-07-platform-engineering]] — 平台运维

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- networking|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic KUDIG Database — Global MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|[[Kubernetes 通用最佳实践参考|Kubernetes 通用最佳实践参考]]]] — Cross-reference
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


## 阿里云专有云与ACK (新增)

| 文档 | 说明 |
|:---|:---|
| 专有云架构概述 | 阿里云专有云产品矩阵、部署模式、与公有云差异 |
| ACK集群运维 | ACK专有版/托管版集群管理、日志监控、安全 |
| Terway-CNI网络 | Terway模式详解、IPAM管理、网络问题排查 |
| 阿里云存储集成 | ESSD/NAS/OSS存储CSI集成与问题排查 |
| 阿里云SLB与Ingress | SLB/ALB/NLB负载均衡与Ingress配置 |
| 专有云远程顾问指南 | 远程诊断方法论与受限场景替代方案 |
| 阿里云文档索引 | 全部阿里云文档的快速入口 |



<!-- risk-assessed -->
