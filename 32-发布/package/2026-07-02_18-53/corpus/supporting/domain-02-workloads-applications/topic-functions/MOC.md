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
| 2 | [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-cert/01-ca-generation|CA 证书生成源码分析]] |  | reference |  |
| 3 | [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-cert/02-apiserver-cert|API Server 证书生成源码分析]] |  | reference |  |
| 4 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-07-platform-engineering/topic-code-analysis/cluster-cert/02-etcd-cert|etcd 证书体系源码分析]] |  | reference |  |
| 5 | [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-cert/03-kubelet-cert|kubelet 证书与 CSR 机制源码分析]] |  | reference |  |
| 6 | [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-cert/04-cert-rotation|证书轮换机制源码分析]] |  | reference |  |
| 7 | [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-cert/05-service-account-keys|ServiceAccount 密钥对源码分析]] |  | reference |  |
| 8 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-07-platform-engineering/topic-code-analysis/cluster-cert/03-rbac-mapping|证书身份到 RBAC 的映射关系]] |  | reference, rbac |  |
| 9 | [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-cert/06-join-cert-flow|kubeadm join 证书分发流程]] |  | reference |  |
| 10 | [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-cert/07-front-proxy-workflow|Front Proxy 聚合层证书工作流]] |  | reference |  |
| 11 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-07-platform-engineering/topic-code-analysis/cluster-cert/04-apiserver-cert-flags|API Server 证书相关启动参数汇总]] |  | reference |  |
| 12 | [[32-发布/package/2026-07-02_18-53/corpus/core/domain-07-platform-engineering/topic-code-analysis/cluster-cert/01-kubeconfig-certs|kubeconfig 中的证书嵌入逻辑]] |  | reference, configuration |  |
| 13 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-07-platform-engineering/topic-code-analysis/cluster-cert/05-cert-config|kubeadm 配置对证书生成的影响]] |  | reference, configuration |  |
| 14 | [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-cert/08-admission-webhook-certs|Admission Webhook 证书体系]] |  | reference |  |
| 15 | [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-cert/09-cert-format-encoding|证书格式与编码详解]] |  | reference |  |
| 16 | [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-cert/10-openssl-cookbook|OpenSSL 证书操作速查手册]] |  | reference |  |
| 17 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-07-platform-engineering/topic-code-analysis/cluster-cert/06-pki-security-best-practices|Kubernetes PKI 安全最佳实践]] |  | reference, security, best-practice |  |
| 18 | [[domain-02-workloads-applications/topic-functions/cluster-create/01-overview.md|kubeadm init 集群初始化概览]] |  | reference, deep-dive |  |
| 19 | [[domain-02-workloads-applications/topic-functions/cluster-create/02-preflight.md|预检流程 (kubeadm preflight)]] |  | reference |  |
| 20 | [[domain-02-workloads-applications/topic-functions/cluster-create/03-certs.md|证书管理 (PKI Infrastructure)]] |  | reference |  |
| 21 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-07-platform-engineering/topic-code-analysis/cluster-create/01-kubeconfig|kubeconfig 阶段 — Kubeconfig Generation 源码分析]] |  | reference, configuration |  |
| 22 | [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-create/04-control-plane|控制面组件部署 (Static Pod Manifests)]] |  | reference |  |
| 23 | [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-create/05-join|节点加入流程 (kubeadm join)]] |  | reference |  |
| 24 | [[32-发布/package/2026-07-02_18-53/corpus/core/domain-02-workloads-applications/topic-functions/cluster-create/01-etcd|etcd 静态 Pod 管理]] |  | reference |  |
| 25 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-02-workloads-applications/topic-functions/cluster-create/07-ha|高可用控制面搭建 — 源码分析]] |  | reference |  |
| 26 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-02-workloads-applications/topic-functions/cluster-create/08-upgrade|集群升级流程 (kubeadm upgrade)]] |  | reference, upgrade |  |
| 27 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-02-workloads-applications/topic-functions/cluster-create/09-cloud-comparison|云厂商方案与 kubeadm 对比]] |  | reference |  |
| 28 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-02-workloads-applications/topic-functions/cluster-create/10-advanced|集群新建进阶: 关键机制详解]] |  | reference |  |
| 29 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-02-workloads-applications/topic-functions/cluster-create/11-join-advanced|节点加入进阶: Discovery 与 TLS Bootstrap 详解]] |  | reference |  |
| 30 | [[32-发布/package/2026-07-02_18-53/corpus/core/domain-02-workloads-applications/topic-functions/cluster-create/02-etcd-advanced|etcd 进阶: HA 集群管理与性能调优]] |  | reference |  |
| 31 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-02-workloads-applications/topic-functions/cluster-create/12-ha-advanced|高可用进阶: 负载均衡与证书分发]] |  | reference |  |
| 32 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-02-workloads-applications/topic-functions/cluster-create/13-upgrade-advanced|集群升级进阶: 滚动升级与回滚策略]] |  | reference, upgrade |  |
| 33 | [[32-发布/package/2026-07-02_18-53/corpus/core/domain-02-workloads-applications/topic-functions/cluster-create/03-security|安全机制: ServiceAccount Token 与 Audit]] |  | reference, security |  |
| 34 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-02-workloads-applications/topic-functions/cluster-create/14-init-phases|init 阶段详解: mark-control-plane 与 upload-config]] |  | reference |  |
| 35 | [[domain-02-workloads-applications/topic-functions/cluster-create/18-cri-runtime.md|CRI 运行时管理 ([[container-runtime]] Interface)]] |  | reference |  |
| 36 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-02-workloads-applications/topic-functions/cluster-create/16-cni-networking|CNI 网络插件与集群网络]] |  | reference, networking |  |
| 37 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-02-workloads-applications/topic-functions/cluster-create/17-node-registration|Node 注册与 kubeadm token 详解]] |  | reference |  |
| 38 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-02-workloads-applications/topic-functions/cluster-create/18-kube-proxy|kube-proxy 与 Service 负载均衡]] |  | reference |  |
| 39 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-02-workloads-applications/topic-functions/cluster-create/19-storage-volumes|存储与卷管理]] |  | reference, storage |  |
| 40 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-02-workloads-applications/topic-functions/cluster-create/20-scheduler|kube-scheduler 调度详解]] |  | reference |  |
| 41 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-02-workloads-applications/topic-functions/cluster-create/21-what-kubeadm-does-not-install|kubeadm 不安装的组件 (What kubeadm Does Not Install)]] |  | reference, configuration |  |
| 42 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-02-workloads-applications/topic-functions/cluster-create/22-resource-management|资源管理与配额控制 (Resource Management)]] |  | reference |  |
| 43 | [[domain-02-workloads-applications/topic-functions/cluster-delete/01-overview.md|Kubernetes 集群删除逻辑 — 基于官方代码分析]] |  | reference, deep-dive |  |
| 44 | [[domain-02-workloads-applications/topic-functions/cluster-delete/02-reset.md|kubeadm reset 源码分析]] |  | reference |  |
| 45 | [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-delete/01-delete-node|节点删除流程 — kubectl delete node 源码分析]] |  | reference |  |
| 46 | [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-delete/02-cleanup|节点清理机制 — cleanup-node 源码分析]] |  | reference |  |
| 47 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-07-platform-engineering/topic-code-analysis/cluster-delete/03-etcd-cleanup|etcd 数据清理与成员移除 — 源码分析]] |  | reference |  |
| 48 | [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-delete/03-force-delete|强制删除与异常场景处理]] |  | reference |  |
| 49 | [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-07-platform-engineering/topic-code-analysis/cluster-delete/04-ha-delete|HA 集群删除注意事项]] |  | reference |  |
| 50 | [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-07-platform-engineering/topic-code-analysis/cluster-delete/04-cloud-delete|云厂商集群删除方案对比]] |  | reference |  |
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
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-03-networking-traffic/00-core-k8s-networking/01-cni-architecture-fundamentals|CNI 架构与核心原理]] — Cross-reference
- [[domain-06-observability/总览/01-observability-architecture-overview.md|Kubernetes 可观测性架构体系]] — Cross-reference
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-14-ai-ml-infra/01-ai-infra/01-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-14-ai-ml-infra/01-ai-infra/02-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-01-cluster-fundamentals/04-kubectl/01-kubectl-commands-reference|kubectl 命令完整参考]] — Cross-reference
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-01-cluster-fundamentals/01-architecture-overview/01-core-components-deep-dive|Kubernetes 核心组件深度剖析]] — Cross-reference
- [[domain-04-storage-data/K8s存储/02-pv-architecture-fundamentals.md|PV/PVC 核心概念与企业级实践]] — Cross-reference
- [[domain-04-storage-data/K8s存储/01-storage-architecture-overview.md|存储架构概览与核心组件]] — Cross-reference
- [[concepts/README.md|Synthesis 综合分析索引]] — Cross-reference
- [[docs/agent-specs/README.md|Agent Specs 索引]] — Cross-reference
- Release Notes 索引 — Cross-reference
- Reports 报告索引 — Cross-reference
- [[assets/presentations/README.md|演示文稿索引]] — Cross-reference
- [[scripts/prompts/README.md|Prompts 索引]] — Cross-reference
- [[scripts/video-scripts/README.md|视频脚本索引]] — Cross-reference
- [[CONTRIBUTING|Contributing Guide]] — Cross-reference
- _meta/corpus-config/embedding-guide.md — Cross-reference
- [[docs/learning-paths/kubernetes-sre-engineer-learning-path.md|SRE 学习路径]] — Cross-reference
- [[_archives/README.md|Archives 归档索引]] — Cross-reference
- _meta/README.md — Cross-reference
- Reports 报告索引 — Cross-reference
- Digest 2026-05-23 — Cross-reference
- [[scripts/templates/moc-template.md|MOC Template]] — Cross-reference
- _meta/metadata/knowledge-map.md — Cross-reference
- [[entities/release-notes-reading-guide.md|Release Notes Reading Guide]] — Cross-reference


<!-- risk-assessed -->
