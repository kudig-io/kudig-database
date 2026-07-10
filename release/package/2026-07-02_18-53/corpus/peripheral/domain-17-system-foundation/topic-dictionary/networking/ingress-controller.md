---
title: 入口控制器
description: Ingress Controller 是实现 Ingress 规则的实际组件。Kubernetes 本身不包含 Ingress Controller
  实现，需要...
summary: Ingress Controller 是实现 Ingress 规则的实际组件。Kubernetes 本身不包含 Ingress Controller
  实现，需要...
category: dictionary
tags:
- k8s
- glossary
- ingress
- networking
- nginx
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 入口控制器 是什么
- Ingress Controller 详解
trigger_keywords:
- 入口控制器
- Ingress Controller
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 入口控制器

> **英文名**: Ingress Controller

## 概述

Ingress Controller 是实现 Ingress 规则的实际组件。Kubernetes 本身不包含 Ingress Controller 实现，需要用户部署第三方控制器来处理 HTTP/HTTPS 路由。

## 核心概念/原理

### 主流 Ingress Controller

- **Nginx Ingress Controller**：最流行，基于 Nginx，功能丰富。
- **Traefik**：自动服务发现，支持多种协议。
- **Kong Ingress Controller**：基于 Kong API 网关。
- **HAProxy Ingress**：基于 HAProxy 的高性能方案。

### 工作原理

Ingress Controller 监听 Ingress 资源的变化，动态更新自身的反向代理配置，将外部流量路由到对应的后端 Service。

## 关键机制或特性

- Ingress Controller 通常以 Deployment + Service（LoadBalancer/NodePort）方式部署。
- 支持通过注解（annotations）扩展路由能力（限流、CORS、重写等）。
- 一个集群可以部署多个 Ingress Controller，通过 `ingressClassName` 区分。

## 使用场景与最佳实践

- 生产环境部署高可用的 Ingress Controller（多副本 + PDB）。
- 使用 HPA 根据流量自动扩缩 Ingress Controller。
- 配置请求限流和 WAF 规则保护后端服务。
- 考虑迁移到 Gateway API 获得更强大的路由能力。

## 参考链接

- [Ingress Controller - Official Documentation](https://kubernetes.io/docs/concepts/services-networking/ingress-controllers/)

## Related

[[domain-17-system-foundation/知识字典/networking/ingress-controllers.md|Ingress Controllers]]


<!-- risk-assessed -->
