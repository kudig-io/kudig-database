# Pod 安全策略

## 概述

PodSecurityPolicy（Pod 安全策略，简称 PSP）是一种已移除的 Kubernetes 安全控制机制。它在 Kubernetes v1.21 中被弃用，并在 **v1.25 中彻底移除**。官方文档不再推荐使用该功能，而是提供了内置和第三方的替代方案来实现相同的 Pod 安全限制。

## 核心概念/原理

PodSecurityPolicy 原本是一种集群级资源，用于在 Pod 创建时强制执行安全策略，控制 Pod 的安全上下文字段（如是否允许特权容器、可添加的 capabilities、是否允许 hostPath 等）。由于其设计复杂、权限模型难以管理，社区决定弃用并移除该 API，转而采用更现代化、更灵活的准入控制机制。

## 关键机制或特性

### 替代方案

PodSecurityPolicy 移除后，可通过以下方式实现类似的 Pod 安全限制：

- **Pod Security Admission（推荐）**：Kubernetes 内置的准入控制器，自 v1.25 起稳定。通过命名空间标签强制执行 Pod 安全标准（Pod Security Standards）的三个级别：`privileged`、`baseline`、`restricted`。
- **第三方准入插件**：如 Kyverno、OPA Gatekeeper、Kubewarden 等，可提供更细粒度、更灵活的策略定义和执行能力。

### 迁移支持

Kubernetes 官方提供了从 PodSecurityPolicy 迁移到内置 Pod Security Admission 控制器的详细指南，帮助现有集群平滑过渡。

## 使用场景

- 正在运行 Kubernetes v1.24 及更早版本并使用 PSP 的集群，需要规划迁移路径。
- 需要为 Pod 创建强制执行安全上下文的集群，应直接采用 Pod Security Admission 或第三方策略引擎。

## 最佳实践/注意事项

- **不要再在新集群中使用 PodSecurityPolicy**；该 API 已在 v1.25 中移除。
- 对于仍在使用 PSP 的集群，应尽快参考官方迁移指南完成升级：
  - *Migrate from PodSecurityPolicy to the Built-In PodSecurity Admission Controller*
- 在迁移过程中，可结合 Pod Security Admission 和第三方 Webhook 准入控制器，逐步将旧策略映射为新的策略规则。
- 评估现有工作负载的实际权限需求，借机清理过度宽松的 PSP 规则，应用最小权限原则。

## 参考链接

- https://kubernetes.io/docs/concepts/security/pod-security-policy/
