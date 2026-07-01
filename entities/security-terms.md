---
title: K8s 安全术语参考
description: '# K8s 安全术语参考'
summary: '# K8s 安全术语参考'
category: references
tags:
- k8s
- dictionary
- security
- etcd
- apiserver
- kubelet
- scheduler
- prometheus
- grafana
- istio
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- K8s 安全术语参考 是什么
- 如何 K8s 安全术语参考
trigger_keywords:
- K8s
- 安全术语参考
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- etcd-basics
- policy-basics
---



# K8s 安全术语参考

本页汇总了 **安全** 领域的 27 个 Kubernetes 术语定义与概念说明。

> **相关领域**: [[k8s-security-compliance]]

---

## 术语速查表

| 术语 | 英文名 | 说明 |
|------|--------|------|
| **应用安全清单** | Application Security Checklist | 本清单旨在从应用开发者的视角，提供在 Kubernetes 上运行应用的安全基线指南 |
| **09 - 云原生安全专家指南** | Cloud Native Security Practices | title: 09 - 云原生安全专家指南
description: '# 09 - 云原生安全专家指南'
category: dictionary
ta... |
| **云原生安全** | Cloud Native Security | Kubernetes 基于云原生架构，借鉴了 CNCF（云原生计算基金会）关于云原生信息安全的最佳实践建议 |
| **控制对 Kubernetes API 的访问** | Controlling Access To The Kubernetes Api | 本页面提供了控制对 Kubernetes API 访问的概览 |
| **Kubernetes Secrets 最佳实践** | Good Practices For Kubernetes Secrets | 在 Kubernetes 中，Secret 是用于存储敏感信息（如密码、OAuth 令牌、SSH 密钥）的对象 |
| **加固指南 - 认证机制** | Hardening Guide   Authentication Mechanisms | 选择适当的认证机制是保护集群安全的关键方面 |
| **加固指南 - 调度器配置** | Hardening Guide   Scheduler Configuration | Kubernetes 调度器（kube-scheduler）是控制平面的关键组件之一 |
| **Kubernetes API Server 绕过风险** | Kubernetes Api Server Bypass Risks | Kubernetes API server 是外部用户和服务与集群交互的主要入口 |
| **Limit Ranges（限制范围）** | Limit Ranges | LimitRange 是 Kubernetes 中的一种策略对象，用于约束在命名空间中可为每种适用对象类型（如 Pod 或 PersistentVolum... |
| **针对 Pod 和容器的 Linux 内核安全约束** | Linux Kernel Security Constraints For Pods And Containers | 本页面概述了可用于加固 Pod 和容器的 Linux 内核安全模块和约束 |
| **多租户** | Multi Tenancy | 共享 Kubernetes 集群可以节省成本并简化管理，但也带来了安全、公平性和“吵闹邻居”（noisy neighbors）等方面的挑战 |
| **Node Resource Managers（节点资源管理器）** | Node Resource Managers | 为了支持对延迟敏感（latency-critical）和高吞吐量（high-throughput）的工作负载，Kubernetes 提供了一套节点资源管理... |
| **Pod 安全准入** | Pod Security Admission | Kubernetes 提供了一个内置的 **Pod Security Admission** 准入控制器，用于强制执行 Pod 安全标准（Pod Secu... |
| **Pod 安全策略** | Pod Security Policies | PodSecurityPolicy（Pod 安全策略，简称 PSP）是一种已移除的 Kubernetes 安全控制机制 |
| **Pod 安全标准** | Pod Security Standards | Pod 安全标准（Pod Security Standards）定义了三个不同级别的策略，用于广泛覆盖安全光谱 |
| **策略即代码（Policy as Code）** | Policy As Code | **策略即代码（Policy as Code）** 是将组织的安全、合规和运维策略以可版本化、可自动化验证的代码形式定义和执行的方法论 |
| **Process ID Limits And Reservations（进程 ID 限制与预留）** | Process Id Limits And Reservations | 进程 ID（PIDs）是节点上的基本资源 |
| **Resource Quotas（资源配额）** | Resource Quotas | ResourceQuota 是 Kubernetes 为管理员提供的一种工具，用于限制命名空间级别的聚合资源消耗 |
| **基于角色的访问控制（RBAC）最佳实践** | Role Based Access Control Good Practices | Kubernetes RBAC 是确保集群用户和工作负载仅拥有执行其角色所需资源访问权限的关键安全控制 |
| **运行时安全** | Runtime Security | **运行时安全（Runtime Security）** 关注的是容器和 Pod 在集群中实际运行时的威胁检测与防护 |
| **密钥管理深度指南** | Secrets Management Deep Dive | 在 Kubernetes 环境中，Secrets（如数据库密码、API 密钥、TLS 证书、OAuth Token）是攻击者最觊觎的目标 |
| **安全清单** | Security Checklist | 本清单旨在提供一个基础的安全指导列表，并附带指向各主题更全面文档的链接 |
| **Linux 节点安全** | Security For Linux Nodes | 本页面描述了针对 Linux 操作系统的安全考虑和最佳实践 |
| **Windows 节点安全** | Security For Windows Nodes | 本页面描述了针对 Windows 操作系统的安全考虑和最佳实践 |
| **服务账号** | Service Accounts | ServiceAccount（服务账号）是 Kubernetes 中的一种非人类账户，用于在集群内提供独立的安全身份 |
| **SPIFFE / SPIRE 与工作负载身份** | Spiffe Spire Identity | 在零信任（Zero Trust）安全架构中，"**永远不要信任，永远要验证**"是核心原则 |
| **软件供应链安全** | Supply Chain Security | 软件供应链攻击（如 SolarWinds、Log4j 事件）已成为云原生环境的首要威胁 |

