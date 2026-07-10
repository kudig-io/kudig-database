---
title: 优雅关闭
description: Graceful Shutdown（优雅关闭）是 Kubernetes 在终止 Pod 时允许容器执行清理操作的机制。它确保应用在退出前有机会完成进行中的请求、...
summary: Graceful Shutdown（优雅关闭）是 Kubernetes 在终止 Pod 时允许容器执行清理操作的机制。它确保应用在退出前有机会完成进行中的请求、...
category: dictionary
tags:
- k8s
- glossary
- configuration
- graceful-shutdown
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 优雅关闭 是什么
- Graceful Shutdown 详解
trigger_keywords:
- 优雅关闭
- Graceful Shutdown
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 优雅关闭

> **英文名**: Graceful Shutdown

## 概述

Graceful Shutdown（优雅关闭）是 Kubernetes 在终止 Pod 时允许容器执行清理操作的机制。它确保应用在退出前有机会完成进行中的请求、关闭连接和释放资源。

## 核心概念/原理

### 关闭流程

```
1. Pod 被标记删除
2. Pod 状态变为 Terminating
3. API Server 发送 SIGTERM 信号给容器主进程
4. 等待 terminationGracePeriodSeconds（默认 30 秒）
5. 超时后发送 SIGKILL 强制终止
6. Pod 被删除
```

### 关键配置

```yaml
spec:
  terminationGracePeriodSeconds: 60  # 给予 60 秒的清理时间
  containers:
  - name: app
    lifecycle:
      preStop:
        exec:
          command: ["/bin/sh", "-c", "sleep 10"]  # 等待负载均衡器更新
```

## 关键机制或特性

- `preStop` hook 在 SIGTERM 之前执行，可用于延迟和通知。
- `terminationGracePeriodSeconds` 包括 preStop + 应用清理的总时间。
- kube-proxy 需要同步时间更新 iptables/IPVS 规则，`preStop` sleep 可以避免流量丢失。

## 使用场景与最佳实践

- 所有生产服务都应实现优雅关闭。
- 应用应监听 SIGTERM 信号并执行清理逻辑。
- 设置 `preStop: sleep 5-10` 避免负载均衡器未及时更新导致的请求失败。
- `terminationGracePeriodSeconds` 应大于预期的清理时间。

## 参考链接

- [Graceful Shutdown - Official Documentation](https://kubernetes.io/docs/concepts/workloads/pods/pod-lifecycle/#pod-termination)

## Related

- [[domain-17-system-foundation/知识字典/configuration/configmap.md|Configmap]]
- [[domain-17-system-foundation/知识字典/security/secret.md|Secret]]
- [[domain-17-system-foundation/知识字典/configuration/env.md|Env]]
- [[domain-17-system-foundation/知识字典/configuration/configmaps.md|Configmaps]]
- [[domain-17-system-foundation/知识字典/configuration/probe.md|Probe]]


<!-- risk-assessed -->
