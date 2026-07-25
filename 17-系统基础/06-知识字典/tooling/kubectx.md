---
title: kubectx
description: kubectx 是一个用于快速切换 Kubernetes 集群上下文的命令行工具。当需要管理多个集群时，kubectx 可以显著简化集群切换操作。...
summary: kubectx 是一个用于快速切换 Kubernetes 集群上下文的命令行工具。当需要管理多个集群时，kubectx 可以显著简化集群切换操作。...
category: dictionary
tags:
- k8s
- glossary
- tooling
- kubectl
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kubectx 是什么
- kubectx 详解
trigger_keywords:
- kubectx
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kubectx

> **英文名**: kubectx

## 概述

kubectx 是一个用于快速切换 Kubernetes 集群上下文的命令行工具。当需要管理多个集群时，kubectx 可以显著简化集群切换操作。

## 核心概念/原理

### 核心命令

```bash
# 列出所有上下文
kubectx

# 切换到指定集群
kubectx <context-name>

# 切换到上一个集群
kubectx -

# 重命名上下文
kubectx <new-name>=<old-name>

# 删除上下文
kubectx -d <context-name>
```

### 工作原理

kubectx 操作 kubeconfig 文件（`~/.kube/config`），修改 `current-context` 字段来切换集群。

## 关键机制或特性

- 等价于 `kubectl config use-context`，但更简洁。
- 支持 fzf 模糊搜索（安装 fzf 后自动启用交互式选择）。
- 可与 kubens 配合使用，实现集群+命名空间的快速切换。

## 使用场景与最佳实践

- 多集群环境中必备工具。
- 使用有意义的上下文名称（如 `prod-us-east`, `staging-eu`）。
- 配合 kubens 使用提升效率。

## 参考链接

- [kubectx - Official Documentation](https://github.com/ahmetb/kubectx)

## Related

- [[17-系统基础/06-知识字典/tooling/kubectl.md|Kubectl]]
- [[17-系统基础/06-知识字典/tooling/kubeadm.md|Kubeadm]]
- [[17-系统基础/06-知识字典/tooling/kubens.md|Kubens]]
- [[17-系统基础/06-知识字典/tooling/k9s.md|K9S]]
- [[17-系统基础/06-知识字典/tooling/stern.md|Stern]]


<!-- risk-assessed -->
