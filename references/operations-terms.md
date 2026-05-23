---
title: K8s 运维运营术语参考
description: '# K8s 运维运营术语参考'
category: references
tags:
- k8s
- dictionary
- operations
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- grafana
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- K8s 运维运营术语参考 是什么
- 如何 K8s 运维运营术语参考
trigger_keywords:
- K8s
- 运维运营术语参考
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- cilium-basics
- cni-basics
- etcd-basics
- kafka-basics
- redis-basics
- mysql-basics
- tracing-basics
created: "2026-05-23"
---

# K8s 运维运营术语参考

本页汇总了 **运维运营** 领域的 20 个 Kubernetes 术语定义与概念说明。

> **相关领域**: [[references/k8s-production-operations|k8s-production-operations]] | [[references/k8s-structured-troubleshooting|k8s-structured-troubleshooting]]

---

## 术语速查表

| 术语 | 英文名 | 说明 |
|------|--------|------|
| **备份与灾难恢复（Backup & Disaster Recovery）** | Backup Disaster Recovery | 在 Kubernetes 生产环境中，**备份与灾难恢复（BDR）** 是保障业务连续性的最后防线 |
| **13 - 容量规划与资源预测** | Capacity Planning Forecasting | title: 13 - 容量规划与资源预测
description: '**生产环境实战经验总结**: 基于万级节点集群容量管理经验，涵盖从资源预测到成本... |
| **Certificates（PKI 证书与要求）** | Certificates | Kubernetes 集群的所有组件之间都通过 **TLS（传输层安全协议）** 进行通信，因此需要一套完整的 **PKI（公钥基础设施）证书体系** 来... |
| **14 - 变更管理与发布策略** | Change Management Release | title: 14 - 变更管理与发布策略
description: '# 14 - 变更管理与发布策略'
category: dictionary
ta... |
| **混沌工程（Chaos Engineering）** | Chaos Engineering | **混沌工程**是一种通过在生产环境中有控制地注入问题，来验证系统韧性和发现潜在弱点的工程实践 |
| **企业级运维最佳实践** | Enterprise Ops Practices | title: 企业级运维最佳实践
description: '# 企业级运维最佳实践'
category: dictionary
tags:
- k8s
... |
| **02 - Kubernetes 故障模式与根因分析字典** | Failure Patterns Analysis | title: 02 - Kubernetes 故障模式与根因分析字典
description: '**本文定位**: 这是一份 Kubernetes 问题... |
| **FinOps 与成本优化** | Finops And Cost Optimization | 随着 Kubernetes 集群规模和复杂度的增长，云资源浪费已成为企业 IT 支出的主要痛点 |
| **GreenOps 与碳感知计算** | Greenops And Carbon Aware Computing | 随着全球对气候变化的重视和企业 ESG（环境、社会与治理）合规要求的提升，**GreenOps** 正在成为云原生运维的重要分支 |
| **12 - 生产事故管理与应急手册** | Incident Management Runbooks | title: 12 - 生产事故管理与应急手册
description: '# 12 - 生产事故管理与应急手册'
category: dictionar... |
| **安装插件（Installing Addons）** | Installing Addons | 插件（Addons）用于扩展 Kubernetes 的功能 |
| **节点自动扩缩容（Node Autoscaling）** | Node Autoscaling | 节点自动扩缩容（Node Autoscaling）能够根据集群中工作负载的需求，自动**供应（provision）**新节点或**整合（consolida... |
| **节点关闭（Node Shutdowns）** | Node Shutdowns | 在 Kubernetes 集群中，节点可能会因为计划内维护或意外原因（如断电）而关闭 |
| **01 - Kubernetes 生产环境运维最佳实践字典** | Operations Best Practices | title: 01 - Kubernetes 生产环境运维最佳实践字典
description: '# 01 - Kubernetes 生产环境运维最佳实... |
| **03 - Kubernetes 性能调优专家指南** | Performance Tuning Expert | title: 03 - Kubernetes 性能调优专家指南
description: '# 03 - Kubernetes 性能调优专家指南'
cat... |
| **16 - 生产环境故障排查剧本** | Production Troubleshooting Playbook | title: 16 - 生产环境故障排查剧本
description: '# 16 - 生产环境故障排查剧本'
category: dictionary
... |
| **15 - SLI/SLO/SLA工程实践** | Sli Slo Sla Engineering | title: 15 - SLI/SLO/SLA工程实践
description: '# 15 - SLI/SLO/SLA工程实践'
category: d... |
| **04 - SRE运维成熟度模型** | Sre Maturity Model | title: 04 - SRE运维成熟度模型
description: '# 04 - SRE运维成熟度模型'
category: dictionary
... |
| **有状态服务运维** | Stateful Services Operations | 虽然 Kubernetes 最初为无状态应用设计，但近年来**有状态工作负载（Stateful Workloads）** 在 K8s 上的运行已日趋成熟 |
| **Swap 内存管理** | Swap Memory Management | Kubernetes 可以配置为在节点上使用 swap（交换）内存，允许内核将不活跃的内存页换出到后备存储，从而释放物理内存 |

