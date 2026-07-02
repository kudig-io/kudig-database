---
title: OpenFaaS
description: OpenFaaS（Functions as a Service）是开源的 Serverless 框架，支持在 Kubernetes 和 Docker
  上运行函数...
summary: OpenFaaS（Functions as a Service）是开源的 Serverless 框架，支持在 Kubernetes 和 Docker
  上运行函数...
category: dictionary
tags:
- k8s
- glossary
- openfaas
- serverless
- faas
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenFaaS 是什么
- OpenFaaS 详解
trigger_keywords:
- OpenFaaS
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# OpenFaaS

> **英文名**: OpenFaaS

## 概述

OpenFaaS（Functions as a Service）是开源的 Serverless 框架，支持在 Kubernetes 和 Docker 上运行函数。它将函数打包为容器镜像，通过 HTTP 触发，支持自动扩缩容和缩到零。

## 核心概念/原理

### 核心组件

| 组件 | 功能 |
|------|------|
| Gateway | API 网关和函数管理 |
| faas-cli | 命令行工具 |
| Function | 函数定义（容器镜像） |
| Queue Worker | 异步函数处理 |

### 与 Knative 对比

| 特性 | OpenFaaS | Knative |
|------|----------|--------|
| 复杂度 | 低 | 高 |
| 依赖 | 少 | 需 Knative Serving |
| 缩到零 | 支持 | 支持 |
| 语言支持 | 任意（容器） | 任意（容器） |

## 关键机制或特性

- **模板**：预构建的函数模板（Python/Node/Go 等）。
- **异步调用**：通过 NATS 队列异步执行函数。
- **Auto-scaling**：基于 RPS 或 CPU 的自动扩缩。
- **Scale-to-Zero**：无调用时缩到零。
- 支持私有函数和认证。

## 使用场景与最佳实践

- 轻量级 Serverless 需求选择 OpenFaaS。
- 使用 faas-cli 快速创建和部署函数。
- 配合 CronJob 实现定时触发的函数执行。
- 使用异步模式处理后台任务。
- 为函数设置合理的超时和资源限制。

## 参考链接

- [OpenFaaS Official](https://www.openfaas.com/)

## Related

- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/knative.md|Knative]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/keda.md|KEDA]]
- [[domain-17-system-foundation/topic-dictionary/workloads/deployment.md|Deployment]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/hpa.md|HPA]]
- [[domain-17-system-foundation/topic-dictionary/networking/ingress.md|Ingress]]


<!-- risk-assessed -->
