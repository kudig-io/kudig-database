---
title: K8s 平台工程术语参考
description: '| **Admission Webhook 最佳实践** | Admission Webhook Good Practices | Admission Webhook 是扩展 Kubernetes API 的强大机制，但在设计和部署时需要格外谨慎
  |'
category: references
tags:
- k8s
- dictionary
- platform-engineering
- apiserver
- helm
- flux
- crd
- operator
- webhook
- gpu
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- K8s 平台工程术语参考 是什么
- 如何 K8s 平台工程术语参考
trigger_keywords:
- K8s
- 平台工程术语参考
prerequisites:
- kubectl-basics
- helm-basics
- iac-basics
- gpu-scheduling-basics
---

# K8s 平台工程术语参考

本页汇总了 **平台工程** 领域的 19 个 Kubernetes 术语定义与概念说明。

> **相关领域**: [[references/k8s-platform-extensions.md|k8s-platform-extensions]] | [[references/k8s-advanced-ecosystem.md|k8s-advanced-ecosystem]]

---

## 术语速查表

| 术语 | 英文名 | 说明 |
|------|--------|------|
| **Admission Webhook 最佳实践** | Admission Webhook Good Practices | Admission Webhook 是扩展 Kubernetes API 的强大机制，但在设计和部署时需要格外谨慎 |
| **API 优先级与公平性（API Priority and Fairness）** | Api Priority And Fairness | FEATURE STATE: `Kubernetes v1 |
| **Cluster API 与集群舰队管理** | Cluster Api And Fleet Management | 随着企业 Kubernetes 集群数量从个位数增长到数十甚至上百个，**集群舰队管理（Fleet Management）** 成为平台工程的核心挑战 |
| **Kubernetes 控制平面组件的兼容版本** | Compatibility Version For Control Plane | 自 Kubernetes v1 |
| **计算、存储与网络扩展** | Compute Storage And Networking Extensions | Kubernetes 提供了多种扩展机制，用于增强集群中节点的能力，或提供连接 Pod 的网络 fabric |
| **协调领导者选举（Coordinated Leader Election）** | Coordinated Leader Election | FEATURE STATE: `Kubernetes v1 |
| **自定义资源** | Custom Resources | 自定义资源（Custom Resources）是 Kubernetes API 的扩展，允许用户在不修改 Kubernetes 核心代码的情况下，为集群添... |
| **开发者门户与平台工程度量** | Developer Portal And Platform Metrics | 随着 Kubernetes 和云原生技术栈的复杂度不断上升，**平台工程（Platform Engineering）** 正在取代传统的 DevOps 模... |
| **设备插件** | Device Plugins | 设备插件（Device Plugins）是 Kubernetes 提供的一种扩展机制，允许集群支持需要厂商特定设置的设备或资源，例如 GPU、高性能网卡（... |
| **动态资源分配（DRA）集群管理员最佳实践** | Dynamic Resource Allocation Good Practices | 动态资源分配（Dynamic Resource Allocation, DRA）是 Kubernetes 中用于管理专用硬件资源（如 GPU、FPGA 等... |
| **扩展 Kubernetes API** | Extending The Kubernetes Api | Kubernetes API 是平台的核心，扩展 Kubernetes API 允许用户在不修改 Kubernetes 核心代码的情况下，为集群添加新的资... |
| **GitOps 与持续交付** | Gitops And Continuous Delivery | **GitOps** 是一种以 Git 为唯一事实来源（Single Source of Truth）的运营模型，将基础设施和应用配置的声明式定义存储在 ... |
| **Kubernetes 基础设施即代码（IaC）** | Infrastructure As Code For Kubernetes | **基础设施即代码（Infrastructure as Code, IaC）** 是通过代码和声明式配置文件来管理和配置基础设施的实践 |
| **Kubernetes API 聚合层** | Kubernetes Api Aggregation Layer | API 聚合层（Aggregation Layer）允许 Kubernetes 通过额外的 API 进行扩展，这些 API 超出了核心 Kubernete... |
| **KubeVirt：在 Kubernetes 上运行虚拟机** | Kubevirt Virtual Machines | **KubeVirt** 是 CNCF 孵化项目，允许在 Kubernetes 集群中像管理 Pod 一样管理虚拟机（VM） |
| **网络插件** | Network Plugins | Kubernetes 允许使用 Container Network Interface（CNI）插件来实现集群网络 |
| **Operator 模式** | Operator Pattern | Operator 是 Kubernetes 的软件扩展，它利用自定义资源（Custom Resources）来管理应用程序及其组件 |
| **Kubernetes 中的代理** | Proxies In Kubernetes | 在 Kubernetes 中，用户和集群管理员可能会遇到多种不同类型的代理 |
| **WebAssembly（Wasm）工作负载** | Webassembly Wasm Workloads | **WebAssembly（Wasm）** 最初为浏览器设计，现已成为云原生领域的新兴运行时标准 |

