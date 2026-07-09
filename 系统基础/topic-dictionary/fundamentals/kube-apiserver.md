---
title: API Server
description: kube-apiserver 是 Kubernetes 控制平面的核心组件，提供 RESTful API 作为集群所有交互的统一入口。用户、集群内部组件和外部工...
summary: kube-apiserver 是 Kubernetes 控制平面的核心组件，提供 RESTful API 作为集群所有交互的统一入口。用户、集群内部组件和外部工...
category: dictionary
tags:
- k8s
- glossary
- apiserver
- control-plane
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- API Server 是什么
- kube-apiserver 详解
trigger_keywords:
- API Server
- kube-apiserver
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# API Server

> **英文名**: kube-apiserver

## 概述

kube-apiserver 是 Kubernetes 控制平面的核心组件，提供 RESTful API 作为集群所有交互的统一入口。用户、集群内部组件和外部工具都通过 API Server 来查询和操作集群状态。

## 核心概念/原理

### 核心职责

- **API 网关**：所有请求（包括 kubectl、控制器、kubelet）都通过 API Server 的 HTTP API。
- **认证（Authentication）**：验证请求者身份（支持多种认证方式：证书、Bearer Token、OIDC 等）。
- **授权（Authorization）**：基于 RBAC、ABAC 或 Node 授权决定请求是否被允许。
- **准入控制（Admission Control）**：在对象持久化前执行验证和变更逻辑。
- **持久化**：通过 etcd 存储对象状态，支持 watch 机制实现变更通知。

### API 请求生命周期

```
请求 → 认证 → 授权 → Mutating Admission → Schema 验证 → Validating Admission → 持久化(etcd)
```

## 关键机制或特性

- API Server 支持 **聚合层（Aggregation Layer）**，允许通过 APIService 注册自定义 API Server。
- 支持 **API Priority and Fairness**，对不同类别的请求进行流量控制。
- 所有对象的 watch 事件通过 API Server 分发给订阅者。
- 支持 OpenAPI v3 规范描述所有 API 端点。

## 使用场景与最佳实践

- 使用 `--audit-log-path` 启用审计日志，记录所有 API 请求。
- 配置 `--max-requests-inflight` 防止 API Server 过载。
- 生产环境中部署多个 API Server 实例并使用负载均衡器。
- 启用 `--encryption-provider-config` 加密 etcd 中的 Secret 数据。

## 参考链接

- [kube-apiserver - Official Documentation](https://kubernetes.io/docs/reference/command-line-tools-reference/kube-apiserver/)

## Related

[[系统基础/topic-dictionary/fundamentals/the-kubernetes-api.md|Kubernetes API]]


<!-- risk-assessed -->