---

### 备份与灾难恢复（Backup & Disaster Recovery）

在 Kubernetes 生产环境中，**备份与灾难恢复（BDR）** 是保障业务连续性的最后防线。2026 年的最佳实践要求企业不仅备份应用数据，还要备份**etcd 集群状态、Kubernetes 资源定义、Secrets 以及容器镜像**。一套完整的 BDR 策略应涵盖 **恢复时间目标（RTO）** 和 **恢复点目标（RPO）**，并通过定期的灾难恢复演练验证其有效性。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/operations/backup-disaster-recovery.md`）*

---

### 13 - 容量规划与资源预测

title: 13 - 容量规划与资源预测
description: '**生产环境实战经验总结**: 基于万级节点集群容量管理经验，涵盖从资源预测到成本优化的全方位最佳实践'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- scheduler
- prometheus
- grafana
- hpa
- job
- operator
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- 容量规划与资源预测 是什么
- 如何 容量规划与资源预测
trigger_keywords:
- 容量规划与资源预测
- dictionary
title_en: Capacity Planning
authors:
- name: KUDIG Team
  role: contributor
k8s_ver...

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/operations/capacity-planning-forecasting.md`）*

---

### Certificates（PKI 证书与要求）

Kubernetes 集群的所有组件之间都通过 **TLS（传输层安全协议）** 进行通信，因此需要一套完整的 **PKI（公钥基础设施）证书体系** 来完成双向身份验证与加密传输。如果你使用 `kubeadm` 安装集群，这些证书会自动生成；但在手动部署或需要更高安全性的场景下，运维人员需要自行创建并管理证书。

> **注意**：原概念页面 `/docs/concepts/cluster-administration/certificates/` 已迁移，当前内容主要基于官方最佳实践文档 `/docs/setup/best-practices/certificates/` 进行总结。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/operations/certificates.md`）*

---

### 14 - 变更管理与发布策略

title: 14 - 变更管理与发布策略
description: '# 14 - 变更管理与发布策略'
category: dictionary
tags:
- k8s
- glossary
- terminology
- prometheus
- istio
- cilium
- calico
- helm
- argocd
- docker
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- 变更管理与发布策略 是什么
- 如何 变更管理与发布策略
trigger_keywords:
- 变更管理与发布策略
- dictionary
title_en: Change Management Release
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '...

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/operations/change-management-release.md`）*

---

### 混沌工程（Chaos Engineering）

**混沌工程**是一种通过在生产环境中有控制地注入问题，来验证系统韧性和发现潜在弱点的工程实践。其核心理念是"**主动制造问题，以避免被动承受问题**"。2026 年的 Kubernetes 生产环境中，混沌工程已成为 SRE 成熟度模型中的关键能力，主流工具包括 **Litmus、Chaos Mesh、Gremlin** 和 Netflix 开源的 **Chaos Monkey**。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/operations/chaos-engineering.md`）*

---

### 企业级运维最佳实践

title: 企业级运维最佳实践
description: '# 企业级运维最佳实践'
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
- jaeger
- istio
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- 企业级运维最佳实践 是什么
- 如何 企业级运维最佳实践
trigger_keywords:
- 企业级运维最佳实践
- dictionary
title_en: Enterprise Ops Practices
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
...

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/operations/enterprise-ops-practices.md`）*

---

### 02 - Kubernetes 故障模式与根因分析字典

title: 02 - Kubernetes 故障模式与根因分析字典
description: '**本文定位**: 这是一份 Kubernetes 故障分析的完整指南，涵盖问题分类、根因分析方法论、FMEA 分析、MTTR 优化、复盘流程和预防体系，帮助团队系统性地处理和预防问题。'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- apiserver
- kubelet
- scheduler
- prometheus
- jaeger
- coredns
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Kubernetes 故障模式与根因分析字典 是什么
- 如何 Kubernetes 故障模式与根因分析字典
trigger_keywords:
- Kubernetes
- 故障模式...

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/operations/failure-patterns-analysis.md`）*

---

### FinOps 与成本优化

随着 Kubernetes 集群规模和复杂度的增长，云资源浪费已成为企业 IT 支出的主要痛点。研究表明，生产集群普遍存在 **40%–60% 的超配（Overprovisioning）**，开发测试环境全天候运行进一步加剧了成本问题。**FinOps** 是将财务管理与云原生运营相结合的实践，通过成本可视化、资源右调优（Right-sizing）、自动伸缩和 spot 实例策略，帮助企业在 2026 年将 Kubernetes 成本降低 30%–40%。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/operations/finops-and-cost-optimization.md`）*

