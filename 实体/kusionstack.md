---
title: KusionStack (entities)
description: '## 概述'
summary: 'KusionStack 是一个云原生可编程技术栈，提供以应用为中心的配置管理和交付能力。'
category: entities
tags:
- k8s
- cncf
- platform
- kusionstack
- containerd
- crd
- operator
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KusionStack 是什么
- 如何 KusionStack
trigger_keywords:
- KusionStack
prerequisites:
- kubectl-basics
- iac-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KusionStack

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Go, KCL

## 概述

KusionStack 是一个云原生可编程技术栈，由蚂蚁集团（Ant Group）开源，2023 年加入 CNCF 沙箱。它提供以应用为中心的配置管理和交付能力，使用 KCL（Kusion Configuration Language）作为配置语言，结合 Kusion 引擎实现从应用配置到多云/多环境的一致性交付。KusionStack 支持 Kubernetes、Terraform 等多种 IaC 后端，让平台团队可以为开发者提供简化的自助式应用交付体验。其核心理念是"配置即代码"（Configuration as Code），通过 KCL 的类型系统和约束验证，在配置编写阶段捕获错误，而不是等到部署时才发现。

## 核心能力

- **KCL 配置语言**: 基于约束的记录与函数式配置语言，支持类型系统、schema 约束和配置合并
- **多云/多后端**: 支持 Kubernetes、Terraform、AWS、阿里云等多种基础设施后端
- **应用为中心**: 以 App 为单位组织配置，屏蔽底层基础设施复杂性
- **Konfig 仓库**: 可复用的配置模块仓库，支持团队间配置共享
- **Preview 审查**: apply 前自动 diff 变更，可视化展示影响范围
- **CI/CD 集成**: 与 ArgoCD/Flux 等 GitOps 工具无缝集成

## 架构

KusionStack 围绕 KCL 语言和 Kusion 引擎构建：

- **KCL 编译器**: Rust 实现的 KCL 语言编译器，解析、类型检查并渲染配置
- **Konfig 仓库**: 按项目（Project）、栈（Stack）组织的配置模块层次结构
- **Kusion 引擎**: 执行配置渲染、状态管理和资源编排
- **State Backend**: 存储资源配置状态（支持本地/远程 state），支持 diff 和收敛
- **Executor**: 通过 Kubernetes API、Terraform Provider 或云 SDK 执行实际资源操作
- **KCL OCI Registry**: 将配置模块打包为 OCI 制品，版本化分发

交付流程：`KCL 配置 → 编译 → 资源 Spec → Preview (diff) → Apply → K8s/Cloud`

## K8s 集成

KusionStack 通过 Kusion 引擎与 Kubernetes 集成。KCL 配置编译后生成标准 Kubernetes 资源 YAML，通过 Kubernetes API 直接 apply。Kusion 引擎管理资源状态，支持三向 diff（配置 vs State vs 集群）。通过 `kusion preview` 可以在应用前检查变更，`kusion apply` 执行实际部署。KusionStack 支持与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 ArgoCD 集成——KCL 渲染输出可以作为 ArgoCD Application 的 source。

## 生产场景

1. **企业平台工程**: 平台团队用 KCL 定义标准化应用模板，开发者填写参数即可部署
2. **多云统一交付**: 同一 KCL 配置同时管理 Kubernetes 资源和云基础设施（RDS、SLB）
3. **多环境管理**: dev/staging/prod 共享基础配置，通过 KCL overlay 实现差异化
4. **配置合规**: 利用 KCL schema 约束在编写阶段拦截不安全/不合规的配置

## 安装

```bash
# 安装 Kusion CLI
curl -fsSL https://www.kusionstack.io/scripts/install.sh | bash
# 或使用 Homebrew
brew install KusionStack/tap/kusion

# 安装 KCL CLI
brew install KusionStack/tap/kcl

# 初始化项目
kusion init

# 编译配置
kcl ci-test

# 预览变更
kusion preview

# 部署
kusion apply
```

## 对比

| 特性 | KusionStack | Crossplane | Terraform CDK | Pulumi |
|------|-------------|------------|---------------|--------|
| 配置语言 | KCL | YAML/CRD | TS/Python | TS/Go/Python |
| K8s 原生 | ✅ | ✅ | ❌ | ❌ |
| 类型约束 | ✅ Schema | ❌ | ⚠️ | ✅ |
| 多后端 | ✅ | ✅ | ✅ | ✅ |

## 架构定位

在 CNCF 生态中，KusionStack 属于 **Platform** 类别，为云原生应用提供可编程配置和交付能力。

## 参考链接

- [[crossplane]]
- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/gitops-principles.md|gitops-principles]]
- [[概念/storage-model.md|storage-model]]

## Related

- [[06-containerd-observability]] — containerd 可观测性
- [[stacker]] — Stacker
- [[opentelemetry]] — OpenTelemetry
- [[kcl]] — KCL (Kusion Configuration Language)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kusionstack
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
