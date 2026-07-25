---
title: 安全上下文
description: SecurityContext 是 Kubernetes 中为 Pod 或容器定义安全配置的字段集合。它控制容器的权限、身份和安全行为，是实现容器安全加固的核心...
summary: SecurityContext 是 Kubernetes 中为 Pod 或容器定义安全配置的字段集合。它控制容器的权限、身份和安全行为，是实现容器安全加固的核心...
category: dictionary
tags:
- k8s
- glossary
- security
- security-context
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 安全上下文 是什么
- SecurityContext 详解
trigger_keywords:
- 安全上下文
- SecurityContext
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 安全上下文

> **英文名**: SecurityContext

## 概述

SecurityContext 是 Kubernetes 中为 Pod 或容器定义安全配置的字段集合。它控制容器的权限、身份和安全行为，是实现容器安全加固的核心机制。

## 核心概念/原理

### 关键配置

```yaml
securityContext:
  runAsNonRoot: true           # 禁止以 root 用户运行
  runAsUser: 1000              # 指定 UID
  runAsGroup: 3000             # 指定 GID
  fsGroup: 2000                # 挂载卷的文件组
  readOnlyRootFilesystem: true # 只读根文件系统
  allowPrivilegeEscalation: false # 禁止提权
  capabilities:
    drop: ["ALL"]              # 删除所有 Linux 能力
  seccompProfile:
    type: RuntimeDefault       # 使用默认 seccomp 配置
```

### Pod 级 vs 容器级

- **Pod SecurityContext**：应用于 Pod 中所有容器和卷。
- **Container SecurityContext**：仅应用于特定容器，可覆盖 Pod 级设置。

## 关键机制或特性

- `allowPrivilegeEscalation: false` 阻止 setuid/setgid 二进制提权。
- `capabilities.drop: ALL` 移除所有 Linux 能力，按需添加。
- `seccompProfile` 限制容器可以执行的系统调用。
- `AppArmor` / `SELinux` 提供额外的 MAC（强制访问控制）层。

## 使用场景与最佳实践

- 所有生产容器都应配置 SecurityContext。
- 始终设置 `runAsNonRoot: true` 和 `readOnlyRootFilesystem: true`。
- 使用 `capabilities.drop: ALL` 并根据需要添加最小能力。
- 配合 Pod Security Standards 的 Restricted 级别强制执行。

## 参考链接

- [SecurityContext - Official Documentation](https://kubernetes.io/docs/tasks/configure-pod-container/security-context/)

## Related

- [[domain-17-system-foundation/知识字典/security/rbac.md|Rbac]]
- [[domain-17-system-foundation/知识字典/security/role.md|Role]]
- [[domain-17-system-foundation/知识字典/security/clusterrole.md|Clusterrole]]
- [[domain-17-system-foundation/知识字典/security/rolebinding.md|Rolebinding]]
- [[domain-17-system-foundation/知识字典/security/clusterrolebinding.md|Clusterrolebinding]]


<!-- risk-assessed -->
