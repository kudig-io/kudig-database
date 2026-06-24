---
title: topic-functions MOC
description: topic-functions 专题导航页，覆盖 82 篇文档
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
created: "2026-05-23"
---

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
| 1 | [[domain-02-workloads-applications/topic-functions/cluster-cert/01-pki-architecture|Kubernetes 集群 PKI 架构总览]] |  | reference, architecture |  |
| 2 | [[domain-02-workloads-applications/topic-functions/cluster-cert/02-ca-generation|CA 证书生成源码分析]] |  | reference |  |
| 3 | [[domain-02-workloads-applications/topic-functions/cluster-cert/03-apiserver-cert|API Server 证书生成源码分析]] |  | reference |  |
| 4 | [[domain-02-workloads-applications/topic-functions/cluster-cert/04-etcd-cert|etcd 证书体系源码分析]] |  | reference |  |
| 5 | [[domain-02-workloads-applications/topic-functions/cluster-cert/05-kubelet-cert|kubelet 证书与 CSR 机制源码分析]] |  | reference |  |
| 6 | [[domain-02-workloads-applications/topic-functions/cluster-cert/06-cert-rotation|证书轮换机制源码分析]] |  | reference |  |
| 7 | [[domain-02-workloads-applications/topic-functions/cluster-cert/07-service-account-keys|ServiceAccount 密钥对源码分析]] |  | reference |  |
| 8 | [[domain-02-workloads-applications/topic-functions/cluster-cert/08-rbac-mapping|证书身份到 RBAC 的映射关系]] |  | reference, rbac |  |
| 9 | [[domain-02-workloads-applications/topic-functions/cluster-cert/09-join-cert-flow|kubeadm join 证书分发流程]] |  | reference |  |
| 10 | [[domain-02-workloads-applications/topic-functions/cluster-cert/10-front-proxy-workflow|Front Proxy 聚合层证书工作流]] |  | reference |  |
| 11 | [[domain-02-workloads-applications/topic-functions/cluster-cert/11-apiserver-cert-flags|API Server 证书相关启动参数汇总]] |  | reference |  |
| 12 | [[domain-02-workloads-applications/topic-functions/cluster-cert/12-kubeconfig-certs|kubeconfig 中的证书嵌入逻辑]] |  | reference, configuration |  |
| 13 | [[domain-02-workloads-applications/topic-functions/cluster-cert/13-cert-config|kubeadm 配置对证书生成的影响]] |  | reference, configuration |  |
| 14 | [[domain-02-workloads-applications/topic-functions/cluster-cert/14-admission-webhook-certs|Admission Webhook 证书体系]] |  | reference |  |
| 15 | [[domain-02-workloads-applications/topic-functions/cluster-cert/15-cert-format-encoding|证书格式与编码详解]] |  | reference |  |
| 16 | [[domain-02-workloads-applications/topic-functions/cluster-cert/16-openssl-cookbook|OpenSSL 证书操作速查手册]] |  | reference |  |
| 17 | [[domain-02-workloads-applications/topic-functions/cluster-cert/17-pki-security-best-practices|Kubernetes PKI 安全最佳实践]] |  | reference, security, best-practice |  |
| 18 | [[domain-02-workloads-applications/topic-functions/cluster-create/01-overview|kubeadm init 集群初始化概览]] |  | reference, deep-dive |  |
| 19 | [[domain-02-workloads-applications/topic-functions/cluster-create/02-preflight|预检流程 (kubeadm preflight)]] |  | reference |  |
| 20 | [[domain-02-workloads-applications/topic-functions/cluster-create/03-certs|证书管理 (PKI Infrastructure)]] |  | reference |  |
| 21 | [[domain-02-workloads-applications/topic-functions/cluster-create/04-kubeconfig|kubeconfig 阶段 — Kubeconfig Generation 源码分析]] |  | reference, configuration |  |
| 22 | [[domain-02-workloads-applications/topic-functions/cluster-create/05-control-plane|控制面组件部署 (Static Pod Manifests)]] |  | reference |  |
| 23 | [[domain-02-workloads-applications/topic-functions/cluster-create/06-join|节点加入流程 (kubeadm join)]] |  | reference |  |
| 24 | [[domain-02-workloads-applications/topic-functions/cluster-create/07-etcd|etcd 静态 Pod 管理]] |  | reference |  |
| 25 | [[domain-02-workloads-applications/topic-functions/cluster-create/08-ha|高可用控制面搭建 — 源码分析]] |  | reference |  |
| 26 | [[domain-02-workloads-applications/topic-functions/cluster-create/09-upgrade|集群升级流程 (kubeadm upgrade)]] |  | reference, upgrade |  |
| 27 | [[domain-02-workloads-applications/topic-functions/cluster-create/10-cloud-comparison|云厂商方案与 kubeadm 对比]] |  | reference |  |
| 28 | [[domain-02-workloads-applications/topic-functions/cluster-create/11-advanced|集群新建进阶: 关键机制详解]] |  | reference |  |
| 29 | [[domain-02-workloads-applications/topic-functions/cluster-create/12-join-advanced|节点加入进阶: Discovery 与 TLS Bootstrap 详解]] |  | reference |  |
| 30 | [[domain-02-workloads-applications/topic-functions/cluster-create/13-etcd-advanced|etcd 进阶: HA 集群管理与性能调优]] |  | reference |  |
| 31 | [[domain-02-workloads-applications/topic-functions/cluster-create/14-ha-advanced|高可用进阶: 负载均衡与证书分发]] |  | reference |  |
| 32 | [[domain-02-workloads-applications/topic-functions/cluster-create/15-upgrade-advanced|集群升级进阶: 滚动升级与回滚策略]] |  | reference, upgrade |  |
| 33 | [[domain-02-workloads-applications/topic-functions/cluster-create/16-security|安全机制: ServiceAccount Token 与 Audit]] |  | reference, security |  |
| 34 | [[domain-02-workloads-applications/topic-functions/cluster-create/17-init-phases|init 阶段详解: mark-control-plane 与 upload-config]] |  | reference |  |
| 35 | [[domain-02-workloads-applications/topic-functions/cluster-create/18-cri-runtime|CRI 运行时管理 ([[container-runtime]] Interface)]] |  | reference |  |
| 36 | [[domain-02-workloads-applications/topic-functions/cluster-create/19-cni-networking|CNI 网络插件与集群网络]] |  | reference, networking |  |
| 37 | [[domain-02-workloads-applications/topic-functions/cluster-create/20-node-registration|Node 注册与 kubeadm token 详解]] |  | reference |  |
| 38 | [[domain-02-workloads-applications/topic-functions/cluster-create/21-kube-proxy|kube-proxy 与 Service 负载均衡]] |  | reference |  |
| 39 | [[domain-02-workloads-applications/topic-functions/cluster-create/22-storage-volumes|存储与卷管理]] |  | reference, storage |  |
| 40 | [[domain-02-workloads-applications/topic-functions/cluster-create/23-scheduler|kube-scheduler 调度详解]] |  | reference |  |
| 41 | [[domain-02-workloads-applications/topic-functions/cluster-create/24-what-kubeadm-does-not-install|kubeadm 不安装的组件 (What kubeadm Does Not Install)]] |  | reference, configuration |  |
| 42 | [[domain-02-workloads-applications/topic-functions/cluster-create/25-resource-management|资源管理与配额控制 (Resource Management)]] |  | reference |  |
| 43 | [[domain-02-workloads-applications/topic-functions/cluster-delete/01-overview|Kubernetes 集群删除逻辑 — 基于官方代码分析]] |  | reference, deep-dive |  |
| 44 | [[domain-02-workloads-applications/topic-functions/cluster-delete/02-reset|kubeadm reset 源码分析]] |  | reference |  |
| 45 | [[domain-02-workloads-applications/topic-functions/cluster-delete/03-delete-node|节点删除流程 — kubectl delete node 源码分析]] |  | reference |  |
| 46 | [[domain-02-workloads-applications/topic-functions/cluster-delete/04-cleanup|节点清理机制 — cleanup-node 源码分析]] |  | reference |  |
| 47 | [[domain-02-workloads-applications/topic-functions/cluster-delete/05-etcd-cleanup|etcd 数据清理与成员移除 — 源码分析]] |  | reference |  |
| 48 | [[domain-02-workloads-applications/topic-functions/cluster-delete/06-force-delete|强制删除与异常场景处理]] |  | reference |  |
| 49 | [[domain-02-workloads-applications/topic-functions/cluster-delete/07-ha-delete|HA 集群删除注意事项]] |  | reference |  |
| 50 | [[domain-02-workloads-applications/topic-functions/cluster-delete/08-cloud-delete|云厂商集群删除方案对比]] |  | reference |  |
| ... | 共 82 篇文档 | | | |

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 82 |

