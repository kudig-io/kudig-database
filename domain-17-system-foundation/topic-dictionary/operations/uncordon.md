---
title: 解封节点
description: 'Uncordon 是恢复节点可调度状态的操作。解封后，节点可以重新接受 Pod 调度。...'
category: dictionary
tags:
- k8s
- glossary
- operations
- node
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 解封节点 是什么
- Uncordon 详解
trigger_keywords:
- 解封节点
- Uncordon
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# 解封节点

> **英文名**: Uncordon

## 概述

Uncordon 是恢复节点可调度状态的操作。解封后，节点可以重新接受 Pod 调度。

## 核心概念/原理

### 命令

```bash
# 解封节点
kubectl uncordon <node-name>
```

### 使用场景

- 节点维护完成后恢复使用。
- 节点问题修复后重新加入调度。

## 关键机制或特性

- Uncordon 只是恢复调度能力，不会主动将 Pod 调度到该节点。
- 解封后调度器会根据集群状态自然地将 Pod 调度到该节点。

## 使用场景与最佳实践

- 维护完成后确认节点健康状态再执行 Uncordon。
- 大量节点维护后分批 Uncordon，避免 Pod 大量迁移导致集群不稳定。

## 参考链接

- [Uncordon - Official Documentation](https://kubernetes.io/docs/reference/generated/kubectl/kubectl-commands#uncordon)

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/kubectl.md|Kubectl]]
- [[domain-17-system-foundation/topic-dictionary/tooling/helm.md|Helm]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kustomize.md|Kustomize]]
- [[domain-17-system-foundation/topic-dictionary/operations/drain.md|Drain]]
- [[domain-17-system-foundation/topic-dictionary/operations/scale.md|Scale]]
