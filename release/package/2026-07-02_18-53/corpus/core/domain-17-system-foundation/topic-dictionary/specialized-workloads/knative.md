---
title: Knative
description: Knative 是 CNCF 孵化项目，为 Kubernetes 提供 Serverless 能力。它包含 Serving（自动扩缩容 +
  缩到零）和 Even...
summary: Knative 是 CNCF 孵化项目，为 Kubernetes 提供 Serverless 能力。它包含 Serving（自动扩缩容 + 缩到零）和
  Even...
category: dictionary
tags:
- k8s
- glossary
- knative
- serverless
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Knative 是什么
- Knative 详解
trigger_keywords:
- Knative
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Knative

> **英文名**: Knative

## 概述

Knative 是 CNCF 孵化项目，为 Kubernetes 提供 Serverless 能力。它包含 Serving（自动扩缩容 + 缩到零）和 Eventing（事件驱动架构）两大模块，让开发者无需管理基础设施即可运行应用。

## 核心概念/原理

### 核心组件

| 组件 | 功能 |
|------|------|
| Serving | HTTP 请求驱动的自动扩缩容 |
| Eventing | 事件生产和消费的标准化 |
| Revision | 不可变的配置快照（类似 ReplicaSet） |
| Route | 流量路由到不同 Revision |

### Scale-to-Zero

```
请求到达 → Activator 拦截 → 扩容 Pod → 流量转发 → 空闲超时 → 缩到零
```

## 关键机制或特性

- **Revision 管理**：每次配置变更自动创建新 Revision。
- **流量拆分**：Route 支持按比例分配流量到多个 Revision（金丝雀）。
- **Concurrency**：控制每个 Pod 的并发请求数。
- **Eventing Broker**：标准化的事件发布和订阅（CloudEvents）。
- **Trigger**：基于事件属性过滤并路由到 Knative Service。

## 使用场景与最佳实践

- 轻量级 HTTP 服务使用 Knative Serving 部署（缩到零节省成本）。
- 使用 Revision 流量拆分实现金丝雀发布。
- 配合 KServe 部署 ML 模型推理服务。
- 使用 Eventing 构建事件驱动的微服务架构。
- 设置合理的 `minScale` 避免冷启动延迟。

## 参考链接

- [Knative Official](https://knative.dev/)

## Related

- [[domain-17-system-foundation/知识字典/specialized-workloads/kserve.md|KServe]]
- [[domain-17-system-foundation/知识字典/scheduling/keda.md|KEDA]]
- [[domain-17-system-foundation/知识字典/scheduling/hpa.md|HPA]]
- [[domain-17-system-foundation/知识字典/workloads/deployment.md|Deployment]]
- [[domain-17-system-foundation/知识字典/networking/ingress.md|Ingress]]


<!-- risk-assessed -->
