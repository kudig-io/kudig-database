---
title: Cloud Native Buildpacks
description: '## 概述'
summary: 'Cloud Native Buildpacks (CNB) 将应用源代码转换为 OCI 容器镜像，无需编写 Dockerfile。'
category: entities
tags:
- k8s
- cncf
- ci-cd
- buildpacks
- docker
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cloud Native Buildpacks 是什么
- 如何 Cloud Native Buildpacks
trigger_keywords:
- Cloud
- Native
- Buildpacks
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Cloud Native [[Buildpacks|Buildpacks]]

> **CNCF 状态**: Incubating | **类别**: CI/CD | **主要语言**: Go

## 概述

Cloud Native Buildpacks（CNB）是将应用源代码转换为 OCI 容器镜像的开源框架，由 Heroku 和 Pivotal（现 VMware）联合开发，2018 年加入 CNCF 孵化。它无需编写 Dockerfile，自动检测应用类型（Java、Node.js、Python、Go 等）、安装依赖、配置运行环境，简化了容器化流程并提高镜像安全性。Buildpacks 的核心理念源于 Heroku 和 Cloud Foundry 的 Buildpack 概念——将应用构建专业知识编码为可复用的 Buildpack 模块。CNB 还支持 Rebasing（无需重新构建即可更新基础镜像的 OS 层）、自动 SBOM 生成和分层缓存等高级特性。它是 Shipwright 和 Tekton 等构建框架的重要策略后端。

## 核心能力

- **自动检测**: 智能识别应用语言和框架（通过 detect 阶段的 buildplan）
- **模块化**: Buildpack 可组合、可复用，多个 Buildpack 组成 Builder
- **Rebasing**: 无需重新构建即可更新基础镜像（OS 层），秒级完成安全补丁
- **SBOM 生成**: 自动生成 Software Bill of Materials（软件物料清单）
- **缓存优化**: 分层缓存（layers）加速重复构建
- **多平台**: 支持 AMD64、ARM64 等多架构构建

## 架构

Cloud Native Buildpacks 采用 Builder + Lifecycle 架构：

- **Builder**: 包含 Buildpacks 集合和基础镜像（Base Image）的可执行镜像
- **Buildpack**: 可执行模块，包含 detect（检测）和 build（构建）两个脚本
- **Lifecycle**: 构建执行引擎（detector → builder → exporter）
- **App Image**: 最终输出的 OCI 容器镜像（包含应用代码 + 运行时层）
- **Project Descriptor (project.toml)**: 项目级构建配置（指定 Builder 等）
- **pack CLI**: 开发者使用的命令行工具（类似 docker build）

构建流程：`pack build → Lifecycle (detect → build) → App Image → Registry`

## K8s 集成

Cloud Native Buildpacks 在 Kubernetes 中通过 Shipwright 框架使用。Shipwright 的 Buildpacks BuildStrategy 将 Build CRD 转化为 Tekton TaskRun，在集群内的 Pod 中执行 `pack build`。也可以在 Tekton Pipeline 中直接使用 `pack` CLI Task。构建结果推送到集群内的 Harbor 或外部 Registry。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的标准 CRD/Controller 和 Tekton 生态集成。

## 生产场景

1. **无 Dockerfile 构建**: 开发者提交源码，Buildpacks 自动构建标准化镜像
2. **多语言项目**: 不同语言（Java/Python/Go/Node.js）统一使用 Buildpacks 构建流程
3. **安全补丁加速**: 通过 Rebase 秒级更新所有镜像的 OS 基础层，无需重新构建
4. **SBOM 合规**: 自动生成 SBOM，满足供应链安全合规要求

## 安装

```bash
# 安装 pack CLI
brew install buildpacks/tap/pack
# 或
curl -fsSL https://github.com/buildpacks/pack/releases/latest/download/pack-$(uname -s)-$(uname -m) -o /usr/local/bin/pack
chmod +x /usr/local/bin/pack

# 从源码构建镜像（无需 Dockerfile）
cd my-nodejs-app
pack build my-registry.io/myorg/app:latest --builder paketobuildpacks/builder:base

# 选择 Builder（tiny = 最小镜像，base = 标准，full = 完整）
pack build my-registry.io/myorg/app:latest --builder paketobuildpacks/builder:tiny

# Rebase 更新基础镜像（秒级完成）
pack rebase my-registry.io/myorg/app:latest --run-image paketobuildpacks/run:tiny

# 查看 SBOM
pack inspect my-registry.io/myorg/app:latest --bom

# 在 Tekton 中使用
kubectl apply -f https://raw.githubusercontent.com/tektoncd/catalog/main/task/buildpacks/0.4/buildpacks.yaml
```

## 对比

| 特性 | Buildpacks | Dockerfile | ko | Jib |
|------|-----------|-----------|-----|-----|
| 无 Dockerfile | ✅ | ❌ | ✅ | ✅ |
| 自动检测 | ✅ | ❌ | ❌ | ❌ |
| Rebase | ✅ 秒级 | ❌ | ❌ | ❌ |
| SBOM | ✅ | ❌ | ✅ | ❌ |

## 架构定位

在 CNCF 生态中，Buildpacks 属于 **CI/CD** 类别，为云原生应用提供无 Dockerfile 的标准化镜像构建能力。

## 参考链接

- [[概念/secrets-management.md|secrets-management]]
- [[概念/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[kepler]] — Kepler
- [[kubestellar]] — KubeStellar
- [[athenz]] — Athenz
- [[metallb]] — MetalLB
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- buildpacks
- [[实体/cncf-runtime.md|[[CNCF 容器运行时与工具链项目全景|CNCF 容器运行时与工具链项目全景]]]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