---

*本文档由 scripts/generate-[[MOC]]s.py 自动生成，最后更新 2026-05-21。*

## Related

- [[concepts/resource-management|resource-management]]
- [[entities/kubernetes|kubernetes]]
- [[entities/cni|cni]]
- [[entities/container-runtime|container-runtime]]

- [[MOC]]
- [[MOC]]
- [[_reports/WIKI-LINT-REPORT-2026-05-21|Wiki Lint Report — 2026-05-21]] — Cross-reference
- [[references/release-notes-storage|发布说明索引 — 存储]] — Cross-reference
- [[references/release-notes-observability|发布说明索引 — 可观测性]] — Cross-reference
- [[references/release-notes-networking|发布说明索引 — 网络]] — Cross-reference
- [[references/release-notes-kubernetes|发布说明索引 — Kubernetes]] — Cross-reference
- [[references/release-notes-security|发布说明索引 — 安全]] — Cross-reference
- [[references/k8s-knowledge-map|Kubernetes Knowledge Map]] — Cross-reference
- [[references/release-notes-cicd-gitops|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[references/release-notes-cli-tools|发布说明索引 — CLI 工具]] — Cross-reference
- [[references/release-notes-core-deps|发布说明索引 — 核心依赖]] — Cross-reference
- [[references/k8s-difficulty-index|Kubernetes Difficulty Index]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- [[domain-03-networking-traffic/00-core-k8s-networking/02-cni-architecture-fundamentals|CNI 架构与核心原理]] — Cross-reference
- [[domain-06-observability/01-overview/01-observability-architecture-overview|Kubernetes 可观测性架构体系]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[domain-01-cluster-fundamentals/05-kubectl/05-kubectl-commands-reference|kubectl 命令完整参考]] — Cross-reference
- [[domain-01-cluster-fundamentals/01-architecture-overview/02-core-components-deep-dive|Kubernetes 核心组件深度剖析]] — Cross-reference
- [[domain-04-storage-data/01-k8s-storage/02-pv-architecture-fundamentals|PV/PVC 核心概念与企业级实践]] — Cross-reference
- [[domain-04-storage-data/01-k8s-storage/01-storage-architecture-overview|存储架构概览与核心组件]] — Cross-reference
- [[synthesis/README|Synthesis 综合分析索引]] — Cross-reference
- [[docs/agent-specs/README|Agent Specs 索引]] — Cross-reference
- [[release-notes/README|Release Notes 索引]] — Cross-reference
- [[_reports/README|Reports 报告索引]] — Cross-reference
- [[assets/presentations/README|演示文稿索引]] — Cross-reference
- [[prompts/README|Prompts 索引]] — Cross-reference
- [[video-scripts/README|视频脚本索引]] — Cross-reference
- [[CONTRIBUTING|Contributing Guide]] — Cross-reference
- [[corpus-config/embedding-guide|Embedding Guide]] — Cross-reference
- [[docs/learning-paths/kubernetes-sre-engineer-learning-path|SRE 学习路径]] — Cross-reference
- [[_archives/README|Archives 归档索引]] — Cross-reference
- [[_meta/README|Meta 元数据索引]] — Cross-reference
- [[reports/README|Reports 报告索引]] — Cross-reference
- [[journal/digest-2026-05-23|Digest 2026-05-23]] — Cross-reference
- [[templates/moc-template|MOC Template]] — Cross-reference
- [[metadata/knowledge-map|Knowledge Map]] — Cross-reference
- [[references/release-notes-reading-guide|Release Notes Reading Guide]] — Cross-reference
