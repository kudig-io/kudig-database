---
title: Limit Ranges（限制范围）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Limit Ranges（限制范围） 是什么
- 如何 Limit Ranges（限制范围）
trigger_keywords:
- Limit
- Ranges
- 限制范围
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---



# Limit Ranges（限制范围）

## 概述

LimitRange 是 [[Kubernetes|Kubernetes]] 中的一种策略对象，用于约束在命名空间中可为每种适用对象类型（如 Pod 或 PersistentVolumeClaim）指定的资源分配（limits 和 requests）。默认情况下，容器在集群中以无限制的_compute resources_运行，LimitRange 能够防止单个对象垄断命名空间内的所有可用资源。

## 核心概念/原理

- **命名空间级策略**：LimitRange 仅在单个命名空间内生效，当该命名空间中至少存在一个 LimitRange 对象时，Kubernetes 就会对资源分配进行约束。
- **Admission Controller 机制**：LimitRange 通过准入控制器在 Pod 准入阶段工作，而不是在运行时持续监控。
- **约束类型**：
  - 强制每个 Pod 或 Container 的最小和最大计算资源使用量（CPU、内存）。
  - 强制每个 PersistentVolumeClaim 的最小和最大存储请求。
  - 强制资源 request 与 limit 之间的比率。
  - 为命名空间设置默认的 request/limit，并在运行时自动注入到未显式声明资源需求的 Container 中。

## 关键机制或特性

1. **两阶段检查**：
   - **第一阶段**：为所有未设置计算资源需求的 Pod（及其容器）应用默认的 request 和 limit 值。
   - **第二阶段**：跟踪使用量，确保不超过任何 LimitRange 中定义的最小、最大和比率限制。
2. **违反约束**：若创建或更新对象时违反 LimitRange 约束，API 服务器将返回 HTTP `403 Forbidden` 并说明被违反的约束。
3. **仅影响准入阶段**：LimitRange 的验证仅在 Pod 准入阶段发生，对已运行的 Pod 不生效；新增或修改 LimitRange 不会影响已存在的 Pod。
4. **多 LimitRange 的不确定性**：若同一命名空间中存在两个或更多 LimitRange 对象，默认值的生效是不确定的。
5. **默认值一致性风险**：LimitRange 不会检查其应用默认值的一致性。例如，若 LimitRange 设置的默认 limit 小于客户端提交的 request，则最终 Pod 将无法调度（报 `Invalid value` 错误）。

## 使用场景

- **防止资源垄断**：在多用户共享的命名空间中，确保单个 Pod 或 PVC 不会占用过多资源。
- **自动注入默认值**：为开发团队提供“免配置”体验，自动为未声明资源需求的容器注入合理的 CPU/内存默认值。
- **存储范围控制**：限制 PVC 的存储请求在合理范围内，避免用户申请过大或过小的存储卷。

## 最佳实践/注意事项

- 若 LimitRange 适用于 `cpu` 和 `memory`，必须为 Pod 显式指定 requests 或 limits，否则系统可能拒绝 Pod 创建。
- 添加或修改 LimitRange 后，已存在的 Pod 不会受到影响，如有必要需手动重建。
- 尽量避免在同一命名空间中创建多个可能产生冲突默认值的 LimitRange。
- 设置默认值时，务必确保 `limit ≥ request`，否则 Pod 将因资源规格无效而无法调度。
- LimitRange 常与 ResourceQuota 配合使用：LimitRange 负责单个对象的资源范围约束，ResourceQuota 负责命名空间级别的总资源配额。

## 参考链接

- [Kubernetes 官方文档 - Limit Ranges](https://kubernetes.io/docs/concepts/policy/limit-range/)

## Related

- [[domain-17-system-foundation/topic-dictionary/security/admission-controller.md|准入控制器]]
- [[domain-17-system-foundation/topic-dictionary/security/application-security-checklist.md|应用安全清单]]
- [[domain-17-system-foundation/topic-dictionary/security/athenz.md|Athenz 身份认证与授权]]