---

### Admission Webhook 最佳实践

Admission Webhook 是扩展 Kubernetes API 的强大机制，但在设计和部署时需要格外谨慎。设计不良的 webhook 可能导致工作负载中断、升级后行为异常，甚至引发集群级故障。本文档为集群运维人员和 webhook 开发者提供了设计和部署 admission webhook 的推荐实践。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/platform-engineering/admission-webhook-good-practices.md`）*

---

### API 优先级与公平性（API Priority and Fairness）

FEATURE STATE: `Kubernetes v1.29 [stable]`

在 Kubernetes 集群中，控制 kube-apiserver 在高负载下的行为是集群管理员的关键任务。API 优先级与公平性（API Priority and Fairness，简称 APF）是一种比传统的 `--max-requests-inflight` 和 `--max-mutating-requests-inflight` 更精细的流量控制机制。APF 能够对请求进行分类和隔离，并引入有限的队列机制，使得在短时突发流量下不会被直接拒绝，同时使用公平队列算法防止单个不良控制器饿死其他客户端。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/platform-engineering/api-priority-and-fairness.md`）*

---

### Cluster API 与集群舰队管理

随着企业 Kubernetes 集群数量从个位数增长到数十甚至上百个，**集群舰队管理（Fleet Management）** 成为平台工程的核心挑战。**Cluster API（CAPI）** 是 Kubernetes 官方的声明式集群生命周期管理项目，它使用 Kubernetes 的 CRD 机制来创建、配置和管理其他 Kubernetes 集群，实现了"用 Kubernetes 管理 Kubernetes"的 Meta-Cluster 模式。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/platform-engineering/cluster-api-and-fleet-management.md`）*

---

### Kubernetes 控制平面组件的兼容版本

自 Kubernetes v1.32 起，控制平面组件引入了可配置的版本兼容和模拟（emulation）选项，使升级更加安全。集群管理员可以通过这些选项更精细地控制升级步骤，降低因版本差异带来的风险。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/platform-engineering/compatibility-version-for-control-plane.md`）*

---

### 计算、存储与网络扩展

Kubernetes 提供了多种扩展机制，用于增强集群中节点的能力，或提供连接 Pod 的网络 fabric。这些扩展并非 Kubernetes 核心自带的组件，但能够灵活地支持新硬件、新存储类型以及不同的网络拓扑。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/platform-engineering/compute-storage-and-networking-extensions.md`）*

---

### 协调领导者选举（Coordinated Leader Election）

FEATURE STATE: `Kubernetes v1.33 [beta]`（默认禁用）

Kubernetes 1.35 引入了一项 beta 特性，允许控制平面组件通过**协调领导者选举（Coordinated Leader Election）**确定性地选择领导者。该特性主要用于满足 Kubernetes 集群升级期间的版本倾斜约束。当前内置的选择策略是 `OldestEmulationVersion`，优先选择模拟版本最低的候选者，其次是二进制版本，最后是创建时间戳。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/platform-engineering/coordinated-leader-election.md`）*

---

### 自定义资源

