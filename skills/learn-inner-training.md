---
title: Kubernetes 培训：Inner Training
description: '- "集群管理"'
category: skills
tags:
- k8s
- learn
- training
- inner-training
- etcd
- apiserver
- kubelet
- scheduler
- prometheus
- grafana
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 45min
intent_queries:
- Kubernetes 培训：Inner Training 是什么
- 如何 Kubernetes 培训：Inner Training
trigger_keywords:
- Kubernetes
- 培训：Inner
- Training
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
created: "2026-05-23"
---

### ACK/ACR/K8S 内部培训 1 个月学习计划

# ACK/ACR/K8S 内部培训 1 个月学习计划

```yaml
---  - "ACK ACR培训内容"
  - "阿里云Kubernetes培训"
  - "一个月学习计划"
  - "内部培训体系"
  - "SRE工程师培训"  - "ACK培训"
  - "ACR培训"
  - "阿里云容器"
  - "Kubernetes培训"
  - "一个月计划"
  - "内部培训"
  - "集群管理"
  - "安全认证"  - 内部运维工程师
  - 技术支持人员
  - SRE工程师
related_domains:
  - domain-01-cluster-fundamentals
  - domain-01-cluster-fundamentals
  - domain-02-workloads-applications
  - domain-05-security-compliance
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/README
  - domain-11-production-operations/topic-learn/inner-training/week-1-ack-acr-lifecycle
  - domain-11-production-operations/topic-learn/inner-training/week-2-security-monitoring
  - domain-11-production-operations/topic-learn/inner-training/week-3-node-workload
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage
id: INNER-TRAINING-001
topic: training
type: training-plan
tags: [training, inner-training, ack, acr, k8s, month-1, k8s-1.28-1.33]
---
```

> **目标人群**: 内部运维工程师、技术支持人员 | **投入**: 4+ 小时/天 | **知识库**: kudig-database (668+ 篇)

---

## 概述

本学习计划为内部运维工程师和技术支持人员设计，覆盖 ACK（阿里云容器服务）、ACR（阿里云容器镜像服务）和 Kubernetes 三大技术栈。通过 28 天的系统性学习，从基础概念到生产运维，逐步建立完整的云原生运维能力。

课程设计遵循知识依赖关系，从 ACK/ACR 服务基础开始，经过安全认证和节点管理，最终掌握网络和存储的核心技能。每个学习阶段有明确的产出目标和评估标准，通过每周的自测检验和实践项目确保学习效果。

**培训目标**: 完成培训后能够独立处理 ACK/ACR/K8S 日常运维工单，具备集群管理、安全配置、故障排查的基本能力。

---

## 快速导航