---

### 应用安全清单

本清单旨在从应用开发者的视角，提供在 Kubernetes 上运行应用的安全基线指南。该列表并非详尽无遗，而是随着时间不断发展。清单中的项目顺序不反映优先级，某些项目在各小节下的段落中有更详细的说明。本文档假设“开发者”是指与命名空间范围对象交互的 Kubernetes 集群用户。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/application-security-checklist.md`）*

---

### 09 - 云原生安全专家指南

title: 09 - 云原生安全专家指南
description: '# 09 - 云原生安全专家指南'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- apiserver
- kubelet
- prometheus
- grafana
- istio
- cilium
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 云原生安全专家指南 是什么
- 如何 云原生安全专家指南
trigger_keywords:
- 云原生安全专家指南
- dictionary
title_en: Cloud Native Security Practices
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
-...

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/cloud-native-security-practices.md`）*

---

### 云原生安全

Kubernetes 基于云原生架构，借鉴了 CNCF（云原生计算基金会）关于云原生信息安全的最佳实践建议。其设计目标之一是帮助用户部署安全的云原生平台。CNCF 云原生安全白皮书将安全控制和实践按照不同的生命周期阶段进行划分，从而在每个阶段实施适当的安全措施。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/cloud-native-security.md`）*

---

### 控制对 Kubernetes API 的访问

本页面提供了控制对 Kubernetes API 访问的概览。用户通过 `kubectl`、客户端库或直接发起 REST 请求访问 Kubernetes API。无论是人类用户还是 Kubernetes 服务账号，都可以被授权访问 API。当请求到达 API 时，会依次经历多个阶段。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/controlling-access-to-the-kubernetes-api.md`）*

---

### Kubernetes Secrets 最佳实践

在 Kubernetes 中，Secret 是用于存储敏感信息（如密码、OAuth 令牌、SSH 密钥）的对象。Secret 提供了对敏感信息使用方式的更多控制，降低了意外暴露的风险。Secret 值以 base64 编码存储，默认情况下以未加密形式保存在 etcd 中，但可以配置为静态加密。以下最佳实践面向集群管理员和应用开发者，旨在提高 Secret 对象的安全性并改善管理效率。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/good-practices-for-kubernetes-secrets.md`）*

---

### 加固指南 - 认证机制

选择适当的认证机制是保护集群安全的关键方面。Kubernetes 提供了多种内置认证机制，每种机制都有其自身的优缺点，在选择最佳认证方案时需要仔细权衡。通常建议启用尽可能少的认证机制，以简化用户管理并防止用户保留不再需要的集群访问权限。需要注意的是，Kubernetes 集群内部没有内置的用户数据库，而是从配置的认证系统中获取用户信息并用于授权决策。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/hardening-guide---authentication-mechanisms.md`）*

