---
title: Helm
description: Helm 是 Kubernetes 的包管理器，被称为「K8s 的 apt/yum」。它通过 Chart（模板化的 YAML 集合）简化应用的打包、分发和部署，...
summary: Helm 是 Kubernetes 的包管理器，被称为「K8s 的 apt/yum」。它通过 Chart（模板化的 YAML 集合）简化应用的打包、分发和部署，...
category: dictionary
tags:
- k8s
- glossary
- helm
- package-manager
- gitops
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Helm 是什么
- Helm 详解
trigger_keywords:
- Helm
- dictionary
prerequisites:
- kubectl-basics
---



# Helm

> **英文名**: Helm

## 概述

Helm 是 Kubernetes 的包管理器，被称为「K8s 的 apt/yum」。它通过 Chart（模板化的 YAML 集合）简化应用的打包、分发和部署，是 Kubernetes 生态中最广泛使用的工具之一。

## 核心概念/原理

### 核心概念

- **Chart**：Helm 包，包含模板化的 K8s 资源定义。
- **Release**：Chart 的一次部署实例。
- **Repository**：Chart 仓库（如 Artifact Hub）。

### Chart 结构

```
mychart/
├── Chart.yaml       # 元数据
├── values.yaml      # 默认配置值
├── templates/       # Go 模板文件
│   ├── deployment.yaml
│   ├── service.yaml
│   └── _helpers.tpl
└── charts/          # 依赖 Chart
```

## 关键机制或特性

- **Helm v3**：移除了 Tiller 服务端组件，直接通过 kubeconfig 与 apiserver 交互。
- **Release 版本管理**：每次 `helm upgrade` 自动生成新版本，支持 `helm rollback`。
- **模板引擎**：基于 Go text/template，支持 values 注入和条件渲染。
- **Hook 机制**：在 install/upgrade/delete 前后执行 Job/Pod。
- **OCI Registry**：Helm v3.8+ 支持将 Chart 推送到 OCI 兼容的容器仓库。

## 使用场景与最佳实践

- 使用 `helm template` 本地渲染 Chart 检查生成的 YAML。
- 使用 `helm lint` 验证 Chart 语法和最佳实践。
- 生产环境推荐配合 Helmfile 或 ArgoCD 进行声明式管理。
- 避免在 Chart 中硬编码镜像版本，使用 values 注入。
- 使用 `helm diff` 插件预览 upgrade 变更。

## 参考链接

- [Helm Official Documentation](https://helm.sh/docs/)

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/kubectl.md|Kubectl]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kustomize.md|Kustomize]]
- [[domain-17-system-foundation/topic-dictionary/workloads/deployment.md|Deployment]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/manifest.md|Manifest]]
- [[domain-17-system-foundation/topic-dictionary/operations/rolling-update.md|Rolling Update]]
