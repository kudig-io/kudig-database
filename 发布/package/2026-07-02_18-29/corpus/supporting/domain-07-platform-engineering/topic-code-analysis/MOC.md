---
title: topic-functions MOC
description: topic-functions 专题导航页，覆盖 82 篇文档
summary: topic-functions 专题导航页，覆盖 82 篇文档
category: moc
tags:
- k8s
- moc
- reference
- etcd
- apiserver
- kubelet
- scheduler
- rbac
- webhook
- rag
tier: supporting
created: '2026-05-23'
last_updated: '2026-05-21'
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- topic-functions MOC 是什么
- 如何 topic-functions MOC
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- topic-functions
- MOC
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- etcd-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# topic-functions [[MOC|MOC]]

> **[[MOC]] 版本**: 1.0
> **专题**: topic-functions
> **文档数量**: 82 篇
> **最后更新**: 2026-05-21
> **用途**: 本专题的导航入口，汇总所有相关文档

---

## 专题概述

函数 — 运维脚本常用函数库

### 专题定位

| 维度 | 说明 |
|---|---|
| **专题** | topic-functions |
| **文档数量** | 82 篇（展示前 50 篇） |
| **难度分布** | 入门 0 / 进阶 1 / 高级 6 / 专家 0 |

---

## 文档清单

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|
| 1 | [[domain-02-workloads-applications/topic-functions/cluster-cert/01-pki-architecture.md|Kubernetes 集群 PKI 架构总览]] |  | reference, architecture |  |
| 2 | [[domain-02-workloads-applications/topic-functions/cluster-cert/02-ca-generation.md|CA 证书生成源码分析]] |  | reference |  |
| 3 | [[domain-02-workloads-applications/topic-functions/cluster-cert/03-apiserver-cert.md|API Server 证书生成源码分析]] |  | reference |  |
| 4 | [[domain-02-workloads-applications/topic-functions/cluster-cert/04-etcd-cert.md|etcd 证书体系源码分析]] |  | reference |  |
| 5 | [[domain-02-workloads-applications/topic-functions/cluster-cert/05-kubelet-cert.md|kubelet 证书与 CSR 机制源码分析]] |  | reference |  |
| 6 | [[domain-02-workloads-applications/topic-functions/cluster-cert/06-cert-rotation.md|证书轮换机制源码分析]] |  | reference |  |
| 7 | [[domain-02-workloads-applications/topic-functions/cluster-cert/07-service-account-keys.md|ServiceAccount 密钥对源码分析]] |  | reference |  |
| 8 | [[domain-02-workloads-applications/topic-functions/cluster-cert/08-rbac-mapping.md|证书身份到 RBAC 的映射关系]] |  | reference, rbac |  |
| 9 | [[domain-02-workloads-applications/topic-functions/cluster-cert/09-join-cert-flow.md|kubeadm join 证书分发流程]] |  | reference |  |
| 10 | [[domain-02-workloads-applications/topic-functions/cluster-cert/10-front-proxy-workflow.md|Front Proxy 聚合层证书工作流]] |  | reference |  |
| 11 | [[domain-02-workloads-applications/topic-functions/cluster-cert/11-apiserver-cert-flags.md|API Server 证书相关启动参数汇总]] |  | reference |  |
| 12 | [[domain-02-workloads-applications/topic-functions/cluster-cert/12-kubeconfig-certs.md|kubeconfig 中的证书嵌入逻辑]] |  | reference, configuration |  |
| 13 | [[domain-02-workloads-applications/topic-functions/cluster-cert/13-cert-config.md|kubeadm 配置对证书生成的影响]] |  | reference, configuration |  |
| 14 | [[domain-02-workloads-applications/topic-functions/cluster-cert/14-admission-webhook-certs.md|Admission Webhook 证书体系]] |  | reference |  |
| 15 | [[domain-02-workloads-applications/topic-functions/cluster-cert/15-cert-format-encoding.md|证书格式与编码详解]] |  | reference |  |
| 16 | [[domain-02-workloads-applications/topic-functions/cluster-cert/16-openssl-cookbook.md|OpenSSL 证书操作速查手册]] |  | reference |  |
| 17 | [[domain-02-workloads-applications/topic-functions/cluster-cert/17-pki-security-best-practices.md|Kubernetes PKI 安全最佳实践]] |  | reference, security, best-practice |  |
| 18 | [[domain-02-workloads-applications/topic-functions/cluster-create/01-overview.md|kubeadm init 集群初始化概览]] |  | reference, deep-dive |  |
| 19 | [[domain-02-workloads-applications/topic-functions/cluster-create/02-preflight.md|预检流程 (kubeadm preflight)]] |  | reference |  |
| 20 | [[domain-02-workloads-applications/topic-functions/cluster-create/03-certs.md|证书管理 (PKI Infrastructure)]] |  | reference |  |
| 21 | [[domain-02-workloads-applications/topic-functions/cluster-create/04-kubeconfig.md|kubeconfig 阶段 — Kubeconfig Generation 源码分析]] |  | reference, configuration |  |
| 22 | [[domain-02-workloads-applications/topic-functions/cluster-create/05-control-plane.md|控制面组件部署 (Static Pod Manifests)]] |  | reference |  |
| 23 | [[domain-02-workloads-applications/topic-functions/cluster-create/06-join.md|节点加入流程 (kubeadm join)]] |  | reference |  |
| 24 | [[domain-02-workloads-applications/topic-functions/cluster-create/07-etcd.md|etcd 静态 Pod 管理]] |  | reference |  |
| 25 | [[domain-02-workloads-applications/topic-functions/cluster-create/08-ha.md|高可用控制面搭建 — 源码分析]] |  | reference |  |
| 26 | [[domain-02-workloads-applications/topic-functions/cluster-create/09-upgrade.md|集群升级流程 (kubeadm upgrade)]] |  | reference, upgrade |  |
| 27 | [[domain-02-workloads-applications/topic-functions/cluster-create/10-cloud-comparison.md|云厂商方案与 kubeadm 对比]] |  | reference |  |
| 28 | [[domain-02-workloads-applications/topic-functions/cluster-create/11-advanced.md|集群新建进阶: 关键机制详解]] |  | reference |  |
| 29 | [[domain-02-workloads-applications/topic-functions/cluster-create/12-join-advanced.md|节点加入进阶: Discovery 与 TLS Bootstrap 详解]] |  | reference |  |
| 30 | [[domain-02-workloads-applications/topic-functions/cluster-create/13-etcd-advanced.md|etcd 进阶: HA 集群管理与性能调优]] |  | reference |  |
| 31 | [[domain-02-workloads-applications/topic-functions/cluster-create/14-ha-advanced.md|高可用进阶: 负载均衡与证书分发]] |  | reference |  |
| 32 | [[domain-02-workloads-applications/topic-functions/cluster-create/15-upgrade-advanced.md|集群升级进阶: 滚动升级与回滚策略]] |  | reference, upgrade |  |
| 33 | [[domain-02-workloads-applications/topic-functions/cluster-create/16-security.md|安全机制: ServiceAccount Token 与 Audit]] |  | reference, security |  |
| 34 | [[domain-02-workloads-applications/topic-functions/cluster-create/17-init-phases.md|init 阶段详解: mark-control-plane 与 upload-config]] |  | reference |  |
| 35 | [[domain-02-workloads-applications/topic-functions/cluster-create/18-cri-runtime.md|CRI 运行时管理 (Container Runtime Interface)]] |  | reference |  |
| 36 | [[domain-02-workloads-applications/topic-functions/cluster-create/19-cni-networking.md|CNI 网络插件与集群网络]] |  | reference, networking |  |
| 37 | [[domain-02-workloads-applications/topic-functions/cluster-create/20-node-registration.md|Node 注册与 kubeadm token 详解]] |  | reference |  |
| 38 | [[domain-02-workloads-applications/topic-functions/cluster-create/21-kube-proxy.md|kube-proxy 与 Service 负载均衡]] |  | reference |  |
| 39 | [[domain-02-workloads-applications/topic-functions/cluster-create/22-storage-volumes.md|存储与卷管理]] |  | reference, storage |  |
| 40 | [[domain-02-workloads-applications/topic-functions/cluster-create/23-scheduler.md|kube-scheduler 调度详解]] |  | reference |  |
| 41 | [[domain-02-workloads-applications/topic-functions/cluster-create/24-what-kubeadm-does-not-install.md|kubeadm 不安装的组件 (What kubeadm Does Not Install)]] |  | reference, configuration |  |
| 42 | [[domain-02-workloads-applications/topic-functions/cluster-create/25-resource-management.md|资源管理与配额控制 (Resource Management)]] |  | reference |  |
| 43 | [[domain-02-workloads-applications/topic-functions/cluster-delete/01-overview.md|Kubernetes 集群删除逻辑 — 基于官方代码分析]] |  | reference, deep-dive |  |
| 44 | [[domain-02-workloads-applications/topic-functions/cluster-delete/02-reset.md|kubeadm reset 源码分析]] |  | reference |  |
| 45 | [[domain-02-workloads-applications/topic-functions/cluster-delete/03-delete-node.md|节点删除流程 — kubectl delete node 源码分析]] |  | reference |  |
| 46 | [[domain-02-workloads-applications/topic-functions/cluster-delete/04-cleanup.md|节点清理机制 — cleanup-node 源码分析]] |  | reference |  |
| 47 | [[domain-02-workloads-applications/topic-functions/cluster-delete/05-etcd-cleanup.md|etcd 数据清理与成员移除 — 源码分析]] |  | reference |  |
| 48 | [[domain-02-workloads-applications/topic-functions/cluster-delete/06-force-delete.md|强制删除与异常场景处理]] |  | reference |  |
| 49 | [[domain-02-workloads-applications/topic-functions/cluster-delete/07-ha-delete.md|HA 集群删除注意事项]] |  | reference |  |
| 50 | [[domain-02-workloads-applications/topic-functions/cluster-delete/08-cloud-delete.md|云厂商集群删除方案对比]] |  | reference |  |
| ... | 共 82 篇文档 | | | |

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 82 |

