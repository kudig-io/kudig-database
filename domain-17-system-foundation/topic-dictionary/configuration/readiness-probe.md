---
title: 就绪探针
description: Readiness Probe（就绪探针）用于检测容器是否准备好接受流量。如果探测失败，Pod 会被从 Service 的 Endpoints
  中移除，不再接收...
summary: Readiness Probe（就绪探针）用于检测容器是否准备好接受流量。如果探测失败，Pod 会被从 Service 的 Endpoints 中移除，不再接收...
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
- 就绪探针 是什么
- Readiness Probe 详解
trigger_keywords:
- 就绪探针
- Readiness Probe
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 就绪探针

> **英文名**: Readiness Probe

## 概述

Readiness Probe（就绪探针）用于检测容器是否准备好接受流量。如果探测失败，Pod 会被从 Service 的 Endpoints 中移除，不再接收请求。

## 核心概念/原理

### 与 Liveness Probe 的区别

| 特性 | Liveness Probe | Readiness Probe |
|------|---------------|-----------------|
| 失败行为 | 重启容器 | 从 Service 移除 |
| 用途 | 检测死锁/卡住 | 检测是否就绪 |
| successThreshold | 始终为 1 | 可配置 |

### 典型场景

- 应用启动时需要加载大量数据。
- 依赖的外部服务暂时不可用。
- 应用需要预热缓存后才能接受流量。

## 关键机制或特性

- Readiness Probe 失败不会重启容器，只是暂停接收流量。
- `successThreshold` 默认为 1，可以设置更大的值确保稳定后再接受流量。
- Pod 中的所有 Readiness Probe 都成功后，Pod 才被视为 Ready。

## 使用场景与最佳实践

- 为所有面向流量的服务配置 Readiness Probe。
- Readiness Probe 的检查路径应反映应用的真实就绪状态。
- 避免 Readiness Probe 和 Liveness Probe 使用相同的检查逻辑。
- 设置合理的 `periodSeconds` 平衡检测灵敏度和开销。

## 参考链接

- [Readiness Probe - Official Documentation](https://kubernetes.io/docs/tasks/configure-pod-container/configure-liveness-readiness-startup-probes/)

## Related

- [[domain-17-system-foundation/topic-dictionary/configuration/configmap.md|Configmap]]
- [[domain-17-system-foundation/topic-dictionary/security/secret.md|Secret]]
- [[domain-17-system-foundation/topic-dictionary/configuration/env.md|Env]]
- [[domain-17-system-foundation/topic-dictionary/configuration/configmaps.md|Configmaps]]
- [[domain-17-system-foundation/topic-dictionary/configuration/liveness-probe.md|Liveness Probe]]


<!-- risk-assessed -->
