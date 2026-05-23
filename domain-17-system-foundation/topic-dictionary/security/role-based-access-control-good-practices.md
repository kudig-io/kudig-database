---
title: 基于角色的访问控制（RBAC）最佳实践
description: '# 基于角色的访问控制（RBAC）最佳实践'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- kubelet
- daemonset
- rbac
- networkpolicy
- webhook
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 基于角色的访问控制（RBAC）最佳实践 是什么
- 如何 基于角色的访问控制（RBAC）最佳实践
trigger_keywords:
- 基于角色的访问控制
- RBAC
- 最佳实践
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- etcd-basics
created: "2026-05-23"
---

# 基于角色的访问控制（RBAC）最佳实践

## 概述

[[Kubernetes|Kubernetes]] RBAC 是确保集群用户和工作负载仅拥有执行其角色所需资源访问权限的关键安全控制。设计权限时，集群管理员需要理解可能发生权限升级的区域，以降低过度访问导致安全事件的风险。本文档提供的最佳实践应与通用 RBAC 文档结合阅读。

## 核心概念/原理

RBAC 的核心设计原则是**最小权限（Least Privilege）**。理想情况下，应为用户和服务账号分配最小化的权限，仅授予其明确需要的操作权限。然而，Kubernetes 中的某些权限如果被不当授予，可能导致权限提升或影响集群外部系统。

## 关键机制或特性

### 一般最佳实践

- **最小权限**：
  - 尽可能在命名空间级别分配权限，使用 RoleBinding 而非 ClusterRoleBinding。
  - 避免使用通配符（`*`）权限，尤其是针对所有资源，因为 Kubernetes 是可扩展系统，通配符会授予对将来新增资源类型的访问权。
  - 管理员不应日常使用 `cluster-admin` 账号；可以提供低权限账号并配合 `impersonate` 权限以避免意外修改集群资源。
  - **避免将用户添加到 `system:masters` 组**。该组成员绕过所有 RBAC 检查，拥有不可撤销的超级用户权限，且在使用授权 webhook 时也会绕过该 webhook。

- **最小化特权令牌分发**：
  - 避免为 Pod 分配被授予强大权限的服务账号。
  - 限制运行高权限 Pod 的节点数量，确保 [[DaemonSet|DaemonSet]] 以最小权限运行。
  - 避免将高权限 Pod 与不受信任或暴露于公网的 Pod 运行在同一节点上。可以使用 Taints/Tolerations、NodeAffinity 或 PodAntiAffinity 进行隔离。

- **加固默认配置**：
  - 审查 `system:unauthenticated` 组的绑定并尽可能移除，因为这会向任何能网络访问 API server 的人授予权限。
  - 通过设置 `automountServiceAccountToken: false` 避免默认自动挂载服务账号令牌。

- **定期审查**：
  - 定期审查 Kubernetes RBAC 设置，清理冗余条目和潜在的权限升级路径。
  - 注意：如果攻击者能创建一个与已删除用户同名的用户账号，他可以自动继承该用户的所有权限。

### 特权升级风险

以下权限如果被不当授予，可能导致用户或服务账号提升权限：

- **列出 Secret（list [[Secrets|secrets]]）**：`list` 和 `watch` 访问权限实际上允许用户获取所有 Secret 的内容（例如 `kubectl get secrets -A -o yaml` 的输出包含所有 Secret 数据）。
- **创建工作负载（Workload creation）**：在命名空间中创建 Pod 或管理工作负载的权限，隐式授予对该命名空间中 Secrets、[[ConfigMaps|ConfigMaps]]、PersistentVolumes 的访问权，还可以使用命名空间中的任何 ServiceAccount 的 API 访问级别。
- **创建 PersistentVolume**：允许创建任意 PersistentVolume 意味着可以创建 `hostPath` 卷，从而获得对节点底层文件系统的访问权。
- **访问 `nodes/proxy` 子资源**：拥有 `nodes/proxy` 的 `get` 权限即可访问 Kubelet API，执行节点上任何容器的命令，且该访问**不是只读权限**。
- **escalate 动词**：允许用户创建比自己拥有更多权限的 ClusterRole。
- **bind 动词**：允许用户绑定到比自己权限更高的 Role/ClusterRole。
- **impersonate 动词**：允许用户模拟其他用户或账号，获取其权限。
- **CSR 和证书签发**：具有特定 CSR 和 approval 权限的用户可创建新的客户端证书，以任意名称（包括系统组件名称）认证到集群，实现权限提升。
- **Token request**：对 `serviceaccounts/token` 具有 `create` 权限可发放现有服务账号的令牌。
- **控制 Admission Webhooks**：控制 validating/mutating webhook 配置可以读取、修改进入集群的任何对象。
- **修改 Namespace**：对 Namespace 具有 `patch` 权限的用户可以修改标签，在启用 Pod Security Admission 时可能将命名空间配置为更宽松的策略，或在使用 NetworkPolicy 时间接允许意外访问。

### 拒绝服务风险

- **对象创建 DoS**：具有创建对象权限的用户可能创建过大或过多的对象，导致 etcd OOM。可使用 **ResourceQuota** 限制可创建对象的数量。

## 使用场景

- 为不同团队、租户或工作负载设计安全的 RBAC 策略。
- 审查现有集群的 RBAC 配置，消除权限提升路径。
- 制定最小权限原则实施指南。

## 最佳实践/注意事项

- 始终遵循最小权限原则，定期审计 RBAC 配置。
- 特别注意隐式权限：创建工作负载即隐式获得大量命名空间内资源访问权。
- 严格控制对 `nodes/proxy`、`escalate`、`bind`、`impersonate` 等高风险 API 的访问。
- 在多租户环境中，将不同信任级别的资源分隔到不同的命名空间中，因为同一命名空间内的边界被认为是“弱边界”。
- 对于高权限工作负载，实施节点隔离，避免与低信任度 Pod 共存。

## 参考链接

- https://kubernetes.io/docs/concepts/security/rbac-good-practices/

## Related

- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
