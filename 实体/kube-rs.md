---
title: kube-rs (entities)
description: '## 概述'
summary: 'kube-rs 是 Rust 语言的 Kubernetes 客户端库，提供类型安全的 [[系统基础/知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] 交互能力。'
category: entities
tags:
- k8s
- cncf
- platform
- kube-rs
- prometheus
- grafana
- argocd
- rbac
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kube-rs 是什么
- 如何 kube-rs
trigger_keywords:
- kube-rs
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kube-rs

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Rust

## 概述

kube-rs 是 Rust 语言的 Kubernetes 客户端和 Controller 开发框架，由 clux 维护，2021 年加入 CNCF Sandbox。它提供类型安全的 [[系统基础/知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] 交互能力，包含低级 API 客户端（kube-client）、运行时抽象（kube-runtime）和 CRD 代码生成（kube-derive）。kube-rs 使开发者能用 Rust 构建高性能、内存安全的 Kubernetes Controller 和 Operator，是 Rust 云原生生态（如 Krustlet、Stackable）的核心依赖。

## 核心特性

- **类型安全**: 基于强类型的 Rust API，编译时捕获错误
- **CRD 宏**: `#[derive(CustomResource)]` 自动生成 CRD Schema 和类型定义
- **Reconciler 模式**: 内置 Controller runtime，支持 Watch + Reconcile 调谐循环
- **Reflector/Cache**: 本地资源缓存（Store），减少 API Server 调用
- **认证/鉴权**: 支持 In-cluster、kubeconfig、OIDC 多种认证方式
- **TLS/HTTP2**: 基于 hyper/tower 的高性能异步 HTTP 客户端

## 架构

kube-rs 由三个核心 crate 组成。`kube-client` 封装 Kubernetes API 的 HTTP 客户端，支持 Watch/List/Create/Update/Delete 操作和认证管理。`kube-runtime` 提供 Controller、reflector、wait_condition 等运行时抽象。`kube-derive` 通过过程宏自动生成 CRD 定义代码。Controller 使用 Tokio 异步运行时，通过 Watch API 增量监听资源变更，触发 Reconcile 函数调谐。reflector 维护本地 Store 缓存，减少 API Server 压力。

## Kubernetes 集成

kube-rs 通过标准 Kubernetes API 交互。支持 In-cluster 配置（读取 ServiceAccount Token 和 CA 证书）和 Out-of-cluster 配置（kubeconfig）。`#[derive(CustomResource)]` 宏从 Rust struct 自动生成 CRD OpenAPI Schema 并注册到集群。Controller 通过 `kube::Api::watch` 监听资源变更，`kube::runtime::Controller::new` 创建调谐循环。使用 `tonic`/`tower` 实现 gRPC 和 HTTP 中间件。

## 生产使用场景

1. **高性能 Operator**: 对性能和内存安全要求极高的场景（如安全、金融）
2. **CRD 控制器**: 管理 Custom Resources 的业务逻辑控制器
3. **自动化工具**: 集群巡检、资源清理、合规检查等工具
4. **WASM 运行时**: 如 Krustlet 使用 kube-rs 与 K8s API 交互

## 安装

```rust
# Cargo.toml
[dependencies]
kube = { version = "0.95", features = ["runtime", "derive"] }
k8s-openapi = { version = "0.23", features = ["latest"] }
tokio = { version = "1", features = ["full"] }

# 简单示例
use kube::Api;
#[derive(kube::CustomResource, serde::Serialize, serde::Deserialize)]
#[kube(group = "example.com", version = "v1", kind = "MyApp")]
struct MyAppSpec { replicas: i32 }
```

## 替代方案

| 项目 | 语言 | 优势 | 劣势 |
|------|------|------|------|
| **kube-rs** | Rust | 内存安全、高性能 | 生态较小、学习曲线陡 |
| controller-runtime | Go | 官方支持、生态最大 | GC 开销、内存安全靠测试 |
| Java Operator SDK | Java | 企业生态 | JVM 资源开销大 |
| kubernetes-client/python | Python | 快速原型 | 性能不适合生产 Controller |

## 架构定位

在 CNCF 生态中，kube-rs 属于 **Platform / Client Library** 类别，为 Rust 社区提供 Kubernetes 原生开发能力。它是 Rust 在云原生领域的重要基础设施。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[实体/argocd.md|[[ArgoCD|argocd]]]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/controller-pattern.md|controller-pattern]]

## Related

- [[athenz]] — Athenz
- [[metallb]] — MetalLB
- [[buildpacks]] — Cloud Native Buildpacks
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kube-rs
- [[实体/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
