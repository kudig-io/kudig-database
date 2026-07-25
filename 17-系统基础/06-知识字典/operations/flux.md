---
title: Flux
description: Flux 是 CNCF 毕业项目，提供 Kubernetes 原生的 GitOps 持续交付能力。它通过自动化从 Git 仓库拉取配置并同步到集群，支持多租户、...
summary: Flux 是 CNCF 毕业项目，提供 Kubernetes 原生的 GitOps 持续交付能力。它通过自动化从 Git 仓库拉取配置并同步到集群，支持多租户、...
category: dictionary
tags:
- k8s
- glossary
- flux
- gitops
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Flux 是什么
- Flux 详解
trigger_keywords:
- Flux
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Flux

> **英文名**: Flux

## 概述

Flux 是 CNCF 毕业项目，提供 Kubernetes 原生的 GitOps 持续交付能力。它通过自动化从 Git 仓库拉取配置并同步到集群，支持多租户、多集群和 Helm 原生集成。

## 核心概念/原理

### Flux v2 架构

Flux v2 基于 Kubernetes Controller 模式，由多个专用控制器组成：

| 控制器 | 功能 |
|--------|------|
| Source Controller | 管理 Git/Helm/OCI 等外部源 |
| Kustomize Controller | 渲染和部署 Kustomize 资源 |
| Helm Controller | 管理 Helm Release 生命周期 |
| Notification Controller | 处理告警和 Provider 集成 |
| Image Automation | 自动更新镜像版本到 Git |

## 关键机制或特性

- **Source 抽象**：GitRepository、HelmRepository、OCIRepository、Bucket 等。
- **Kustomization**：声明式的 Kustomize 部署流水线。
- **HelmRelease**：声明式的 Helm 部署，支持 valuesFrom。
- **Image Update**：自动检测新镜像版本并提交 PR 到 Git。
- **多租户**：通过 RBAC 和 Namespace 隔离不同团队。

## 使用场景与最佳实践

- 作为 Argo CD 的替代方案，特别适合多租户场景。
- 使用 Image Automation 实现镜像版本的自动更新。
- 配合 Kustomize 管理多环境配置差异。
- 使用 Flux 的 Webhook 接收实现即时同步。
- 监控 Flux 控制器的 reconciliation 状态。

## 参考链接

- [Flux Official](https://fluxcd.io/)

## Related

- [[17-系统基础/06-知识字典/operations/argo.md|Argo]]
- [[17-系统基础/06-知识字典/operations/gitops.md|GitOps]]
- [[17-系统基础/06-知识字典/tooling/helm.md|Helm]]
- [[17-系统基础/06-知识字典/tooling/kustomize.md|Kustomize]]
- [[17-系统基础/06-知识字典/workloads/deployment.md|Deployment]]


<!-- risk-assessed -->
