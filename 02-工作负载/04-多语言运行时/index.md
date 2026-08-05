---
title: "多语言运行时索引"
description: "工作负载/多语言运行时目录索引：Go、Python、Rust、GPU 与多集群分发的生产实践导航"
summary: "多语言运行时子目录的导航索引，汇总各语言与专项工作负载在 Kubernetes 上的生产实践文档。"
category: 工作负载
tags:
- index
- runtime
- go
- python
- rust
- gpu
tier: supporting
created: '2026-07-19'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- 开发工程师
estimated_read_time: 5min
intent_queries:
- "多语言运行时有哪些文档"
- "Go Python Rust 在 K8s 上如何部署"
trigger_keywords:
- runtime
- go
- python
- rust
- gpu
- multicluster
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 多语言运行时索引

> 本页为 `工作负载/多语言运行时` 目录的导航索引。

## 概述

不同编程语言在 Kubernetes 上的生产落地各有其独特的挑战与最佳实践。Go 需要处理 GOMAXPROCS 与 cgroup 的协同，Python 受困于 GIL 与镜像体积，Rust 追求极致镜像与无 GC 停顿，GPU 工作负载面临稀缺资源调度，多集群分发则解决跨集群可用性问题。本子目录系统覆盖这些主题，为 SRE 与平台工程师提供可直接落地的生产指南。

## 文档导航

### 语言运行时实践

| 文档 | 主题 | 关键内容 | 难度 |
|------|------|---------|------|
| [[02-工作负载/04-多语言运行时/01-go-on-kubernetes-production.md\|Go 生产实践]] | Go on K8s | 多阶段构建、distroless、GOMAXPROCS、graceful shutdown、pprof | advanced |
| [[02-工作负载/04-多语言运行时/02-python-on-kubernetes-production.md\|Python 生产实践]] | Python on K8s | GIL、gunicorn/uvicorn、镜像优化、AI/ML 负载 | advanced |
| [[02-工作负载/04-多语言运行时/03-rust-on-kubernetes-production.md\|Rust 生产实践]] | Rust on K8s | musl 静态链接、tokio 调优、内存安全、性能监控 | advanced |

### 专项工作负载

| 文档 | 主题 | 关键内容 | 难度 |
|------|------|---------|------|
| [[02-工作负载/04-多语言运行时/04-gpu-workload-management.md\|GPU 工作负载管理]] | GPU 调度 | Device Plugin、MIG、多 GPU、健康检查、配额 | advanced |
| [[02-工作负载/04-多语言运行时/05-multicluster-workload-distribution.md\|多集群工作负载分发]] | 多集群 | Karmada/OCM、ApplicationSet、权重路由、故障转移 | advanced |

## 阅读建议

- **新手入门**：先读 [[02-工作负载/04-多语言运行时/01-go-on-kubernetes-production.md|Go 生产实践]]，理解通用的探针、生命周期与镜像构建模式。
- **AI 平台工程师**：重点阅读 [[02-工作负载/04-多语言运行时/02-python-on-kubernetes-production.md|Python 生产实践]] 与 [[02-工作负载/04-多语言运行时/04-gpu-workload-management.md|GPU 工作负载管理]]。
- **平台架构师**：关注 [[02-工作负载/04-多语言运行时/05-multicluster-workload-distribution.md|多集群工作负载分发]] 与 [[05-网络/01-K8s网络核心/51-multicluster-network-federation.md|多集群网络联邦]]。

## 跨域关联

- 镜像安全：[[08-安全/05-供应链/12-image-security-scanning.md|镜像安全扫描]]
- Pod 安全加固：[[08-安全/04-策略治理/03-pod-security-standards.md|Pod Security Standards]]
- 可观测性：[[09-可观测性/README|可观测性]]
- Java 运行时：[[02-工作负载/02-Java-on-K8s/01-spring-boot-kubernetes-production.md|Spring Boot on Kubernetes 生产实践指南]]

## Related

- [[02-工作负载/index.md|工作负载索引]]
- [[02-工作负载/04-多语言运行时/01-go-on-kubernetes-production.md|Go 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/02-python-on-kubernetes-production.md|Python 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/03-rust-on-kubernetes-production.md|Rust 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/04-gpu-workload-management.md|GPU 工作负载管理]]
- [[02-工作负载/04-多语言运行时/05-multicluster-workload-distribution.md|多集群工作负载分发]]
