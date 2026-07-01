---
title: GitOps 工具演进
description: '- 集群注册安全改进（修复 client-cert 凭证持久化问题 #1742）'
category: concepts
tags:
- k8s
- release-notes
- argocd
- flux
- tekton
- gitops
- cicd
- helm
- rbac
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- GitOps 工具演进 是什么
- 如何 GitOps 工具演进
trigger_keywords:
- GitOps
- 工具演进
prerequisites:
- kubectl-basics
- helm-basics
- gitops-basics
created: "2026-05-23"
---

# GitOps 工具演进

> 本文档综合了 `domain-19-landscape-references/_archived-release-notes/cicd-gitops/` 目录下 Argo CD、[[flux|flux]] 和 Tekton 三大工具的 171 个版本发布说明 ^[inferred]

## 工具概览

| 工具 | 项目 | 版本范围 | 定位 |
|---|---|---|---|
| Argo CD | argoproj/argo-cd | v0.1 - v3.3 | GitOps CD 持续交付 |
| Flux | fluxcd/flux2 | v0.0 - v0.17+ | GitOps CD 持续交付 |
| Tekton | tektoncd/pipeline | v0.1 - v0.80 | CI/CD Pipeline 编排 |

## Argo CD 版本演进

Argo CD 是声明式 GitOps 持续交付工具，支持 Kubernetes 资源同步。

### 早期版本（v0.1 - v0.12）

- 初始版本，基本 GitOps 功能
- 支持 Git 仓库与应用同步
- 基本 Web UI

### v1.0 - 首次 GA

- 生产就绪版本
- 集群注册安全改进（修复 client-cert 凭证持久化问题 #1742）
- 支持 HA 部署模式
- 快速安装命令：`kubectl apply -n [[entities/argocd.md|[[ArgoCD|argocd]]]] -f install.yaml`

### v1.x 系列

- 多集群管理
- 改进的 RBAC
- 应用健康评估增强
- SSO 集成（OIDC、SAML）
- 支持 Helm、Kustomize、Jsonnet 等部署工具

### v2.0 - 重大里程碑

- 应用集（ApplicationSet）引入，支持大规模 GitOps
- 改进的 Git 认证
- 更好的多租户支持
- 资源树可视化改进

### v2.x 系列（v2.1 - v2.14）

- 应用集功能持续增强
- 改进的同步波浪（Sync Waves）
- 更好的健康检查
- 通知集成
- 改进的 UI/UX
- 支持 Git 仓库验证和预同步检查

### v3.x 系列（v3.0 - v3.3）

- 架构现代化
- 改进的性能和可扩展性
- 更好的扩展性 ^[inferred]

## Flux 版本演进

Flux 是 CNCF 孵化的 GitOps 工具集，Flux v2（Flux CD）基于 GitOps Toolkit 构建。

### v0.0 - v0.17（Flux v2 早期）

- source-controller：Git/Helm 仓库源管理
- kustomize-controller：Kustomize 渲染与部署
- helm-controller：Helm Release 管理
- notification-controller：事件通知

每个版本持续改进控制器功能和稳定性。

### Flux v2 核心组件

| 组件 | 功能 |
|---|---|
| source-controller | 管理 Git 仓库、Helm Chart 等 artifact 源 |
| kustomize-controller | 渲染 Kustomize 配置并应用到集群 |
| helm-controller | 管理 Helm Release 生命周期 |
| notification-controller | 接收和转发事件，支持 Alertmanager、Slack、Discord |
| image-reflector-controller | 扫描容器镜像仓库 |
| image-automation-controller | 自动更新 Git 仓库中的镜像引用 |

## Tekton Pipelines 版本演进

Tekton 是云原生 CI/CD Pipeline 框架，基于 Kubernetes CRD 构建。

### v0.1 - v0.10

- 基础 Pipeline 和 Task CRD
- 基本的 Step 执行
- PipelineRun 和 TaskRun 生命周期

### v0.11 - v0.30

- Workspaces 支持，共享数据
- 改进的 Task 复用
- Pipeline 结果和参数
- Better 错误处理和重试

### v0.31 - v0.50

- 改进的 Tekton CLI（tkn）
- Remote Resolvers
- CEL 表达式支持
- 更好的事件和通知

### v0.51 - v0.80

- 大规模 Pipeline 支持
- 改进的安全模型
- 更好的可观测性
- Enterprise 功能增强 ^[inferred]

## 工具对比

| 维度 | Argo CD | Flux | Tekton |
|---|---|---|---|
| 主要用途 | CD（持续交付） | CD（持续交付） | CI/CD Pipeline 编排 |
| GitOps 核心 | 应用同步 + 漂移检测 | 声明式基础设施 | Pipeline 即代码 |
| 部署方式 | Argo CD Server + Repo Server | 控制器模式 | Controller + Webhook |
| UI | 完整 Web UI | CLI 为主 + Weave GitOps 可选 | Dashboard 可选 |
| 多集群 | 原生支持 | 原生支持 | 通过 Pipeline 设计支持 |
| 扩展性 | 插件 + ApplicationSet | GitOps Toolkit 组合 | Task/Step 组合 |

## 选择建议

1. **选择 Argo CD**：需要完整的 CD 解决方案、Web UI、多集群管理
2. **选择 Flux**：偏好声明式配置、与 CNCF 生态深度集成
3. **选择 Tekton**：需要灵活的 CI/CD Pipeline 编排能力
4. **组合使用**：Tekton（CI）+ Argo CD/Flux（CD）是常见模式

## 来源文档

- domain-19-landscape-references/_archived-release-notes/cicd-gitops/argo-cd/（40 个文件）
- domain-19-landscape-references/_archived-release-notes/cicd-gitops/flux/（51 个文件）
- domain-19-landscape-references/_archived-release-notes/cicd-gitops/tekton/（80 个文件）

## Related

- [[helm]] — Helm
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[argo]] — Argo Workflows
- [[flux]] — Flux
- [[entities/argocd.md|argocd]] — ArgoCD