---

*本文档由 scripts/generate-[[MOC]]s.py 自动生成，最后更新 2026-05-21。*

## Related

- [[reference|#reference Hub]] — tag hub

- [[concepts/resource-management.md|resource-management]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/cni.md|cni]]
- [[entities/container-runtime.md|container-runtime]]

- [[MOC]]
- [[MOC]]
- Wiki Lint Report — 2026-05-21 — Cross-reference
- [[entities/release-notes-storage.md|发布说明索引 — 存储]] — Cross-reference
- [[entities/release-notes-observability.md|发布说明索引 — 可观测性]] — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- [[entities/release-notes-kubernetes.md|发布说明索引 — Kubernetes]] — Cross-reference
- [[entities/release-notes-security.md|发布说明索引 — 安全]] — Cross-reference
- [[entities/k8s-knowledge-map.md|Kubernetes Knowledge Map]] — Cross-reference
- [[entities/release-notes-cicd-gitops.md|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[entities/release-notes-cli-tools.md|发布说明索引 — CLI 工具]] — Cross-reference
- [[entities/release-notes-core-deps.md|发布说明索引 — 核心依赖]] — Cross-reference
- [[entities/k8s-difficulty-index.md|Kubernetes Difficulty Index]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- [[domain-03-networking-traffic/K8s网络核心/02-cni-architecture-fundamentals.md|CNI 架构与核心原理]] — Cross-reference
- [[domain-06-observability/总览/01-observability-architecture-overview.md|Kubernetes 可观测性架构体系]] — Cross-reference
- [[domain-14-ai-ml-infra/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[domain-01-cluster-fundamentals/kubectl/05-kubectl-commands-reference.md|kubectl 命令完整参考]] — Cross-reference
- [[domain-01-cluster-fundamentals/架构总览/02-core-components-deep-dive.md|Kubernetes 核心组件深度剖析]] — Cross-reference
- [[domain-04-storage-data/K8s存储/02-pv-architecture-fundamentals.md|PV/PVC 核心概念与企业级实践]] — Cross-reference
- [[domain-04-storage-data/K8s存储/01-storage-architecture-overview.md|存储架构概览与核心组件]] — Cross-reference


<!-- risk-assessed -->