---

### 加固指南 - 调度器配置

Kubernetes 调度器（kube-scheduler）是控制平面的关键组件之一。配置错误的调度器可能产生安全影响，例如被用于针对特定节点并驱逐共享该节点及其资源的工作负载或应用，从而协助攻击者实施 **Yo-Yo 攻击**（针对脆弱自动扩缩容器的攻击）。本文档介绍如何提高调度器的安全态势。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/hardening-guide---scheduler-configuration.md`）*

---

### Kubernetes API Server 绕过风险

Kubernetes API server 是外部用户和服务与集群交互的主要入口。作为这一角色，API server 具有多项关键的内置安全控制，例如审计日志和准入控制器。然而，存在一些可以修改集群配置或内容的方式，能够绕过这些控制。本文描述了 API server 内置安全控制可能被绕过的途径，以便集群运营人员和安全架构师能够确保这些绕过途径得到适当限制。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/kubernetes-api-server-bypass-risks.md`）*

---

### Limit Ranges（限制范围）

LimitRange 是 Kubernetes 中的一种策略对象，用于约束在命名空间中可为每种适用对象类型（如 Pod 或 PersistentVolumeClaim）指定的资源分配（limits 和 requests）。默认情况下，容器在集群中以无限制的_compute resources_运行，LimitRange 能够防止单个对象垄断命名空间内的所有可用资源。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/limit-ranges.md`）*

---

### 针对 Pod 和容器的 Linux 内核安全约束

本页面概述了可用于加固 Pod 和容器的 Linux 内核安全模块和约束。这些功能是 Linux 内核内置的安全能力，常见特性包括 seccomp、AppArmor 和 SELinux。在 Pod 规范中使用 `securityContext` 字段来配置这些约束。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/linux-kernel-security-constraints-for-pods-and-containers.md`）*

---

### 多租户

共享 Kubernetes 集群可以节省成本并简化管理，但也带来了安全、公平性和“吵闹邻居”（noisy neighbors）等方面的挑战。集群共享可以有多种形式：不同应用运行在同一集群中，或同一应用的不同实例（面向不同终端用户）运行在同一集群中。这些共享方式通常统称为**多租户（multi-tenancy）**。虽然 Kubernetes 没有原生的“租户”或“终端用户”一等概念，但它提供了多种功能来帮助管理不同的租户需求。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/multi-tenancy.md`）*

---

### Node Resource Managers（节点资源管理器）

为了支持对延迟敏感（latency-critical）和高吞吐量（high-throughput）的工作负载，Kubernetes 提供了一套节点资源管理器（Node Resource Managers）。这些管理器旨在协调和优化节点上为 Pod 分配 CPU、设备（devices）和内存（大页，hugepages）资源时的对齐方式，以最大程度地提升工作负载性能。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/node-resource-managers.md`）*

---

### Pod 安全准入

Kubernetes 提供了一个内置的 **Pod Security Admission** 准入控制器，用于强制执行 Pod 安全标准（Pod Security Standards）。该功能自 Kubernetes v1.25 起达到稳定（Stable）状态。Pod 安全限制在 Pod 创建时应用于命名空间级别。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/pod-security-admission.md`）*

---

### Pod 安全策略

PodSecurityPolicy（Pod 安全策略，简称 PSP）是一种已移除的 Kubernetes 安全控制机制。它在 Kubernetes v1.21 中被弃用，并在 **v1.25 中彻底移除**。官方文档不再推荐使用该功能，而是提供了内置和第三方的替代方案来实现相同的 Pod 安全限制。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/pod-security-policies.md`）*

---

### Pod 安全标准

