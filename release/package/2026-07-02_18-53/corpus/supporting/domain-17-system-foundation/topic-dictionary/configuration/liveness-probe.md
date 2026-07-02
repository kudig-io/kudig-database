---
title: 存活探针
description: Liveness Probe（存活探针）用于检测容器是否处于运行状态。如果探测失败，kubelet 会终止容器并根据 restartPolicy
  决定是否重启。...
summary: Liveness Probe（存活探针）用于检测容器是否处于运行状态。如果探测失败，kubelet 会终止容器并根据 restartPolicy
  决定是否重启。...
category: dictionary
tags:
- k8s
- glossary
- probe
- health-check
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 存活探针 是什么
- Liveness Probe 详解
trigger_keywords:
- 存活探针
- Liveness Probe
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 存活探针

> **英文名**: Liveness Probe

## 概述

Liveness Probe（存活探针）用于检测容器是否处于运行状态。如果探测失败，kubelet 会终止容器并根据 restartPolicy 决定是否重启。

## 核心概念/原理

### 探测方式

- **HTTP GET**：发送 HTTP 请求，200-399 为成功。
- **TCP Socket**：尝试建立 TCP 连接。
- **Exec**：在容器内执行命令，退出码 0 为成功。
- **gRPC**：执行 gRPC Health Check（v1.24+ stable）。

### 配置参数

```yaml
livenessProbe:
  httpGet:
    path: /healthz
    port: 8080
  initialDelaySeconds: 15
  periodSeconds: 10
  timeoutSeconds: 5
  failureThreshold: 3
  successThreshold: 1
```

## 关键机制或特性

- `initialDelaySeconds`：容器启动后等待多久开始探测。
- `failureThreshold`：连续失败多少次才判定为失败。
- Liveness Probe 失败会导致容器重启。
- 不要将依赖外部服务的检查放入 Liveness Probe（会导致级联重启）。

## 使用场景与最佳实践

- Liveness Probe 应只检查容器自身的健康状态。
- 设置合理的 `initialDelaySeconds` 避免启动期间误杀。
- 避免在 Liveness Probe 中执行重量级检查。
- 对于慢启动应用，使用 Startup Probe 替代。

## 参考链接

- [Liveness Probe - Official Documentation](https://kubernetes.io/docs/tasks/configure-pod-container/configure-liveness-readiness-startup-probes/)

## Related

[[domain-17-system-foundation/topic-dictionary/configuration/liveness-readiness-and-startup-probes.md|Probes]]


<!-- risk-assessed -->
