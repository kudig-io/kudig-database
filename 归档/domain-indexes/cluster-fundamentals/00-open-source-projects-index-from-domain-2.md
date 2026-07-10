---
title: Domain-2 设计原则 — 开源项目索引
description: '# Domain-2 设计原则 — 开源项目索引'
summary: '# Domain-2 设计原则 — 开源项目索引'
category: design-principles
tags:
- k8s
- design
- principles
- etcd
- crd
- webhook
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- Domain-2 设计原则 — 开源项目索引 是什么
- 如何 Domain-2 设计原则 — 开源项目索引
- Kubernetes 2 design principles 最佳实践
trigger_keywords:
- Domain-2
- 设计原则
- 开源项目索引
- design
- principles
prerequisites:
- kubectl-basics
- kubernetes-concepts
- etcd-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Domain-2 设计原则 — 开源项目索引

> **最后更新**: 2026-04-24

---

<!-- chunk: 核心项目 -->
## 核心项目

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Kubernetes** | 声明式 API 与控制器的典范实现 | Graduated | v1.33.0 | 115k+ | Apache-2.0 |
| **etcd** | 分布式一致性存储 (Raft) | Graduated | v3.5.21 | 48k+ | Apache-2.0 |
| **Controller-runtime** | K8s 控制器框架 | K8s SIG | v0.20.0 | 2.5k+ | Apache-2.0 |
| **client-go** | K8s 官方 Go 客户端 | K8s SIG | v0.33.0 | - | Apache-2.0 |
| **Informer** | K8s 缓存与事件机制 | K8s 核心 | v1.33.0 | - | Apache-2.0 |
| **Workqueue** | K8s 速率限制队列 | K8s 核心 | v1.33.0 | - | Apache-2.0 |
| **Cel-go** | CEL 表达式引擎 (K8s 验证) | Google | v0.24.0 | 1k+ | Apache-2.0 |
| **controller-gen** | CRD / Webhook 代码生成 | K8s SIG | v0.17.0 | - | Apache-2.0 |

---

<!-- chunk: 参考链接 -->
## 参考链接

- [K8s 架构设计文档](https://kubernetes.io/docs/concepts/architecture/)
- [controller-runtime](https://github.com/kubernetes-sigs/controller-runtime)
- [client-go 示例](https://github.com/kubernetes/client-go/tree/master/examples)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 集群基础 MOC
- [[集群基础/README.md|Domain-2: Kubernetes 设计原则与核心机制]]
- Kubernetes 设计原则与哲学
- 声明式 API 与面向终态设计
- 控制器模式与调谐循环
- 04 - List-Watch 机制深度解析 (List-Watch)
- 05 - Informer 架构与工作队列 (Informer & Workqueue)
- 06 - 资源版本与并发控制 (Concurrency Control)
- 07 - 分布式共识与 etcd 原理 (etcd & Raft)
- 08 - 高可用架构模式 (HA Patterns)
- 09 - Kubernetes 源码结构与阅读指南 (Source Code)
- 10 - CAP 定理与分布式系统基础 (CAP Theorem)


<!-- risk-assessed -->
