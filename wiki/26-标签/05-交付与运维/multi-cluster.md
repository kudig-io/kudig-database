---
title: multi-cluster
description: 多集群管理标签枢纽 — 涵盖集群联邦、多集群网络、多集群安全、多集群可观测性、DR 自动化、Fleet 管理等全部多集群领域知识
category: tag-index
tags:
- multi-cluster
- federation
- fleet
- cross-cluster
tier: core
difficulty: advanced
domain: platform-engineering
created: '2026-07-11'
last_updated: '2026-07-21'
---

# multi-cluster Tag Hub

> 多集群管理页面 — 集群联邦、多集群网络、多集群安全、多集群可观测性、DR 自动化等。

## 核心定义

**多集群管理（Multi-Cluster Management）** 是在多个 Kubernetes 集群之间实现统一治理、工作负载分发、网络互通、安全一致性的系统化实践。常见场景包括多区域高可用、多环境隔离、多租户分离等。

### 多集群架构模式

| 模式 | 描述 | 适用场景 |
|------|------|----------|
| Hub-Spoke | 中心集群管控成员集群 | 统一治理 |
| 对等联邦 | 集群间对等互联 | 跨区域 DR |
| 独立集群 | 各集群独立运营 | 多租户隔离 |
| 混合云 | 公有云 + 私有云 | 合规 + 弹性 |


## 概念 (Concepts)

- [[wiki/22-概念/05-安全/multi-cluster-security|多集群安全]]
- [[wiki/22-概念/06-可观测性/multi-cluster-observability-federation|多集群可观测性联邦]]
- [[wiki/22-概念/08-可靠性与运维/multi-cluster-dr-automation|多集群灾备自动化]]
- [[wiki/22-概念/08-可靠性与运维/cost-optimization-multi-cluster|多集群成本优化]]
- [[wiki/22-概念/12-研究/edge-cloud-continuum|边缘云连续体]]
- [[wiki/22-概念/11-交叉分析/IaC × 多集群管理|IaC 与多集群管理]]

## 生产运维 (Production Operations)

- [[wiki/13-生产运维/07-运维手册/05-multi-cluster-operations|多集群运营]]

## GitOps 多集群 (GitOps Multi-Cluster)

- [[wiki/11-发布变更/01-GitOps/10-fleet-gitops-operations-guide|Fleet GitOps 运营指南]]
- [[wiki/11-发布变更/01-GitOps/14-fleet-gitops-operations-guide|Fleet GitOps 运营指南]]
- [[wiki/03-清单模式/05-GitOps模式/02-argocd-applicationset-multi-cluster|ArgoCD ApplicationSet 多集群]]

## 网络 (Networking)

- [[wiki/05-网络/01-K8s网络核心/33-multi-cluster-federation|多集群联邦]]
- [[wiki/05-网络/01-K8s网络核心/34-multi-cluster-networking|多集群网络]]
- [[wiki/05-网络/01-K8s网络核心/04d-flannel-multi-cluster|Flannel 多集群]]

## 可靠性 / 灾备 (Reliability & DR)

- [[wiki/12-可靠性/02-灾难恢复/23-automated-dr-patterns-2025|自动化 DR 模式 2025]]
- [[wiki/04-应用模式/03-生产模式/multi-cluster-dr-patterns|多集群 DR 模式]]

## 故障诊断 (Troubleshooting)

- [[wiki/19-故障诊断/04-高级排障/03-multi-cluster-management-troubleshooting|多集群管理排障]]
- [[wiki/19-故障诊断/04-高级排障/06-large-scale-cluster-operations|大规模集群运营]]
- [[wiki/22-概念/14-案例研究/2026-09-15-multicluster-network-partition|多集群网络分区案例]]

## 平台工程 (Platform Engineering)

- [[wiki/10-平台工程/02-运维/10-multi-cluster-management|多集群管理]]
- [[wiki/10-平台工程/02-运维/16-virtual-clusters|虚拟集群]]

## 云厂商 (Cloud Providers)

- [[wiki/18-云厂商/07-多云混合/00-multi-cloud-hybrid-deployment-strategy|多云混合部署策略]]
- [[wiki/18-云厂商/07-多云混合/04-google-gke-enterprise-multicloud|GKE 企业级多云]]
- [[wiki/18-云厂商/07-多云混合/08-multicloud-federation-karmada|多云联邦 Karmada]]

## 研究 (Research)

- [[wiki/25-研究/03-平台与交付/gitops-multi-cluster|GitOps 多集群]]
- [[wiki/25-研究/03-平台与交付/multi-cluster-management|多集群管理]]

