---
title: platform-engineering
description: 平台工程标签枢纽 — 涵盖内部开发者平台(IDP)、集群生命周期管理、GitOps 交付、平台治理、开发者体验与自动化工具链的完整知识索引
category: tag-index
tags:
- platform-engineering
- idp
- developer-experience
- infrastructure
- automation
tier: core
difficulty: intermediate-to-advanced
domain: platform-engineering
k8s_versions: ["1.28", "1.30", "1.32", "1.34"]
created: '2026-07-21'
last_updated: '2026-07-21'
---

# platform-engineering Tag Hub

> 平台工程领域页面 — 内部开发者平台 (IDP)、集群生命周期管理、GitOps 交付、平台治理、开发者体验、自动化工具链等。

## 核心定义

**平台工程（Platform Engineering）** 是设计和构建内部开发者平台（Internal Developer Platform, IDP）的学科，旨在为开发团队提供自助式、标准化的基础设施和服务能力，降低认知负载，加速软件交付。

### 平台工程核心能力

| 能力域 | 描述 | 关键工具 |
|--------|------|----------|
| 基础设施供给 | 集群/环境的自动化创建与管理 | Terraform, Crossplane, Cluster API |
| 应用交付 | 标准化的部署流水线 | ArgoCD, Flux, Helm, Tekton |
| 开发者门户 | 统一的服务目录与自助操作 | Backstage, Port, Cortex |
| 可观测性即服务 | 开箱即用的监控/日志/追踪 | Prometheus Stack, Grafana, OTel |
| 安全合规 | 策略即代码、准入控制 | Kyverno, OPA, Falco |
| 成本管理 | 资源可视化与优化建议 | Kubecost, OpenCost, Parca |
| 配置管理 | 集中化配置与密钥管理 | Vault, External Secrets, Sealed Secrets |

### 平台成熟度模型

| 级别 | 名称 | 特征 |
|------|------|------|
| L1 | 临时型 | 手动操作，无标准化 |
| L2 | 可重复 | 脚本化部署，基本文档 |
| L3 | 已定义 | 标准化模板，自助服务 |
| L4 | 已管理 | 度量驱动，持续优化 |
| L5 | 优化型 | AI 辅助，全自动运营 |

## 平台构建 (Platform Building)

- [[平台工程/构建/01-platform-engineering-overview|平台工程概览]]
- [[平台工程/构建/03-backstage-deployment|Backstage 部署]]
- [[平台工程/构建/02-idp-architecture-design|IDP 架构设计]]
- [[平台工程/构建/04-developer-portal-patterns|开发者门户模式]]

## 运维管理 (Operations)

- [[平台工程/运维/02-cluster-lifecycle-management|集群生命周期管理]]
- [[平台工程/运维/13-multi-cluster-management|多集群管理]]
- [[平台工程/运维/15-production-troubleshooting|生产环境故障排查]]
- [[平台工程/99-karpenter-node-autoscaling-guide|Karpenter 节点弹性伸缩指南]]

## 治理 (Governance)

- [[平台工程/治理/01-platform-governance-model|平台治理模型]]
- [[平台工程/治理/02-policy-as-code-governance|策略即代码治理]]
- [[平台工程/治理/03-cost-governance-optimization|成本治理优化]]

## 开发体验 (Developer Experience)

- [[平台工程/开发体验/01-developer-experience-overview|开发者体验概览]]
- [[平台工程/开发体验/02-self-service-infrastructure|自助式基础设施]]
- [[平台工程/开发体验/03-golden-path-templates|黄金路径模板]]

## 代码分析 (Code Analysis)

- [[平台工程/代码分析/README|代码分析索引]]

## GitOps 与交付

