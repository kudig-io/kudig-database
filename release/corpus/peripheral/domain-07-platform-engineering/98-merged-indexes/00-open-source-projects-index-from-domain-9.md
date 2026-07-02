---
title: Domain-9 平台运维 — 开源项目索引
description: '# Domain-9 平台运维 — 开源项目索引'
summary: '# Domain-9 平台运维 — 开源项目索引'
category: platform-ops
tags:
- k8s
- platform
- operations
- devops
- etcd
- helm
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Domain-9 平台运维 — 开源项目索引 是什么
- 如何 Domain-9 平台运维 — 开源项目索引
- Kubernetes 9 platform ops 最佳实践
trigger_keywords:
- Domain-9
- 平台运维
- 开源项目索引
- platform
- ops
prerequisites:
- kubectl-basics
- platform-engineering-basics
- helm-basics
- etcd-basics
- backup-basics
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
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: domain
  path: ../domain-15-specialized-tech/
  label: '相关知识域: domain-15-specialized-tech'
- type: domain
  path: ../domain-10-troubleshooting-diagnostics/
  label: '相关知识域: domain-10-troubleshooting-diagnostics'
---



# Domain-9 平台运维 — 开源项目索引

> **最后更新**: 2026-04-24

---

<!-- chunk: 核心项目 -->
## 核心项目

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Kubernetes** | 平台核心 | Graduated | v1.33.0 | 115k+ | Apache-2.0 |
| **Rancher** | 多集群管理平台 | SUSE | v2.10.0 | 23k+ | Apache-2.0 |
| **KubeSphere** | 企业级容器平台 | 非 CNCF | v4.1.0 | 15k+ | Apache-2.0 |
| **Lens (OpenLens)** | K8s IDE | Mirantis | v6.5.0 | 25k+ | MIT |
| **Headlamp** | 开源 K8s Web UI | 社区 | v0.29.0 | 3k+ | Apache-2.0 |
| **K9s** | 终端 K8s UI | 社区 | v0.40.0 | 27k+ | Apache-2.0 |
| **kubectl** | 官方 CLI | K8s | v1.33.0 | - | Apache-2.0 |
| **Helm** | 包管理 | Graduated | v3.17.0 | 27k+ | Apache-2.0 |
| **kustomize** | 配置定制 | K8s SIG | v5.6.0 | 11k+ | Apache-2.0 |
| **Velero** | 集群备份恢复 | 非 CNCF | v1.15.0 | 9k+ | Apache-2.0 |
| **etcd** | 配置存储 | Graduated | v3.5.21 | 48k+ | Apache-2.0 |

---

<!-- chunk: 参考链接 -->
## 参考链接

- [K8s 运维文档](https://kubernetes.io/docs/tasks/)
- [Velero 文档](https://velero.io/docs/)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-07-platform-engineering MOC
- [[domain-07-platform-engineering/README.md|Platform Ops Domain (平台运维领域)]]
- 平台运维概述
- 集群生命周期管理
- 容量规划与资源评估 (Capacity Planning & Resource Assessment)
- 性能基准测试与调优 (Performance Benchmarking & Tuning)
- 运维指标体系建设 (Operations Metrics System)
- 监控告警体系
- GitOps配置管理 (GitOps Configuration Management)
- 运维自动化工具链 (Operations Automation Toolchain)
- 成本优化与FinOps实践 (Cost Optimization & FinOps)
- 安全合规管理 (Security & Compliance Management)
