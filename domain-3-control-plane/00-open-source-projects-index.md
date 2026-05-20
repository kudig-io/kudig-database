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
cross_refs:
- type: domain
  path: ../domain-2-design-principles/
  label: '相关知识域: domain-2-design-principles'
- type: domain
  path: ../domain-4-workloads/
  label: '相关知识域: domain-4-workloads'
- type: domain
  path: ../domain-5-networking/
  label: '相关知识域: domain-5-networking'
- type: domain
  path: ../domain-6-storage/
  label: '相关知识域: domain-6-storage'
- type: domain
  path: ../domain-7-security/
  label: '相关知识域: domain-7-security'
- type: cheatsheet
  path: ../topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
---

# Domain-3 控制平面 — 开源项目索引

> **最后更新**: 2026-04-24

---

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

## 参考链接

- [K8s 控制平面文档](https://kubernetes.io/docs/concepts/overview/components/)
- [etcd 文档](https://etcd.io/docs/)
- [CoreDNS 文档](https://coredns.io/manual/toc/)