---

### GreenOps 与碳感知计算

随着全球对气候变化的重视和企业 ESG（环境、社会与治理）合规要求的提升，**GreenOps** 正在成为云原生运维的重要分支。GreenOps 将环境可持续性纳入 IT 运营决策，通过**碳感知调度（Carbon-aware Scheduling）、资源效率优化和可再生能源优先**等手段，降低 Kubernetes 工作负载的碳足迹。2026 年，欧盟 CSRD 等法规已要求大型企业披露数字基础设施的碳排放数据。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/operations/greenops-and-carbon-aware-computing.md`）*

---

### 12 - 生产事故管理与应急手册

title: 12 - 生产事故管理与应急手册
description: '# 12 - 生产事故管理与应急手册'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- apiserver
- scheduler
- controller-manager
- prometheus
- grafana
- helm
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 1h
intent_queries:
- 生产事故管理与应急手册 是什么
- 如何 生产事故管理与应急手册
trigger_keywords:
- 生产事故管理与应急手册
- dictionary
title_en: Incident Management Runbooks
authors:
- name: KUDIG Team
  role: contributor
k8s_ver...

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/operations/incident-management-runbooks.md`）*

---

### 安装插件（Installing Addons）

插件（Addons）用于扩展 Kubernetes 的功能。Kubernetes 本身不提供原生的完整集群功能（如 DNS、网络、仪表板等），而是通过插件生态来补充这些能力。本文档列出了 Kubernetes 官方文档中提到的一些可用插件，并提供其安装说明的链接。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/operations/installing-addons.md`）*

---

### 节点自动扩缩容（Node Autoscaling）

节点自动扩缩容（Node Autoscaling）能够根据集群中工作负载的需求，自动**供应（provision）**新节点或**整合（consolidate）**现有节点，以提供所需容量的同时优化成本。执行这些操作的组件称为**节点自动扩缩器（Node autoscalers）**。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/operations/node-autoscaling.md`）*

---

### 节点关闭（Node Shutdowns）

在 Kubernetes 集群中，节点可能会因为计划内维护或意外原因（如断电）而关闭。如果节点在关闭前未被清空（drain），可能导致工作负载失败。节点关闭分为**优雅关闭（graceful）**和**非优雅关闭（non-graceful）**两种类型。Kubernetes 提供了相应的机制来尽量降低节点关闭对工作负载的影响。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/operations/node-shutdowns.md`）*

---

### 01 - Kubernetes 生产环境运维最佳实践字典

title: 01 - Kubernetes 生产环境运维最佳实践字典
description: '# 01 - Kubernetes 生产环境运维最佳实践字典'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- istio
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 20min
intent_queries:
- Kubernetes 生产环境运维最佳实践字典 是什么
- 如何 Kubernetes 生产环境运维最佳实践字典
trigger_keywords:
- Kubernetes
- 生产环境运维最佳实践字典
- dictionary
title_en: Operations Best Pr...

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/operations/operations-best-practices.md`）*

---

### 03 - Kubernetes 性能调优专家指南

title: 03 - Kubernetes 性能调优专家指南
description: '# 03 - Kubernetes 性能调优专家指南'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- apiserver
- kubelet
- scheduler
- prometheus
- grafana
- istio
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- Kubernetes 性能调优专家指南 是什么
- 如何 Kubernetes 性能调优专家指南
trigger_keywords:
- Kubernetes
- 性能调优专家指南
- dictionary
title_en: Performance Tuning Expert
authors:
- name: KUDIG ...

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/operations/performance-tuning-expert.md`）*

---

### 16 - 生产环境故障排查剧本

title: 16 - 生产环境故障排查剧本
description: '# 16 - 生产环境故障排查剧本'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- apiserver
- kubelet
- scheduler
- prometheus
- grafana
- cilium
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- 生产环境故障排查剧本 是什么
- 如何 生产环境故障排查剧本
- 生产环境故障排查剧本 故障排查
- 生产环境故障排查剧本 排障步骤
trigger_keywords:
- 生产环境故障排查剧本
- dictionary
title_en: Production Troubleshooting Playbook
authors:
- name: KUDIG ...

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/operations/production-troubleshooting-playbook.md`）*

