---
title: Linux 节点安全
description: '# Linux 节点安全'
category: dictionary
tags:
- k8s
- glossary
- terminology
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Linux 节点安全 是什么
- 如何 Linux 节点安全
trigger_keywords:
- Linux
- 节点安全
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
created: "2026-05-23"
---

# Linux 节点安全

## 概述

本页面描述了针对 Linux 操作系统的安全考虑和最佳实践。Linux 节点在 [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] 集群中承担着运行容器工作负载的重要角色，某些内核和系统配置会直接影响 Secret 等敏感数据的保护效果。

## 核心概念/原理

在 Linux 节点上，基于内存的卷（例如 Secret 卷挂载，或设置了 `medium: Memory` 的 `emptyDir`）是通过 `tmpfs` 文件系统实现的。`tmpfs` 将数据存储在内存中，而不是持久化磁盘上，从而在正常情况下提供更好的保密性。

然而，如果节点启用了 **swap（交换分区/文件）**，并且使用的 Linux 内核版本较旧（或使用了不受支持的 Kubernetes 配置），则这些内存支持卷中的数据可能被写入到持久化的 swap 存储中，从而导致敏感数据泄露风险。

## 关键机制或特性

- **tmpfs 与 swap 的交互**：
  - `tmpfs` 的内容默认驻留在内存中。
  - 在旧内核或特定配置下，当系统内存压力较大时，`tmpfs` 页面可能被交换（swap）到磁盘。
- **noswap 选项**：
  - Linux 内核从 **6.3** 版本开始正式支持 `noswap` 挂载选项。
  - 启用 `noswap` 后，可以阻止 `tmpfs` 的内容被交换到磁盘，从而保护 Secret 等敏感内存数据。

## 使用场景

- 在启用了 swap 的 Linux 节点上运行 Kubernetes 工作负载。
- 对 Secret 数据保护有较高安全要求的集群环境。
- 需要评估和加固节点层面数据保密性的场景。

## 最佳实践/注意事项

- 如果 Linux 节点启用了 swap，建议将内核升级至 **6.3 或更高版本**（或通过 backport 获得 `noswap` 支持）。
- 阅读 Kubernetes 关于 swap 内存管理的官方文档，了解如何在集群中正确配置 swap 行为。
- 定期检查节点的 swap 配置和内核版本，确保敏感数据不会因 swap 机制而泄露到持久存储。
- 考虑在节点层面使用额外的加密措施（如全盘加密），以进一步降低数据泄露风险。

## 参考链接

- https://kubernetes.io/docs/concepts/security/linux-security/

## Related

- [[domain-17-system-foundation/topic-dictionary/security/admission-controller.md|准入控制器]]
- [[domain-17-system-foundation/topic-dictionary/security/application-security-checklist.md|应用安全清单]]
- [[domain-17-system-foundation/topic-dictionary/security/athenz.md|Athenz 身份认证与授权]]