## 知识字典 (Knowledge Dictionary)

- [[wiki/17-系统基础/06-知识字典/multi-cloud/federation|集群联邦]]
- [[wiki/17-系统基础/06-知识字典/networking/clusternet|Clusternet]]
- [[wiki/17-系统基础/06-知识字典/networking/k8gb|K8GB]]
- [[wiki/17-系统基础/06-知识字典/networking/kubeslice|KubeSlice]]
- [[wiki/17-系统基础/06-知识字典/networking/submariner|Submariner]]
- [[wiki/17-系统基础/06-知识字典/networking/network-service-mesh|Network Service Mesh]]
- [[wiki/17-系统基础/06-知识字典/platform-engineering/armada|Armada]]
- [[wiki/17-系统基础/06-知识字典/platform-engineering/karmada|Karmada]]
- [[wiki/17-系统基础/06-知识字典/platform-engineering/kubestellar|KubeStellar]]
- [[wiki/17-系统基础/06-知识字典/platform-engineering/open-cluster-management|Open Cluster Management]]
- [[wiki/17-系统基础/06-知识字典/platform-engineering/rancher|Rancher]]
- [[wiki/17-系统基础/06-知识字典/scheduling/kubefleet|KubeFleet]]
- [[wiki/17-系统基础/06-知识字典/security/paralus|Paralus]]

## 实体 (Entities)

- [[wiki/23-实体/09-编排调度/karmada|Karmada]]
- [[wiki/23-实体/09-编排调度/kubestellar|KubeStellar]]
- [[wiki/23-实体/09-编排调度/clusternet|Clusternet]]
- [[wiki/23-实体/09-编排调度/kubefleet|KubeFleet]]
- [[wiki/23-实体/09-编排调度/open-cluster-management|Open Cluster Management]]
- [[wiki/23-实体/04-网络/k8gb|K8GB]]
- [[wiki/23-实体/04-网络/kubeslice|KubeSlice]]
- [[wiki/23-实体/04-网络/submariner|Submariner]]
- [[wiki/23-实体/15-参考与索引/cncf-orchestration|CNCF Orchestration]]
- [[wiki/23-实体/15-参考与索引/k8s-networking-ecosystem|K8s Networking Ecosystem]]

## 集群基础 (Cluster Fundamentals)

- [[wiki/01-集群基础/03-控制平面/26-multi-cloud-hybrid-deployment|多云混合部署架构]]
- [[wiki/21-生态参考/02-论文/04-kubernetes-multi-cloud-hybrid-deployment|多云混合部署架构与实践]]
- [[wiki/21-生态参考/02-论文/26-kubernetes-vcluster-virtual-cluster-multi-tenancy|vCluster 虚拟集群多租户]]

## 扩展机制 (Extension Mechanisms)

- [[wiki/16-专项技术/03-扩展机制/14-multi-cluster-management|多集群管理]]

## 多集群技术全景

### 多集群架构模式

| 模式 | 特点 | 适用场景 |
|---|---|---|
| 主从模式 | 中心管理+工作集群 | 统一管控 |
| 对等模式 | 独立集群互联 | 地域分布 |
| 联邦模式 | KubeFed 统一调度 | 跨云部署 |
| 网格模式 | 服务网格多集群 | 流量管理 |

### 核心挑战与解决方案

| 挑战 | 解决方案 |
|---|---|
| 配置一致性 | GitOps + Cluster API |
| 服务发现 | 服务网格 + DNS 联邦 |
| 流量调度 | Istio/Linkerd 多集群 |
| 安全互信 | mTLS + 证书管理 |

## 面试要点

1. **Q：多集群架构的核心价值？**
   A：高可用(跨集群容灾)、合规(数据主权)、性能(就近访问)、成本(资源优化)。

2. **Q：多集群 vs 单集群大规模？**
   A：单集群：简单、一致性好，但有规模上限。多集群：复杂、但可扩展、容灾强。

3. **Q：多集群流量调度策略？**
   A：基于地域、基于权重、基于健康、基于成本。工具：Istio、Submariner、Admiralty。

## Related Tags

- [[wiki/26-标签/05-交付与运维/gitops|gitops]]
- [[wiki/26-标签/02-网络与存储/networking|networking]]
- [[wiki/26-标签/05-交付与运维/reliability|reliability]]
- [[wiki/26-标签/05-交付与运维/production|production]]
- [[wiki/26-标签/03-安全与合规/security|security]]
- [[skills/01-集群运维/cluster-upgrade/最佳实践/scen-README|生产场景导航]]
