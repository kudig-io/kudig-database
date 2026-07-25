---
title: 云原生安全
description: '# 云原生安全'
summary: '# 云原生安全'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- networkpolicy
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 云原生安全 是什么
- 如何 云原生安全
trigger_keywords:
- 云原生安全
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 云原生安全

## 概述

[[Kubernetes|Kubernetes]] 基于云原生架构，借鉴了 CNCF（云原生计算基金会）关于云原生信息安全的最佳实践建议。其设计目标之一是帮助用户部署安全的云原生平台。CNCF 云原生安全白皮书将安全控制和实践按照不同的生命周期阶段进行划分，从而在每个阶段实施适当的安全措施。

## 核心概念/原理

云原生安全覆盖以下四个生命周期阶段：

- **开发（Develop）**：确保开发环境完整性，遵循安全设计原则，将终端用户安全纳入方案设计。可采用零信任架构、代码审查、威胁建模、模糊测试（fuzzing）和安全混沌工程等手段。
- **分发（Distribute）**：确保容器镜像及集群组件供应链安全。包括扫描镜像漏洞、使用加密传输与可信链、及时更新依赖、使用数字证书验证、将镜像存放在私有仓库等。
- **部署（Deploy）**：限制可部署的内容、部署人员及部署位置。通过命名空间进行应用和组件隔离，容器和命名空间均提供与信息安全相关的隔离机制。
- **运行时（Runtime）**：分为以下关键领域：
  - **访问（Access）**：保护 [[17-系统基础/06-知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]]，实施有效的认证与授权，使用 ServiceAccount 管理工作负载身份，启用 TLS 保护 API 流量。
  - **计算（Compute）**：强制执行 Pod 安全标准，使用专为容器化工作负载设计的操作系统（不可变镜像），定义 ResourceQuota 和 LimitRanges，实施节点隔离，使用提供安全限制的容器运行时，在 Linux 节点上使用 AppArmor 或 seccomp 等 Linux 安全模块。
  - **存储（Storage）**：集成支持静态加密的外部存储插件，为 API 对象启用静态加密，定期备份并验证恢复，对网络存储连接进行认证，在应用层实现数据加密，使用硬件安全模块（HSM）保护密钥。
  - **网络（Networking）**：使用 NetworkPolicy 或服务网格保护网络，部分网络插件可通过 VPN 覆盖层提供集群网络加密。
  - **可观测性（Observability）**：确保监控和日志链路具备足够的弹性和完整性保护，部署加密措施使日志既防篡改又保密。

## 关键机制或特性

- **TLS 加密**：默认使用 TLS 保护 API 流量，包括节点与控制平面之间的通信。
- **Pod 安全标准（Pod Security Standards）**：为应用定义必要的权限边界。
- **ServiceAccount**：为工作负载和集群组件提供和管理安全身份。
- **不可变操作系统（Immutable OS）**：仅提供运行容器所必需的服务，减少容器逃逸后的攻击面。
- **节点隔离**：通过 Taints/Tolerations、NodeAffinity 等机制将不同信任上下文的工作负载分隔到不同的节点组上。
- **Linux 安全模块**：如 AppArmor、seccomp，用于限制容器的系统调用和资源访问。

## 使用场景

- 构建企业级安全的云原生应用平台。
- 保护容器镜像供应链，防止带有已知漏洞的镜像进入生产环境。
- 确保多租户或混合信任环境中工作负载的运行时隔离。
- 满足合规性要求，对静态数据和传输中的数据进行加密保护。

## 最佳实践/注意事项

- 采用零信任架构，最小化攻击面，即使是内部威胁也要防范。
- 定义并执行代码审查流程，关注安全问题。
- 建立系统的威胁模型，识别信任边界并据此处理风险。
- 使用私有镜像仓库，仅允许授权客户端拉取镜像。
- 为 etcd 中的 Secret 和 API 对象启用静态加密。
- 定期备份数据并验证恢复能力。
- 对日志实施加密保护，确保其不可篡改且机密。

## 参考链接

- https://kubernetes.io/docs/concepts/[[17-系统基础/06-知识字典/security/cloud-native-security.md|cloud-native-security]]/

## Related
- [[21-生态参考/03-领域索引/security-index.md|Security 安全知识图谱索引]]


<!-- risk-assessed -->
