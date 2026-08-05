---
title: gitops
description: GitOps 标签枢纽 — 涵盖 ArgoCD、Flux、渐进式交付、CI/CD 管道、声明式基础设施、多集群 GitOps 等全部 GitOps 领域知识
category: tag-index
tags:
- gitops
- argocd
- flux
- progressive-delivery
- ci-cd
- declarative
tier: core
difficulty: intermediate
domain: release-management
k8s_versions: ["1.28", "1.30", "1.32", "1.34"]
created: '2026-07-11'
last_updated: '2026-07-21'
---

# gitops Tag Hub

> GitOps 领域页面 — ArgoCD、Flux、GitOps 模式、渐进式交付、CI/CD 管道等。

## 核心定义

**GitOps** 是一种以 Git 仓库为唯一事实来源（Single Source of Truth）来管理基础设施和应用的运维方法论。通过声明式配置 + 自动化调和循环，实现可审计、可回滚、可重现的持续交付。

### GitOps 四大原则

| 原则 | 描述 |
|------|------|
| 声明式 | 系统期望状态以声明式方式定义 |
| 版本化 | 期望状态存储在 Git 中，可追溯 |
| 自动调和 | 持续将实际状态向期望状态收敛 |
| 可审计 | 所有变更通过 Git 历史可审计 |

### GitOps 工具对比

| 工具 | 模式 | 多集群 | 渐进式交付 | 特色 |
|------|------|--------|------------|------|
| ArgoCD | Pull | 支持 | Argo Rollouts | UI 友好、Application CRD |
| Flux | Pull | 支持 | Flagger | 轻量、Kustomize 原生 |
| Fleet | Pull | 原生 | 支持 | Rancher 生态、大规模 |

## GitOps 平台 (GitOps Platforms)

- [[11-发布变更/01-GitOps/01-argo-cd-enterprise-gitops|ArgoCD 企业级 GitOps]]
- [[11-发布变更/01-GitOps/03-jenkins-enterprise-cicd|Jenkins 企业级 CI/CD]]
- [[11-发布变更/01-GitOps/04-gitlab-enterprise-cicd|GitLab 企业级 CI/CD]]
- [[11-发布变更/01-GitOps/05-github-actions-enterprise|GitHub Actions 企业级]]
- [[11-发布变更/01-GitOps/06-tekton-cloud-native-cicd|Tekton 云原生 CI/CD]]
- [[11-发布变更/01-GitOps/07-flux-gitops-continuous-delivery|Flux GitOps 持续交付]]
- [[11-发布变更/01-GitOps/08-gitops-security-compliance|GitOps 安全合规]]
- [[11-发布变更/01-GitOps/09-cicd-pipeline-patterns|CI/CD 管道模式]]
- [[11-发布变更/01-GitOps/10-fleet-gitops-operations-guide|Fleet GitOps 运营指南]]
- [[11-发布变更/01-GitOps/17-argo-cd-gitops-guide|ArgoCD GitOps 指南]]
- [[11-发布变更/01-GitOps/18-flux-gitops-guide|Flux GitOps 指南]]
- [[11-发布变更/01-GitOps/20-tekton-cicd-guide|Tekton CI/CD 指南]]

## 清单模式 (Manifest Patterns)

- [[03-清单模式/05-GitOps模式/01-argocd-app-of-apps|ArgoCD App-of-Apps]]
- [[03-清单模式/05-GitOps模式/02-argocd-applicationset-multi-cluster|ArgoCD ApplicationSet 多集群]]
- [[03-清单模式/05-GitOps模式/03-flux-kustomization-patterns|Flux Kustomization 模式]]
- [[03-清单模式/05-GitOps模式/04-gitops-directory-structure|GitOps 目录结构]]
- [[03-清单模式/05-GitOps模式/05-gitops-secret-management|GitOps 密钥管理]]
- [[03-清单模式/05-GitOps模式/06-gitops-wave-sync|GitOps Wave 同步]]
- [[03-清单模式/05-GitOps模式/07-gitops-drift-detection|GitOps 漂移检测]]
- [[03-清单模式/05-GitOps模式/08-gitops-progressive-delivery|GitOps 渐进式交付]]

## 概念 (Concepts)

- [[22-概念/09-平台与发布/gitops-principles|GitOps 原则]]
- [[22-概念/09-平台与发布/gitops-production-operations|GitOps 生产运营]]
- [[22-概念/09-平台与发布/gitops-release-gate|GitOps 发布门禁]]
- [[22-概念/09-平台与发布/gitops-sre-release-gate|GitOps SRE 发布门禁]]
- [[22-概念/12-研究/gitops-tool-evolution|GitOps 工具演进]]
- [[22-概念/09-平台与发布/helm-argocd-gitops|Helm ArgoCD GitOps]]
- [[22-概念/11-交叉分析/GitOps × 平台工程|GitOps 与平台工程]]
- [[22-概念/09-平台与发布/progressive-delivery-strategies|渐进式交付策略]]

## 部署方案 (Deployment)

