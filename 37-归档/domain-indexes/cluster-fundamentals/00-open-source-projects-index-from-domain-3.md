---
title: Domain-3 控制平面 — 开源项目索引
description: '| **etcd** | 控制平面数据存储 | Graduated | v3.5.21 | 48k+ | Apache-2.0 |'
summary: '| **etcd** | 控制平面数据存储 | Graduated | v3.5.21 | 48k+ | Apache-2.0 |'
category: control-plane
tags:
- k8s
- control-plane
- etcd
- apiserver
- scheduler
- controller-manager
- coredns
tier: peripheral
created: '2026-05-23'
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
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../工作负载/
  label: '相关知识域: 工作负载'
- type: domain
  path: ../网络/
  label: '相关知识域: 网络'
- type: domain
  path: ../存储/
  label: '相关知识域: 存储'
- type: domain
  path: ../安全/
  label: '相关知识域: 安全'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

- 集群基础 MOC
- [[01-集群基础/README.md|Domain-3: Kubernetes控制平面]]
- Kubernetes 控制平面架构总览 (Control Plane Architecture Overview)
- 控制平面组件交互详解 (Control Plane Components Interaction Deep Dive)
- 控制平面高可用部署模式 (Control Plane High Availability Deployment Patt...
- 控制平面安全加固指南 (Control Plane Security Hardening Guide)
- 控制平面监控与可观测性 (Control Plane Monitoring & Observability)
- 控制平面故障排查手册 (Control Plane Troubleshooting Handbook)
- 控制平面升级与迁移策略 (Control Plane Upgrade & Migration Strategy)
- 控制平面性能基准测试 (Control Plane Performance Benchmarking)
- 控制平面扩缩容指南 (Control Plane Scalability Guide)
- 控制平面备份与灾备方案 (Control Plane Backup & Disaster Recovery)


<!-- risk-assessed -->
