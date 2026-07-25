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

- [[22-概念/05-安全/multi-cluster-security|多集群安全]]
- [[22-概念/06-可观测性/multi-cluster-observability-federation|多集群可观测性联邦]]
- [[22-概念/08-可靠性与运维/multi-cluster-dr-automation|多集群灾备自动化]]
- [[22-概念/08-可靠性与运维/cost-optimization-multi-cluster|多集群成本优化]]
- [[22-概念/12-研究/edge-cloud-continuum|边缘云连续体]]
- [[22-概念/11-交叉分析/IaC × 多集群管理|IaC 与多集群管理]]

## 生产运维 (Production Operations)

- [[13-生产运维/07-运维手册/06-multi-cluster-operations|多集群运营]]

## GitOps 多集群 (GitOps Multi-Cluster)

- [[11-发布变更/01-GitOps/08-fleet-gitops-operations-guide|Fleet GitOps 运营指南]]
- [[11-发布变更/01-GitOps/12-fleet-gitops-operations-guide|Fleet GitOps 运营指南]]
- [[03-清单模式/05-GitOps模式/02-argocd-applicationset-multi-cluster|ArgoCD ApplicationSet 多集群]]

## 网络 (Networking)

- [[05-网络/01-K8s网络核心/31-multi-cluster-federation|多集群联邦]]
- [[05-网络/01-K8s网络核心/32-multi-cluster-networking|多集群网络]]
- [[05-网络/01-K8s网络核心/04d-flannel-multi-cluster|Flannel 多集群]]

## 可靠性 / 灾备 (Reliability & DR)

- [[12-可靠性/02-灾难恢复/20-automated-dr-patterns-2025|自动化 DR 模式 2025]]
- [[04-应用模式/03-生产模式/multi-cluster-dr-patterns|多集群 DR 模式]]

## 故障诊断 (Troubleshooting)

- [[19-故障诊断/04-高级排障/37-multi-cluster-management-troubleshooting|多集群管理排障]]
- [[19-故障诊断/04-高级排障/40-large-scale-cluster-operations|大规模集群运营]]
- [[22-概念/14-case-studies/2026-09-15-multicluster-network-partition|多集群网络分区案例]]

## 平台工程 (Platform Engineering)

- [[10-平台工程/02-运维/13-multi-cluster-management|多集群管理]]
- [[10-平台工程/02-运维/25-virtual-clusters|虚拟集群]]

## 云厂商 (Cloud Providers)

- [[18-云厂商/07-多云混合/00-multi-cloud-hybrid-deployment-strategy|多云混合部署策略]]
- [[18-云厂商/07-多云混合/04-google-gke-enterprise-multicloud|GKE 企业级多云]]
- [[18-云厂商/07-多云混合/08-multicloud-federation-karmada|多云联邦 Karmada]]

## 研究 (Research)

- [[25-研究/03-平台与交付/gitops-multi-cluster|GitOps 多集群]]
- [[25-研究/03-平台与交付/multi-cluster-management|多集群管理]]

## 知识字典 (Knowledge Dictionary)

- [[17-系统基础/06-知识字典/multi-cloud/federation|集群联邦]]
- [[17-系统基础/06-知识字典/networking/clusternet|Clusternet]]
- [[17-系统基础/06-知识字典/networking/k8gb|K8GB]]
- [[17-系统基础/06-知识字典/networking/kubeslice|KubeSlice]]
- [[17-系统基础/06-知识字典/networking/submariner|Submariner]]
- [[17-系统基础/06-知识字典/networking/network-service-mesh|Network Service Mesh]]
- [[17-系统基础/06-知识字典/platform-engineering/armada|Armada]]
- [[17-系统基础/06-知识字典/platform-engineering/karmada|Karmada]]
- [[17-系统基础/06-知识字典/platform-engineering/kubestellar|KubeStellar]]
- [[17-系统基础/06-知识字典/platform-engineering/open-cluster-management|Open Cluster Management]]
- [[17-系统基础/06-知识字典/platform-engineering/rancher|Rancher]]
- [[17-系统基础/06-知识字典/scheduling/kubefleet|KubeFleet]]
- [[17-系统基础/06-知识字典/security/paralus|Paralus]]

## 实体 (Entities)

- [[23-实体/09-编排调度/karmada|Karmada]]
- [[23-实体/09-编排调度/kubestellar|KubeStellar]]
- [[23-实体/09-编排调度/clusternet|Clusternet]]
- [[23-实体/09-编排调度/kubefleet|KubeFleet]]
- [[23-实体/09-编排调度/open-cluster-management|Open Cluster Management]]
- [[23-实体/04-网络/k8gb|K8GB]]
- [[23-实体/04-网络/kubeslice|KubeSlice]]
- [[23-实体/04-网络/submariner|Submariner]]
- [[23-实体/15-参考与索引/cncf-orchestration|CNCF Orchestration]]
- [[23-实体/15-参考与索引/k8s-networking-ecosystem|K8s Networking Ecosystem]]

## 集群基础 (Cluster Fundamentals)

- [[01-集群基础/03-控制平面/25-multi-cloud-hybrid-deployment|多云混合部署架构]]
- [[21-生态参考/02-论文/04-kubernetes-multi-cloud-hybrid-deployment|多云混合部署架构与实践]]
- [[21-生态参考/02-论文/26-kubernetes-vcluster-virtual-cluster-multi-tenancy|vCluster 虚拟集群多租户]]

## 扩展机制 (Extension Mechanisms)

- [[16-专项技术/03-扩展机制/14-multi-cluster-management|多集群管理]]

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

- [[27-标签/gitops|gitops]]
- [[27-标签/networking|networking]]
- [[27-标签/reliability|reliability]]
- [[27-标签/production|production]]
- [[27-标签/security|security]]
