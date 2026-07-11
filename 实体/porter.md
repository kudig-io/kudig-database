---
title: Porter (entities)
description: 'summary: "Porter 是一个 CNAB (Cloud Native Application Bundle) 包管理器，用于将复杂的云原生应用及其所有依赖（Helm
  Charts、Terraform 模块、Kubernetes manifests、脚本等）打包为可分发、可安装的 Bundle。它解决了"我的应用需要先部署数据库，再配置网络，最后部署应用"这类多步骤安装流程的自动化问题，支'
summary: 'summary: "Porter 是一个 CNAB (Cloud Native Application Bundle) 包管理器，用于将复杂的云原生应用及其所有依赖（Helm
  Charts、Terraform 模块、Kubernetes manifests、脚本等）打包为可分发、可安装的 Bundle。它解决了"我的应用需要先部署数据库，再配置网络，最后部署应用"这类多步骤安装流程的自动化问题，支'
category: general
tags:
- k8s
- helm
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Porter 是什么
- 如何 Porter
trigger_keywords:
- Porter
prerequisites:
- kubectl-basics
- helm-basics
- iac-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: "Porter"
category: entities
summary: "Porter 是一个 CNAB (Cloud Native Application Bundle) 包管理器，用于将复杂的云原生应用及其所有依赖（Helm Charts、Terraform 模块、Kubernetes manifests、脚本等）打包为可分发、可安装的 Bundle。它解决了"我的应用需要先部署数据库，再配置网络，最后部署应用"这类多步骤安装流程的自动化问题，支持安装、升级、..."
tags: k8s, cncf, config, porter]
sources: ["docs/生态参考/sandbox/porter/porter.md", "生态参考/sandbox/porter/porter.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: reviewed
lifecycle_changed: "2026-05-21"
tier: reference
base_confidence: 0.7
---

# Porter

> **CNCF 状态**: Sandbox | **类别**: Config | **主要语言**: Go

## 概述

Porter 是一个 CNAB（Cloud Native Application Bundle）包管理器，由 Microsoft 开源，2019 年加入 CNCF Sandbox。它将复杂的云原生应用及其所有依赖（Helm Charts、Terraform 模块、Kubernetes manifests、脚本等）打包为可分发、可安装的 Bundle。Porter 解决了"我的应用需要先部署数据库，再配置网络，最后部署应用"这类多步骤安装流程的自动化问题，支持安装、升级、卸载的完整生命周期管理。

## 核心特性

- **CNAB 标准**: 遵循 CNAB（Cloud Native Application Bundle）规范
- **Mixin 架构**: 通过 Mixin 复用 Helm、Terraform、Kubernetes、exec 等组件
- **凭证管理**: 安全的凭证集（Credential Set）管理敏感信息
- **参数化**: 支持参数和输出，实现可定制安装
- **OCI 分发**: Bundle 打包为 OCI 制品，通过标准 Registry 分发
- **跨平台**: 同一 Bundle 可部署到不同云平台

## 架构

Porter 架构围绕 CNAB 规范构建。开发者使用 `porter.yaml` 定义 Bundle——声明 Mixin（如 helm、terraform）、参数、凭证和步骤（install/upgrade/uninstall）。Porter CLI 将定义编译为 invocation image（Docker 镜像，包含所有工具和脚本）和 CNAB Bundle 元数据。执行时，Porter 运行 invocation image 容器，按序执行步骤，通过环境变量传递参数和凭证。Mixin 是可插拔的执行器——helm Mixin 调用 helm CLI，terraform Mixin 调用 terraform CLI。

## Kubernetes 集成

Porter 通过 Kubernetes Mixin 或 Helm Mixin 与 Kubernetes 集成。Kubernetes Mixin 直接应用 manifest YAML，Helm Mixin 管理 Helm Release。Porter 本身可以在集群内运行（Porter Agent）或集群外运行。安装 Bundle 时，Porter 通过 kubeconfig 连接目标集群，按步骤执行资源部署。Bundle 的 OCI 制品可存储在 Harbor、Distribution 等标准 Registry 中。

## 生产使用场景

1. **复杂应用打包**: 将需要 Helm + Terraform + 配置脚本的应用打包为单一 Bundle
2. **一键部署**: 客户/运维通过 `porter install` 一键完成多步骤部署
3. **版本管理**: Bundle 版本化管理，确保可复现的环境部署
4. **跨环境迁移**: 同一 Bundle 通过不同参数部署到 dev/staging/prod

## 安装

```bash
# 安装 Porter CLI
curl -L https://cdn.porter.sh/latest/install-mac.sh | bash
# 创建新 Bundle
porter create
# 构建并发布
porter build
porter publish
# 安装 Bundle
porter install myapp --reference ghcr.io/myorg/myapp:v0.1.0 \
  --param db_password=$DB_PASSWORD \
  --credential-set azure-prod
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Porter** | CNAB 标准、多工具集成 | 社区较小、学习曲线陡 |
| Helm | K8s 生态标准、简单 | 仅限 K8s 资源 |
| ArgoCD ApplicationSet | GitOps 多集群 | 仅 K8s 资源管理 |
| Terraform Modules | IaC 模块化 | 非 K8s 原生 |

## 架构定位

在 CNCF 生态中，Porter 属于 **Config / Application Packaging** 类别，是 CNAB 规范的参考实现。它将超越 Kubernetes 的应用打包标准化。

## 参考链接

- [[deployment]]

## Related

- [[helm]] — Helm
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
