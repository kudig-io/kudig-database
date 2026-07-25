---
title: 应用安全清单
description: '# 应用安全清单'
summary: '# 应用安全清单'
category: dictionary
tags:
- k8s
- glossary
- terminology
- rbac
- networkpolicy
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 应用安全清单 是什么
- 如何 应用安全清单
trigger_keywords:
- 应用安全清单
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 应用安全清单

## 概述

本清单旨在从应用开发者的视角，提供在 [[23-实体/kubernetes.md|[[Kubernetes|kubernetes]]]] 上运行应用的安全基线指南。该列表并非详尽无遗，而是随着时间不断发展。清单中的项目顺序不反映优先级，某些项目在各小节下的段落中有更详细的说明。本文档假设“开发者”是指与命名空间范围对象交互的 Kubernetes 集群用户。

## 核心概念/原理

应用安全加固分为两个层次：

- **基础安全加固**：适用于大多数部署到 Kubernetes 的应用的通用建议。
- **高级安全加固**：根据 Kubernetes 环境设置的不同，可能具有价值的更深层次安全点。

## 关键机制或特性

### 应用设计

- 在设计应用时遵循正确的安全原则。
- 通过资源请求和限制为应用配置适当的 QoS 类。
  - 内存限制设置为等于或大于请求的值。
  - 可在敏感工作负载上设置 CPU 限制。

### ServiceAccount

- **避免使用 `default` ServiceAccount**。为每个工作负载或微服务创建独立的 ServiceAccount。
- 除非 Pod 明确需要访问 [[17-系统基础/06-知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] 才能运行，否则应将 `automountServiceAccountToken` 设置为 `false`。

### Pod 级 `securityContext` 建议

- 设置 `runAsNonRoot: true`。
- 将容器配置为以较低权限用户执行（例如使用 `runAsUser` 和 `runAsGroup`），并在容器镜像内部配置文件或目录的适当权限。
- 可选地使用 `fsGroup` 添加补充组以访问持久卷。
- 将应用部署到强制执行适当 Pod 安全标准的命名空间中。如果无法控制部署集群的强制执行设置，请通过文档或额外的纵深防御措施加以考虑。

### 容器级 `securityContext` 建议

- 使用 `allowPrivilegeEscalation: false` 禁用特权提升。
- 使用 `readOnlyRootFilesystem: true` 将根文件系统配置为只读。
- 避免运行特权容器（设置 `privileged: false`）。
- 从容器中丢弃（drop）所有 capabilities，然后仅添加操作容器所需的特定 capabilities。

### 基于角色的访问控制（RBAC）

- 仅在必要时授予 **create**、**patch**、**update** 和 **delete** 权限。
- 避免创建可能导致权限提升的创建或更新角色的 RBAC 权限。
- 审查 `system:unauthenticated` 组的绑定并尽可能移除。

**说明**：**patch** 动词如果允许在 Namespace 上执行，可能允许用户更新命名空间或部署的标签，从而增加攻击面。对于敏感工作负载，可考虑提供推荐的 ValidatingAdmissionPolicy 以进一步限制允许的写操作。

### 镜像安全

- 在 Kubernetes 集群中部署容器之前，使用镜像扫描工具扫描镜像。
- 在部署到 Kubernetes 集群之前，使用容器签名验证容器镜像签名。

### 网络策略

- 配置 NetworkPolicies，仅允许来自 Pod 的预期入站和出站流量。
- 确保集群提供并强制执行 [[NetworkPolicy|NetworkPolicy]]。如果编写的应用将部署到不同的集群，请考虑是否可以假设 NetworkPolicy 可用且已强制执行。

### Linux 容器安全

为 Pod 和容器配置安全上下文：

- 为容器设置 Seccomp 配置文件。
- 使用 AppArmor 限制容器对资源的访问。
- 为容器分配 SELinux 标签。

### 运行时类（Runtime Classes）

为容器配置适当的运行时类。某些容器可能需要与集群默认运行时提供的不同的隔离级别。可以在 Pod 规范中使用 `runtimeClassName` 定义不同的运行时类。

对于敏感工作负载，可考虑使用：

- 内核仿真工具，如 **gVisor**
- 虚拟化隔离机制，如 **kata-containers**
- 在高信任环境中，使用**机密虚拟机**进一步提升集群安全

## 使用场景

- 应用开发者在构建和部署 Kubernetes 应用时遵循安全最佳实践。
- DevOps 团队制定应用部署的安全检查清单。
- 安全团队审查应用部署清单（manifest）时作为参考标准。

## 最佳实践/注意事项

- 清单本身不足以单独实现良好的安全态势；安全需要持续关注和改进。
- 某些建议可能对特定安全需求过于严格或过于宽松，需根据具体环境评估。
- 始终遵循最小权限原则，为每个工作负载使用独立的 ServiceAccount。
- 尽量以非 root 用户运行容器，使用只读根文件系统，并丢弃不必要的 capabilities。
- 在部署前务必扫描镜像漏洞并验证镜像签名。
- 为应用配置 NetworkPolicy，实施“默认拒绝、按需允许”的网络策略。
- 对于运行不受信任代码或高敏感性数据的应用，考虑使用沙箱运行时（如 gVisor 或 Kata Containers）。

## 参考链接

- https://kubernetes.io/docs/concepts/security/application-security-checklist/

## Related

- [[17-系统基础/06-知识字典/security/admission-controller.md|准入控制器]]
- [[17-系统基础/06-知识字典/security/athenz.md|Athenz 身份认证与授权]]
- [[17-系统基础/06-知识字典/security/bank-vaults.md|Bank Vaults Vault 集成]]


<!-- risk-assessed -->