| 周次 | 主题 | 核心产出 | 目录 |
|------|------|---------|------|
| Week 1 | ACK/ACR 基础与集群生命周期 | 集群全生命周期操作能力 | [we

> *（内容已精简，完整内容请参阅源文件）*

---

### ACK/ACR/K8S 内部培训大纲authors:
- name: KUDIG Team
  role: contributor---

# ACK/ACR/K8S 内部培训大纲

```yaml
---  - "ACK培训课程"
  - "阿里云Kubernetes培训"
  - "四周学习路径"
  - "内部培训体系"
  - "K8s运维培训"  - "ACK培训"
  - "ACR培训"
  - "阿里云容器"
  - "Kubernetes培训"
  - "四周计划"
  - "内部培训"
  - "集群生命周期"
  - "安全认证"  - 内部运维工程师
  - 技术支持人员
  - SRE工程师
related_domains:
  - domain-01-cluster-fundamentals
  - domain-05-security-compliance
  - domain-12-cloud-providers
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/inner-one-month-training
  - domain-11-production-operations/topic-learn/inner-training/week-1-ack-acr-lifecycle
  - domain-11-production-operations/topic-learn/inner-training/week-2-security-monitoring
id: INNER-TRAINING-INDEX-001
topic: training
type: training-plan
tags: [training, inner-training, ack, acr, k8s, month-1, k8s-1.28-1.33]
---
```

## 概述

本培训大纲为内部运维工程师和技术支持人员设计，覆盖 ACK（阿里云容器服务）、ACR（阿里云容器镜像服务）和 Kubernetes 三大技术栈，通过 28 天的系统性学习，从基础概念到生产运维，逐步建立完整的云原生运维能力。

培

> *（内容已精简，完整内容请参阅源文件）*

---

### P1: ACK 集群生命周期管理authors:
- name: KUDIG Team
  role: contributor---

---  - ACK cluster lifecycle management full流程
  - aliyun cs cluster creation deletion upgrade
  - Kubernetes cluster VPC vSwitch network planning
  - ACK cluster certificate renewal
  - Cluster upgrade replacement strategy  - cluster lifecycle
  - create cluster
  - delete cluster
  - upgrade cluster
  - VPC
  - vSwitch
  - certificate
  - CIDR
  - kubeconfig  - ACK beginners
  - DevOps engineers
  - Platform engineers
related_domains:
  - domain-12-cloud-providers
  - domain-01-cluster-fundamentals
  - domain-10-troubleshooting-diagnostics
related_topics:
  - cluster-creation
  - cluster-deletion
  - cluster-upgrade
  - cluster-certificate
---

# P1: ACK 集群生命周期管理

> **对应周次**: Week 1 | **预计时间**: 3-4 小时 | **难度**: ⭐⭐

---

## 概述

本项目将带你完成 ACK 集群的完整生命周期管理：从网络规划、集群创建、节点池配置、集群升级到最终删除清理。全程使用 aliyun CLI + 控制台双通道操作，

> *（内容已精简，完整内容请参阅源文件）*

---

### P2: 安全与监控体系搭建authors:
- name: KUDIG Team
  role: contributor---
---  - ACK RBAC RAM two-layer permission model
  - Prometheus monitoring alerting configuration
  - Kubernetes audit log SLS integration
  - ResourceQuota LimitRange configuration
  - Security hardening best practices  - RBAC
  - RAM
  - permission
  - Prometheus
  - alert
  - audit log
  - SLS
  - ResourceQuota
  - LimitRange
  - security  - ACK operators
  - SRE engineers
  - Security engineers
related_domains:
  - domain-05-security-compliance
  - domain-06-observability
  - domain-12-cloud-providers
related_topics:
  - ram-integration
  - vulnerability
  - risk-prevention
  - cluster-monitoring
---

# P2: 安全与监控体系搭建

> **对应周次**: Week 2 | **预计时间**: 3-4 小时 | **难度**: ⭐⭐⭐

---

## 项目目标

为 ACK 集群搭建完整的安全权限体系和监控告警系统：配置 RBAC + RAM 双层权限，部署 Prometheus 监控，配置审计日志，实施资源配额管理。

## 前置条件

- [ ] 完成 Week 2 全部教案 (Day 8-14)
- [ ] 有运行中的 ACK 集群
- [ ] 拥有 RAM 管理权限
- [ ] 了解 RBAC 和 Prometheu

> *（内容已精简，完整内容请参阅源文件）*

---

### P3: 节点与工作负载管理实践authors:
- name: KUDIG Team
  role: contributor---

---  - ACK multi-nodepool architecture design
  - Kubernetes node maintenance drain uncordon
  - Pod scheduling affinity anti-affinity
  - Kubernetes health probes configuration
  - Cluster autoscaler scaling policy  - nodepool
  - node maintenance
  - drain
  - cordon
  - uncordon
  - scheduling
  - affinity
  - probes
  - Cluster Autoscaler
  - spot instance  - ACK operators
  - SRE engineers
  - Platform engineers
related_domains:
  - domain-3-node
  - domain-9-workload
  - domain-12-cloud-providers
  - domain-10-troubleshooting-diagnostics
related_topics:
  - node-basics
  - node-advanced
  - nodepool-basics
  - nodepool-advanced
  - pod-basics
  - pod-advanced
---

# P3: 节点与工作负载管理实践

> **对应周次**: Week 3 | **预计时间**: 3-4 小时 | **难度**: ⭐⭐⭐

---

## 概述

本实践项目要求你设计一个多节点池架构，完成节点运维操作（扩缩容、维护、升级），部署多种工作负载并配置调度策略与健康检查。通过这个项目，你将综合运用

> *（内容已精简，完整内容请参阅源文件）*

---

### P4: 网络与存储综合实践authors:
- name: KUDIG Team
  role: contributor---

---  - ACK microservice deployment network storage
  - Kubernetes Ingress DNS service discovery
  - StatefulSet PVC persistent storage
  - CNI network policy verification
  - ACK storage CSI integration  - microservice
  - network
  - storage
  - Ingress
  - StatefulSet
  - PVC
  - DNS
  - service discovery
  - CNI
  - NetworkPolicy  - ACK operators
  - Platform engineers
  - DevOps engineers
related_domains:
  - domain-6-networking
  - domain-7-storage
  - domain-12-cloud-providers
  - domain-10-troubleshooting-diagnostics
related_topics:
  - service-networking
  - ingress
  - cni
  - storage
  - pvc
---

# P4: 网络与存储综合实践

> **对应周次**: Week 4 | **预计时间**: 3-4 小时 | **难度**: ⭐⭐⭐

---

## 项目目标

在 ACK 集群中部署一个完整的微服务应用，配置 Service 网络暴露、Ingress 路由、持久化存储，并验证 CNI 网络连通性。

## 前置条件

- [ ] 完成 Week 4 全部教案 (Day 22-28)
- [ ] 有运行中的 ACK 集群
- [ ]

> *（内容已精简，完整内容请参阅源文件）*

---

### P5: 毕业综合项目authors:
- name: KUDIG Team
  role: contributor---

---  - ACK comprehensive cluster operation project
  - Kubernetes multi-tier architecture deployment
  - ACK end-to-end cluster lifecycle management
  - Production-grade cluster security hardening
  - Microservices deployment ACK best practices  - graduation
  - comprehensive
  - full-stack
  - project
  - ACK lifecycle
  - security hardening
  - monitoring
  - alerting
  - network
  - storage  - ACK learners (completion project)
  - DevOps engineers
  - Platform engineers
related_domains:
  - domain-12-cloud-providers
  - domain-05-security-compliance
  - domain-06-observability
  - domain-9-workload
related_topics:
  - ack-cluster-lifecycle
  - security-monitoring
  - node-workload-management
  - network-storage-practice
---

# P5: 毕业综合项目

> **对应周次**: 全部 4 周 | **预计时间**: 6-8 小时 | **难度**: ⭐⭐⭐⭐

---

## 项目

> *（内容已精简，完整内容请参阅源文件）*

---

### ACK/ACR/K8S 命令速查表related_domains:
- domain-12-cloud-providers
- domain-01-cluster-fundamentals
related_topics:
- knowledge-map
- reading-sequence  role: contributor---

# ACK/ACR/K8S 命令速查表

> **适用场景**: 日常运维快速参考 | **更新日期**: 2024

---

## 一、aliyun CLI — ACK 集群管理

### 集群操作

```bash
# 查看集群列表
aliyun cs GET /clusters

# 查看集群详情
aliyun cs GET /clusters/<cluster_id>

# 创建集群
aliyun cs POST /clusters --body '{ ... }'

# 删除集群
aliyun cs DELETE /clusters/<cluster_id>

# 获取 kubeconfig
aliyun cs GET /k8s/<cluster_id>/user_config

# 查看集群升级状态
aliyun cs GET /clusters/<cluster_id>/upgradestatus

# 升级集群
aliyun cs POST /clusters/<cluster_id>/upgrade --body '{"version": "<ver>"}'
```

### 节点池操作

```bash
# 查看节点池列表
aliyun cs GET /clusters/<cluster_id>/nodepools

# 查看节点池详情
aliyun cs GET /clust

> *（内容已精简，完整内容请参阅源文件）*

---

### ACK/ACR/K8S 内部培训知识图谱related_domains:
- domain-12-cloud-providers
- domain-01-cluster-fundamentals
related_topics:
- reading-sequence
- commands-cheatsheet  role: contributor---

# ACK/ACR/K8S 内部培训知识图谱

> 按周组织的核心知识体系，用于系统回顾和查漏补缺

---

## 总览

```
ACK/ACR/K8S 内部培训
├── Week 1: ACK/ACR 基础与集群生命周期
├── Week 2: 安全认证与监控运维
├── Week 3: 节点与工作负载管理
└── Week 4: 网络与存储
```

---

## Week 1: ACK/ACR 基础与集群生命周期

```
ACK/ACR 管控
├── ACK 服务架构
│   ├── 托管版 (ManagedKubernetes)
│   ├── 专有版 (DedicatedKubernetes)
│   └── Serverless (ASK)
├── ACR 镜像服务
│   ├── 个人版 (免费)
│   └── 企业版 (ACR EE)
└── 管控层 SR
    ├── API Server 入口
    └── 区域与可用区

SDK & API
├── OpenAPI (ROA 风格)
│   ├── RESTful 路径设计
│   └── 签名认证
├── aliyun CLI
│   ├── 安装与配置
│   └── cs 子命令

> *（内容已精简，完整内容请参阅源文件）*

---

### 阅读顺序指南related_domains:
- domain-12-cloud-providers
- domain-01-cluster-fundamentals
related_topics:
- knowledge-map
- commands-cheatsheet  role: contributor---

# 阅读顺序指南

> 按天排列的 kudig-database 文档阅读顺序，配合每日教案使用

---

## 使用说明

- 每天在开始教案前，先按顺序阅读对应的参考文档
- 文件路径均相对于 `inner-training/` 目录
- 标注 ⭐ 为核心必读，标注 📖 为补充阅读
- 建议每篇文档阅读时间 15-30 分钟

---

## Week 1: ACK/ACR 基础与集群生命周期

### Day 1: ACK ACR 管控 SR

| 序号 | 文档 | 重点 |
|:---:|------|------|
| ⭐1 | `../../domain-12-cloud-providers/04-alicloud-ack/200-ack-overview.md` | ACK 产品概览与架构 |
| ⭐2 | `../../domain-12-cloud-providers/04-alicloud-ack/205-ack-cluster-types.md` | 集群类型对比 |
| 📖3 | `../../domain-12-cloud-providers/04-alicloud-ack/280-ack-acr-integration.md` | ACR 镜像服务集成 |

### Day 2: ACK SDK & API

| 序号 | 文档 | 重点 |
|:---:|------|-

> *（内容已精简，完整内容请参阅源文件）*

---

### Week 1: ACK/ACR 基础与集群生命周期 (Days 1-7)

---  - ACK ACR cluster lifecycle week 1 curriculum
  - Kubernetes cluster lifecycle management
  - ACK week 1 learning path guide
  - ACK cluster type selection
  - Kubernetes cluster certificate management  - week 1
  - ACK ACR
  - cluster lifecycle
  - 集群生命周期
  - cluster type
  - 集群类型
  - SDK
  - API
  - console
  - 控制台
  - upgrade
  - 升级
  - certificate
  - 证书  - All week 1 learners
  - ACK beginners
  - New joiners
related_domains:
  - domain-12-cloud-providers
  - domain-01-cluster-fundamentals
related_topics:
  - day-1-ack-acr-sr
  - day-2-ack-sdk-api
  - day-3-ack-acr-console
  - day-4-cluster-creation
  - day-5-cluster-deletion
  - day-6-cluster-upgrade
  - day-7-cluster-certificate
---

# Week 1: ACK/ACR 基础与集群生命周期 (Days 1-7)

## 概述

第一周是整个培训计划的基础阶段，聚焦于阿里云容器服务 ACK（Alibaba Cloud Kubernetes）和容器镜像服务 ACR（Alibaba Cloud Container Registry）的核心概念与操作。本周将带你从理解 ACK/ACR 的服务架构开始，逐步掌握 SDK/API 调用、控制台操作，最终能够独立完成集群的创建、删除、升级和证书管理全流程。

ACK 是阿里云提供的托管 Kubernetes 服务，它将 K8s 的复杂性封装起来，让你专注于业务应用的部署和管理。ACR 是企业级的容器镜像仓库服务，支持镜像的安全存储、高效分发和漏洞扫描。作为 K8s 运维工程师，深入理解这两个服务的架构和操作是所有后续工作的基础。

### 学习目标

- 理解 ACK 托管版、专有版、Serverless 三种集群类型的架构差异与选型依据
- 了解 ACR 企业版与个人版的区别及典型使用场景
- 掌握 ACK SDK 的使用方式与核心 API 调用
- 熟悉 ACK/ACR 控制台的功能布局与核心操作
- 能够独立完成集群的创建、删除、升级和证书管理
- **产出**: 能够独立完成集群创建、升级、删除全流程操作

---

## 核心概念详解

### ACK 集群架构与类型选择

阿里云 ACK 提供三

> *（内容已精简，完整内容请参阅源文件）*

---

### Week 1 Checkpoint: 自测检验authors:
- name: KUDIG Team
  role: contributor---

---  - ACK cluster lifecycle self-test quiz
  - Kubernetes week 1 knowledge assessment
  - ACK ACR fundamental concepts test
  - Self-checkpoint quiz questions
  - Knowledge evaluation  - checkpoint
  - self-test
  - quiz
  - assessment
  - week 1
  - evaluation
  - 自我检验
  - 自测  - Week 1 learners
  - ACK beginners
related_domains:
  - domain-12-cloud-providers
  - domain-01-cluster-fundamentals
related_topics:
  - day-1-ack-acr-sr
  - day-2-ack-sdk-api
  - day-3-ack-acr-console
  - day-4-cluster-creation
  - day-5-cluster-deletion
  - day-6-cluster-upgrade
  - day-7-cluster-certificate
---

# Week 1 Checkpoint: 自测检验

> 完成本周学习后，请独立完成以下自测题，不要查阅资料。

---

## 概述

本测验覆盖 Week 1 全部核心知识点，包括 ACK/ACR 服务架构、SDK/API 调用、集群创建/删除/升级流程和证书管理。测验分为四个部分，总计 80 分。答题时间限制 90 分钟。

---

##

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 1: ACK/ACR 管控 SRauthors:
- name: KUDIG Team
  role: contributor---

---  - ACK ACR service architecture overview
  - ACK managed dedicated serverless cluster types
  - ACR personal enterprise edition difference
  - ACK SR service request handling process
  - ACK cluster manager meta-service  - ACK
  - ACR
  - 服务架构
  - 托管版
  - 专有版
  - Serverless
  - 产品形态
  - SR
  - 服务请求
  - 管控层  - All learners
  - New joiners
  - Anyone interested in ACK/ACR
related_domains:
  - domain-12-cloud-providers
  - domain-01-cluster-fundamentals
related_topics:
  - ack-overview
  - ack-cluster-types
  - ack-acr-integration
---

# Day 1: ACK/ACR 管控 SR

> **学习时间**: 4-5 小时 | **主题**: ACK/ACR 服务架构与管控层基本概念

---

## 概述

作为内部培训的第一天，本课程将系统性地介绍阿里云容器服务 ACK（Alibaba Cloud Kubernetes）和容器镜像服务 ACR（Alibaba Cloud Container Registry）的服务架构、产品形态和管控层组件。理解 ACK/ACR 的整体架构是后续所有运维工作的基础——只有了解

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 2: ACK SDK & APIauthors:
- name: KUDIG Team
  role: contributor---

---  - ACK OpenAPI SDK Python JavaScript
  - aliyun cs GET POST DELETE API calls
  - ACK API authentication AK SK STS
  - aliyun CLI installation configuration
  - DescribeClusterUserKubeconfig API  - SDK
  - API
  - OpenAPI
  - aliyun CLI
  - Python SDK
  - authentication
  - AK
  - SK
  - STS
  - RAM role
  - cluster management  - Developers
  - DevOps engineers
  - Platform engineers
related_domains:
  - domain-12-cloud-providers
  - domain-01-cluster-fundamentals
related_topics:
  - ack-overview
  - ack-openapi
  - ack-ram-authorization
---

# Day 2: ACK SDK & API

> **学习时间**: 4-5 小时 | **主题**: ACK SDK 使用与 API 调用方式

---

## 概述

ACK 提供了完整的 OpenAPI 接口，支持通过 aliyun CLI、Python SDK、Java SDK 等多种方式调用。掌握 API 调用是自动化运维的基础——从集群创建到节点管理，从组件安装到证书轮换，所有操作都可以通过 API 完成。今天你将学习 ACK API 的认证机制、核心接口分类，以及如何

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 3: ACK/ACR 控制台 & 功能authors:
- name: KUDIG Team
  role: contributor---

---  - ACK console function modules overview
  - ACR console image management
  - ACK console kubectl command mapping
  - ACK cluster console operations guide
  - Kubernetes console operations tutorial  - console
  - 控制台
  - kubectl
  - operations
  - cluster management
  - node management
  - workload
  - network
  - storage
  - configuration  - All learners
  - Beginners
  - Operations personnel
related_domains:
  - domain-12-cloud-providers
  - domain-01-cluster-fundamentals
related_topics:
  - ack-overview
  - ack-practical-guide
  - kubectl-commands-reference
---

# Day 3: ACK/ACR 控制台 & 功能

> **学习时间**: 4-5 小时 | **主题**: 熟悉 ACK/ACR 控制台界面与核心功能操作

---

## 概述

虽然 kubectl 和 API 是运维自动化的主要工具，但控制台在日常查看、紧急操作和新人上手方面仍然不可替代。今天你将系统性地巡览 ACK 和 ACR 控制台的所有功能模块，理解每个界面背后对应的 K8s 资源和 AP

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 4: K8S 新建集群authors:
- name: KUDIG Team
  role: contributor---
---  - ACK cluster creation process VPC vSwitch
  - Kubernetes cluster network CIDR planning
  - aliyun cs POST clusters API
  - ACK console cluster creation wizard
  - Terway Flannel CNI selection  - create cluster
  - VPC
  - vSwitch
  - CIDR
  - Pod CIDR
  - Service CIDR
  - CNI
  - Terway
  - Flannel
  - cluster creation  - ACK beginners
  - DevOps engineers
  - Platform engineers
related_domains:
  - domain-12-cloud-providers
  - domain-6-networking
  - domain-10-troubleshooting-diagnostics
related_topics:
  - ack-overview
  - ack-vpc-network
  - ack-ecs-compute
---

# Day 4: K8S 新建集群

> **学习时间**: 4-5 小时 | **主题**: 掌握集群创建流程与配置选项

---

## 今日目标

- [ ] 掌握 ACK 集群创建的完整参数配置
- [ ] 理解 VPC/vSwitch/安全组等网络前置依赖
- [ ] 能通过控制台和 API 两种方式创建集群
- [ ] 了解不同集群类型的创建差异

---

## 理论学习 (2h)

### 必读文档

1. **ACK 服务总览与集群类型**
   - 文件: `../../../doma

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 5: K8S 集群删除authors:
- name: KUDIG Team
  role: contributor---

---  - ACK cluster deletion resource cleanup
  - Kubernetes cluster removal retain resources
  - SLB ENI security group cleanup
  - aliyun cs DELETE cluster API
  - Cluster deletion failure troubleshooting  - delete cluster
  - 集群删除
  - resource cleanup
  - 资源清理
  - retain resources
  - 保留资源
  - SLB
  - ENI
  - deletion failure  - ACK operators
  - SRE engineers
  - Platform engineers
related_domains:
  - domain-12-cloud-providers
  - domain-01-cluster-fundamentals
  - domain-10-troubleshooting-diagnostics
related_topics:
  - cluster-lifecycle-management
  - cluster-creation
  - cluster-upgrade
---

# Day 5: K8S 集群删除

> **学习时间**: 4-5 小时 | **主题**: 理解集群删除流程与注意事项

---

## 概述

集群删除是集群生命周期管理的最后一个环节，也是最容易被忽视的环节。不当的删除操作可能导致数据丢失、资源残留、费用持续产生等问题。今天你将学习集群删除的完整流程、删除前的检查清单、保留资源与完全删除的区别，以及删除失败时的排查方法。

---

## 今日目标

- [ ] 掌握集群删除的完整流程和先决条

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 6: K8S 集群升级authors:
- name: KUDIG Team
  role: contributor---

---  - ACK cluster upgrade strategy in-place replacement
  - Kubernetes version upgrade path compatibility
  - kubent API deprecation check upgrade
  - Cluster upgrade verification rollback
  - ACK managed cluster upgrade process  - cluster upgrade
  - version upgrade
  - upgrade path
  - kubent
  - replacement upgrade
  - in-place upgrade
  - API deprecation
  - control plane
  - node upgrade  - ACK operators
  - SRE engineers
  - Platform engineers
related_domains:
  - domain-01-cluster-fundamentals
  - domain-07-platform-engineering
  - domain-12-cloud-providers
  - domain-10-troubleshooting-diagnostics
related_topics:
  - cluster-lifecycle-management
  - upgrade-paths-strategy
  - upgrade-migration-strategy
  - cluster-certificate
---

# Day 6: K8S 集群升级

> **学习时间**: 4-5 小时 | **主题**: 掌握集群版本升级策略与操作步骤

---

## 概述

集群升级是生产环境中

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 7: K8S 集群证书authors:
- name: KUDIG Team
  role: contributor---

---  - Kubernetes certificate system CA API Server etcd
  - ACK kubeconfig certificate renewal
  - Kubernetes certificate expiration troubleshooting
  - kubelet TLS Bootstrap
  - Certificate renewal certrenew API  - certificate
  - CA
  - kubeconfig
  - kubelet
  - TLS
  - etcd
  - API Server
  - certificate renewal
  - certrenew
  - x509  - ACK operators
  - SRE engineers
  - Platform engineers
related_domains:
  - domain-05-security-compliance
  - domain-01-cluster-fundamentals
  - domain-10-troubleshooting-diagnostics
  - domain-12-cloud-providers
related_topics:
  - certificate-management
  - security-architecture
  - certificate-troubleshooting
---

# Day 7: K8S 集群证书

> **学习时间**: 4-5 小时 | **主题**: 理解集群证书管理与更新机制

---

## 概述

本文深入讲解 Kubernetes 集群的证书体系，包括证书类型、有效期管理、轮换机制和故障排查。证书是 K8s 集群安全的基石—

> *（内容已精简，完整内容请参阅源文件）*

---

### Week 2: 安全认证与监控运维 (Days 8-14)

# Week 2: 安全认证与监控运维 (Days 8-14)

```yaml
---  - "Kubernetes安全监控培训"
  - "Week2培训内容"
  - "RBAC权限管理"
  - "审计日志配置"
  - "集群监控搭建"  - "Week2"
  - "安全"
  - "监控"
  - "RBAC"
  - "审计"
  - "配额"
  - "监控告警"
  - "Prometheus"
  - "Grafana"
  - "安全运维"  - sre工程师
  - ops工程师
  - 安全工程师
related_domains:
  - domain-05-security-compliance
  - domain-06-observability
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-1-ack-acr-lifecycle
  - domain-11-production-operations/topic-learn/inner-training/week-3-node-workload
  - domain-11-production-operations/topic-learn/inner-training/week-2-security-monitoring/day-8-rbac
  - domain-11-production-operations/topic-learn/inner-training/week-2-security-monitoring/day-12-cluster-audit
  - domain-11-production-operations/topic-learn/inner-training/week-2-security-monitoring/day-14-quota-license
id: WEEK2-INDEX
topic: training
type: week-index
tags: [week-2, security, monitoring, rbac, audit, k8s, k8s-1.28-1.33]
---
```

## 概述

第二周进入 K8s 集群的安全与监控领域。在第一周中，你掌握了集群的创建、删除、升级等生命周期管理操作。本周将学习如何保护集群安全、识别和防范安全风险、配置审计日志，以及搭建基础监控体系。

安全是生产环境的底线。一个配置不当的 K8s 集群可能面临权限滥用、容器逃逸、数据泄露等严重风险。监控是运维的"眼睛"，没有完善的监控体系，你就无法及时发现问题、更无法在问题发生时快速响应。

### 学习目标

- 深入理解 RBAC 权限模型并能够根据实际需求设计权限方案
- 掌握 RAM（阿里云资源访问管理）与 K8s 权限的集成配置
- 了解 ACK、ACR 和 K8s 常见漏洞类型及其防范措施
- 掌握集群审计日志的配置、采集与分析方法
- 能够搭建基于 Prometheus + Grafana 的基础监控体系
- 了解集群配额管理和 License 管理
- **产出**: 能够配置集群 RBAC 权限、识别安全风险、搭建基础监控

---

## 核心概念详解

### RBAC 权限模型详解

RBAC（Role-Based Acc

> *（内容已精简，完整内容请参阅源文件）*

---

### Week 2 Checkpoint: 自测检验authors:
- name: KUDIG Team
  role: contributor---

# Week 2 Checkpoint: 自测检验

```yaml
---  - "Kubernetes安全自测"
  - "Week2测试题"
  - "RBAC自测"
  - "监控告警测试"  - "自测"
  - "Week2"
  - "RBAC"
  - "审计"
  - "监控"
  - "配额"
  - "ResourceQuota"
  - "LimitRange"
  - "PSS"
  - "Pod安全"  - sre工程师
  - ops工程师
  - 运维工程师
related_domains:
  - domain-05-security-compliance
  - domain-06-observability
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-2-security-monitoring/day-8-rbac
  - domain-11-production-operations/topic-learn/inner-training/week-2-security-monitoring/day-12-cluster-audit
  - domain-11-production-operations/topic-learn/inner-training/week-2-security-monitoring/day-14-quota-license
id: WEEK2-CHECKPOINT
topic: training
type: checkpoint
tags: [week-2, checkpoint, self-test, security, monitoring, k8s, k8s-1.28-1.33]
---
```

> 完成本周学习后，请独立完成以下自测题，不要查阅资料。

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 10: ACK/ACR/K8S 漏洞authors:
- name: KUDIG Team
  role: contributor---

---  - ACK CVE vulnerability scanning ACR image security
  - Kubernetes CVE-2024-21626 runc container escape
  - ACK security bulletin vulnerability management
  - Trivy image scanning Kubernetes cluster
  - Pod security admission PSS configuration  - CVE
  - vulnerability
  - container escape
  - image scan
  - Trivy
  - kube-bench
  - CIS benchmark
  - security baseline
  - CVE-2024-21626
  - runc  - ACK operators
  - Security engineers
  - SRE engineers
related_domains:
  - domain-05-security-compliance
  - domain-10-troubleshooting-diagnostics
  - domain-12-cloud-providers
related_topics:
  - pod-security-standards
  - secret-management
  - RBAC configuration
  - certificate-management
---

# Day 10: ACK/ACR/K8S 漏洞

> **学习时间**: 4-5 小时 | **主题**: 常见漏洞类型与防护措施

---

## 概述

Kubernetes 安全漏洞管理是每

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 11: 风险点识别与防范authors:
- name: KUDIG Team
  role: contributor---

---  - ACK Kubernetes security risk assessment checklist
  - Pod Security Standards PSS configuration
  - Kubernetes security baseline hardening
  - NetworkPolicy zero trust security
  - SecurityContext container hardening  - security risk
  - PSS
  - Pod Security Standards
  - Baseline
  - Restricted
  - NetworkPolicy
  - SecurityContext
  - privilege escalation
  - defense in depth
  - RBAC minimum privilege  - ACK operators
  - Security engineers
  - SRE engineers
related_domains:
  - domain-05-security-compliance
  - domain-10-troubleshooting-diagnostics
  - domain-12-cloud-providers
related_topics:
  - pod-security-standards
  - rbac-configuration
  - network-policy
  - secret-management
---

# Day 11: 风险点识别与防范

> **学习时间**: 4-5 小时 | **主题**: 安全风险评估与最佳实践

---

## 概述

安全是 K8s 生产环境的底线。一个配置不当的集群可能面临容器逃逸、权限

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 12: K8S 集群审计authors:
- name: KUDIG Team
  role: contributor---

# Day 12: K8S 集群审计

```yaml
---  - "Kubernetes审计日志"
  - "审计日志配置"
  - "SLS日志分析"
  - "API Server审计"
  - "安全审计"  - "审计"
  - "审计日志"
  - "Audit"
  - "SLS"
  - "日志分析"
  - "API Server"
  - "审计策略"
  - "合规"  - sre工程师
  - 安全工程师
  - 运维工程师
related_domains:
  - domain-05-security-compliance
  - domain-06-observability
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-2-security-monitoring/day-8-rbac
  - domain-11-production-operations/topic-learn/inner-training/week-2-security-monitoring/day-14-quota-license
  - domain-06-observability/03-logging-architecture
id: WEEK2-DAY12
topic: training
type: hands-on
tags: [week-2, day-12, audit, security, logging, k8s, k8s-1.28-1.33]
---
```

> **学习时间**: 4-5 小时 | **主题**: 审计日志配置与分析方法

---

## 概述

Kubernetes 审计（Audit）是集群安全体系中的重要组成部分，它记录了集群中发生的所有 API 操作，包括谁在什么时候对什么资源执行了什么操作。审计日志是安全合规、问题追溯和操作审计的基础，在安全事件响应中发挥着关键作用。

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 13: K8S 集群监控authors:
- name: KUDIG Team
  role: contributor---

---  - ACK ARMS Prometheus monitoring configuration
  - Kubernetes PrometheusQuery PromQL queries
  - Grafana dashboard Kubernetes monitoring
  - PrometheusRule alerting rules configuration
  - kube-state-metrics cluster monitoring  - Prometheus
  - Grafana
  - ARMS
  - monitoring
  - alerting
  - metrics
  - PromQL
  - ServiceMonitor
  - kube-state-metrics
  - node-exporter  - SRE engineers
  - ACK operators
  - Platform engineers
related_domains:
  - domain-06-observability
  - domain-10-troubleshooting-diagnostics
  - domain-12-cloud-providers
related_topics:
  - monitoring-metrics-system
  - alerting-management
  - prometheus-monitoring
---

# Day 13: K8S 集群监控

> **学习时间**: 4-5 小时 | **主题**: 监控体系搭建与告警配置

---

## 概述

监控是运维的"眼睛"，没有完善的监控体系，你就无法及时发现和定位问题。K8s 集群监控通常基于 Prometheus + Grafana 方案，在 ACK

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 14: K8S 集群配额 & Licenseauthors:
- name: KUDIG Team
  role: contributor---

# Day 14: K8S 集群配额 & License

```yaml
---  - "Kubernetes资源配额"
  - "ResourceQuota"
  - "LimitRange"
  - "QoS等级"
  - "ACK配额管理"  - "ResourceQuota"
  - "LimitRange"
  - "配额"
  - "QoS"
  - "资源限制"
  - "容器资源"
  - "requests"
  - "limits"
  - "集群配额"  - sre工程师
  - ops工程师
  - 运维工程师
related_domains:
  - domain-02-workloads-applications
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-2-security-monitoring/checkpoint
  - domain-02-workloads-applications/23-resource-management
  - domain-10-troubleshooting-diagnostics/24-quota-limitrange-troubleshooting
id: WEEK2-DAY14
topic: training
type: hands-on
tags: [week-2, day-14, quota, resource, limitrange, k8s, k8s-1.28-1.33]
---
```

> **学习时间**: 4-5 小时 | **主题**: 资源配额管理与许可证相关

---

## 概述

在多团队共享集群的场景下，资源配额管理是保障公平性和稳定性的关键机制。今天你将学习 K8s 原生的 Resour

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 8: K8S 集群 RBACauthors:
- name: KUDIG Team
  role: contributor---

# Day 8: K8S 集群 RBAC

```yaml
---  - "Kubernetes RBAC"
  - "Role ClusterRole"
  - "RoleBinding"
  - "权限配置"
  - "ServiceAccount"  - "RBAC"
  - "Role"
  - "ClusterRole"
  - "RoleBinding"
  - "ClusterRoleBinding"
  - "权限"
  - "ServiceAccount"
  - "最小权限"
  - "多租户"
  - "kubectl auth can-i"  - sre工程师
  - 安全工程师
  - 运维工程师
related_domains:
  - domain-05-security-compliance
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-2-security-monitoring/day-12-cluster-audit
  - domain-11-production-operations/topic-learn/inner-training/week-2-security-monitoring/day-14-quota-license
  - domain-05-security-compliance/01-authentication-authorization-system
  - domain-05-security-compliance/07-rbac-matrix-configuration
id: WEEK2-DAY8
topic: training
type: hands-on
tags: [week-2, day-8, rbac, security, authorization, k8s, k8s-1.28-1.33]
---
```

> **学习时间**: 4-5

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 9: RAM 账号管理authors:
- name: KUDIG Team
  role: contributor---
---  - ACK RAM authorization Kubernetes RBAC integration
  - aliyun cs grant_permissions RAM user cluster access
  - RAM role assume role Kubernetes
  - Multi-team RBAC namespace isolation
  - kubeconfig RAM user authentication  - RAM
  - RBAC
  -权限管理
  - grant_permissions
  - kubeconfig
  - 双层权限
  - 云平台权限
  - 集群权限
  - AssumeRole  - ACK operators
  - DevOps engineers
  - Platform engineers
related_domains:
  - domain-05-security-compliance
  - domain-12-cloud-providers
  - domain-10-troubleshooting-diagnostics
related_topics:
  - rbac-configuration
  - ack-cluster-lifecycle
  - certificate-management
---

# Day 9: RAM 账号管理

> **学习时间**: 4-5 小时 | **主题**: RAM 账号与 K8S 集成方案

---

## 今日目标

- [ ] 理解阿里云 RAM 与 ACK 权限的映射关系
- [ ] 掌握 RAM 用户授权 ACK 集群的操作流程
- [ ] 了解 RAM 角色扮演在 ACK 中的应用
- [ ] 能够为不同团队配置分级权限

---

## 理论学习 (2h)

### 必读文档

1. **ACK RAM 授权**
   - 文件: `../../../domain-17-c

> *（内容已精简，完整内容请参阅源文件）*

---

### Week 3: 节点与工作负载管理 (Days 15-21)

---  - ACK week 3 node workload management curriculum
  - Kubernetes node pool management learning path
  - Pod lifecycle scheduling management
  - Kubernetes core components operations
  - Week 3 project based learning  - week 3
  - node
  - workload
  - 节点
  - 工作负载
  - nodepool
  - 节点池
  - Pod
  - 调度
  - component
  - 组件  - All week 3 learners
  - ACK operators
  - SRE engineers
related_domains:
  - domain-3-node
  - domain-9-workload
  - domain-12-cloud-providers
related_topics:
  - node-basics
  - node-advanced
  - nodepool-basics
  - nodepool-advanced
  - pod-basics
  - pod-advanced
  - component-ops
---

# Week 3: 节点与工作负载管理 (Days 15-21)

## 概述

第三周进入 Kubernetes 运维的核心实战领域——节点管理与工作负载管理。在前两周中，你已经了解了集群的生命周期管理（创建、删除、升级）和安全监控体系。本周将深入到集群内部的日常运维操作：如何管理 Node 节点、如何使用节点池实现高效运维、如何管理 Pod 的生命周期与调度、以及如何维护 K8s 核心组件的稳定运行。

节点和工作负载是 Kubernetes 最基础也最重要的两个概念。节点是集群的计算资源单元，工作负载（尤其是 Pod）是应用的运行载体。一个优秀的 K8s 运维工程师需要深刻理解这两个层面的工作原理，才能在问题发生时快速定位问题、在架构设计时做出合理决策。

### 学习目标

- 深入理解 Node 节点的架构组成、状态机制与日常管理操作
- 掌握 ACK 节点池的概念、创建配置、扩缩容与生命周期管理
- 理解 Pod 的完整生命周期、健康检查机制与调度策略
- 掌握 K8s 核心组件（kube-apiserver、etcd、kube-scheduler 等）的运维方法
- **产出**: 能够独立管理节点池、排查 Pod 问题、维护 K8s 核心组件

---

## 核心概念详解

### Node 节点架构深度解析

Kubernetes 中的每个 Node 节点都运行着三个核心组件：**kubelet**、**kube-proxy** 和 **容器运行时（Container Runtime）**。

**kubelet** 是节点上的"大管家"。它通过 Watch 机制持续监

> *（内容已精简，完整内容请参阅源文件）*

---

### Week 3 自测: 节点与工作负载管理authors:
- name: KUDIG Team
  role: contributor---

---  - Kubernetes week 3 self-test assessment
  - Node and workload knowledge test
  - Node management troubleshooting quiz
  - Pod scheduling self-check
  - Cluster autoscaler troubleshooting  - checkpoint
  - self-test
  - quiz
  - week 3
  - 节点
  - 工作负载
  - 自测
  - 评估  - Week 3 learners
  - ACK beginners
related_domains:
  - domain-3-node
  - domain-9-workload
  - domain-10-troubleshooting-diagnostics
related_topics:
  - node-basics
  - node-advanced
  - nodepool-basics
  - nodepool-advanced
  - pod-basics
  - pod-advanced
  - component-ops
---

# Week 3 自测: 节点与工作负载管理

> **满分**: 50 分 | **建议用时**: 60 分钟

---

## 概述

Week 3 的学习聚焦于 Kubernetes 集群中最核心的运维对象——节点（Node）和工作负载（Workload）。节点是运行 Pod 的物理机或虚拟机，理解节点的状态管理、调度约束和维护操作是保障集群稳定运行的基础。工作负载是运行业务应用的载体，掌握 Pod 的生命周期管理、资源配额和

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 15: Node 节点基础authors:
- name: KUDIG Team
  role: contributor---

---  - Kubernetes Node architecture kubelet kube-proxy
  - Node status conditions Ready NotReady
  - Node capacity allocatable resource management
  - Kubernetes node monitoring troubleshooting
  - containerd CRI interface  - Node
  - kubelet
  - kube-proxy
  - containerd
  - Ready
  - NotReady
  - MemoryPressure
  - DiskPressure
  - capacity
  - allocatable
  - resource management  - ACK operators
  - SRE engineers
  - Platform engineers
related_domains:
  - domain-3-node
  - domain-10-troubleshooting-diagnostics
  - domain-12-cloud-providers
related_topics:
  - node-overview
  - node-management
  - node-notready-diagnosis
---

# Day 15: Node 节点基础

## 概述

Node（节点）是 Kubernetes 集群的工作引擎，是实际运行容器应用的地方。每个 Node 都是一台物理机或虚拟机，上面运行着三个核心组件：kubelet、kube-proxy 和容器运行时（containerd）。理解 Node 的架构、状态机制和管理操作是 K8s 运维的基础。

今天的学习从理解

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 16: Node 节点进阶authors:
- name: KUDIG Team
  role: contributor---

---  - Kubernetes node labels management
  - Node taint toleration mechanism
  - Node maintenance cordon drain uncordon
  - Kubernetes node scheduling constraints
  - ACK node maintenance operations  - node labels
  - 节点标签
  - taint
  - 污点
  - toleration
  - 容忍
  - cordon
  - drain
  - uncordon
  - maintenance
  - 维护
  - nodeSelector
  - nodeAffinity  - ACK operators
  - SRE engineers
  - Platform engineers
related_domains:
  - domain-3-node
  - domain-10-troubleshooting-diagnostics
  - domain-12-cloud-providers
related_topics:
  - node-basics
  - node-management
  - pod-scheduling-strategies
---

# Day 16: Node 节点进阶

> **学习时间**: 4-5 小时 | **主题**: 节点维护、标签与调度约束

---

## 概述

节点（Node）是 Kubernetes 集群中运行工作负载的基础设施单元。在生产环境中，节点的日常维护、标签管理和调度约束配置是运维工程师最频繁操作的任务之一。无论是进行节点硬件升级、内核补丁安

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 17: 节点池基础authors:
- name: KUDIG Team
  role: contributor---

---  - ACK node pool concept architecture
  - ACK managed vs self-managed node pool
  - Node pool creation configuration
  - Node pool scaling management
  - Multi-layer node pool architecture design  - nodepool
  - 节点池
  - managed nodepool
  - 托管节点池
  - self-managed nodepool
  - 自管理节点池
  - auto repair
  - auto upgrade
  - 节点池架构
  - scaling
  - 扩缩容  - ACK operators
  - SRE engineers
  - Platform engineers
related_domains:
  - domain-3-node
  - domain-12-cloud-providers
  - domain-10-troubleshooting-diagnostics
related_topics:
  - nodepool-advanced
  - node-basics
  - ack-ecs-compute
---

# Day 17: 节点池基础

## 概述

节点池（NodePool）是阿里云 ACK 的核心概念之一，它将一组具有相同配置的节点组织在一起进行统一管理。在传统的 K8s 集群中，管理员需要逐个管理节点，这在节点数量较多时非常低效。节点池解决了这个问题——你可以通过一个配置来管理数十甚至数百个节点，包括它们的实例规格、网络配置、标签和污点。

理解节点池对于 ACK 运维至关重要，因为几乎所有的节点管理操作（扩容、缩容、升级、维护）都是以节点池为粒度进行的。

### 学习目

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 18: 节点池进阶authors:
- name: KUDIG Team
  role: contributor---

---  - ACK Cluster Autoscaler configuration
  - Spot instance mixed scaling policy
  - Node pool upgrade management
  - Node pool scaling troubleshooting
  - PDB PodDisruptionBudget configuration  - Cluster Autoscaler
  - auto scaling
  - Spot
  - 抢占式实例
  - nodepool upgrade
  - 节点池升级
  - PDB
  - PodDisruptionBudget
  - scale up
  - scale down  - ACK operators
  - SRE engineers
  - Platform engineers
related_domains:
  - domain-3-node
  - domain-12-cloud-providers
  - domain-10-troubleshooting-diagnostics
related_topics:
  - nodepool-basics
  - cluster-autoscaler-troubleshooting
  - pod-scheduling
---

# Day 18: 节点池进阶

## 概述

在 Day 17 学习了节点池基础之后，今天将深入节点池的高级特性：自动伸缩策略配置、Spot 实例混合策略、节点池升级流程、以及节点池故障排查。这些进阶能力是生产环境运维的核心技能。

### 学习目标

- 掌握节点池自动伸缩的原理和配置
- 理解 Spot 实例与按量付费混合策略以降低成本
- 掌握节点池升级（K8s 版本和 OS 补丁）的操作流程
- 能够排查节点池相关的常见问题

---

## 核心概念详解

### 自动伸缩原理

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 19: Pod 容器组基础authors:
- name: KUDIG Team
  role: contributor---

---  - Kubernetes Pod lifecycle Pending Running Succeeded Failed
  - Pod container debugging logs exec
  - Kubernetes Sidecar multi-container pattern
  - Init Container initialization
  - Pod restartPolicy configuration  - Pod
  - lifecycle
  - Pending
  - Running
  - Sidecar
  - Init Container
  - restartPolicy
  - logs
  - exec
  - container  - ACK operators
  - Developers
  - SRE engineers
related_domains:
  - domain-9-workload
  - domain-10-troubleshooting-diagnostics
  - domain-12-cloud-providers
related_topics:
  - pod-overview
  - pod-lifecycle
  - pod-troubleshooting
---

# Day 19: Pod 容器组基础

> **学习时间**: 4-5 小时 | **主题**: Pod 生命周期与基本操作

---

## 概述

本文深入讲解 Kubernetes 中最核心的概念——Pod。Pod 是 K8s 调度的最小单元，理解 Pod 的生命周期、多容器模式、健康检查和基本操作是所有 K8s 运维工作的基础。通过本文的学习，你将掌握 Pod 的创建、查看、调试和删除操作，以及 Sidecar 多容器模式和 Init Container 的使用场景。

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 20: Pod 容器组进阶authors:
- name: KUDIG Team
  role: contributor---
---  - Kubernetes Pod scheduling affinity anti-affinity
  - Pod liveness readiness startup probes
  - Pod resources requests limits
  - Kubernetes nodeSelector nodeAffinity
  - Pod tolerations taint  - Pod scheduling
  - affinity
  - anti-affinity
  - nodeAffinity
  - nodeSelector
  - tolerations
  - taint
  - livenessProbe
  - readinessProbe
  - startupProbe
  - resources
  - requests
  - limits  - ACK operators
  - Developers
  - SRE engineers
related_domains:
  - domain-9-workload
  - domain-10-troubleshooting-diagnostics
  - domain-12-cloud-providers
related_topics:
  - pod-scheduling-strategies
  - pod-probes
  - pod-lifecycle
---

# Day 20: Pod 容器组进阶

> **学习时间**: 4-5 小时 | **主题**: Pod 调度、探针与资源配置

---

## 今日目标

- [ ] 掌握 Pod 调度策略 (nodeSelector / affinity / tolerations)
- [ ] 理解健康探针 (liveness / readiness / startup)
- [ ] 能够配置 Pod 资

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 21: K8S 组件运维authors:
- name: KUDIG Team
  role: contributor---

---  - Kubernetes control plane components运维
  - CoreDNS troubleshooting DNS resolution
  - kube-proxy iptables IPVS mode
  - CNI Terway Flannel network troubleshooting
  - API Server etcd health check  - component operations
  - 组件运维
  - CoreDNS
  - kube-proxy
  - CNI
  - Terway
  - Flannel
  - CSI
  - API Server
  - etcd
  - health check
  - component upgrade  - ACK operators
  - SRE engineers
  - Platform engineers
related_domains:
  - domain-01-cluster-fundamentals
  - domain-10-troubleshooting-diagnostics
  - domain-12-cloud-providers
related_topics:
  - apiserver-deep-dive
  - etcd-deep-dive
  - coredns-troubleshooting
  - kube-proxy-troubleshooting
---

# Day 21: K8S 组件运维

> **学习时间**: 4-5 小时 | **主题**: 核心组件状态检查与故障处理

---

## 概述

本文是 Kubernetes 组件运维的实战指南，帮助你掌握 K8s 核心组件（API Server、etcd、Controlle

> *（内容已精简，完整内容请参阅源文件）*

---

### Week 4: 网络与存储 (Day 22-28)

# Week 4: 网络与存储 (Day 22-28)

```yaml
---  - "Kubernetes网络存储培训"
  - "Week4培训内容"
  - "Service Ingress学习"
  - "PV PVC StorageClass"  - "Week4"
  - "网络与存储"
  - "Service"
  - "Ingress"
  - "Terway"
  - "Flannel"
  - "PV"
  - "PVC"
  - "StorageClass"
  - "CNI"  - sre工程师
  - ops工程师
  - 运维工程师
related_domains:
  - domain-03-networking-traffic
  - domain-04-storage-data
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-3-node-workload
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-22-service-basics
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-23-ingress
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-24-terway-cni
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-25-flannel-cni
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-26-storage-create-delete
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-27-storage-mount
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-28-comprehensive-review
id: WEEK4-INDEX
topic: training
type: week-index
tags: [week-4, networking, storage, service, ingress, k8s, k8s-1.28-1.33]
---
```

## 概述

第四周聚焦于 K8s 中两个最核心的基础设施领域：**网络**和**存储**。在前面三周中，你已经掌握了集群管理、安全监控和节点工作负载管理。本周将深入理解 K8s 的网络模型和存储体系，这是运行生产级应用的基础。

网络是 K8s 中最复杂的子系统之一。K8s 的网络模型要求每个 Pod 都有独立的 IP 地址，且 Pod 之间可以直接通信，无需 NAT。这个看似简单的要求背后涉及 CNI 插件、Service 负载均

> *（内容已精简，完整内容请参阅源文件）*

---

### Week 4 自测: 网络与存储authors:
- name: KUDIG Team
  role: contributor---

# Week 4 自测: 网络与存储

```yaml
---  - "Kubernetes网络存储自测"
  - "Week4测试题"
  - "Service Ingress自测"
  - "PV PVC测试"  - "自测"
  - "Week4"
  - "网络"
  - "存储"
  - "Service"
  - "Ingress"
  - "PV"
  - "PVC"
  - "Terway"
  - "Flannel"  - sre工程师
  - ops工程师
  - 运维工程师
related_domains:
  - domain-03-networking-traffic
  - domain-04-storage-data
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-22-service-basics
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-23-ingress
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-26-storage-create-delete
id: WEEK4-CHECKPOINT
topic: training
type: checkpoint
tags: [week-4, checkpoint, self-test, networking, storage, k8s, k8s-1.28-1.33]
---
```

> **满分**: 50 分 | **建议用时**: 60 分钟

---

## 概述

Week 4 是整个培训的收官阶段，涵盖了 Kubernetes 网络和存储两大核心基

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 22: Service 基础authors:
- name: KUDIG Team
  role: contributor---

# Day 22: Service 基础

```yaml
---  - "Kubernetes Service类型"
  - "ClusterIP NodePort LoadBalancer"
  - "Service Endpoints"
  - "kube-proxy配置"
  - "ACK SLB集成"  - "Service基础"
  - "ClusterIP"
  - "NodePort"
  - "LoadBalancer"
  - "Headless Service"
  - "Endpoints"
  - "kube-proxy"
  - "SLB负载均衡"  - sre工程师
  - ops工程师
  - 运维工程师
related_domains:
  - domain-03-networking-traffic
  - domain-04-storage-data
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-23-ingress
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-24-terway-cni
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-25-flannel-cni
id: WEEK4-DAY22
topic: training
type: hands-on
tags: [week-4, day-22, service, networking, k8s, k8s-1.28-1.33]
---
```

> **学习时间**: 4-5 小时 | **主题**: Service 类型与配置实践

-

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 23: Ingressauthors:
- name: KUDIG Team
  role: contributor---
# Day 23: Ingress

```yaml
---  - "Kubernetes Ingress配置"
  - "Ingress Controller"
  - "Nginx Ingress"
  - "ALB Ingress"
  - "IngressClass"  - "Ingress"
  - "Ingress Controller"
  - "Nginx Ingress"
  - "ALB Ingress"
  - "IngressClass"
  - "TLS证书"
  - "灰度发布"
  - "金丝雀发布"  - sre工程师
  - ops工程师
  - 运维工程师
related_domains:
  - domain-03-networking-traffic
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-22-service-basics
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-24-terway-cni
  - domain-03-networking-traffic/19-ingress-fundamentals
id: WEEK4-DAY23
topic: training
type: hands-on
tags: [week-4, day-23, ingress, networking, k8s, k8s-1.28-1.33]
---
```

> **学习时间**: 4-5 小时 | **主题**: Ingress 路由规则与控制器配置

---

## 今日目标

- [ ] 理解 Ingress 资源与 IngressClass 概念
- [ ] 掌握 ACK 中 ALB Ingress Controller 和 Nginx Ingress

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 24: Terway 网络authors:
- name: KUDIG Team
  role: contributor---
# Day 24: Terway 网络

```yaml
---  - "Terway CNI"
  - "阿里云网络插件"
  - "Terway ENIIP模式"
  - "Kubernetes CNI"
  - "Pod网络配置"  - "Terway"
  - "Terway CNI"
  - "ENIIP"
  - "ENI模式"
  - "弹性网卡"
  - "VPC网络"
  - "NetworkPolicy"
  - "CNI插件"  - sre工程师
  - ops工程师
  - 运维工程师
related_domains:
  - domain-03-networking-traffic
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-25-flannel-cni
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-23-ingress
  - domain-03-networking-traffic/02-cni-architecture-fundamentals
id: WEEK4-DAY24
topic: training
type: hands-on
tags: [week-4, day-24, terway, cni, networking, k8s, k8s-1.28-1.33]
---
```

> **学习时间**: 4-5 小时 | **主题**: Terway CNI 架构与配置

---

## 今日目标

- [ ] 理解 Terway CNI 的整体架构
- [ ] 掌握 Terway 的三种模式 (VPC / ENI / ENIIP)
- [ ] 能够查看和排查 Terway 网络配置

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 25: Flannel 网络authors:
- name: KUDIG Team
  role: contributor---
# Day 25: Flannel 网络

```yaml
---  - "Flannel CNI"
  - "VxLAN"
  - "Kubernetes网络"
  - "Flannel配置"
  - "Pod CIDR"  - "Flannel"
  - "Flannel CNI"
  - "VxLAN"
  - "Overlay网络"
  - "Pod CIDR"
  - "网络插件"
  - "Flannel配置"  - sre工程师
  - ops工程师
  - 运维工程师
related_domains:
  - domain-03-networking-traffic
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-24-terway-cni
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-23-ingress
  - domain-03-networking-traffic/02-cni-architecture-fundamentals
id: WEEK4-DAY25
topic: training
type: hands-on
tags: [week-4, day-25, flannel, cni, networking, k8s, k8s-1.28-1.33]
---
```

> **学习时间**: 4-5 小时 | **主题**: Flannel 网络模型与故障排查

---

## 今日目标

- [ ] 理解 Flannel CNI 的架构与 VxLAN 模式
- [ ] 掌握 Flannel 网络下的 Pod CIDR 分配
- [ ] 能排查 Flannel 网络常见问题
- [ ] 

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 26: 存储卷创建 & 删除authors:
- name: KUDIG Team
  role: contributor---

# Day 26: 存储卷创建 & 删除

```yaml
---  - "Kubernetes PV PVC"
  - "StorageClass"
  - "存储卷创建删除"
  - "阿里云云盘"
  - "动态供给"  - "PV"
  - "PVC"
  - "StorageClass"
  - "云盘"
  - "NAS"
  - "OSS"
  - "存储卷"
  - "动态供给"
  - "CSI"  - sre工程师
  - ops工程师
  - 运维工程师
related_domains:
  - domain-04-storage-data
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-27-storage-mount
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/checkpoint
  - domain-04-storage-data/01-storage-architecture-overview
id: WEEK4-DAY26
topic: training
type: hands-on
tags: [week-4, day-26, storage, pv, pvc, k8s, k8s-1.28-1.33]
---
```

> **学习时间**: 4-5 小时 | **主题**: PV/PVC 创建与生命周期管理

---

## 概述

本文深入讲解 Kubernetes 存储体系的核心机制——PV/PVC/StorageClass 的创建、绑定和生命周期管理。存储是有状态应用（数据库、消息队列）运行的基石。在 ACK 环境中，你将学习如何使用阿里云的云盘（ESSD）、NAS、OSS 等存储产品，通过静态和动态

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 27: 存储卷挂载authors:
- name: KUDIG Team
  role: contributor---
# Day 27: 存储卷挂载

```yaml
---  - "Kubernetes存储挂载"
  - "Volume挂载"
  - "PVC挂载"
  - "ConfigMap Secret挂载"
  - "StatefulSet存储"  - "存储挂载"
  - "Volume"
  - "PVC"
  - "ConfigMap"
  - "Secret"
  - "subPath"
  - "emptyDir"
  - "hostPath"
  - "StatefulSet"
  - "volumeMounts"  - sre工程师
  - ops工程师
  - 运维工程师
related_domains:
  - domain-04-storage-data
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-26-storage-create-delete
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/checkpoint
  - domain-04-storage-data/02-pv-architecture-fundamentals
id: WEEK4-DAY27
topic: training
type: hands-on
tags: [week-4, day-27, storage, volume, mount, k8s, k8s-1.28-1.33]
---
```

> **学习时间**: 4-5 小时 | **主题**: 存储挂载方式与最佳实践

---

## 今日目标

- [ ] 掌握 Volume、PVC、ConfigMap、Secret 等多种挂载方式
- [ ] 能为 Deployment/StatefulSet 配置持久化存储
- [ ] 了解 subPath、readOnly 等

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 28: 综合复习与实践authors:
- name: KUDIG Team
  role: contributor---
# Day 28: 综合复习与实践

```yaml
---  - "Kubernetes四周培训复习"
  - "综合复习题目"
  - "培训自测"
  - "K8s知识点回顾"  - "综合复习"
  - "四周总结"
  - "自测"
  - "培训考核"
  - "K8s复习"  - sre工程师
  - ops工程师
  - 运维工程师
related_domains:
  - domain-01-cluster-fundamentals
  - domain-02-workloads-applications
  - domain-03-networking-traffic
  - domain-04-storage-data
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/checkpoint
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-22-service-basics
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-26-storage-create-delete
id: WEEK4-DAY28
topic: training
type: review
tags: [week-4, day-28, review, checkpoint, k8s, k8s-1.28-1.33]
---
```

> **学习时间**: 4-5 小时 | **主题**: 全流程实操与问题答疑

---

## 今日目标

- [ ] 回顾 4 周核心知识点
- [ ] 完成端到端的综合实操演练
- [ ] 识别个人薄弱环节并制定补强计划
- [ ] 为毕业项目做准备

---

> *（内容已精简，完整内容请参阅源文件）*

## Related

- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/resource-management|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[pod-lifecycle]] — Pod Lifecycle
- [[concepts/service-networking|service-networking]] — Service Networking