自定义资源（Custom Resources）是 Kubernetes API 的扩展，允许用户在不修改 Kubernetes 核心代码的情况下，为集群添加新的资源类型。自定义资源可以通过动态注册在运行的集群中出现或消失，安装后用户可以像操作内置资源（如 Pod）一样使用 `kubectl` 来创建和访问它们。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/platform-engineering/custom-resources.md`）*

---

### 开发者门户与平台工程度量

随着 Kubernetes 和云原生技术栈的复杂度不断上升，**平台工程（Platform Engineering）** 正在取代传统的 DevOps 模式，成为企业提升开发者效率和交付速度的核心方法论。**开发者门户（Developer Portal）** 是平台工程的关键载体，它通过自助服务（Self-service）界面将底层基础设施的复杂性抽象化，让应用开发者能够专注于业务代码。2026 年的主流实现包括 **Backstage（由 Spotify 开源，现由 CNCF 托管）** 和 **Port** 等商业方案。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/platform-engineering/developer-portal-and-platform-metrics.md`）*

---

### 设备插件

设备插件（Device Plugins）是 Kubernetes 提供的一种扩展机制，允许集群支持需要厂商特定设置的设备或资源，例如 GPU、高性能网卡（NIC）、FPGA 或非易失性主内存。该特性自 Kubernetes v1.26 起进入稳定（Stable）状态。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/platform-engineering/device-plugins.md`）*

---

### 动态资源分配（DRA）集群管理员最佳实践

动态资源分配（Dynamic Resource Allocation, DRA）是 Kubernetes 中用于管理专用硬件资源（如 GPU、FPGA 等）的一套机制。本文档面向集群管理员，介绍在配置和使用 DRA 时的最佳实践，包括驱动部署、升级、监控和性能调优等方面的建议。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/platform-engineering/dynamic-resource-allocation-good-practices.md`）*

---

### 扩展 Kubernetes API

Kubernetes API 是平台的核心，扩展 Kubernetes API 允许用户在不修改 Kubernetes 核心代码的情况下，为集群添加新的资源类型和功能。Kubernetes 提供了两种主要的 API 扩展方式：CustomResourceDefinitions（CRD）和 API Aggregation（AA）。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/platform-engineering/extending-the-kubernetes-api.md`）*

---

### GitOps 与持续交付

**GitOps** 是一种以 Git 为唯一事实来源（Single Source of Truth）的运营模型，将基础设施和应用配置的声明式定义存储在 Git 仓库中，通过自动化控制器持续同步集群状态与 Git 中的期望状态。2026 年，GitOps 已成为 Kubernetes 平台工程和多云交付的事实标准，主流实现包括 **Argo CD** 和 **Flux**。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/platform-engineering/gitops-and-continuous-delivery.md`）*

---

### Kubernetes 基础设施即代码（IaC）

**基础设施即代码（Infrastructure as Code, IaC）** 是通过代码和声明式配置文件来管理和配置基础设施的实践。在 Kubernetes 生态中，IaC 不仅包括集群本身的创建（Terraform / Pulumi / Crossplane），还包括集群内部资源的编排（YAML / Helm / Kustomize / GitOps）。2026 年的最佳实践要求企业建立**从底层云资源到 K8s 应用配置的完整 IaC 流水线**，实现版本控制、可审计、可重复和自动化的基础设施管理。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/platform-engineering/infrastructure-as-code-for-kubernetes.md`）*

---

### Kubernetes API 聚合层

API 聚合层（Aggregation Layer）允许 Kubernetes 通过额外的 API 进行扩展，这些 API 超出了核心 Kubernetes API 所提供的范围。无论是现成的解决方案（如 metrics server），还是用户自行开发的 API，都可以通过聚合层无缝集成到 Kubernetes API 中。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/platform-engineering/kubernetes-api-aggregation-layer.md`）*

---

### KubeVirt：在 Kubernetes 上运行虚拟机

**KubeVirt** 是 CNCF 孵化项目，允许在 Kubernetes 集群中像管理 Pod 一样管理虚拟机（VM）。随着企业从 VMware 等传统虚拟化平台向云原生迁移，以及 AI/ML、数据库等有状态工作负载在 Kubernetes 上的成熟，KubeVirt 在 2025–2026 年迅速成为混合负载平台的核心技术。它实现了容器与虚拟机在同一控制平面下的统一编排。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/platform-engineering/kubevirt-virtual-machines.md`）*

---

### 网络插件

Kubernetes 允许使用 Container Network Interface（CNI）插件来实现集群网络。CNI 插件是实现 Kubernetes 网络模型的必要组件，负责为 Pod 分配 IP、建立网络连通性，并支持网络策略、端口映射等高级功能。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/platform-engineering/network-plugins.md`）*

