---
title: Spin (entities)
description: '## 概述'
summary: 'Spin 是由 Fermyon 开发的 WebAssembly (Wasm) 微服务框架，用于构建和运行基于事件驱动的 Wasm 应用。它提供极快的冷启动时间（亚毫秒级），支持多种编程语言（Rust、Go、Python、JavaScript、C#等），并内置 HTTP 触发器、Redis 触发器、键值存储、SQL 数据库等能力。'
category: entities
tags:
- k8s
- cncf
- runtime
- spin
- argocd
- containerd
- redis
- crd
- operator
- wasm
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Spin 是什么
- 如何 Spin
trigger_keywords:
- Spin
prerequisites:
- kubectl-basics
- gitops-basics
- redis-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Spin

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Rust

## 概述

Spin 是由 Fermyon 开发的 WebAssembly (Wasm) 微服务框架，用于构建和运行基于事件驱动的 Wasm 应用。它提供极快的冷启动时间（亚毫秒级），支持多种编程语言（Rust、Go、Python、JavaScript、C#等），并内置 HTTP 触发器、Redis 触发器、键值存储、SQL 数据库等能力。Spin 应用可以部署到本地、Kubernetes（通过 Spi...

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **组件粒度**: 每个路由前缀使用独立组件，实现最小权限和独立部署
- **最小权限**: 通过 `allowed_outbound_hosts` 限制组件的外部访问范围
- **存储选择**: 简单 KV 用内置 KV Store，关系数据用 SQLite 或外部 DB
- **OCI 分发**: 使用 OCI Registry 管理 Spin 应用版本
- **Wasm 优化**: 使用 `wasm-opt` 优化 Wasm 二进制大小

## 架构定位

在 CNCF 生态中，spin 属于 **Runtime** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[containerd]]
- [[entities/argocd.md|[[ArgoCD|argocd]]]]
- [[operator-pattern]]

## Related

- [[spinkube]] — SpinKube
- [[wasmedge]] — WasmEdge
- [[entities/cncf-runtime.md|cncf-runtime]] — CNCF 容器运行时与工具链项目全景
- [[04-containerd-upgrade-migration]] — containerd 升级迁移
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 03-spinkube-framework
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
