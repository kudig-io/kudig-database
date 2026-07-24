---
title: '[[deployment]] × Secret 管理'
description: 'title: Deployment × Secret 管理'
summary: 'title: Deployment × Secret 管理'
category: general
tags:
- k8s
- etcd
- docker
- ingress
- rbac
- operator
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- '[[deployment]] × Secret 管理 是什么'
- 如何 [[deployment]] × Secret 管理
trigger_keywords:
- '[[deployment]]'
- Secret
- 管理
prerequisites:
- kubectl-basics
- etcd-basics
relationships:
- target: '[[实体/etcd.md]]'
  type: uses
- target: '[[实体/external-secrets.md]]'
  type: uses
- target: '[[系统基础/知识字典/networking/ingress.md]]'
  type: uses
- target: '[[实体/kubernetes.md]]'
  type: uses
- target: '[[最佳实践/best-practices/security/pod-security.md]]'
  type: uses
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Deployment × Secret 管理
category: synthesis
tags:
- k8s
- deployment
- secrets
- security
- workloads
- rbac
sources:
- entities/deployment.md
- concepts/secrets-management.md
- concepts/pod-lifecycle.md
- entities/vault.md
created: 2026-05-21 16:07:01+08:00
updated: '2026-05-21T16:10:00Z'
summary: "Deployment 是 Secret 最频繁使用者，注入方式决定工作负载安全 posture。两者结合构成 K8s 应用安全基线，涵盖镜像拉取、配置注入、Vault 动态凭证。"
  posture。两者结合构成了 K8s 应用安全的基线。
provenance:
  extracted: 0.2
  inferred: 0.7
  ambiguous: 0.1
base_confidence: 0.88
lifecycle: reviewed
lifecycle_changed: 2026-05-21
tier: supporting
relationships:
- target: "[[实体/deployment.md|deployment]]"
  type: uses
---


# [[deployment]] × Secret 管理

## 连接点

[[deployment]] 是 K8s 中最常用的无状态工作负载控制器，[[secrets-management]] 覆盖密钥的安全存储与分发。wiki 将两者分属不同章节，但在生产环境中它们**不可分割**：几乎每个 Deployment 都需要 Secret——镜像拉取凭证、数据库密码、API 密钥、TLS 证书。Deployment 是 Secret 最频繁的使用者，而 Secret 的注入方式（环境变量 vs 挂载卷、不可变 Secret vs 动态凭证）直接决定了工作负载的安全 posture。

两者的结合点不是简单的"使用关系"，而是**安全与可用性的永恒张力**：
- 为了可用性，Secret 需要被 Pod 快速访问（环境变量读取最快）
- 为了安全，Secret 不应该被写入容器文件系统或暴露在进程环境中（内存卷挂载更安全）
- 为了可维护性，Secret 变更后需要自动同步到运行中的 Pod（但 K8s 默认不自动重新加载 Secret）

## 共现场景

两者在以下场景中共现：

- **镜像拉取 Secret**：Deployment 的 imagePullSecrets 引用 Docker registry 凭证，这是 Secret 最基础的用途。私有镜像仓库的认证直接绑定到 Deployment 的 Pod 模板
- **应用配置 Secret**：Deployment 将数据库连接字符串、API 密钥作为环境变量注入容器。这是最常见的模式，也是最容易泄露的模式（环境变量会出现在进程列表、core dump、应用日志中）
- **TLS 证书挂载**：[[系统基础/知识字典/networking/ingress.md|Ingress]] Controller 或 API 服务的 Deployment 通过 Secret 卷挂载 TLS 证书。证书到期后需要滚动更新 Deployment 才能重新加载
- **Vault Agent Sidecar**：Deployment 的 Pod 模板中注入 Vault Agent Sidecar，将动态凭证以内存卷形式挂载到应用容器。这是生产环境推荐的模式，但增加了 Pod 复杂度和启动延迟
- **[[实体/external-secrets.md|External Secrets]] Operator**：Deployment 引用由 ESO 自动同步的 K8s Secret，将外部密钥管理（Vault、AWS Secrets Manager）与 Deployment 的声明式配置解耦

## 交叉洞察

**核心洞察：Deployment 的 Secret 使用模式是评估集群安全成熟度的最佳指标。**

一个集群中 Deployment 引用 Secret 的方式，直接反映了该组织的安全意识和工程成熟度：

| 成熟度级别 | Secret 注入方式 | 风险等级 | 典型特征 |
|-----------|----------------|---------|---------|
| **L0（危险）** | 环境变量注入明文密码 | 高 | Secret 出现在进程环境、core dump、日志中 |
| **L1（基础）** | 环境变量引用 K8s Secret | 中 | Secret 值通过 envFrom 注入，但仍暴露在进程环境 |
| **L2（标准）** | 卷挂载 K8s Secret | 低 | Secret 以文件形式存在，不暴露在进程环境，但仍写入 tmpfs |
| **L3（高级）** | Vault Agent Sidecar + 内存卷 | 很低 | 动态凭证、短期 TTL、不落地磁盘 |
| **L4（最佳）** | Workload Identity + 无 Secret | 最低 | Pod 通过身份直接获取凭证（如 GCP Workload Identity、AWS IRSA） |

