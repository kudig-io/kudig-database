---
title: kpt (entities)
description: '## 概述'
summary: 'kpt 是一个以 Git 为中心的 Kubernetes 配置包管理工具，由 Google 开发。它使用 Git 分发 Kubernetes 资源包（package），通过函数 (KRM Functions) 实现配置的声明式转换、验证和修改，并提供 GitOps 风格的资源管理能力。'
category: entities
tags:
- k8s
- cncf
- config
- kpt
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
- kpt 是什么
- 如何 kpt
trigger_keywords:
- kpt
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# kpt

> **CNCF 状态**: Sandbox | **类别**: Config | **主要语言**: Go

## 概述

kpt 是一个以 Git 为中心的 Kubernetes 配置包管理工具，由 Google 开发，2020 年加入 CNCF 沙箱。它使用 Git 分发 Kubernetes 资源包（package），通过 KRM Functions（Kubernetes Resource Model Functions）实现配置的声明式转换、验证和修改，并提供 GitOps 风格的资源管理能力。与 Helm 不同，kpt 不使用模板，而是直接操作原生 YAML 配置，通过可编程的函数管道（Function Pipeline）实现配置的生成、修改和验证。kpt 的核心理念是将 Kubernetes 配置视为一等公民的代码资产，以 Git 作为 source of truth，使平台团队可以为开发者提供可复用、可组合的配置包。

## 核心能力

- **Git 作为 Source of Truth**: 以 Git 仓库作为配置包存储和分发中心
- **KRM Functions**: 可编程的配置转换函数（容器化），支持 set-namespace、apply-replacements 等
- **Function Pipeline**: 声明式函数管道，串联多个 mutator 和 validator
- **包依赖管理**: kpt pkg get 从 Git 仓库拉取配置包，支持版本锁定
- **ResourceGroup**: 声明式资源生命周期管理（kpt live），替代 kubectl apply
- **多包组合**: 一个包可以依赖其他包，实现配置复用

## 架构

kpt 的核心设计围绕配置包（Package）和函数管道（Pipeline）：

- **Package**: 包含 Kubernetes YAML 资源和 `Kptfile`（包元数据）的目录
- **Kptfile**: 包配置文件，定义 pipeline、upstream 依赖和包元信息
- **KRM Function**: 容器化的配置处理函数，输入/输出为 Kubernetes 资源流
- **Function Pipeline**: 在 Kptfile 中定义的有序函数列表，依次执行
- **ResourceGroup CRD**: 跟踪包中所有资源的期望状态，支持 prune 和 status 检查
- **kpt live**: 资源管理器，基于 ResourceGroup 实现声明式 apply/wait/prune

工作流：`kpt pkg get (拉取包) → kpt fn render (执行函数) → kpt live apply (部署)`

## K8s 集成

kpt 通过 ResourceGroup CRD 与 Kubernetes 集成。`kpt live apply` 将包中所有资源应用到集群，并创建 ResourceGroup 资源跟踪状态。`kpt live status` 持续检查资源状态直到所有资源 Ready。KRM Functions 以容器化方式运行，通过 CRI 执行。kpt 可与 ArgoCD 集成——ArgoCD 原生支持 kpt，在 sync 阶段自动执行 `kpt fn render`。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 GitOps 工作流深度集成。

## 生产场景

1. **平台配置标准化**: 将组织标准配置（namespace、network policy、RBAC）封装为 kpt 包分发
2. **多环境部署**: 使用函数管道实现同一配置包在不同环境中的参数化（namespace、replicas）
3. **GitOps 配置管理**: 配合 ArgoCD，使用 kpt 渲染 + GitOps 同步实现声明式部署
4. **配置合规验证**: 在 CI/CD 中运行 validator 函数，拦截不符合安全/合规标准的配置

## 安装

```bash
# 安装 kpt CLI
curl -s "https://raw.githubusercontent.com/GoogleContainerTools/kpt/main/scripts/install-install.sh" | bash
# 或使用 Homebrew
brew install kpt

# 拉取配置包
kpt pkg get https://github.com/examples/config-packages.git/wordpress wordpress

# 渲染函数管道
cd wordpress && kpt fn render

# 部署到集群
kpt live init
kpt live apply
```

## 对比

| 特性 | kpt | Helm | Kustomize | Carvel ytt |
|------|-----|------|-----------|------------|
| 配置方式 | 原生 YAML + 函数 | 模板渲染 | Overlay 叠加 | 模板渲染 |
| 函数可编程 | ✅ KRM Functions | ⚠️ 模板函数 | ❌ | ✅ Starlark |
| Git 原生 | ✅ | ⚠️ Chart Repo | ⚠️ | ⚠️ |
| 资源管理 | ✅ kpt live | ⚠️ helm install | ❌ 需 kubectl | ❌ 需 kubectl |

## 架构定位

在 CNCF 生态中，kpt 属于 **Config** 类别，为云原生应用提供以 Git 为中心的配置包管理能力。

## 参考链接

- [[deployment]]
- [[概念/gitops-principles.md|gitops-principles]]

## Related

- [[contour]] — Contour
- [[kured]] — Kured (KUbernetes REboot Daemon)
- [[opengemini]] — openGemini
- [[kmesh]] — Kmesh
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kpt
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
