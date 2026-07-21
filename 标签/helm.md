---
title: helm
description: Helm 包管理标签枢纽 — 涵盖 Helm Charts 管理、高级运维、Hooks、值模式、库 Chart、OCI Registry 等全部 Helm 领域知识
category: tag-index
tags:
- helm
- charts
- package-management
- templating
tier: core
difficulty: intermediate
domain: release-management
created: '2026-07-11'
last_updated: '2026-07-21'
---

# helm Tag Hub

> Helm 相关页面 — Helm Charts 管理、高级运维、Hooks、值模式、库 Chart 等。

## 核心定义

**Helm** 是 Kubernetes 的包管理器，通过 Chart（模板化 YAML + 默认值 + 依赖管理）实现应用的可重复部署、版本管理和配置参数化。Helm 3 移除了 Tiller，直接通过 kubeconfig 与 API Server 交互。

### Helm 核心概念

| 概念 | 描述 |
|------|------|
| Chart | 模板化 K8s 资源包（templates/ + values.yaml + Chart.yaml） |
| Release | Chart 的一次部署实例 |
| Repository | Chart 的远程存储（OCI / HTTP） |
| Values | 参数化配置，覆盖默认值 |
| Hooks | 生命周期钩子（pre-install, post-upgrade 等） |


## Helm 清单模式 (Helm Manifest Patterns)

- [[清单模式/Helm值模式/01-helm-values-best-practices|Helm Values 最佳实践]]
- [[清单模式/Helm值模式/02-helm-hooks-lifecycle|Helm Hooks 生命周期]]
- [[清单模式/Helm值模式/03-helm-library-charts-reuse|Helm Library Charts 复用]]
- [[清单模式/YAML参考/36-ecosystem-kustomize-helm-argocd|Kustomize/Helm/ArgoCD 生态]]
- [[清单模式/Kustomize模式/03-kustomize-remote-build-gitops|Kustomize 远程构建 GitOps]]

## 扩展机制 (Extension Mechanisms)

- [[专项技术/扩展机制/05-package-management-tools|包管理工具]]
- [[专项技术/扩展机制/06-helm-charts-management|Helm Charts 管理]]
- [[专项技术/扩展机制/07-helm-advanced-operations|Helm 高级运维]]

## GitOps 与 Helm (GitOps & Helm)

- [[发布变更/GitOps/99-helm-production-guide|Helm 生产指南]]
- [[发布变更/GitOps/01-argo-cd-enterprise-gitops|ArgoCD 企业级 GitOps]]
- [[发布变更/GitOps/99-tekton-cicd-guide|Tekton CI/CD 指南]]

## 概念 (Concepts)

- [[概念/helm-argocd-gitops|Helm ArgoCD GitOps]]
- [[概念/cli-tools-evolution|CLI 工具演进]]
- [[概念/gitops-tool-evolution|GitOps 工具演进]]
- [[概念/bp-common-best-practices|通用最佳实践]]

## 故障诊断 (Troubleshooting)

- [[故障诊断/高级排障/36-helm-chart-troubleshooting|Helm Chart 排障]]
- [[故障诊断/高级排障/structural-08-cluster-operations/03-helm-troubleshooting|Helm 结构化排障]]
- [[故障诊断/FTA故障树/list/helm-fta|Helm 故障树分析]]
- [[故障诊断/技能体系/26-helm-chart-failure|Helm Chart 故障]]
- [[技能/helm-fta|Helm FTA]]

## 集群基础 (Cluster Fundamentals)

- [[集群基础/控制平面/31-kubectl-complete-reference|kubectl 完整参考]]
- [[集群基础/架构总览/99-kubernetes-v1.33-practical-cookbook|K8s v1.33 实用手册]]
- [[集群基础/设计原则/11-extensibility-design-patterns|扩展性设计模式]]

## 安全 (Security)

- [[安全/策略治理/99-kyverno-policy-guide|Kyverno 策略指南]]
- [[安全/供应链/02-supply-chain-maturity-model|供应链成熟度模型]]
- [[安全/身份与访问/11-secret-management-tools|密钥管理工具]]

## 存储 (Storage)

