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

- [[03-清单模式/03-Helm值模式/01-helm-values-best-practices|Helm Values 最佳实践]]
- [[03-清单模式/03-Helm值模式/02-helm-hooks-lifecycle|Helm Hooks 生命周期]]
- [[03-清单模式/03-Helm值模式/03-helm-library-charts-reuse|Helm Library Charts 复用]]
- [[03-清单模式/01-YAML参考/36-ecosystem-kustomize-helm-argocd|Kustomize/Helm/ArgoCD 生态]]
- [[03-清单模式/02-Kustomize模式/03-kustomize-remote-build-gitops|Kustomize 远程构建 GitOps]]

## 扩展机制 (Extension Mechanisms)

- [[16-专项技术/03-扩展机制/05-package-management-tools|包管理工具]]
- [[16-专项技术/03-扩展机制/06-helm-charts-management|Helm Charts 管理]]
- [[16-专项技术/03-扩展机制/07-helm-advanced-operations|Helm 高级运维]]

## GitOps 与 Helm (GitOps & Helm)

- [[11-发布变更/01-GitOps/99-helm-production-guide|Helm 生产指南]]
- [[11-发布变更/01-GitOps/01-argo-cd-enterprise-gitops|ArgoCD 企业级 GitOps]]
- [[11-发布变更/01-GitOps/99-tekton-cicd-guide|Tekton CI/CD 指南]]

## 概念 (Concepts)

- [[22-概念/09-平台与发布/helm-argocd-gitops|Helm ArgoCD GitOps]]
- [[22-概念/12-研究/cli-tools-evolution|CLI 工具演进]]
- [[22-概念/12-研究/gitops-tool-evolution|GitOps 工具演进]]
- [[22-概念/10-最佳实践/bp-common-best-practices|通用最佳实践]]

## 故障诊断 (Troubleshooting)

- [[19-故障诊断/04-高级排障/36-helm-chart-troubleshooting|Helm Chart 排障]]
- [[19-故障诊断/04-高级排障/structural-08-cluster-operations/03-helm-troubleshooting|Helm 结构化排障]]
- [[19-故障诊断/06-FTA故障树/list/helm-fta|Helm 故障树分析]]
- [[19-故障诊断/08-技能体系/26-helm-chart-failure|Helm Chart 故障]]
- [[26-技能/helm-fta|Helm FTA]]

## 集群基础 (Cluster Fundamentals)

- [[01-集群基础/03-控制平面/31-kubectl-complete-reference|kubectl 完整参考]]
- [[01-集群基础/01-架构总览/99-kubernetes-v1.33-practical-cookbook|K8s v1.33 实用手册]]
- [[01-集群基础/02-设计原则/11-extensibility-design-patterns|扩展性设计模式]]

## 安全 (Security)

- [[08-安全/04-策略治理/99-kyverno-policy-guide|Kyverno 策略指南]]
- [[08-安全/05-供应链/02-supply-chain-maturity-model|供应链成熟度模型]]
- [[08-安全/01-身份与访问/11-secret-management-tools|密钥管理工具]]

## 存储 (Storage)

- [[06-存储/01-K8s存储/04-storageclass-dynamic-provisioning|StorageClass 动态供给]]
- [[06-存储/01-K8s存储/14-cloud-native-storage|云原生存储]]
- [[06-存储/01-K8s存储/16-csi-migration-in-tree-to-csi|CSI 迁移]]

## 网络与可观测性 (Networking & Observability)

- [[05-网络/04-API网关/14-api-gateway-production-operations|API 网关生产运营]]
- [[05-网络/03-服务网格/99-istio-service-mesh-guide|Istio Service Mesh 指南]]
- [[09-可观测性/02-指标/99-prometheus-enterprise-guide|Prometheus 企业级指南]]
- [[09-可观测性/01-总览/14-chaos-engineering|混沌工程]]

## 平台工程 (Platform Engineering)

- [[10-平台工程/01-构建/01-platform-engineering-overview|平台工程概览]]
- [[10-平台工程/01-构建/07-crossplane-platform-composition|Crossplane 平台组合]]
- [[10-平台工程/02-运维/07-gitops-configuration-management|GitOps 配置管理]]
- [[10-平台工程/01-构建/20-crd-operator-development|CRD/Operator 开发]]

## 技能 (Skills)

- [[20-最佳实践/01-best-practices/common-best-practices|通用最佳实践]]
- [[20-最佳实践/02-deployment/02-single-node-deployment|单节点部署]]
- [[20-最佳实践/02-deployment/04-production-environment-deployment|生产环境部署]]
- [[26-技能/01-集群运维/cluster-upgrade/诊断排障/ts-cluster-operations.md|集群运维排障]]
- [[26-技能/04-工作负载/pod/培训/learn-04-debug-tools-setup.md|调试工具配置]]