Pod 安全标准（Pod Security Standards）定义了三个不同级别的策略，用于广泛覆盖安全光谱。这些策略是累积的，从高度宽松到高度受限不等。本页面详细说明了每个策略的具体要求，为集群管理员和开发者提供统一的安全语言。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/pod-security-standards.md`）*

---

### 策略即代码（Policy as Code）

**策略即代码（Policy as Code）** 是将组织的安全、合规和运维策略以可版本化、可自动化验证的代码形式定义和执行的方法论。在 Kubernetes 环境中，策略即代码通过准入控制器（Admission Controller）在资源创建或更新时进行实时校验和变异，确保集群状态始终符合组织策略。2026 年的主流实现包括 **Open Policy Agent（OPA/Gatekeeper）** 和 **Kyverno**。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/policy-as-code.md`）*

---

### Process ID Limits And Reservations（进程 ID 限制与预留）

进程 ID（PIDs）是节点上的基本资源。Kubernetes 允许限制单个 Pod 可使用的 PID 数量，同时也可为节点预留一定数量的可分配 PID，供操作系统和 Kubernetes 守护进程使用。PID 耗尽很容易在未触及其他资源限制的情况下发生，进而导致主机守护进程（如 kubelet、kube-proxy、容器运行时）无法运行，引发节点不稳定。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/process-id-limits-and-reservations.md`）*

---

### Resource Quotas（资源配额）

ResourceQuota 是 Kubernetes 为管理员提供的一种工具，用于限制命名空间级别的聚合资源消耗。当多个用户或团队共享一个节点数量固定的集群时，ResourceQuota 可以防止某个团队使用超过其公平份额的资源。它不仅能限制计算资源、存储资源和扩展资源的总量，还能限制命名空间中各类 Kubernetes API 对象的数量。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/resource-quotas.md`）*

---

### 基于角色的访问控制（RBAC）最佳实践

Kubernetes RBAC 是确保集群用户和工作负载仅拥有执行其角色所需资源访问权限的关键安全控制。设计权限时，集群管理员需要理解可能发生权限升级的区域，以降低过度访问导致安全事件的风险。本文档提供的最佳实践应与通用 RBAC 文档结合阅读。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/role-based-access-control-good-practices.md`）*

---

### 运行时安全

**运行时安全（Runtime Security）** 关注的是容器和 Pod 在集群中实际运行时的威胁检测与防护。即使镜像通过了漏洞扫描和签名验证，运行时仍可能遭遇零日漏洞利用、配置漂移、内部威胁或供应链后门的激活。2026 年的最佳实践强调通过 **eBPF 技术**实现内核级、零侵入的运行时安全监控和实时响应。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/runtime-security.md`）*

---

### 密钥管理深度指南

在 Kubernetes 环境中，Secrets（如数据库密码、API 密钥、TLS 证书、OAuth Token）是攻击者最觊觎的目标。2026 年的安全最佳实践认为，**单纯依赖 Kubernetes 原生的 Secret 资源已不足以应对企业级安全要求**。现代密钥管理需要结合**外部密钥管理系统（KMS）、自动轮转、最小权限访问、审计日志和硬件安全模块（HSM）**，构建端到端的 Secret 生命周期管理体系。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/secrets-management-deep-dive.md`）*

---

### 安全清单

本清单旨在提供一个基础的安全指导列表，并附带指向各主题更全面文档的链接。它并不声称是详尽的，而是会随着时间不断发展。清单中的项目顺序不反映优先级，某些项目可能在各小节下的段落中有更详细的说明。请记住，清单本身不足以单独实现良好的安全态势；安全是一条永无止境的旅程，需要持续关注和改进。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/security-checklist.md`）*

---

### Linux 节点安全

本页面描述了针对 Linux 操作系统的安全考虑和最佳实践。Linux 节点在 Kubernetes 集群中承担着运行容器工作负载的重要角色，某些内核和系统配置会直接影响 Secret 等敏感数据的保护效果。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/security-for-linux-nodes.md`）*

---

### Windows 节点安全

本页面描述了针对 Windows 操作系统的安全考虑和最佳实践。Windows 节点在 Kubernetes 集群中的行为与 Linux 节点存在显著差异，特别是在 Secret 保护、容器用户和 Pod 安全隔离方面。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/security-for-windows-nodes.md`）*

---

### 服务账号

