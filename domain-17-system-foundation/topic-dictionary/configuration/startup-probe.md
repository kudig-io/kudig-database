---
title: 启动探针
description: Startup Probe（启动探针）用于检测容器是否已完成启动。在 Startup Probe 成功之前，Liveness 和 Readiness
  Probe...
summary: Startup Probe（启动探针）用于检测容器是否已完成启动。在 Startup Probe 成功之前，Liveness 和 Readiness
  Probe...
category: dictionary
tags:
- k8s
- glossary
- probe
- health-check
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 启动探针 是什么
- Startup Probe 详解
trigger_keywords:
- 启动探针
- Startup Probe
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 启动探针

> **英文名**: Startup Probe

## 概述

Startup Probe（启动探针）用于检测容器是否已完成启动。在 Startup Probe 成功之前，Liveness 和 Readiness Probe 会被禁用，防止慢启动应用被误杀。

## 核心概念/原理

### 核心价值

对于启动时间较长的应用（如 Java 应用加载 JVM、加载大量数据），Startup Probe 提供了一个"启动宽限期"：

```yaml
startupProbe:
  httpGet:
    path: /healthz
    port: 8080
  failureThreshold: 30
  periodSeconds: 10
  # 最多等待 30 × 10 = 300 秒启动
```

### 工作流程

1. 容器启动 → Startup Probe 开始探测。
2. Startup Probe 成功 → 启用 Liveness 和 Readiness Probe。
3. Startup Probe 失败（达到 failureThreshold）→ 容器被终止。

## 关键机制或特性

- Startup Probe 从 K8s v1.20 起达到 stable。
- `failureThreshold × periodSeconds` 定义了最大启动等待时间。
- 一旦 Startup Probe 成功一次，就不再执行。

## 使用场景与最佳实践

- 启动时间不确定的应用（Java、大型数据库）应配置 Startup Probe。
- 设置足够大的 `failureThreshold` 容纳最坏情况的启动时间。
- 避免将 Startup Probe 的检查逻辑设置得过于复杂。

## 参考链接

- [Startup Probe - Official Documentation](https://kubernetes.io/docs/tasks/configure-pod-container/configure-liveness-readiness-startup-probes/)

## Related

- [[domain-17-system-foundation/topic-dictionary/configuration/configmap.md|Configmap]]
- [[domain-17-system-foundation/topic-dictionary/security/secret.md|Secret]]
- [[domain-17-system-foundation/topic-dictionary/configuration/env.md|Env]]
- [[domain-17-system-foundation/topic-dictionary/configuration/configmaps.md|Configmaps]]
- [[domain-17-system-foundation/topic-dictionary/configuration/liveness-probe.md|Liveness Probe]]


<!-- risk-assessed -->
