---
title: Pod 安全准入
description: '# Pod 安全准入'
summary: '# Pod 安全准入'
category: dictionary
tags:
- k8s
- glossary
- terminology
- apiserver
- prometheus
- job
- webhook
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pod 安全准入 是什么
- 如何 Pod 安全准入
trigger_keywords:
- Pod
- 安全准入
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pod 安全准入

## 概述

[[Kubernetes|Kubernetes]] 提供了一个内置的 **Pod Security Admission** 准入控制器，用于强制执行 Pod 安全标准（Pod Security Standards）。该功能自 Kubernetes v1.25 起达到稳定（Stable）状态。Pod 安全限制在 Pod 创建时应用于命名空间级别。

## 核心概念/原理

Pod Security Admission 根据 Pod 安全标准定义的三个级别对 Pod 的安全上下文及相关字段提出要求：

- `privileged`（特权）
- `baseline`（基线）
- `restricted`（受限）

这些级别的详细要求在 Pod 安全标准页面中有详细定义。

## 关键机制或特性

### 命名空间标签配置

通过在命名空间上设置特定标签，可以定义每个命名空间要使用的 Pod 安全准入控制模式：

```
pod-security.kubernetes.io/<MODE>: <LEVEL>
pod-security.kubernetes.io/<MODE>-version: <VERSION>
```

其中：

- **MODE**：`enforce`（强制执行）、`audit`（审计）或 `warn`（警告）
- **LEVEL**：`privileged`、`baseline` 或 `restricted`
- **VERSION**：有效的 Kubernetes 次要版本（如 `v1.35`）或 `latest`

三种模式的行为如下：

- **enforce**：策略违规将导致 Pod 被拒绝创建。
- **audit**：策略违规会在审计日志中添加审计注解，但允许创建。
- **warn**：策略违规会向用户返回警告信息，但允许创建。

一个命名空间可以同时配置一种或多种模式，甚至为不同模式设置不同的级别。

### 工作负载资源与 Pod 模板

Pod 通常通过 Deployment、Job 等工作负载对象间接创建。为了尽早发现违规：

- `audit` 和 `warn` 模式会应用于工作负载资源本身（检查 Pod 模板）。
- `enforce` 模式**不会**应用于工作负载资源，仅应用于最终生成的 Pod 对象。

### 豁免（Exemptions）

可以在准入控制器配置中静态定义豁免规则。符合豁免条件的请求将被完全忽略（跳过所有 enforce、audit 和 warn 行为）。豁免维度包括：

- **Usernames**：来自特定认证（或模拟）用户名的请求。
- **RuntimeClassNames**：指定了 exempt RuntimeClass 的 Pod 和工作负载资源。
- **Namespaces**：位于 exempt 命名空间中的 Pod 和工作负载资源。

**注意**：大多数 Pod 由控制器根据工作负载资源创建，因此仅豁免终端用户只能豁免直接创建 Pod 的情况，而不能豁免通过工作负载资源创建 Pod 的情况。通常不应豁免控制器服务账号（如 `system:serviceaccount:kube-system:replicaset-controller`）。

### 更新豁免

对以下 Pod 字段的更新不受策略检查限制（即使 Pod 当前违反策略也不会被拒绝）：

- 除 seccomp 或 AppArmor 相关注解外的任何 metadata 更新
- `.spec.activeDeadlineSeconds` 的有效更新
- `.spec.tolerations` 的有效更新

### Metrics

kube-apiserver 暴露以下 Prometheus 指标：

- `pod_security_errors_total`：阻止正常评估的错误数量
- `pod_security_evaluations_total`：已发生的策略评估次数
- `pod_security_exemptions_total`：豁免请求的数量

## 使用场景

- 在集群中按命名空间级别强制执行统一的 Pod 安全配置。
- 逐步推进安全策略：先以 `warn` 和 `audit` 模式引入，再切换到 `enforce` 模式。
- 与第三方准入 Webhook 结合使用，实现更细粒度的安全控制。

## 最佳实践/注意事项

- 对于多租户集群，建议为不同租户命名空间配置不同的 Pod 安全级别。

- 在从 PodSecurityPolicy（PSP）迁移时，可以参考官方迁移指南，将策略映射到 Pod Security Admission 和/或第三方准入控制器。
- 谨慎配置豁免规则，避免过度放宽导致安全策略失效。
- 使用版本标签（`-version`）将策略固定在特定的 Kubernetes 版本，以避免集群升级后策略行为发生意外变化。

## 参考链接

- https://kubernetes.io/docs/concepts/security/pod-security-admission/

## Related

- [[生态参考/topic-index/security-index.md|Security 安全知识图谱索引]]


<!-- risk-assessed -->
