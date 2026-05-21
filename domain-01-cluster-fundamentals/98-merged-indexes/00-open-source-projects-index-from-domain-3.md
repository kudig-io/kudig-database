---
title: Domain-3 控制平面 — 开源项目索引
description: '| **etcd** | 控制平面数据存储 | Graduated | v3.5.21 | 48k+ | Apache-2.0 |'
category: control-plane
tags:
- k8s
- control-plane
- etcd
- apiserver
- scheduler
- controller-manager
- coredns
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Domain-3 控制平面 — 开源项目索引 是什么
- 如何 Domain-3 控制平面 — 开源项目索引
- Kubernetes 3 control plane 最佳实践
trigger_keywords:
- Domain-3
- 控制平面
- 开源项目索引
- control
- plane
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
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-02-workloads-applications/
  label: '相关知识域: domain-02-workloads-applications'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-04-storage-data/
  label: '相关知识域: domain-04-storage-data'
- type: domain
  path: ../domain-05-security-compliance/
  label: '相关知识域: domain-05-security-compliance'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
---

# Domain-3 控制平面 — 开源项目索引

> **最后更新**: 2026-04-24

---

<!-- chunk: 核心项目 -->
## 核心项目

| 项目 | 作用 | CN8F 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Kubernetes (kube-apiserver)** | API 服务器 | Graduated | v1.33.0 | 115k+ | Apache-2.0 |
| **etcd** | 控制平面数据存储 | Graduated | v3.5.21 | 48k+ | Apache-2.0 |
| **CoreDNS** | 集群 DNS | Graduated | v1.12.0 | 11k+ | Apache-2.0 |
| **kube-scheduler** | 默认调度器 | K8s 核心 | v1.33.0 | - | Apache-2.0 |
| **kube-controller-manager** | 核心控制器 | K8s 核心 | v1.33.0 | - | Apache-2.0 |
| **cloud-controller-manager** | 云厂商控制器 | K8s 核心 | v1.33.0 | - | Apache-2.0 |
| **Konnectivity** | API Server 网络代理 | K8s SIG | v0.31.0 | 1k+ | Apache-2.0 |
| **apiserver-builder** | 聚合 API 构建框架 | K8s SIG | v2.0.0 | 600+ | Apache-2.0 |
| **Kubeadm** | 控制平面部署工具 | K8s SIG | v1.33.0 | - | Apache-2.0 |
| **kOps** | 生产级集群运维 | K8s SIG | v1.31.0 | 15k+ | Apache-2.0 |

---

<!-- chunk: 参考链接 -->
## 参考链接

- [K8s 控制平面文档](https://kubernetes.io/docs/concepts/overview/components/)
- [etcd 文档](https://etcd.io/docs/)
- [CoreDNS 文档](https://coredns.io/manual/toc/)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- [[domain-01-cluster-fundamentals/MOC.md|domain-01-cluster-fundamentals MOC]]
- [[domain-01-cluster-fundamentals/README.md|Domain-3: Kubernetes控制平面]]
- [[domain-01-cluster-fundamentals/01-plane-architecture-overview.md|Kubernetes 控制平面架构总览 (Control Plane Architecture Overview)]]
- [[domain-01-cluster-fundamentals/02-plane-components-interaction.md|控制平面组件交互详解 (Control Plane Components Interaction Deep Dive)]]
- [[domain-01-cluster-fundamentals/03-plane-high-availability.md|控制平面高可用部署模式 (Control Plane High Availability Deployment Patt...]]
- [[domain-01-cluster-fundamentals/04-plane-security-hardening.md|控制平面安全加固指南 (Control Plane Security Hardening Guide)]]
- [[domain-01-cluster-fundamentals/05-plane-monitoring-observability.md|控制平面监控与可观测性 (Control Plane Monitoring & Observability)]]
- [[domain-01-cluster-fundamentals/06-plane-troubleshooting.md|控制平面故障排查手册 (Control Plane Troubleshooting Handbook)]]
- [[domain-01-cluster-fundamentals/07-plane-upgrade-migration.md|控制平面升级与迁移策略 (Control Plane Upgrade & Migration Strategy)]]
- [[domain-01-cluster-fundamentals/08-plane-performance-benchmarking.md|控制平面性能基准测试 (Control Plane Performance Benchmarking)]]
- [[domain-01-cluster-fundamentals/09-plane-scalability-guide.md|控制平面扩缩容指南 (Control Plane Scalability Guide)]]
- [[domain-01-cluster-fundamentals/10-plane-backup-disaster-recovery.md|控制平面备份与灾备方案 (Control Plane Backup & Disaster Recovery)]]
