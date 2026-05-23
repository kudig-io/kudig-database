---
title: CNCF 基础设施与混沌工程项目全景
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- chaos-engineering
- virtualization
- messaging
- infrastructure
- kafka
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CNCF 基础设施与混沌工程项目全景 是什么
- 如何 CNCF 基础设施与混沌工程项目全景
trigger_keywords:
- CNCF
- 基础设施与混沌工程项目全景
prerequisites:
- kubectl-basics
- kafka-basics
created: "2026-05-23"
---

# CNCF 基础设施与混沌工程项目全景

> 聚合页面 | 涵盖 26 个 CNCF 基础设施和辅助工具项目

## 概述

CNCF 基础设施项目覆盖 **混沌工程**、**消息中间件**、**API 与服务**、**管理工具** 等辅助领域。

---

## [[domain-17-system-foundation/topic-dictionary/operations/chaos-engineering.md|混沌工程（Chaos Engineering）]]

### [[chaos-mesh]] — 孵化项目

[[Chaos Mesh|Chaos Mesh]] 是 K8s 原生的混沌工程平台。

- Pod 故障注入（kill、failure、延迟）
- 网络故障（延迟、丢包、分区）
- 文件系统和内核故障
- 仪表板可视化

### [[litmus]] — 孵化项目

Litmus 是 CNCF 混沌工程框架。

- ChaosHub 实验库
- 自动化混沌工作流
- 与 Argo 集成

### [[chaosblade]] — 沙箱项目

ChaosBlade 是阿里巴巴开源的混沌工程工具。

### [[krkn]] — 沙箱项目

Krkn（原 Kraken）是 K8s 混沌测试工具。

### [[kuberhealthy]] — 沙箱项目

Kuberhealthy 提供 K8s 集群健康检查。

---

## 消息中间件

### [[nats]] — 孵化项目

NATS 是高性能消息系统。

- 发布/订阅、请求/回复
- JetStream 持久化消息流
- 轻量级，适合微服务通信

### [[strimzi]] — 孵化项目

Strimzi 在 K8s 上运行 Apache Kafka。

- Kafka 集群 Operator
- 配额、镜像、连接管理

### [[cadence]] — 沙箱项目

Cadence 是分布式工作流编排引擎。

### [[drasi]] — 沙箱项目

Drasi 是实时变更数据捕获（CDC）框架。

---

## API 与服务工具

### [[openfeature]] — 孵化项目

OpenFeature 是功能开关（Feature Flag）的标准 API 规范。

- 厂商无关的 SDK
- 与 LaunchDarkly、Flagsmith 等集成

### [[easegress]] — 沙箱项目

Easegress 是高性能 API 网关和编排引擎。

### [[connect-rpc]] — 沙箱项目

Connect 是轻量级 RPC 框架（兼容 gRPC 和 gRPC-Web）。

### [[microcks]] — 沙箱项目

Microcks 提供 API 模拟和测试。

---

## 管理与可视化

### [[headlamp]] — 沙箱项目

Headlamp 是 K8s 的 Web 管理界面。

### tools]] — 沙箱项目

VS Code [[Kubernetes|Kubernetes]] 扩展。

### [[cloud-custodian]] — 孵化项目

Cloud Custodian 管理云资源合规策略。

---

## ML 模型管理

### [[kitops]] — 沙箱项目

KitOps 使用 OCI 格式打包 ML 模型和数据集。

### [[modelpack]] — 沙箱项目

ModelPack 标准化 ML 模型打包。

### [[xregistry]] — 沙箱项目

xRegistry 定义跨注册表的统一 API 规范。

---

## 其他基础设施

### porter — 沙箱项目

porter 使用 CNAB 打包和分发云应用。

### [[hexa]] — 沙箱项目

Hexa 是多云环境抽象层。

### [[sermant]] — 沙箱项目

Sermant 是 Java 微服务的无侵入增强框架。

### [[kube-burner]] — 沙箱项目

Kube-Burner 是 K8s 性能基准测试工具。

### [[runme-notebooks]] — 沙箱项目

Runme Notebooks 将 Markdown 转化为可执行笔记本。

### [[kube-rs]] — 沙箱项目

kube-rs 是 Rust 的 K8s 客户端库。

---

## 相关页面

- [[entities/cncf-observability.md|cncf-observability]] — 可观测性
- [[entities/cncf-orchestration.md|cncf-orchestration]] — 编排与应用管理
- [[entities/cncf-edge-ai.md|cncf-edge-ai]] — 边缘计算与 AI/ML

## Related

- [[argo]] — Argo Workflows
- [[chaos-mesh]] — Chaos Mesh
- [[grpc]] — gRPC
- [[cloud-custodian]] — Cloud Custodian
- [[runme-notebooks]] — Runme