ServiceAccount（服务账号）是 Kubernetes 中的一种非人类账户，用于在集群内提供独立的安全身份。应用 Pod、系统组件以及集群内外的实体都可以使用特定 ServiceAccount 的凭据来标识自己。该身份在多种场景下非常有用，例如向 API server 认证或实施基于身份的安全策略。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/service-accounts.md`）*

---

### SPIFFE / SPIRE 与工作负载身份

在零信任（Zero Trust）安全架构中，"**永远不要信任，永远要验证**"是核心原则。传统的基于 IP 地址或网络边界的身份验证在 Kubernetes 动态环境中已不再可靠。**SPIFFE（Secure Production Identity Framework For Everyone）** 和 **SPIRE（SPIFFE Runtime Environment）** 是 CNCF 孵化的开源项目，为跨云、跨集群的工作负载提供了统一、自动化、可密码学验证的身份标准。2026 年，SPIFFE/SPIRE 正在成为服务网格、mTLS、API 网关和云原生工作负载身份管理的事实标准。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/spiffe-spire-identity.md`）*

---

### 软件供应链安全

软件供应链攻击（如 SolarWinds、Log4j 事件）已成为云原生环境的首要威胁。2026 年的 Kubernetes 安全最佳实践要求将**供应链安全**纳入整个应用生命周期，从镜像构建、签名、扫描到准入控制和运行时验证，形成端到端的可信交付链。核心能力包括 **SBOM（软件物料清单）、镜像签名（Sigstore/Cosign）、漏洞扫描和 SLSA 合规**。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/security/supply-chain-security.md`）*

---

## 相关页面

- [[k8s-security-compliance]]

## 来源文件

- `domain-17-system-foundation/topic-dictionary/security/application-security-checklist.md`
- `domain-17-system-foundation/topic-dictionary/security/cloud-native-security-practices.md`
- `domain-17-system-foundation/topic-dictionary/security/cloud-native-security.md`
- `domain-17-system-foundation/topic-dictionary/security/controlling-access-to-the-kubernetes-api.md`
- `domain-17-system-foundation/topic-dictionary/security/good-practices-for-kubernetes-secrets.md`
- `domain-17-system-foundation/topic-dictionary/security/hardening-guide---authentication-mechanisms.md`
- `domain-17-system-foundation/topic-dictionary/security/hardening-guide---scheduler-configuration.md`
- `domain-17-system-foundation/topic-dictionary/security/kubernetes-api-server-bypass-risks.md`
- `domain-17-system-foundation/topic-dictionary/security/limit-ranges.md`
- `domain-17-system-foundation/topic-dictionary/security/linux-kernel-security-constraints-for-pods-and-containers.md`
- `domain-17-system-foundation/topic-dictionary/security/multi-tenancy.md`
- `domain-17-system-foundation/topic-dictionary/security/node-resource-managers.md`
- `domain-17-system-foundation/topic-dictionary/security/pod-security-admission.md`
- `domain-17-system-foundation/topic-dictionary/security/pod-security-policies.md`
- `domain-17-system-foundation/topic-dictionary/security/pod-security-standards.md`
- `domain-17-system-foundation/topic-dictionary/security/policy-as-code.md`
- `domain-17-system-foundation/topic-dictionary/security/process-id-limits-and-reservations.md`
- `domain-17-system-foundation/topic-dictionary/security/resource-quotas.md`
- `domain-17-system-foundation/topic-dictionary/security/role-based-access-control-good-practices.md`
- `domain-17-system-foundation/topic-dictionary/security/runtime-security.md`
- `domain-17-system-foundation/topic-dictionary/security/secrets-management-deep-dive.md`
- `domain-17-system-foundation/topic-dictionary/security/security-checklist.md`
- `domain-17-system-foundation/topic-dictionary/security/security-for-linux-nodes.md`
- `domain-17-system-foundation/topic-dictionary/security/security-for-windows-nodes.md`
- `domain-17-system-foundation/topic-dictionary/security/service-accounts.md`
- `domain-17-system-foundation/topic-dictionary/security/spiffe-spire-identity.md`
- `domain-17-system-foundation/topic-dictionary/security/supply-chain-security.md`

## Related

- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[secrets-management]] — Secrets Management
- [[supply-chain-security]] — Software Supply Chain Security
- [[entities/k8s-glossary-index.md|K8s 术语表索引]] — Cross-reference