- [[发布变更/GitOps/01-argocd-enterprise-gitops|ArgoCD 企业级 GitOps]]
- [[发布变更/GitOps/02-flux-enterprise-gitops|Flux 企业级 GitOps]]
- [[发布变更/GitOps/08-fleet-gitops-operations-guide|Fleet GitOps 运营]]
- [[发布变更/IaC/01-terraform-kubernetes-infrastructure|Terraform K8s 基础设施]]
- [[发布变更/IaC/02-crossplane-cloud-native-iac|Crossplane 云原生 IaC]]

## 生产就绪评估

- [[平台工程/99-production-readiness-review-template|生产就绪评估模板]]
- [[平台工程/12-automated-operations-toolchain|自动化运维工具链]]

## 平台工程关键指标

| 指标 | 目标 | 度量方式 |
|------|------|----------|
| 环境供给时间 | < 30 分钟 | 从请求到可用的时间 |
| 部署频率 | 按需 / 每日多次 | 每周部署次数 |
| 变更前置时间 | < 1 小时 | 从提交到生产的时间 |
| 变更失败率 | < 5% | 导致回滚的变更比例 |
| 恢复时间 (MTTR) | < 30 分钟 | 从故障到恢复的时间 |
| 开发者满意度 | > 80% | 季度 NPS 调查 |
| 自助服务比例 | > 90% | 无需平台团队介入的操作比例 |

## 工具生态

| 类别 | 工具 | 用途 |
|------|------|------|
| IaC | Terraform, Pulumi, Crossplane | 基础设施即代码 |
| GitOps | ArgoCD, Flux, Fleet | 声明式持续交付 |
| 门户 | Backstage, Port, Cortex | 开发者门户 |
| 集群管理 | Cluster API, Rancher, KubeSphere | 多集群生命周期 |
| 策略 | Kyverno, OPA Gatekeeper | 准入控制与策略 |
| 密钥 | Vault, External Secrets Operator | 密钥管理 |
| 成本 | Kubecost, OpenCost | 成本可视化 |
| CI/CD | Tekton, GitHub Actions, GitLab CI | 构建流水线 |

## 概念 (Concepts)

- [[概念/platform-engineering|平台工程]]
- [[概念/internal-developer-platform|内部开发者平台]]
- [[概念/gitops|GitOps]]
- [[概念/infrastructure-as-code|基础设施即代码]]

## 实体 (Entities)

- [[实体/backstage|Backstage]]
- [[实体/argocd|ArgoCD]]
- [[实体/flux|Flux]]
- [[实体/crossplane|Crossplane]]
- [[实体/cluster-api|Cluster API]]

## 平台工程全景

### 平台工程核心组件

| 组件 | 功能 | 工具 |
|---|---|---|
| IDP | 内部开发者平台 | Backstage, Port |
| IaC | 基础设施即代码 | Terraform, Pulumi |
| CI/CD | 持续集成/部署 | GitHub Actions, ArgoCD |
| 自助服务 | 资源自助申请 | Crossplane, KubeVela |

### 平台工程成熟度模型

```
L1: 手动运维 → L2: 脚本自动化 → L3: 平台化 → L4: 自助服务 → L5: 智能运维
```

## 面试要点

1. **Q：平台工程的核心价值？**
   A：降低认知负载、提升开发效率、标准化最佳实践、自助服务、可观测性。

2. **Q：IDP 的核心功能？**
   A：服务目录、模板化创建、自助部署、可观测性集成、文档中心、成本可视化。

3. **Q：如何衡量平台工程效果？**
   A：DORA 指标、开发者满意度、自助服务率、部署频率、故障恢复时间。

## Related Tags

- [[标签/k8s|k8s — Kubernetes 核心]]
- [[标签/gitops|gitops — GitOps 交付]]
- [[标签/production|production — 生产运营]]
- [[标签/reliability|reliability — 可靠性工程]]
- [[标签/sre|sre — 站点可靠性工程]]
- [[标签/multi-cluster|multi-cluster — 多集群管理]]
- [[标签/best-practices|best-practices — 最佳实践]]
- [[标签/helm|helm — 包管理]]
- [[标签/operator|operator — Operator 模式]]
