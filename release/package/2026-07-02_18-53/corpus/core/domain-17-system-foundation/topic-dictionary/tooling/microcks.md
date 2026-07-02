---
title: Microcks API 模拟测试
description: Microcks 是 CNCF Sandbox 项目，提供 API 的模拟和测试能力，支持 OpenAPI/AsyncAPI/gRPC/GraphQL
  等多种 ...
summary: Microcks 是 CNCF Sandbox 项目，提供 API 的模拟和测试能力，支持 OpenAPI/AsyncAPI/gRPC/GraphQL
  等多种 ...
category: dictionary
tags:
- k8s
- glossary
- tooling
- testing
- api
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Microcks API 模拟测试 是什么
- Microcks 详解
trigger_keywords:
- Microcks API 模拟测试
- Microcks
- dictionary
prerequisites:
- kubernetes
---



# Microcks API 模拟测试（Microcks）

## 概述

Microcks 是 CNCF Sandbox 项目，提供 API 的模拟和测试能力，支持 OpenAPI/AsyncAPI/gRPC/GraphQL 等多种 API 规范的 Mock 生成和契约测试。

## 核心概念/原理

- **API Mocking**：从 API 规范自动生成 Mock 服务
- **契约测试**：验证 API 实现是否符合规范
- **CNCF Sandbox**：活跃的 API 测试社区
- **多协议**：REST/SOAP/gRPC/GraphQL/AsyncAPI

## 关键机制或特性

- 导入 OpenAPI/AsyncAPI/Postman/GraphQL 规范
- 自动生成 Mock 端点和响应
- 契约测试（Conformance Testing）
- 测试数据管理（Dataset）
- 延迟和错误模拟
- Kubernetes Operator 部署
- CLI 和 Web UI

## 使用场景与最佳实践

- 微服务 API 的集成测试
- API 规范的 Mock 服务
- 消费者驱动的契约测试
- 前后端并行开发
- API 变更的影响评估

## 参考链接

- https://microcks.io/
- https://github.com/microcks/microcks

## Related

- [[domain-17-system-foundation/topic-dictionary/platform-engineering/grpc.md|gRPC]]
- [[domain-17-system-foundation/topic-dictionary/networking/connect-rpc.md|Connect RPC]]
- [[domain-17-system-foundation/topic-dictionary/operations/kube-burner.md|kube-burner]]