---

### 15 - SLI/SLO/SLA工程实践

title: 15 - SLI/SLO/SLA工程实践
description: '# 15 - SLI/SLO/SLA工程实践'
category: dictionary
tags:
- k8s
- glossary
- terminology
- prometheus
- grafana
- redis
- postgresql
- job
- webhook
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SLI/SLO/SLA工程实践 是什么
- 如何 SLI/SLO/SLA工程实践
trigger_keywords:
- SLI
- SLO
- SLA工程实践
- dictionary
title_en: Sli Slo Sla Engineering
authors:
- name: KUDIG Team
  role: contributor
k8s_vers...

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/operations/sli-slo-sla-engineering.md`）*

---

### 04 - SRE运维成熟度模型

title: 04 - SRE运维成熟度模型
description: '# 04 - SRE运维成熟度模型'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- apiserver
- scheduler
- prometheus
- grafana
- jaeger
- helm
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- SRE运维成熟度模型 是什么
- 如何 SRE运维成熟度模型
trigger_keywords:
- SRE运维成熟度模型
- dictionary
title_en: Sre Maturity Model
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'...

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/operations/sre-maturity-model.md`）*

---

### 有状态服务运维

虽然 Kubernetes 最初为无状态应用设计，但近年来**有状态工作负载（Stateful Workloads）** 在 K8s 上的运行已日趋成熟。数据库（MySQL、PostgreSQL、MongoDB）、消息队列（Kafka、RabbitMQ）、缓存（Redis）和搜索引擎（Elasticsearch）等关键基础设施组件，越来越多地通过 **StatefulSet** 和 **Operator** 模式部署在 Kubernetes 中。2026 年的最佳实践要求 SRE 掌握有状态服务的高可用、备份恢复、存储性能和滚动升级策略。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/operations/stateful-services-operations.md`）*

---

### Swap 内存管理

Kubernetes 可以配置为在节点上使用 swap（交换）内存，允许内核将不活跃的内存页换出到后备存储，从而释放物理内存。这对具有大内存占用但只在特定时间访问部分内存的工作负载非常有用，也有助于防止 Pod 在内存压力峰值期间被终止，并提高节点内存管理的灵活性。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/operations/swap-memory-management.md`）*

---

## 相关页面

- [[references/k8s-production-operations|k8s-production-operations]]
- [[references/k8s-structured-troubleshooting|k8s-structured-troubleshooting]]

## 来源文件

- `domain-17-system-foundation/topic-dictionary/operations/backup-disaster-recovery.md`
- `domain-17-system-foundation/topic-dictionary/operations/capacity-planning-forecasting.md`
- `domain-17-system-foundation/topic-dictionary/operations/certificates.md`
- `domain-17-system-foundation/topic-dictionary/operations/change-management-release.md`
- `domain-17-system-foundation/topic-dictionary/operations/chaos-engineering.md`
- `domain-17-system-foundation/topic-dictionary/operations/enterprise-ops-practices.md`
- `domain-17-system-foundation/topic-dictionary/operations/failure-patterns-analysis.md`
- `domain-17-system-foundation/topic-dictionary/operations/finops-and-cost-optimization.md`
- `domain-17-system-foundation/topic-dictionary/operations/greenops-and-carbon-aware-computing.md`
- `domain-17-system-foundation/topic-dictionary/operations/incident-management-runbooks.md`
- `domain-17-system-foundation/topic-dictionary/operations/installing-addons.md`
- `domain-17-system-foundation/topic-dictionary/operations/node-autoscaling.md`
- `domain-17-system-foundation/topic-dictionary/operations/node-shutdowns.md`
- `domain-17-system-foundation/topic-dictionary/operations/operations-best-practices.md`
- `domain-17-system-foundation/topic-dictionary/operations/performance-tuning-expert.md`
- `domain-17-system-foundation/topic-dictionary/operations/production-troubleshooting-playbook.md`
- `domain-17-system-foundation/topic-dictionary/operations/sli-slo-sla-engineering.md`
- `domain-17-system-foundation/topic-dictionary/operations/sre-maturity-model.md`
- `domain-17-system-foundation/topic-dictionary/operations/stateful-services-operations.md`
- `domain-17-system-foundation/topic-dictionary/operations/swap-memory-management.md`

## Related

- [[etcd]] — etcd
- [[litmus]] — LitmusChaos
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[entities/argocd|argocd]] — ArgoCD