- [[11-发布变更/06-部署方案/02-single-node-deployment|单节点部署]]
- [[11-发布变更/06-部署方案/04-production-environment-deployment|生产环境部署]]
- [[11-发布变更/04-变更管理/02-canary-release-strategy|金丝雀发布策略]]

## IaC (Infrastructure as Code)

- [[11-发布变更/02-IaC/06-infrastructure-as-code|基础设施即代码]]
- [[11-发布变更/02-IaC/07-crossplane-platform-guide|Crossplane 平台指南]]

## 集群基础 (Cluster Fundamentals)

- [[01-集群基础/03-控制平面/27-gitops-automation-operations|GitOps 自动化运营]]
- [[01-集群基础/02-设计原则/12-extensibility-design-patterns|扩展性设计模式]]

## 故障诊断 (Troubleshooting)

- [[19-故障诊断/04-高级排障/04-gitops-argocd-troubleshooting|GitOps ArgoCD 排障]]
- [[19-故障诊断/04-高级排障/structural-08-cluster-operations/03-helm-troubleshooting|Helm 排障]]
- [[19-故障诊断/04-高级排障/structural-11-gitops-devops/01-gitops-devops-troubleshooting|GitOps/DevOps 排障]]
- [[19-故障诊断/06-FTA故障树/list/gitops-argocd-fta|GitOps ArgoCD 故障树]]

## 技能 (Skills)

- [[26-技能/01-集群运维/gitops-argocd/gitops-argocd-fta|GitOps ArgoCD FTA]]
- [[26-技能/01-集群运维/gitops-argocd/诊断排障/ts-gitops-devops.md|GitOps/DevOps 排障]]
- [[20-最佳实践/07-scenarios/gitops-workflow|GitOps 工作流最佳实践]]

## 平台工程 (Platform Engineering)

- [[10-平台工程/02-运维/05-gitops-configuration-management|GitOps 配置管理]]
- [[10-平台工程/02-运维/06-automation-toolchain|自动化工具链]]

## 研究 (Research)

- [[25-研究/03-平台与交付/gitops-multi-cluster|GitOps 多集群]]

## 综合 (Synthesis)

- [[24-综合/02-交付与GitOps/argocd-gitops|ArgoCD GitOps]]
- [[24-综合/02-交付与GitOps/helm-gitops|Helm GitOps]]
- [[24-综合/02-交付与GitOps/crossplane-iac|Crossplane IaC]]

## 实体 (Entities)

- [[23-实体/08-交付与制品/argocd|ArgoCD]]
- [[23-实体/08-交付与制品/flux|Flux]]
- [[23-实体/08-交付与制品/argo|Argo]]
- [[23-实体/15-参考与索引/cncf-cicd|CNCF CI/CD]]
- [[23-实体/15-参考与索引/release-notes-cicd-gitops|Release Notes: CI/CD GitOps]]

## 扩展机制 (Extension Mechanisms)

- [[16-专项技术/03-扩展机制/08-cicd-pipelines|CI/CD 管道]]
- [[16-专项技术/03-扩展机制/09-gitops-workflow-argocd|GitOps 工作流 ArgoCD]]

## 知识字典 (Knowledge Dictionary)

- [[17-系统基础/06-知识字典/operations/gitops|GitOps]]
- [[17-系统基础/06-知识字典/operations/argo|Argo]]
- [[17-系统基础/06-知识字典/operations/flux|Flux]]
- [[17-系统基础/06-知识字典/operations/pipecd|PipeCD]]
- [[17-系统基础/06-知识字典/platform-engineering/gitops-and-continuous-delivery|GitOps 与持续交付]]
- [[17-系统基础/05-速查卡/gitops|GitOps 速查卡]]

## GitOps 技术全景

### GitOps 核心原则

| 原则 | 说明 |
|---|---|
| 声明式 | 系统状态以声明式配置描述 |
| 版本化 | 配置存储在 Git 中，可追溯 |
| 自动同步 | 自动将实际状态向期望状态调和 |
| 持续调和 | 检测漂移并自动修复 |

### GitOps 工具对比

| 工具 | 特点 | 适用场景 |
|---|---|---|
| ArgoCD | UI 丰富、多集群 | 复杂环境 |
| Flux | 轻量、原生 K8s | 简单环境 |
| Jenkins X | CI/CD 集成 | 全流程 |

## 面试要点

1. **Q：GitOps 的核心价值？**
   A：可审计、可回滚、一致性、自动化、协作友好。

2. **Q：ArgoCD vs Flux 如何选择？**
   A：ArgoCD：需要 UI、多集群、应用集。Flux：轻量、Helm 集成、简单环境。

3. **Q：GitOps 的安全控制？**
   A：分支保护、PR 审批、镜像签名、RBAC、审计日志、Secret 加密。

## Related Tags

- [[27-标签/05-交付与运维/helm|helm]]
- [[27-标签/01-核心平台/operator|operator]]
- [[27-标签/05-交付与运维/multi-cluster|multi-cluster]]
- [[27-标签/05-交付与运维/production|production]]