**Deployment 滚动更新与 Secret 加载的耦合困境：**

K8s Secret 被更新后，已运行的 Pod 不会自动感知变更。这意味着：
- 如果数据库密码被轮换，引用该 Secret 的 Deployment 必须执行滚动更新才能加载新密码
- 滚动更新的速度受 maxSurge/maxUnavailable 限制，大规模 Deployment（数千 Pod）的完全更新可能需要数分钟
- 在这数分钟内，旧 Pod 使用旧密码，新 Pod 使用新密码——如果数据库端同时切换密码，服务将中断

**解决方案不是技术性的，而是流程性的**：Secret 轮换必须与 Deployment 滚动更新协调。Vault 的动态凭证模式（每次请求生成新凭证）从根本上解决了这个问题——不需要更新 Secret，因为 Secret 本身就是临时的。

## 张力与权衡

| 张力 | 详情 |
|------|------|
| **环境变量 vs 卷挂载** | 环境变量读取方便（直接作为进程环境变量），但会暴露在 /proc/<pid>/environ 和 core dump 中。卷挂载更安全（不暴露在进程环境），但应用需要修改代码读取文件路径 |
| **不可变 Secret 的困境** | K8s v1.21 引入不可变 Secret（immutable: true）可以提升性能和安全性，但意味着 Secret 一旦创建就不能修改——任何变更都需要创建新 Secret 并更新 Deployment 引用 |
| **镜像拉取 Secret 的命名空间限制** | imagePullSecrets 是 Pod 级别的，每个命名空间需要独立的镜像拉取 Secret。在多租户集群中，这导致 Secret 的重复创建和同步开销 |
| **RBAC 粒度与运维效率** | 最小权限原则要求每个 Deployment 的 ServiceAccount 只能访问其所需的 Secret。但在微服务架构中，这导致大量的 Role 和 RoleBinding，增加管理复杂度 |
| **Secret 大小限制** | K8s Secret 大小限制为 1MB（[[实体/etcd.md|etcd]] 的 value 大小限制）。大型 TLS 证书链或 CA 捆绑包可能超过此限制，需要拆分为多个 Secret 或使用 ConfigMap |

## 开放问题

- **Secret 热加载**：K8s 不支持 Secret 变更后自动重新加载到运行中的 Pod。社区方案（如 Reloader、Stakater）通过监听 Secret 变更触发 Deployment 滚动更新，但这本质上是 workaround。K8s 是否应该原生支持 Secret 的透明热加载？
- **Secret 的 GitOps 困境**：GitOps 要求所有配置存储在 Git 中，但 Secret 不应该以明文形式提交。Sealed Secrets、SOPS、External Secrets Operator 等方案各有取舍，但没有一个成为事实标准。GitOps 工作流中的 Secret 管理最佳实践是什么？
- **Deployment 的 Secret 引用审计**：如何审计一个集群中所有 Deployment 引用了哪些 Secret？kubectl get deployments -A 不直接显示 Secret 引用，需要遍历 Pod 模板。生产环境是否应该有自动化的 Secret 引用图谱？
- **跨命名空间 Secret 引用**：K8s 不支持跨命名空间引用 Secret。ServiceAccount 的 imagePullSecrets 和 Pod 的 envFrom 都只能引用同命名空间的 Secret。跨命名共享 Secret 需要复制或使用 External Secrets Operator，增加了复杂度
- **Secret 与 Pod 安全标准的冲突**：[[最佳实践/best-practices/security/pod-security.md|Pod Security]] Standards（Restricted）禁止以 root 运行、要求只读根文件系统。但某些旧版应用读取 Secret 文件时需要特定权限，导致安全策略与应用需求的冲突

## 相关

- [[deployment]]
- [[secrets-management]]
- [[vault]]
- [[pod-lifecycle]]
- [[security-defense-depth]]
- [[k8s-pod-security-guide]]

> *This page synthesizes patterns across multiple sources and domains.* ^[inferred]

## Related

- [[deployment]]
- [[实体/k8s-workloads-domain-guide.md|Kubernetes Workloads Domain Guide]] — Cross-reference
- [[实体/kubernetes.md|kubernetes]]-events/13-security-admission-rbac-events|13 - 安全、准入控制与 RBAC 事件]] — Cross-reference
- [[概念/纵深防御 × 供应链安全.md|纵深防御 x 供应链安全]] — Cross-reference
- [[概念/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]] — Cross-reference


<!-- risk-assessed -->
