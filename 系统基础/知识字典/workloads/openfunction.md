---
title: OpenFunction Serverless
description: OpenFunction 是青云科技开源的 CNCF Sandbox 项目，云原生 FaaS 平台，支持同步/异步函数、多种运行时和事件源，集成
  Knative...
summary: OpenFunction 是青云科技开源的 CNCF Sandbox 项目，云原生 FaaS 平台，支持同步/异步函数、多种运行时和事件源，集成
  Knative...
category: dictionary
tags:
- k8s
- glossary
- workloads
- serverless
- cncf
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenFunction Serverless 是什么
- OpenFunction 详解
trigger_keywords:
- OpenFunction Serverless
- OpenFunction
- dictionary
prerequisites:
- kubernetes
---



# OpenFunction Serverless（OpenFunction）

## 概述

OpenFunction 是青云科技开源的 CNCF Sandbox 项目，云原生 FaaS 平台，支持同步/异步函数、多种运行时和事件源，集成 Knative 和 OpenFuncAsync（Dapr）两种运行模式。

## 核心概念/原理

- **双模式**：Knative（同步 HTTP）+ OpenFuncAsync（异步事件）
- **多运行时**：支持 Node.js/Go/Python/Java/Rust
- **CNCF Sandbox**：青云科技主导
- **Dapr 集成**：利用 Dapr 的构建块能力

## 关键机制或特性

- Function CRD 定义函数
- Builder CRD 函数镜像构建
- Serving CRD 函数运行时管理
- Knative 同步服务
- OpenFuncAsync 异步事件驱动（Dapr + KEDA）
- Shipwright 镜像构建集成
- 多事件源（Kafka/NATS/Redis 等）

## 使用场景与最佳实践

- 事件驱动的 Serverless 函数
- 微服务的函数化拆分
- 数据处理的异步 Pipeline
- API 后端的 Serverless 化
- 多运行时函数的统一管理

## 参考链接

- https://openfunction.dev/
- https://github.com/openfunction/openfunction

## Related

- [[系统基础/知识字典/specialized-workloads/knative.md|Knative]]
- [[系统基础/知识字典/specialized-workloads/openfaas.md|OpenFaaS]]
- [[系统基础/知识字典/scheduling/keda.md|KEDA]]
