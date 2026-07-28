---
title: 命名空间
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- webhook
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 命名空间 是什么
- 如何 命名空间
trigger_keywords:
- 命名空间
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 命名空间

## 概述

在 [[kubernetes|Kubernetes]] 中，命名空间（Namespaces）提供了一种在单个集群内隔离资源组的机制。资源名称需要在命名空间内唯一，但不必跨命名空间唯一。命名空间作用域仅适用于命名空间资源（如 Deployment、[[service|Service]]），不适用于集群范围资源（如 StorageClass、Node、PersistentVolume）。

## 核心概念/原理

### 何时使用多个命名空间

- 适用于多用户、多团队或多项目的环境。
- 对于只有几个到几十个用户的集群，通常不需要考虑命名空间。
- 命名空间提供名称作用域，不能相互嵌套，每个 Kubernetes 资源只能属于一个命名空间。
- 命名空间是划分集群资源给多个用户的一种方式（通过 ResourceQuota）。
- **不需要**使用多个命名空间来区分略有不同的资源（如同一软件的不同版本），这种情况应在同一命名空间内使用标签区分。

### 初始命名空间

Kubernetes 启动时包含四个初始命名空间：

- `default`：默认命名空间，方便用户立即开始使用集群。
- `kube-node-lease`：包含与每个节点关联的 Lease 对象，用于 [[kubelet|kubelet]] 发送心跳，使控制平面能够检测节点问题。
- `kube-public`：所有客户端（包括未认证的）都可读取，主要用于集群范围内的公共资源。
- `kube-system`：Kubernetes 系统创建的对象所在的命名空间。

## 关键机制或特性

- **DNS 条目**：创建 Service 时会生成对应的 DNS 条目，格式为 `<service-name>.<namespace-name>.svc.cluster.local`。如果只使用 `<service-name>`，会解析到同一命名空间内的本地服务。跨命名空间访问需要使用完全限定域名（FQDN）。
- **自动标签**：自 Kubernetes 1.22 [stable] 起，控制平面会在所有命名空间上设置不可变的标签 `kubernetes.io/metadata.name`，其值为命名空间名称。

## 使用场景

- 在多租户环境中按团队或项目隔离资源。
- 通过 ResourceQuota 限制不同命名空间的资源使用。
- 区分开发、测试和生产环境（虽然有时也使用不同集群）。

## 最佳实践/注意事项

- 对于生产集群，建议不要使用 `default` 命名空间，而应创建并使用其他命名空间。
- 避免创建以 `kube-` 为前缀的命名空间，因为它保留给 Kubernetes 系统命名空间使用。
- 注意，如果创建的命名空间名称与公共顶级域名相同，可能导致服务短 DNS 名称与公共 DNS 记录重叠。应限制命名空间创建权限，并可配置准入 Webhook 进行拦截。

## 参考链接

- [Namespaces - Official Documentation](https://kubernetes.io/docs/concepts/overview/working-with-objects/namespaces/)

## Related

- [[17-系统基础/06-知识字典/fundamentals/about-cgroup-v2.md|About cgroup v2（关于 cgroup v2）]]
- [[17-系统基础/06-知识字典/fundamentals/annotations.md|注解]]
- [[17-系统基础/06-知识字典/fundamentals/bpfman.md|bpfman eBPF 管理器]]


<!-- risk-assessed -->
