---
title: Carvel (entities)
description: '## 概述'
summary: 'Carvel 是一组专注于 Kubernetes 应用构建、配置和部署的工具集。它采用 Unix 哲学，每个工具专注于单一任务并可组合使用。主要包括 ytt (YAML 模板)、kbld (镜像构建)、kapp (应用部署)、imgpkg (OCI 镜像打包)、vendir (依赖管理) 和 kapp-controller (GitOps)。'
category: entities
tags:
- k8s
- cncf
- ci-cd
- carvel
- crd
- operator
- kubeflow
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Carvel 是什么
- 如何 Carvel
trigger_keywords:
- Carvel
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Carvel

> **CNCF 状态**: Sandbox | **类别**: CI/CD | **主要语言**: Go

## 概述

Carvel（原名 k14s，Kubernetes Tools）是由 VMware（Pivotal 团队）开发的一组轻量级 Kubernetes 应用管理工具集，2020 年进入 CNCF Sandbox。它遵循 **Unix 哲学**——每个工具专注于一个任务，通过管道（pipe）组合使用。Carvel 的核心工具包括：**ytt**（YAML 模板引擎）、**kbld**（镜像构建和 digest 锁定）、**kapp**（声明式应用部署）、**imgpkg**（OCI 镜像打包）、**vendir**（依赖管理）和 **kapp-controller**（GitOps 控制器）。

与 Helm 这种"一体化"工具不同，Carvel 的每个工具都可以独立使用或与其他工具组合。例如，你可以只用 `ytt` 做模板渲染而不用 `kapp` 部署，或者只用 `kapp` 部署而不用 `ytt`。`kapp-controller` 将这些工具整合为 Kubernetes 原生的 GitOps 控制器，提供类似 ArgoCD/Flux 的持续部署能力，但更适合需要精细控制部署过程的场景。

## Key Features（核心工具）

- **ytt**：基于 Starlark（Python 方言）的 YAML 模板引擎，比 Helm 模板更安全、更可测试
- **kbld**：构建镜像并将 `image: latest` 标签解析/锁定为不可变的 `image@sha256:...`
- **kapp**：声明式部署工具，基于"应用（App）"概念管理一组 K8s 资源，支持精确的 diff 和回滚
- **imgpkg**：将应用配置和镜像打包为 OCI Bundle，实现可重定位分发
- **vendir**：声明式管理外部依赖（Git 仓库、Helm Chart、OCI 镜像），锁定版本
- **kapp-controller**：GitOps 控制器，在集群内协调 AppCRD，实现持续部署

## Architecture

Carvel 的工具链形成完整的 Kubernetes 应用生命周期管理闭环：**vendir** 拉取依赖 → **ytt** 渲染模板 → **kbld** 构建并锁定镜像 → **imgpkg** 打包为 Bundle → **kapp** 部署到集群。`kapp-controller` 在集群内自动化这个流程——通过 AppCRD 定义"从哪里拉取配置、如何渲染、部署到哪里"，控制器定期协调确保集群状态与 Git 仓库一致。

## K8s 集成

`kapp-controller` 是 Carvel 在 Kubernetes 中的核心运行时组件。它作为 Operator 运行，管理 `Package`、`PackageInstall` 和 `App` 三种 CRD。`Package` 定义 OCI Bundle 中的应用模板，`PackageInstall` 从 Package 创建实例，`App` 定义自由形式的 GitOps 部署。kapp-controller 内部调用 ytt/kbld/kapp/imgpkg 工具完成实际的渲染、构建和部署。

## 生产部署要点

- **模块化配置**：使用 ytt overlay 分离环境配置
- **镜像锁定**：使用 kbld 锁定镜像 digest
- **Bundle 打包**：使用 imgpkg 打包可重定位的应用
- **依赖管理**：使用 vendir 统一管理外部依赖
- **GitOps**：使用 kapp-controller 实现声明式部署
- **版本控制**：锁文件纳入版本控制

## 生产场景

1. **可移植应用分发**：将应用打包为 OCI Bundle，通过 imgpkg 分发到不同环境
2. **精细模板控制**：使用 ytt 的 Starlark 逻辑替代 Helm 模板的复杂条件
3. **安全镜像锁定**：用 kbld 将所有 `latest` 标签解析为 digest，确保不可变部署
4. **内嵌 GitOps**：kapp-controller 在集群内运行 GitOps，无需外部 CD 工具

## 安装

```bash
# 安装所有 Carvel 工具（一键脚本）
wget -O- https://carvel.dev/install.sh | bash

# 或单独安装工具
brew tap carvel-dev/carvel
brew install ytt kbld kapp imgpkg vendir kapp-controller

# 示例工作流
# 1. vendir 拉取依赖
vendir sync

# 2. ytt 渲染模板
ytt -f config/ -v namespace=production > rendered.yaml

# 3. kbld 构建并锁定镜像
kbld -f rendered.yaml --images-annotation=false > locked.yaml

# 4. kapp 部署
kapp deploy -a myapp -f locked.yaml

# 安装 kapp-controller 到集群
kapp deploy -a kc -f https://github.com/carvel-dev/kapp-controller/releases/latest/download/release.yml
```

## 对比

| 特性 | Carvel (ytt+kapp) | Helm | Kustomize | ArgoCD |
|------|-------------------|------|-----------|--------|
| 模板引擎 | ytt (Starlark) | Go template | Overlay（无逻辑） | - |
| 镜像锁定 | ✅ kbld | ⚠️ | ❌ | ❌ |
| Bundle/OCI | ✅ imgpkg | ✅ OCI | ❌ | ❌ |
| GitOps | ✅ kapp-controller | ❌ | ❌ | ✅ |

## 参考链接

- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/gitops-principles.md|gitops-principles]]
- [[概念/secrets-management.md|secrets-management]]

## Related

- [[kubeflow]] — Kubeflow
- [[spiffe]] — SPIFFE
- [[kubeelasti]] — [[实体/kubeelasti.md|KubeElastic]]
- [[xregistry]] — xRegistry
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- carvel
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
