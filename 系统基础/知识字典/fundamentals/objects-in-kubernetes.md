---
title: Kubernetes 中的对象
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- statefulset
- ingress
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 中的对象 是什么
- 如何 Kubernetes 中的对象
trigger_keywords:
- Kubernetes
- 中的对象
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Kubernetes|Kubernetes]] 中的对象

## 概述

Kubernetes 对象是 Kubernetes 系统中持久存在的实体。Kubernetes 使用这些实体来表示集群的状态。通过创建、修改或删除对象，用户可以向 Kubernetes 系统传达期望的集群状态。

## 核心概念/原理

### 什么是 Kubernetes 对象

Kubernetes 对象是"意图记录"（record of intent）。一旦创建对象，Kubernetes 系统就会持续工作以确保该对象存在。对象可以描述：

- 哪些容器化应用正在运行（以及在哪些节点上运行）
- 这些应用可用的资源
- 应用的行为策略，如重启策略、升级策略和容错策略

### 对象的 spec 和 status

几乎每个 Kubernetes 对象都包含两个嵌套字段：

- **spec（规范）**：描述对象的期望状态，由用户在创建对象时设置。
- **status（状态）**：描述对象的当前状态，由 Kubernetes 系统及其组件提供和更新。

Kubernetes 控制平面持续主动管理每个对象的实际状态，使其与期望状态匹配。例如，[[entities/deployment.md|[[Kubernetes 部署策略最佳实践|deployment]]]] 的 `spec` 指定了 3 个副本，如果某个实例失败，Kubernetes 会自动启动替代实例。

### 描述对象的必需字段

在对象的 manifest（YAML 或 JSON）中，必须设置以下字段：

- `apiVersion`：创建对象时使用的 [[系统基础/知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] 版本
- `kind`：要创建的对象类型
- `metadata`：帮助唯一标识对象的数据，包括 `name`、可选的 `namespace` 等
- `spec`：对象的期望状态

## 关键机制或特性

- **Manifest 文件**：通常使用 YAML 格式定义对象规范，`kubectl` 等工具会将其转换为 JSON 后通过 HTTP 发送到 API 服务器。
- **服务端字段验证（Server Side Field Validation）**：自 Kubernetes v1.25 起，API 服务器提供服务端字段验证，可检测对象中无法识别或重复的字段。`kubectl` 默认使用 `--validate=true`（即 strict 模式）。

## 使用场景

- 使用 Deployment、[[StatefulSet|StatefulSet]] 等工作负载对象部署应用。
- 使用 [[Service|Service]]、Ingress 等网络对象暴露应用。
- 使用 ConfigMap、Secret 等配置对象管理应用配置。

## 最佳实践/注意事项

- 使用 `kubectl apply` 进行声明式对象管理，便于版本控制和审计。
- 编写 YAML 时遵循 Kubernetes 配置最佳实践。
- 了解不同对象的 `spec` 和 `status` 结构，可参考 Kubernetes API 文档。

## 参考链接

- [Objects In Kubernetes - Official Documentation](https://kubernetes.io/docs/concepts/overview/working-with-objects/)

## Related

- [[系统基础/知识字典/fundamentals/about-cgroup-v2.md|About cgroup v2（关于 cgroup v2）]]
- [[系统基础/知识字典/fundamentals/annotations.md|注解]]
- [[系统基础/知识字典/fundamentals/bpfman.md|bpfman eBPF 管理器]]


<!-- risk-assessed -->