---

### Operator 模式

Operator 是 Kubernetes 的软件扩展，它利用自定义资源（Custom Resources）来管理应用程序及其组件。Operator 遵循 Kubernetes 的设计原则，尤其是控制循环（Control Loop）模式。其核心目标是捕获人类运维专家管理服务的知识和行为，并通过代码实现自动化。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/platform-engineering/operator-pattern.md`）*

---

### Kubernetes 中的代理

在 Kubernetes 中，用户和集群管理员可能会遇到多种不同类型的代理。理解这些代理的区别和用途，对于正确访问集群服务、调试网络问题以及设计集群架构非常重要。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/platform-engineering/proxies-in-kubernetes.md`）*

---

### WebAssembly（Wasm）工作负载

**WebAssembly（Wasm）** 最初为浏览器设计，现已成为云原生领域的新兴运行时标准。在 Kubernetes 上运行 Wasm 工作负载具有**毫秒级冷启动、极小的镜像体积、沙箱级安全隔离**等优势，特别适用于边缘计算、Serverless、微服务和高并发事件驱动场景。2026 年，Wasm 正在成为 Kubernetes 的"第三运行时"（与容器、VM 并列）。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/platform-engineering/webassembly-wasm-workloads.md`）*

---

## 相关页面

- [[references/k8s-platform-extensions.md|k8s-platform-extensions]]
- [[references/k8s-advanced-ecosystem.md|k8s-advanced-ecosystem]]

## 来源文件

- `domain-17-system-foundation/topic-dictionary/platform-engineering/admission-webhook-good-practices.md`
- `domain-17-system-foundation/topic-dictionary/platform-engineering/api-priority-and-fairness.md`
- `domain-17-system-foundation/topic-dictionary/platform-engineering/cluster-api-and-fleet-management.md`
- `domain-17-system-foundation/topic-dictionary/platform-engineering/compatibility-version-for-control-plane.md`
- `domain-17-system-foundation/topic-dictionary/platform-engineering/compute-storage-and-networking-extensions.md`
- `domain-17-system-foundation/topic-dictionary/platform-engineering/coordinated-leader-election.md`
- `domain-17-system-foundation/topic-dictionary/platform-engineering/custom-resources.md`
- `domain-17-system-foundation/topic-dictionary/platform-engineering/developer-portal-and-platform-metrics.md`
- `domain-17-system-foundation/topic-dictionary/platform-engineering/device-plugins.md`
- `domain-17-system-foundation/topic-dictionary/platform-engineering/dynamic-resource-allocation-good-practices.md`
- `domain-17-system-foundation/topic-dictionary/platform-engineering/extending-the-kubernetes-api.md`
- `domain-17-system-foundation/topic-dictionary/platform-engineering/gitops-and-continuous-delivery.md`
- `domain-17-system-foundation/topic-dictionary/platform-engineering/infrastructure-as-code-for-kubernetes.md`
- `domain-17-system-foundation/topic-dictionary/platform-engineering/kubernetes-api-aggregation-layer.md`
- `domain-17-system-foundation/topic-dictionary/platform-engineering/kubevirt-virtual-machines.md`
- `domain-17-system-foundation/topic-dictionary/platform-engineering/network-plugins.md`
- `domain-17-system-foundation/topic-dictionary/platform-engineering/operator-pattern.md`
- `domain-17-system-foundation/topic-dictionary/platform-engineering/proxies-in-kubernetes.md`
- `domain-17-system-foundation/topic-dictionary/platform-engineering/webassembly-wasm-workloads.md`

## Related

- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[argo]] — Argo Workflows
- [[operator-pattern]] — Operator Pattern (CRD + Controller)
- [[concepts/infrastructure-as-code.md|infrastructure-as-code]] — Infrastructure as Code
