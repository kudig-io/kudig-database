---
title: Podman Desktop [entities]
description: '## 概述'
summary: 'Podman Desktop 是一个开源的桌面容器管理工具，为开发者提供图形化界面来管理容器、镜像、Pod 和 Kubernetes 集群。它支持 Podman、Docker 和 Kubernetes 等多种容器引擎，让开发者可以在本地无缝地开发、测试和调试容器化应用，并轻松迁移到 Kubernetes 环境。'
category: entities
tags:
- k8s
- cncf
- runtime
- podman-container-tools
- containerd
- docker
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Podman Desktop 是什么
- 如何 Podman Desktop
trigger_keywords:
- Podman
- Desktop
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Podman Desktop

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: TypeScript

## 概述

Podman Desktop 是由 Red Hat 开发的开源桌面容器管理工具，2022 年进入 CNCF Sandbox。它为 macOS、Windows 和 Linux 开发者提供统一的图形化界面来管理容器、镜像、Pod 和 Kubernetes 集群。支持 Podman（Red Hat 的 daemonless 容器引擎）、Docker 和 containerd 等多种容器运行时，帮助开发者从本地开发无缝过渡到 Kubernetes 生产环境。

Podman Desktop 的核心优势在于 **Rootless 模式**——容器以非特权用户运行，大幅提升开发环境的安全性。它集成了 Kind、Minikube、Developer Sandbox 等 Kubernetes 本地发行版，开发者可以一键创建/销毁本地 K8s 集群进行测试。

## Key Features

- **多引擎支持**：统一界面管理 Podman、Docker、Lima 等容器引擎
- **Rootless 容器**：基于 Podman 的 rootless 模式，无需 root 权限运行容器
- **本地 K8s 管理**：集成 Kind、Minikube、CRC（OpenShift Local）创建本地集群
- **Pod 管理**：创建和管理 Podman Pod，可导出为 Kubernetes YAML
- **镜像构建**：Containerfile/Dockerfile 构建支持，多架构镜像构建
- **扩展系统**：插件化架构，支持自定义功能和第三方集成

## Architecture

Podman Desktop 基于 **Electron + TypeScript + Svelte** 构建，前端通过 Podman/Docker API 与底层容器引擎通信。在 macOS/Windows 上通过 **Podman Machine**（基于 Lima/WSL2 的 Linux 虚拟机）运行 Linux 容器。扩展系统使用 VS Code Extension API 模式，允许第三方贡献 UI 和功能。

## K8s 集成

Podman Desktop 深度集成 Kubernetes 工作流：可以将 Podman Pod 一键导出为 Kubernetes Deployment YAML（`podman generate kube`），也可以直接连接远程 Kubernetes 集群管理资源。内置 Kind 集成允许开发者快速创建本地 K8s 集群，推送镜像到集群内部 Registry，并进行端到端测试。

## 生产部署要点

- **Rootless 优先**：使用 Podman 的 rootless 模式提高安全性
- **资源限制**：为 Podman Machine 配置合适的 CPU 和内存
- **镜像清理**：定期清理未使用的镜像和卷
- **本地 K8s**：使用 KIND 快速创建一次性测试集群
- **扩展生态**：探索扩展目录，增强开发体验

## 生产场景

1. **本地容器开发**：开发者构建、运行、调试容器化应用
2. **Kubernetes YAML 生成**：从 Podman Pod 生成 K8s YAML，用于部署到生产集群
3. **多集群测试**：使用 Kind 创建多个本地集群，测试多集群场景
4. **安全开发**：Rootless 模式确保开发环境不影响宿主系统

## 安装

```bash
# macOS
brew install --cask podman-desktop

# Windows (winget)
winget install RedHat.Podman-Desktop

# Linux (Flatpak)
flatpak install flathub io.podman_desktop.PodmanDesktop

# 安装 Podman 引擎
brew install podman
podman machine init
podman machine start
```

## 对比

| 特性 | Podman Desktop | Docker Desktop | Rancher Desktop |
|------|---------------|----------------|-----------------|
| 开源 | ✅ Apache 2.0 | ❌ 商业 | ✅ Apache 2.0 |
| Rootless | ✅ | ❌ | ⚠️ |
| Podman 引擎 | ✅ | ❌ | ✅ |
| 商业许可限制 | ❌ 无 | ✅ 大企业需付费 | ❌ 无 |

## 参考链接

- [[containerd]]
- [[deployment]]
- [[概念/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[devspace]] — DevSpace
- [[openfeature]] — OpenFeature
- [[docker]] — Docker
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[podman-desktop]] — Podman Desktop

- podman-container-tools
- [[实体/cncf-runtime.md|[[CNCF 容器运行时与工具链项目全景|CNCF 容器运行时与工具链项目全景]]]] — Cross-reference


<!-- risk-assessed -->
