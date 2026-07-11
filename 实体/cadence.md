---
title: Cadence (entities)
description: '## 概述'
summary: 'Cadence 是一个分布式、可扩展、持久化的工作流编排引擎，用于以可靠、可扩展的方式执行异步长时间运行的业务逻辑。Cadence 由 Uber 开源，能将复杂的分布式系统交互逻辑简化为简单的编程模型，自动处理失败重试、状态持久化和超时管理。'
category: entities
tags:
- k8s
- cncf
- streaming
- cadence
- mysql
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cadence 是什么
- 如何 Cadence
trigger_keywords:
- Cadence
prerequisites:
- kubectl-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Cadence

> **CNCF 状态**: Sandbox | **类别**: Streaming | **主要语言**: Go

## 概述

Cadence 是一个 CNCF 孵化项目，由 Uber 开发，是一个分布式、高可用的 workflow 编排引擎。它简化了构建和管理大规模异步业务流程的复杂性，通过代码定义工作流而非 YAML/JSON 配置。Cadence 保证工作流执行的可靠性——即使进程崩溃、网络中断，工作流也能在恢复后从断点继续执行。它已在 Uber 内部运行超过数千个业务工作流，处理数百万并发执行。

## Key Features（核心能力）

- **代码即工作流**：使用 Go/Java/Python 代码定义工作流逻辑，而非声明式配置
- **执行保证**：工作流在进程崩溃或网络故障后可恢复继续执行
- **Activity 隔离**：将工作流逻辑（确定性的）与副作用操作（Activity）分离
- **自动重试**：Activity 失败自动重试，支持自定义重试策略
- **版本管理**：支持工作流版本控制，确保运行中工作流的兼容性
- **可视化**：提供 Web UI 查看工作流执行历史和状态

## 架构与工作原理

Cadence 架构包含多个组件：Frontend 处理 gRPC/Thrift API 请求；History Service 管理工作流状态机，是核心引擎； Matching Service 负责将 Activity Task 分配给 Worker；Worker（客户端 SDK）执行 Activity 和工作流逻辑。底层使用 Cassandra/MySQL/PostgreSQL 作为持久化存储，通过事件溯源（Event Sourcing）模式记录工作流执行历史，重建工作流状态。

## K8s 集成

Cadence Server 可通过 Helm Chart 部署到 Kubernetes。History Service 和 Worker 通过 Deployment 部署，使用 PVC 或外部数据库存储。Cadence Worker SDK 运行在应用 Pod 中，通过 gRPC 与 Cadence Server 通信。K8s 的滚动更新与 Cadence 的工作流版本管理配合，可实现无缝的工作流代码升级。

## 生产用例

- **订单处理流程**：电商订单的多步骤异步处理（支付、库存、物流）
- **数据管道编排**：编排 ETL 管道、数据校验、异常处理的执行顺序
- **微服务编排**：Saga 模式的分布式事务编排
- **CI/CD 流水线**：替代传统 CI 工具的代码定义式流水线

## 安装与快速开始

```bash
helm repo add cadence https:// cadence-worker.s3.amazonaws.com
helm install cadence cadence/cadence -n cadence --create-namespace
# 或使用 Docker
docker compose up -f docker-compose.yml
```

## 对比替代方案

相比 Argo Workflow（基于 YAML 的 DAG 工作流），Cadence 使用代码定义工作流，更灵活且支持复杂逻辑。相比 Temporal（Cadence 原团队的商业版本），Cadence 是社区驱动的开源版本。

## Related

- [[koordinator]] — Koordinator
- [[oxia]] — Oxia
- [[krkn]] — Krkn
- [[opengitops]] — OpenGitOps
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cadence
- [[实体/drasi.md|[[Drasi|Drasi]]]]
- [[实体/tremor.md|[[Tremor|Tremor]]]]
- [[实体/nats.md|NATS]]
- [[实体/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference


<!-- risk-assessed -->