- [[存储/K8s存储/04-storageclass-dynamic-provisioning|StorageClass 动态供给]]
- [[存储/K8s存储/14-cloud-native-storage|云原生存储]]
- [[存储/K8s存储/16-csi-migration-in-tree-to-csi|CSI 迁移]]

## 网络与可观测性 (Networking & Observability)

- [[网络/API网关/14-api-gateway-production-operations|API 网关生产运营]]
- [[网络/服务网格/99-istio-service-mesh-guide|Istio Service Mesh 指南]]
- [[可观测性/指标/99-prometheus-enterprise-guide|Prometheus 企业级指南]]
- [[可观测性/总览/14-chaos-engineering|混沌工程]]

## 平台工程 (Platform Engineering)

- [[平台工程/构建/01-platform-engineering-overview|平台工程概览]]
- [[平台工程/构建/07-crossplane-platform-composition|Crossplane 平台组合]]
- [[平台工程/运维/07-gitops-configuration-management|GitOps 配置管理]]
- [[平台工程/构建/20-crd-operator-development|CRD/Operator 开发]]

## 技能 (Skills)

- [[技能/best-practices/best-practices/common-best-practices|通用最佳实践]]
- [[技能/best-practices/deployment/02-single-node-deployment|单节点部署]]
- [[技能/best-practices/deployment/04-production-environment-deployment|生产环境部署]]
- [[技能/ts-cluster-operations|集群运维排障]]
- [[技能/learn-04-debug-tools-setup|调试工具配置]]

## 容器运行时 (Container Runtime)

- [[容器运行时/镜像管理/01-harbor-enterprise-image-registry|Harbor 企业级镜像仓库]]
- [[容器运行时/镜像管理/99-harbor-enterprise-guide|Harbor 企业级指南]]

## 数据库中间件 (Database Middleware)

- [[数据库中间件/数据库/07-redis-kubernetes-operator|Redis K8s Operator]]
- [[数据库中间件/数据库/08-kafka-kubernetes-strimzi|Kafka Strimzi]]
- [[数据库中间件/数据库/99-cloudnativepg-enterprise-guide|CloudNativePG 企业级指南]]
- [[数据库中间件/消息队列/03-message-queue-comparison|消息队列对比]]

## 知识字典 (Knowledge Dictionary)

- [[系统基础/知识字典/tooling/helm|Helm]]
- [[系统基础/知识字典/tooling/kustomize|Kustomize]]
- [[系统基础/知识字典/tooling/werf|Werf]]
- [[系统基础/知识字典/configuration/helm-values|Helm Values]]
- [[系统基础/知识字典/operations/installing-addons|安装 Addons]]
- [[系统基础/知识字典/operations/change-management-release|变更管理发布]]
- [[系统基础/速查卡/helm|Helm 速查卡]]

## 实体 (Entities)

- [[实体/helm|Helm]]
- [[实体/harbor|Harbor]]
- [[实体/argocd|ArgoCD]]
- [[实体/flux|Flux]]
- [[实体/crossplane|Crossplane]]
- [[实体/cdk8s|CDK8s]]
- [[实体/werf|Werf]]
- [[实体/operator-framework|Operator Framework]]
- [[实体/release-notes-cli-tools|Release Notes: CLI Tools]]
- [[实体/KUDIG Cheat Sheet Index|KUDIG Cheat Sheet Index]]

## 发布说明 (Release Notes)

- [[归档/release-notes/cli-tools/helm/RELEASE-NOTES-3.20|Helm 3.20 Release Notes]]
- [[归档/release-notes/cli-tools/helm/RELEASE-NOTES-4.0|Helm 4.0 Release Notes]]
- [[归档/release-notes/cli-tools/helm/RELEASE-NOTES-4.1|Helm 4.1 Release Notes]]

## 综合 (Synthesis)

- [[综合/helm-gitops|Helm GitOps]]

## 生态参考 (Ecosystem)

- [[生态参考/领域索引/helm-index|Helm 索引]]
- [[生态参考/论文/05-kubernetes-gitops-complete-practice-guide|GitOps 完整实践指南]]
- [[生态参考/论文/21-kubernetes-platform-engineering-internal-developer-platform|平台工程与内部开发者平台]]

## Related Tags

- [[标签/gitops|gitops]]
- [[标签/operator|operator]]
- [[标签/production|production]]
- [[标签/k8s|k8s]]
