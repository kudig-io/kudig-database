---
title: 对象名称和 ID
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
- 对象名称和 ID 是什么
- 如何 对象名称和 ID
trigger_keywords:
- 对象名称和
- ID
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 对象名称和 ID

## 概述

集群中的每个对象都有一个在同类资源中唯一的名称（Name），以及一个在整个集群中唯一的 UID。名称用于在资源 URL 中引用对象，而 UID 用于区分集群生命周期内所有对象的历史实例。

## 核心概念/原理

### 名称（Name）

名称是由客户端提供的字符串，用于在资源 URL 中引用对象（如 `/api/v1/pods/some-name`）。

- 同一时间，同一类型的对象只能有一个给定的名称。
- 如果删除对象，可以创建一个同名的新对象。
- **名称必须在同一资源的所有 API 版本中唯一**。API 资源由 API 组、资源类型、命名空间（针对命名空间资源）和名称区分，API 版本在此上下文中无关紧要。

**自动生成名称**：当在创建请求中提供 `generateName` 而非 `name` 时，服务器会将提供的值作为前缀，并附加生成的后缀。自 [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] v1.31 起，服务器会最多尝试 8 次生成唯一名称，然后才返回 HTTP 409 冲突响应。

### 命名约束类型

1. **DNS 子域名名称**：最多 253 个字符，仅包含小写字母数字、'-' 或 '.'，以字母数字开头和结尾。
2. **RFC 1123 标签名称**：最多 63 个字符，仅包含小写字母数字或 '-'，以字母开头，以字母数字结尾。
3. **RFC 1035 标签名称**：与 RFC 1123 类似，当前实现要求以字母开头（启用 `RelaxedServiceNameValidation` 特性门控后，[[Service|Service]] 名称允许以数字开头）。
4. **路径段名称**：不能是 "." 或 ".."，不能包含 "/" 或 "%"。

### UID

UID 是 Kubernetes 系统生成的字符串，用于在整个集群生命周期内唯一标识对象。Kubernetes UID 是 UUID（通用唯一标识符），遵循 ISO/IEC 9834-8 和 ITU-T X.667 标准。

## 使用场景

- 通过名称在 `kubectl` 命令或 API 调用中引用特定对象。
- 通过 UID 追踪对象的历史实例，确保不会与已删除后重新创建的同名对象混淆。
- 在事件、日志或监控系统中唯一标识对象。

## 最佳实践/注意事项

- 为对象选择有意义的名称，便于识别和管理。
- 当对象代表物理实体（如 Node 代表物理主机）时，若主机以相同名称重建而未删除并重新创建 Node，Kubernetes 会将新主机视为旧主机，可能导致不一致。
- 不同资源类型可能有额外的名称限制，创建前应查阅对应资源的文档。

## 参考链接

- [Object Names and IDs - Official Documentation](https://kubernetes.io/docs/concepts/overview/working-with-objects/names/)

## Related

- [[domain-17-system-foundation/topic-dictionary/fundamentals/about-cgroup-v2.md|About cgroup v2（关于 cgroup v2）]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/annotations.md|注解]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/bpfman.md|bpfman eBPF 管理器]]


<!-- risk-assessed -->
