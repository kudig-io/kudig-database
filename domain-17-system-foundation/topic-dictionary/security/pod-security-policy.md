---
title: Pod 安全策略
description: Pod Security Policy（PSP）是 Kubernetes 早期用于控制 Pod 安全配置的集群级资源。**PSP 在 K8s
  v1.21 中被弃...
summary: Pod Security Policy（PSP）是 Kubernetes 早期用于控制 Pod 安全配置的集群级资源。**PSP 在 K8s v1.21
  中被弃...
category: dictionary
tags:
- k8s
- glossary
- security
- psp
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pod 安全策略 是什么
- Pod Security Policy (PSP) 详解
trigger_keywords:
- Pod 安全策略
- Pod Security Policy (PSP)
- dictionary
prerequisites:
- kubectl-basics
---



# Pod 安全策略

> **英文名**: Pod Security Policy (PSP)

## 概述

Pod Security Policy（PSP）是 Kubernetes 早期用于控制 Pod 安全配置的集群级资源。**PSP 在 K8s v1.21 中被弃用，v1.25 中被移除**，已被 Pod Security Standards（PSS）+ Pod Security Admission 替代。

## 核心概念/原理

### PSP 的历史

- **K8s v1.3-v1.20**：PSP 是控制 Pod 安全的主要机制。
- **K8s v1.21**：PSP 被标记为弃用（deprecated）。
- **K8s v1.25**：PSP 被完全移除。

### PSP 的功能（已弃用）

PSP 可以控制：
- 特权容器（privileged）
- 宿主机命名空间（hostNetwork, hostPID, hostIPC）
- 宿主机端口范围
- 卷类型
- 文件系统组
- 用户/组范围
- 允许的能力（capabilities）
- SELinux 上下文

## 关键机制或特性

- PSP 是集群级资源，通过 RBAC 控制谁可以使用哪些 PSP。
- PSP 的复杂性导致难以正确配置，是弃用的主要原因之一。
- 替代方案 PSS（Pod Security Standards）通过命名空间标签实施，更简洁。

## 使用场景与最佳实践

- 如果集群仍在使用 PSP（K8s < v1.25），应计划迁移到 PSS。
- 迁移步骤：1) 审计现有 PSP 规则 → 2) 映射到 PSS 级别 → 3) 在命名空间上应用 PSS 标签 → 4) 验证 → 5) 删除 PSP。
- 新集群直接使用 Pod Security Admission。

## 参考链接

- [Pod Security Policy (PSP) - Official Documentation](https://kubernetes.io/docs/concepts/security/pod-security-standards/)

## Related

[[domain-17-system-foundation/topic-dictionary/security/pod-security-standards.md|Pod Security Standards]]
