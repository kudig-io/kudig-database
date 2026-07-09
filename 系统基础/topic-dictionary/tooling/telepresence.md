---
title: Telepresence 远程开发
description: Telepresence 是 Ambassadeur Labs 开源的 Kubernetes 远程开发工具，将本地开发环境与远程 K8s
  集群网络打通，开发者可...
summary: Telepresence 是 Ambassadeur Labs 开源的 Kubernetes 远程开发工具，将本地开发环境与远程 K8s 集群网络打通，开发者可...
category: dictionary
tags:
- k8s
- glossary
- tooling
- development
- debugging
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Telepresence 远程开发 是什么
- Telepresence 详解
trigger_keywords:
- Telepresence 远程开发
- Telepresence
- dictionary
prerequisites:
- kubernetes
---



# Telepresence 远程开发（Telepresence）

## 概述

Telepresence 是 Ambassadeur Labs 开源的 Kubernetes 远程开发工具，将本地开发环境与远程 K8s 集群网络打通，开发者可在本地编码同时访问集群内服务和被集群内服务回调。

## 核心概念/原理

- **网络打通**：本地进程可直接访问集群内 Service（DNS + IP 透明）
- **流量拦截**：将集群中指定服务的流量重定向到本地进程
- **本地开发体验**：保留本地 IDE、调试器，无需在集群中部署
- **CNCF Sandbox**：远程开发领域的标准工具

## 关键机制或特性

- `telepresence connect` 建立本地到集群的网络隧道
- `telepresence intercept` 拦截指定 Service 流量到本地
- 支持 Preview URL 分享开发中的服务
- 环境变量自动注入
- 多命名空间访问
- 与 VS Code / JetBrains 集成

## 使用场景与最佳实践

- 微服务架构中的本地开发和调试
- 服务间调用的端到端测试
- 避免本地搭建完整 K8s 环境
- Code Review 中的 Preview 环境搭建
- 远程集群的 API 调试

## 参考链接

- https://www.telepresence.io/
- https://github.com/telepresenceio/telepresence

## Related

- [[系统基础/topic-dictionary/tooling/skaffold.md|Skaffold]]
- [[系统基础/topic-dictionary/networking/linkerd.md|Linkerd]]
- [[系统基础/topic-dictionary/networking/consul.md|Consul]]
