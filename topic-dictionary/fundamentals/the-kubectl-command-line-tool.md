---
title: kubectl 命令行工具
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kubectl 命令行工具 是什么
- 如何 kubectl 命令行工具
trigger_keywords:
- kubectl
- 命令行工具
- dictionary
title_en: The Kubectl Command Line Tool
---


# kubectl 命令行工具

## 概述

`kubectl` 是与 Kubernetes 集群的控制平面进行通信的主要命令行工具。它通过 Kubernetes API 发送请求，是用户管理集群资源、检查集群状态和调试应用的主要接口。

## 核心概念/原理

### kubectl 的角色

`kubectl` 是创建、检查、更新和删除 Kubernetes 对象的主要界面。它与运行在集群内部的 Kubernetes 组件以及这些组件实现的 Kubernetes API 相辅相成。无论是在笔记本电脑上运行还是从集群内的 Pod 中运行，`kubectl` 都会向 API 服务器发送请求。

### kubectl 如何工作

`kubectl` 连接到 API 服务器并使用 kubeconfig 文件中定义的集群、用户和上下文进行认证：

- 从集群外部运行时，`kubectl` 使用 kubeconfig 文件查找 API 服务器地址和凭据。
- 从 Pod 内部运行时（如 CI/CD 流水线中），`kubectl` 可以基于挂载到 Pod 中的 ServiceAccount 令牌使用集群内认证（in-cluster authentication）。

执行命令时，`kubectl` 将用户意图转换为一个或多个发往 Kubernetes API 的 HTTP 请求。API 服务器验证每个请求，将其应用到存储在 etcd 中的集群状态，并返回结果。

### kubeconfig 配置

`kubectl` 默认查找 `$HOME/.kube/config` 文件。可以通过设置 `KUBECONFIG` 环境变量或使用 `--kubeconfig` 标志指定其他 kubeconfig 文件。一个 kubeconfig 可以定义多个集群、用户和上下文，使用 `kubectl config use-context` 可以切换活动上下文。

## 关键机制或特性

`kubectl` 支持的操作大致分为以下几类：

- **管理资源**：创建、更新和删除 Pod、Deployment、Service 等对象。推荐使用 `kubectl apply` 进行声明式管理。
- **检查集群状态**：列出和描述对象、查看事件、检查资源使用情况。
- **调试**：查看容器日志、在运行中的容器内执行命令、端口转发到 Pod。
- **集群操作**：排空节点进行维护、封锁节点以防止新工作负载调度、管理集群配置。
- **脚本和自动化**：使用 JSON、YAML 或 JSONPath 自定义列格式化输出，便于在脚本和流水线中使用。

### 声明式 vs 命令式

- **声明式管理**：生产工作负载的首选方式，使用 `kubectl apply` 配合版本控制的配置文件。有助于追踪变更、协作和集成 GitOps 工作流。
- **命令式管理**：如 `kubectl create` 或 `kubectl run`，适用于开发和实验，但难以复现和审计。

### 插件扩展

`kubectl` 可以通过插件扩展新的子命令。插件是遵循 `kubectl-<plugin-name>` 命名约定的独立二进制文件。Krew 是 Kubernetes 社区维护的插件管理器。

### 版本兼容性

`kubectl` 支持与集群控制平面相差一个次要版本的版本偏差（plus-or-minus one minor version）。例如，`kubectl` v1.32 可以与 v1.31、v1.32 和 v1.33 的控制平面配合使用。

## 使用场景

- 日常运维：部署应用、查看 Pod 状态、查看日志。
- 故障排查：进入容器执行命令、端口转发、查看事件。
- 集群维护：节点排水（drain）、封锁（cordon）、污点（taint）管理。
- 自动化脚本：将 `kubectl` 集成到 CI/CD 流水线中。

## 最佳实践/注意事项

- 生产环境优先使用声明式管理（`kubectl apply -f`）和 GitOps 工作流。
- 保持 `kubectl` 版本与集群控制平面版本兼容，避免意外行为。
- 利用 `kubectl` 插件生态（通过 Krew）扩展功能，提高效率。

## 参考链接

- [The kubectl command-line tool - Official Documentation](https://kubernetes.io/docs/concepts/overview/kubectl/)