## 容器运行时 (Container Runtime)

- [[14-容器运行时/02-镜像管理/01-harbor-enterprise-image-registry|Harbor 企业级镜像仓库]]
- [[14-容器运行时/02-镜像管理/99-harbor-enterprise-guide|Harbor 企业级指南]]

## 数据库中间件 (Database Middleware)

- [[07-数据库中间件/01-数据库/07-redis-kubernetes-operator|Redis K8s Operator]]
- [[07-数据库中间件/01-数据库/08-kafka-kubernetes-strimzi|Kafka Strimzi]]
- [[07-数据库中间件/01-数据库/99-cloudnativepg-enterprise-guide|CloudNativePG 企业级指南]]
- [[07-数据库中间件/03-消息队列/03-message-queue-comparison|消息队列对比]]

## 知识字典 (Knowledge Dictionary)

- [[17-系统基础/06-知识字典/tooling/helm|Helm]]
- [[17-系统基础/06-知识字典/tooling/kustomize|Kustomize]]
- [[17-系统基础/06-知识字典/tooling/werf|Werf]]
- [[17-系统基础/06-知识字典/configuration/helm-values|Helm Values]]
- [[17-系统基础/06-知识字典/operations/installing-addons|安装 Addons]]
- [[17-系统基础/06-知识字典/operations/change-management-release|变更管理发布]]
- [[17-系统基础/05-速查卡/helm|Helm 速查卡]]

## 实体 (Entities)

- [[23-实体/08-交付与制品/helm|Helm]]
- [[23-实体/08-交付与制品/harbor|Harbor]]
- [[23-实体/08-交付与制品/argocd|ArgoCD]]
- [[23-实体/08-交付与制品/flux|Flux]]
- [[23-实体/08-交付与制品/crossplane|Crossplane]]
- [[23-实体/08-交付与制品/cdk8s|CDK8s]]
- [[23-实体/08-交付与制品/werf|Werf]]
- [[23-实体/10-平台与开发工具/operator-framework|Operator Framework]]
- [[23-实体/15-参考与索引/release-notes-cli-tools|Release Notes: CLI Tools]]
- [[23-实体/15-参考与索引/KUDIG Cheat Sheet Index|KUDIG Cheat Sheet Index]]

## 发布说明 (Release Notes)

- [[37-归档/release-notes/cli-tools/helm/RELEASE-NOTES-3.20|Helm 3.20 Release Notes]]
- [[37-归档/release-notes/cli-tools/helm/RELEASE-NOTES-4.0|Helm 4.0 Release Notes]]
- [[37-归档/release-notes/cli-tools/helm/RELEASE-NOTES-4.1|Helm 4.1 Release Notes]]

## 综合 (Synthesis)

- [[24-综合/02-交付与GitOps/helm-gitops|Helm GitOps]]

## 生态参考 (Ecosystem)

- [[21-生态参考/03-领域索引/helm-index|Helm 索引]]
- [[21-生态参考/02-论文/05-kubernetes-gitops-complete-practice-guide|GitOps 完整实践指南]]
- [[21-生态参考/02-论文/21-kubernetes-platform-engineering-internal-developer-platform|平台工程与内部开发者平台]]

## Helm 技术全景

### Helm 核心概念

| 概念 | 说明 |
|---|---|
| Chart | 应用打包格式 |
| Release | Chart 的一次部署实例 |
| Repository | Chart 存储仓库 |
| Values | 配置参数化 |

### Helm 常用命令

```bash
# 🟢 搜索 Chart
helm search repo <keyword>
# 🟡 安装/升级
helm upgrade --install <release> <chart> -f values.yaml
# 🟢 查看历史
helm history <release>
# 🔴 回滚
helm rollback <release> <revision>
```

## 面试要点

1. **Q：Helm 的核心价值？**
   A：包管理、参数化配置、版本管理、依赖管理、回滚能力。

2. **Q：Helm 2 vs Helm 3 的区别？**
   A：Helm 3 移除 Tiller、原生 K8s API、支持 CRD、更安全。

3. **Q：如何设计高质量的 Chart？**
   A：参数化配置、完善文档、合理默认值、依赖管理、测试覆盖。

## Related Tags

- [[27-标签/gitops|gitops]]
- [[27-标签/operator|operator]]
- [[27-标签/production|production]]
- [[27-标签/k8s|k8s]]
