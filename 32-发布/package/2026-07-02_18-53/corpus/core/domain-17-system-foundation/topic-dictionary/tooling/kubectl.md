---
title: kubectl
description: kubectl 是 Kubernetes 的官方命令行工具，通过与 API Server 通信来管理集群资源。它是 Kubernetes
  用户和运维人员最常用的...
summary: kubectl 是 Kubernetes 的官方命令行工具，通过与 API Server 通信来管理集群资源。它是 Kubernetes 用户和运维人员最常用的...
category: dictionary
tags:
- k8s
- glossary
- kubectl
- tooling
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kubectl 是什么
- kubectl 详解
trigger_keywords:
- kubectl
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kubectl

> **英文名**: kubectl

## 概述

kubectl 是 Kubernetes 的官方命令行工具，通过与 API Server 通信来管理集群资源。它是 Kubernetes 用户和运维人员最常用的工具。

## 核心概念/原理

### 常用命令分类

| 类别 | 命令示例 |
|------|---------|
| 查看资源 | `kubectl get pods`, `kubectl describe pod <name>` |
| 创建/更新 | `kubectl apply -f`, `kubectl create` |
| 调试 | `kubectl logs`, `kubectl exec`, `kubectl port-forward` |
| 集群管理 | `kubectl drain`, `kubectl cordon`, `kubectl top` |
| 配置 | `kubectl config use-context`, `kubectl config set-cluster` |

### 高级功能

- **Dry-run**：`kubectl apply --dry-run=client/server` 预览变更。
- **Server-side Apply**：`kubectl apply --server-side` 使用服务端合并。
- **输出格式**：`-o json`, `-o yaml`, `-o jsonpath`, `-o custom-columns`。

## 关键机制或特性

- kubectl 通过 kubeconfig 文件连接集群。
- 支持插件机制（通过 PATH 中的 `kubectl-*` 二进制）。
- `kubectl explain` 查看资源的文档说明。
- `kubectl api-resources` 和 `kubectl api-versions` 查看可用 API。

## 使用场景与最佳实践

- 使用 `kubectl` 别名和自动补全提高效率。
- 生产环境操作前使用 `--dry-run=server` 验证。
- 使用 `kubectl auth can-i` 验证权限。
- 安装常用插件（krew 管理）：stern、kubens、kubectx。

## 参考链接

- [kubectl - Official Documentation](https://kubernetes.io/docs/reference/kubectl/)

## Related

- [[domain-17-system-foundation/知识字典/tooling/kubeadm.md|Kubeadm]]
- [[domain-17-system-foundation/知识字典/tooling/kubectx.md|Kubectx]]
- [[domain-17-system-foundation/知识字典/tooling/kubens.md|Kubens]]
- [[domain-17-system-foundation/知识字典/tooling/k9s.md|K9S]]
- [[domain-17-system-foundation/知识字典/tooling/stern.md|Stern]]


<!-- risk-assessed -->
