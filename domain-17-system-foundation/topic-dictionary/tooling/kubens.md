---
title: kubens
description: 'kubens 是一个用于快速切换 Kubernetes 命名空间的命令行工具。它简化了在多个命名空间之间切换的操作。...'
category: dictionary
tags:
- k8s
- glossary
- tooling
- kubectl
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kubens 是什么
- kubens 详解
trigger_keywords:
- kubens
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# kubens

> **英文名**: kubens

## 概述

kubens 是一个用于快速切换 Kubernetes 命名空间的命令行工具。它简化了在多个命名空间之间切换的操作。

## 核心概念/原理

### 核心命令

```bash
# 列出所有命名空间
kubens

# 切换到指定命名空间
kubens <namespace>

# 切换到上一个命名空间
kubens -
```

### 工作原理

kubens 修改 kubeconfig 中当前上下文的 `namespace` 字段。

## 关键机制或特性

- 等价于 `kubectl config set-context --current --namespace=<ns>`。
- 支持 fzf 模糊搜索（交互式选择）。
- 与 kubectx 属于同一项目（kubectx/kubens）。

## 使用场景与最佳实践

- 在频繁切换命名空间的场景中非常有用。
- 配合 kubectx 使用，实现集群+命名空间的快速切换。

## 参考链接

- [kubens - Official Documentation](https://github.com/ahmetb/kubectx)

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/kubectl.md|Kubectl]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kubeadm.md|Kubeadm]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kubectx.md|Kubectx]]
- [[domain-17-system-foundation/topic-dictionary/tooling/k9s.md|K9S]]
- [[domain-17-system-foundation/topic-dictionary/tooling/stern.md|Stern]]
