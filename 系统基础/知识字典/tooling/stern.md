---
title: stern
description: stern 是一个多 Pod 日志聚合跟踪工具。它可以同时跟踪多个 Pod 的日志输出，并以不同颜色区分，非常适合调试微服务架构。...
summary: stern 是一个多 Pod 日志聚合跟踪工具。它可以同时跟踪多个 Pod 的日志输出，并以不同颜色区分，非常适合调试微服务架构。...
category: dictionary
tags:
- k8s
- glossary
- tooling
- logging
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- stern 是什么
- stern 详解
trigger_keywords:
- stern
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# stern

> **英文名**: stern

## 概述

stern 是一个多 Pod 日志聚合跟踪工具。它可以同时跟踪多个 Pod 的日志输出，并以不同颜色区分，非常适合调试微服务架构。

## 核心概念/原理

### 核心命令

```bash
# 跟踪匹配名称的所有 Pod 日志
stern my-app

# 跟踪特定命名空间
stern my-app -n production

# 使用正则匹配
stern "app-.*"

# 跟踪特定容器
stern my-app -c sidecar

# 显示时间戳
stern my-app --timestamps

# 跟踪最近的日志（类似 tail -n）
stern my-app --tail 100
```

## 关键机制或特性

- stern 使用正则表达式匹配 Pod 名称。
- 自动发现新创建的 Pod 并加入跟踪。
- 支持 `--output` 指定输出格式（default, raw, json）。
- stern 是原项目（wercker/stern）的社区维护分支（stern/stern）。

## 使用场景与最佳实践

- 调试微服务时同时跟踪多个 Pod 的日志。
- 使用 `--since` 限制日志时间范围。
- 配合 `grep` 过滤关键日志信息。
- 使用 `--template` 自定义日志格式。

## 参考链接

- [stern - Official Documentation](https://github.com/stern/stern)

## Related

- [[系统基础/知识字典/tooling/kubectl.md|Kubectl]]
- [[系统基础/知识字典/tooling/kubeadm.md|Kubeadm]]
- [[系统基础/知识字典/tooling/kubectx.md|Kubectx]]
- [[系统基础/知识字典/tooling/kubens.md|Kubens]]
- [[系统基础/知识字典/tooling/k9s.md|K9S]]


<!-- risk-assessed -->
