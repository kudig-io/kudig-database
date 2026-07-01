---
title: Kubernetes 培训：Public Training
description: '### K8s 运维实战培训（四周体系）'
summary: '### K8s 运维实战培训（四周体系）'
category: skills
tags:
- k8s
- learn
- training
- public-training
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 1h
intent_queries:
- Kubernetes 培训：Public Training 是什么
- 如何 Kubernetes 培训：Public Training
trigger_keywords:
- Kubernetes
- 培训：Public
- Training
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- etcd-basics
- redis-basics
- mysql-basics
- gpu-scheduling-basics
- policy-basics
- logging-basics
---



### K8s8s 运维实战培训（四周体系）|K8s 运维实战培训（四周体系）]]

# K8s 运维实战培训（四周体系）

```yaml
---  - "K8s培训课程"
  - "四周学习路径"
  - "实操教程索引"
  - "SRE工程师培训"
  - "Kubernetes认证准备"  - "K8s培训"
  - "四周学习"
  - "实操培训"
  - "运维工程师"
  - "集群管理"
  - "安全监控"
  - "故障排查"
  - "云原生运维"  - sre工程师
  - ops工程师
  - 运维工程师
related_domains:
  - domain-01-cluster-fundamentals
  - domain-02-workloads-applications
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/quick-start
  - domain-10-troubleshooting-diagnostics/topic-skills/assessment/k8s-fundamentals-quiz
  - domain-17-system-foundation/topic-dictionary/k8s-glossary
id: TRAINING-INDEX-001
topic: training
type: index
tags: [training, learning-path, week-1-4, hands-on, k8s-1.28-1.33]
---
```

> **适用版本**: K8s 1.28-1.33 | **目标受众**: SRE / Ops 工程师 | **最后更新**: 2026-05

---

## 概述

本培训体系为 K8s 运维工程师设计，覆盖从零基础到独立处理 oncall 工单的完整学习路径。培训周期 28 天（四周），采用每日主题学习 + 实操练习的形式，结合理论知识与真实场景演练，确保学以致用。

### 培训设计理念

| 原则 | 说明 |
|------|------|
| 渐进式学习 | 基础 → 核心 → 进阶 → 综合，每周难度递增 |
| 理论 + 实操 | 每日 2h 理论 + 2.5h 实操，知识与实践结合 |
| 费曼复述 | 每日用自己的语言复述核心概念，检验理解深度 |
| 项目驱动 | 每周一个综合项目，整合本周所学 |
| 自测评估 | 每周 checkpoint 自测，识别薄弱环节 |

---

## 培训概述

- **培训周期**: 28 天（四周）
- **培训形式**: 每日主题学习 + 实操练习
- **目标**: 从入门到能够独立处理 oncall 工单

---

## 学习路径

```
Week 1: 集群生命周期管理 (基础)
  ↓
Week 2: 安全监控运维 (进阶)
  ↓
Week 3: 节点与工作负载管理 (进阶)
  ↓
Week 4: 网络与存储 (进阶)
```

---

## Week 1: 集群生命周期（基础）

> **目标**: 掌握集群创建、升级、删除全流程

| Day | 主题 | 文件 |
|:---:|------|------|
| Day 1 | ACK/ACR 管控架构（云厂商） | 

> *（内容已精简，完整内容请参阅源文件）*

---

### Kubernetes 生产运维 1 个月学习计划

# Kubernetes 生产运维 1 个月学习计划

```yaml
---  - "Kubernetes运维学习计划"
  - "一个月学习路径"
  - "云原生工程师培训"
  - "全栈运维课程"  - "K8s学习"
  - "一个月计划"
  - "云原生工程师"
  - "全栈运维"
  - "Docker"
  - "Kubernetes"
  - "监控排障"
  - "GitOps"  - 运维工程师
  - sre工程师
  - devops工程师
related_domains:
  - domain-01-cluster-fundamentals
  - domain-01-cluster-fundamentals
  - domain-02-workloads-applications
  - domain-03-networking-traffic
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/public-training/README
  - domain-11-production-operations/topic-learn/quick-start
  - domain-10-troubleshooting-diagnostics
id: PUBLIC-TRAINING-PLAN-001
topic: training
type: training-plan
tags: [training, 28-days, k8s, learning-path, sre, devops, k8s-1.28-1.33]
---
```

> **目标人群**: 入门级 -> 全栈运维 | **投入**: 4+ 小时/天 | **知识库**: kudig-database (668+ 篇)

---

## 概述

本学习计划旨在帮助运维工程师在一个月内从 Kubernetes 入门级提升到全栈运维能力。课程设计遵循"理论 + 实践"的黄金比例（40% 理论 : 60% 实践），每天 4-5 小时的学习时间，通过 28 天的系统性学习，覆盖从集群搭建到生产运维的完整知识体系。

整个学习计划基于 kudig-database 知识库中 668+ 篇文档构建，分为四个阶段：地基建设期（Week 1）、核心技术构建期（Week 2）、运维作战能力期（Week 3）、企业级进阶期（Week 4）。每个阶段有明确的产出目标和评估标准，确保学习效果可量化验证。

**学习路线核心思路**: 先建立全局认知（架构全貌），再深入核心组件（控制平面、网络、存储），然后构建运维能力（监控、排障、安全），最后掌握企业级实践（GitOps、生产运维）。

---

## 快速导航

| 周次 | 主题 | 核心产出 | 目录 |
|------|------|---------|------|
| Week 1 | 地基建设期 | K8s 集群环境 + 架构图 | [week-1-foundation/](./week-1-foundation/) |
| Week 2 | 核心技术构建期 | 生产级应用 YAML 编排 | [week-2-core-

> *（内容已精简，完整内容请参阅源文件）*

---

### 项目 P1: 从零搭建 K8s 集群authors:
- name: KUDIG Team
  role: contributor---

---  - kind kubernetes 集群本地搭建教程
  - kubectl 基本命令操作示例
  - 从零创建 deployment service namespace
  - k8s 故障排查入门练习  - kind
  - kubectl
  - Deployment
  - Service
  - Namespace
  - 集群搭建
  - 入门
  - 滚动更新
  - 回滚  - beginner-devops
  - platform-engineer
  - developer
related_domains:
  - domain-01-cluster-fundamentals
  - domain-02-workloads-applications
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p2-production-app-orchestration
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p3-observability-fault-drill
---

# 项目 P1: 从零搭建 K8s 集群

> **所属周**: Week 1 | **预计时间**: 2.5 小时

---

## 概述

本项目将带你从零开始搭建一个完整的 Kubernetes 集群，并在其上部署一个可访问的 nginx Web 应用。通过这个项目，你将实践 Week 1 学到的所有核心知识：Docker 容器化、Kubernetes 架构理解、kubectl 命令行操作、以及 Pod/Deployment/Service 三大核心资源的使用。

项目使用 kind（Kub

> *（内容已精简，完整内容请参阅源文件）*

---

### 项目 P2: 生产级应用全栈编排authors:
- name: KUDIG Team
  role: contributor---

---  - kubernetes statefulset deployment hpa 完整部署示例
  - 生产环境 k8s 应用架构怎么设计
  - k8s 网络策略 ingress service 联动配置
  - pvc storageclass dynamic provisioning 配置  - StatefulSet
  - HPA
  - Ingress
  - NetworkPolicy
  - StorageClass
  - PVC
  - Headless Service
  - 滚动更新
  - 动态供给
  - 生产级部署  - sre-engineer
  - devops-engineer
  - platform-engineer
related_domains:
  - domain-02-workloads-applications
  - domain-03-networking-traffic
  - domain-04-storage-data
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p1-k8s-cluster-setup
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p3-observability-fault-drill
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p5-graduation-project
---

# 项目 P2: 生产级应用全栈编排

> **所属周**: Week 2 | **预计时间**: 2.5 小时

---

## 概述

本实践项目要

> *（内容已精简，完整内容请参阅源文件）*

---

### 项目 P3: 可观测性体系搭建 + 故障演练authors:
- name: KUDIG Team
  role: contributor---

---  - prometheus grafana loki alertmanager 完整部署
  - k8s 可观测性体系搭建步骤
  - 故障注入演练 fta febm 方法论
  - kube-prometheus-stack 部署配置  - Prometheus
  - Grafana
  - Loki
  - Alertmanager
  - PrometheusRule
  - 可观测性
  - 故障演练
  - FTA
  - FEBM
  - 监控告警  - sre-engineer
  - devops-engineer
  - platform-engineer
related_domains:
  - domain-06-observability
  - domain-10-troubleshooting-diagnostics
  - domain-20-enterprise-monitoring-alerting
  - topic-fta
  - topic-febm
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-17-observability-1
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-18-observability-2
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-19-troubleshooting-methodology
---

# 项目 P3: 可

> *（内容已精简，完整内容请参阅源文件）*

---

### 项目 P4: GitOps 流水线authors:
- name: KUDIG Team
  role: contributor---

---  - argocd gitops 完整部署配置
  - kustomize 多环境管理 base overlays
  - argocd application 同步策略配置
  - 多集群 gitops 部署方案  - ArgoCD
  - GitOps
  - Kustomize
  - Application
  - SyncPolicy
  - multi-environment
  - base
  - overlays
  - 自动同步
  - 回滚  - sre-engineer
  - devops-engineer
  - platform-engineer
related_domains:
  - domain-08-release-change-management
  - domain-11-production-operations
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/day-23-logging-gitops
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p5-graduation-project
---

# 项目 P4: GitOps 流水线

> **所属周**: Week 4 | **预计时间**: 2 小时

---

## 概述

本实践项目要求你使用 ArgoCD 搭建一个完整的 GitOps 流水线，包括多环境部署（dev/staging/prod）、自动同步和手动审批策略、以及回滚操作。GitOps 是现代云原生应用部署的推荐模式，理解其工作原理对于构建自动化、可审计的部署流程至关重要。

### 项目目标

使用 ArgoCD 搭建 GitOps 流水线：
- 部署 ArgoCD 并完成初始配置
- 创建 GitO

> *（内容已精简，完整内容请参阅源文件）*

---

### 项目 P5: 毕业综合实践项目authors:
- name: KUDIG Team
  role: contributor---

---  - kubernetes 毕业项目生产级平台搭建完整方案
  - k8s 全栈部署包含哪些组件
  - argocd gitops 完整项目实战
  - pvc prometheus grafana loki 一体化部署  - GitOps
  - ArgoCD
  - Prometheus
  - Grafana
  - Loki
  - 毕业项目
  - 生产级架构
  - 全栈部署
  - 故障排查手册
  - 变更管理  - sre-engineer
  - devops-engineer
  - platform-engineer
related_domains:
  - domain-02-workloads-applications
  - domain-03-networking-traffic
  - domain-04-storage-data
  - domain-05-security-compliance
  - domain-06-observability
  - domain-10-troubleshooting-diagnostics
  - domain-11-production-operations
  - domain-08-release-change-management
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p1-k8s-cluster-setup
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p2-production-app-orchestration
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p3-observability-fault-drill
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p4-

> *（内容已精简，完整内容请参阅源文件）*

---

### 🔥 Kubernetes 生产运维实战训练营 🔥authors:
- name: KUDIG Team
  role: contributor---
<div align="center">

```yaml
---  - "Kubernetes运维培训"
  - "28天训练营"
  - "SRE工程师培训"
  - "云原生运维课程"  - "K8s培训"
  - "28天课程"
  - "SRE训练营"
  - "云原生"
  - "生产运维"
  - "故障排查"
  - "监控告警"
  - "GitOps"  - sre工程师
  - devops工程师
  - 运维工程师
  - 开发工程师转型
related_domains:
  - domain-01-cluster-fundamentals
  - domain-02-workloads-applications
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/quick-start
  - domain-11-production-operations/topic-learn/public-training/one-month/README
id: PUBLIC-TRAINING-BOOT-001
topic: training
type: landing-page
tags: [training, bootcamp, 28-days, k8s, sre, devops, k8s-1.28-1.33]
---
```

# 🔥 Kubernetes 生产运维实战训练营 🔥

### ━━━━━━━━ 28 天，从入门到全栈运维 ━━━━━━━━

<br/>

> **"别再只是看文档了。28 天后，你就是团队里那个能扛事儿的人。"**

<br/>

---

## ⏰ 每晚 20:00 - 21:00 | 直播授课 + 实时答疑

---

</div>

<br/>

## 你是不是也有这些困扰？

- 😫 看了一堆 K8s 文档，遇到生产

> *（内容已精简，完整内容请参阅源文件）*

---

### K8s 命令速查表related_domains:
- domain-01-cluster-fundamentals
- domain-10-troubleshooting-diagnostics
related_topics:
- domain-11-production-operations/topic-learn/public-training/one-month/resources/reading-sequence
- domain-11-production-operations/topic-learn/public-training/one-month/resources/knowledge-map  role: contributor---

# K8s 命令速查表

常用 kubectl 命令快速参考，按使用场景分类整理。

---

## 集群信息

```bash
# 集群基本信息
kubectl cluster-info                          # 显示集群 API 地址
kubectl version                               # 客户端和服务端版本
kubectl config view                           # 查看 kubeconfig 配置
kubectl config current-context                # 当前上下文
kubectl config get-contexts                   # 所有上下文列表

# 切换集群/上下文
kubectl config use-context <context-name>     # 切换上下文
kubectl config set-context --current --namespace=<ns>  # 切换默认 namespace

# 节点信息
kubectl get nodes                             # 节点列表
kubectl get nodes -o

> *（内容已精简，完整内容请参阅源文件）*

---

### 知识图谱模板related_domains:
- domain-01-cluster-fundamentals
- domain-02-workloads-applications
- domain-03-networking-traffic
- domain-05-security-compliance
- domain-06-observability
related_topics:
- domain-11-production-operations/topic-learn/public-training/one-month/resources/reading-sequence
- domain-11-production-operations/topic-learn/public-training/one-month/resources/commands-cheatsheet  role: contributor---

# 知识图谱模板

使用此模板记录你的学习成果，构建个人知识图谱。每完成一个模块的学习，在对应区域用自己的语言总结核心概念、记录仍需加强的领域，并画出你理解的架构图。

---

## Week 1: 地基建设期

### Docker

**核心概念:**
- [ ] Docker Engine 架构（Client-Server 模型、daemon、containerd）
- [ ] 镜像 vs 容器（镜像 = 只读模板，容器 = 运行实例）
- [ ] Union Filesystem（分层存储、Copy-on-Write）
- [ ] 网络模式 (bridge/host/overlay/none)
- [ ] 存储 (Volume/Bind Mount/tmpfs)

**我的理解:**
```
Docker 是容器化平台，核心是 Linux Namespace（隔离）和 Cgroup（资源限制）。
镜像通过分层存储实现高效复用，容器运行时共享宿主机内核。

关键命令速查:
docker build -t app:v1 .         # 构建镜像
docker run -d -p 80:80 nginx     # 运行容器
docker exec -it <id> sh          # 进入容器
d

> *（内容已精简，完整内容请参阅源文件）*

---

### 文档阅读顺序索引related_domains:
- domain-01-cluster-fundamentals
- domain-02-workloads-applications
- domain-03-networking-traffic
- domain-05-security-compliance
- domain-06-observability
- domain-10-troubleshooting-diagnostics
related_topics:
- domain-11-production-operations/topic-learn/public-training/one-month/resources/knowledge-map
- domain-11-production-operations/topic-learn/public-training/one-month/resources/commands-cheatsheet  role: contributor---

# 文档阅读顺序索引

## 概述

本文档按学习计划顺序整理了 kudig-database 知识库中的关键文档，帮助你按照从基础到进阶、从理论到实践的路径系统化地学习 Kubernetes 运维知识。

知识库中的文档按域名（domain）组织，每个域名覆盖一个技术领域。但学习时需要按特定顺序跨域名阅读，而不是按域名逐一学习。本文档的作用就是为你规划最优的阅读路径。

### 使用方法

- 按周次和天次顺序阅读
- 标有 ⭐ 的为核心文档，必须精读
- 其他文档按需阅读，可根据时间调整深度
- 每个文档预计阅读时间 30-60 分钟
- 建议阅读时做笔记，记录关键概念和疑问点

---

## Week 1: 地基建设期

本周的学习目标是建立容器技术、Linux 运维和 K8s 架构的完整认知。这三者是所有后续学习的基础。

### Day 1-2: Docker 基础与进阶

Docker 是 Kubernetes 运行容器的底层技术。这两天的学习帮助你理解"容器到底是什么"，并掌握 Docker 的基本操作。

1. `domain-13-container-runtime/01-docker-architecture-overview.md`
   - Docker Engine 的架构组成：Client、Daemon、Registry 

> *（内容已精简，完整内容请参阅源文件）*

---

### Week 1: 地基建设期 (Days 1-7)

# Week 1: 地基建设期 (Days 1-7)

```yaml
---
id: LEARN-ONE-MONTH-W1-README
topic: kubernetes
type: guide
tags: [week-1, docker, linux, kubernetes, namespace, cgroup, one-month]  - "Docker 容器本质是什么"
  - "Linux namespace 和 cgroup 区别"
  - "K8s Master/Node 架构"
  - "kubectl 基础命令"  - 容器
  - Docker
  - namespace
  - cgroup
  - UnionFS
  - 镜像分层
  - Kubernetes
  - Master
  - Node
  - kubectl
  - 集群部署
  - 声明式管理  - sre
  - ops-engineer
  - developer
related_domains:
  - domain-13-container-runtime
  - domain-17-system-foundation
  - domain-01-cluster-fundamentals
related_topics:
  - docker
  - linux
  - kubernetes
  - container  - domain-11-production-operations/topic-learn/public-training/one-month/week-2-core-tech/README.md
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p1-k8s-cluster-setup.md
---
```

## 概述

第一周是整个一个月学习计划的基石阶段。Kubernetes 是一个复杂的分布式系统，要真正掌握它，需要先打好三个基础：**容器技术（Docker）**、**Linux 运维基础**和 **K8s 架构全貌**。这三个模块环环相扣——Docker 是 K8s 运行容器的底层技术，Linux 是 K8s 节点的操作系统，K8s 架构则是所有后续学习的技术地图。

本周的学习目标是帮助你建立从"容器是什么"到"能部署第一个应用到 K8s 集群"的完整认知链条。我们不追求面面俱到，而是聚焦于最核心、最实用的知识点，为后续三周的深入学习打下坚实基础。

### 学习目标

- 理解容器的本质（namespace + cgroup + UnionFS）并掌握 Docker 完整生命周期
- 具备 Linux 运维基础能力（进程管理、网络配置、文件系统、性能分析）
- 理解 K8s Master/Node 架构全貌并流利使用 kubectl 命令
- 成功部署一个 K8s 集群并运行第一个 Deployment
- **产出**: 成功部署一个 K8s 集群，跑通第一个 Deployment

---

## 核心概念详解

### 容器技术本质

容器技术的核心是 Linux 内核提供的三项隔离能力：*

> *（内容已精简，完整内容请参阅源文件）*

---

### Week 1 Checkpoint: 自测检验authors:
- name: KUDIG Team
  role: contributor---

# Week 1 Checkpoint: 自测检验

```yaml
---
id: LEARN-ONE-MONTH-W1-CHECKPOINT
topic: kubernetes
type: checkpoint
tags: [checkpoint, self-test, week-1, docker, linux, kubernetes, namespace, cgroup]  - "K8s Week 1 自测题"
  - "Docker 容器原理题"
  - "namespace cgroup 区别"
  - "K8s 架构组件题"  - 自测
  - checkpoint
  - 概念理解
  - 命令实操
  - 场景分析
  - 综合设计
  - 评分标准
  - 薄弱点
  - 知识点速查  - sre
  - ops-engineer
  - developer
related_domains:
  - domain-13-container-runtime
  - domain-17-system-foundation
  - domain-01-cluster-fundamentals
  - domain-10-troubleshooting-diagnostics
related_topics:
  - docker
  - linux
  - kubernetes
  - troubleshooting  - domain-11-production-operations/topic-learn/public-training/one-month/week-1-foundation/README.md
  - domain-11-production-operations/topic-learn/public-training/one-month/week-1-foundation/day-1-docker-basics.md
---
```

> 完成本周学习后，请独立完成以下

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 1: Docker 容器基础authors:
- name: KUDIG Team
  role: contributor---

# Day 1: Docker 容器基础

```yaml
---
id: LEARN-ONE-MONTH-W1-DAY1
topic: docker
type: hands-on-guide
tags: [docker, container, image, dockerfile, build, run, namespace, cgroup, hands-on, week-1]  - "Docker 容器本质是什么"
  - "Dockerfile 怎么写"
  - "镜像构建优化怎么做"
  - "Docker 和 K8s 什么关系"  - Docker
  - 容器
  - 镜像
  - Dockerfile
  - docker build
  - docker run
  - docker pull
  - docker ps
  - docker logs
  - docker exec
  - Namespace
  - Cgroup
  - UnionFS
  - 镜像分层
  - 容器生命周期  - sre
  - ops-engineer
  - developer
related_domains:
  - domain-13-container-runtime
  - domain-17-system-foundation
related_topics:
  - docker
  - container
  - image  - domain-11-production-operations/topic-learn/public-training/one-month/week-1-foundation/day-2-docker-advanced.md
  - domain-13-container-runtime/01-docker-fundamentals-concepts.md
---
```

## 概述

Docker 是学习 Kubernetes 的第一块基石。Kubernetes 的核心功能就是编排和管理容器，而 Docker 是目前

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 2: Docker 网络 + 存储 + 安全authors:
- name: KUDIG Team
  role: contributor---

# Day 2: Docker 网络 + 存储 + 安全

```yaml
---
id: LEARN-ONE-MONTH-W1-DAY2
topic: docker
type: hands-on-guide
tags: [docker, network, bridge, host, overlay, volume, bind-mount, security, hands-on, week-1]  - "Docker 网络模式有哪些"
  - "Volume 和 Bind Mount 区别"
  - "Docker 安全最佳实践"
  - "docker-compose 网络怎么配"  - Docker 网络
  - bridge
  - host
  - overlay
  - none
  - macvlan
  - Volume
  - Bind Mount
  - tmpfs
  - docker-compose
  - 非 root 用户
  - 资源限制
  - 只读文件系统  - sre
  - ops-engineer
  - developer
related_domains:
  - domain-13-container-runtime
related_topics:
  - docker
  - networking
  - storage
  - security  - domain-11-production-operations/topic-learn/public-training/one-month/week-1-foundation/day-1-docker-basics.md
  - domain-13-container-runtime/04-docker-networking-deep-dive.md
---
```

> **学习时间**: 4-5 小时 | **主题**: Docke

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 3: Linux 核心基础authors:
- name: KUDIG Team
  role: contributor---

# Day 3: Linux 核心基础

```yaml
---
id: LEARN-ONE-MONTH-W1-DAY3
topic: linux
type: hands-on-guide
tags: [linux, namespace, cgroup, process, signal, system-call, container, hands-on, week-1]  - "Linux namespace 是什么"
  - "cgroup 怎么限制资源"
  - "容器隔离原理是什么"
  - "进程信号怎么用"  - Linux
  - namespace
  - PID namespace
  - NET namespace
  - MNT namespace
  - cgroup
  - control group
  - CPU限制
  - 内存限制
  - 进程
  - 信号
  - SIGTERM
  - SIGKILL
  - 系统调用
  - 容器原理  - sre
  - ops-engineer
  - developer
related_domains:
  - domain-17-system-foundation
  - domain-13-container-runtime
related_topics:
  - linux
  - container
  - namespace
  - cgroup  - domain-11-production-operations/topic-learn/public-training/one-month/week-1-foundation/day-4-linux-network.md
  - domain-17-system-foundation/01-linux-system-architecture.md
---
```

> **学习时间**: 4-5 小时 | **主题**

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 4: Linux 网络 + 性能调优authors:
- name: KUDIG Team
  role: contributor---
# Day 4: Linux 网络 + 性能调优

```yaml
---
id: LEARN-ONE-MONTH-W1-DAY4
topic: linux
type: hands-on-guide
tags: [linux, network, ip, iptables, tcpdump, sysctl, performance, tuning, hands-on, week-1]  - "Linux 网络命令怎么用"
  - "ip netns 网络命名空间怎么用"
  - "iptables NAT 规则怎么看"
  - "K8s 节点内核参数怎么调优"  - ip addr
  - ip route
  - ip netns
  - veth
  - iptables
  - NAT
  - tcpdump
  - ss
  - sysctl
  - ip_forward
  - 内核调优
  - 网络排障  - sre
  - ops-engineer
related_domains:
  - domain-17-system-foundation
  - domain-03-networking-traffic
related_topics:
  - linux
  - networking
  - performance  - domain-11-production-operations/topic-learn/public-training/one-month/week-1-foundation/day-3-linux-core.md
  - domain-17-system-foundation/04-linux-networking-configuration.md
---
```

> **学习时间**: 4-5 小时 | **主题**: Linux 网络配置与内核调优

---

## 今日目标

- [ ] 掌握 Linux 网络配置 (ip、iptables、路由)
- [ ] 理解网络命名空间 (K8

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 5: Kubernetes 架构全貌authors:
- name: KUDIG Team
  role: contributor---

# Day 5: Kubernetes 架构全貌

```yaml
---
id: LEARN-ONE-MONTH-W1-DAY5
topic: kubernetes
type: hands-on-guide
tags: [kubernetes, architecture, master, node, etcd, apiserver, scheduler, kubelet, hands-on, week-1]  - "K8s 整体架构是什么"
  - "Master/Node 组件有哪些"
  - "kubectl apply 执行流程"
  - "kind 集群怎么搭建"  - Kubernetes
  - Master
  - Node
  - etcd
  - API Server
  - Scheduler
  - Controller Manager
  - kubelet
  - kube-proxy
  - kind
  - minikube
  - kubectl
  - 架构图  - sre
  - ops-engineer
  - developer
related_domains:
  - domain-01-cluster-fundamentals
  - domain-10-troubleshooting-diagnostics
related_topics:
  - kubernetes
  - architecture
  - kubectl  - domain-11-production-operations/topic-learn/public-training/one-month/week-1-foundation/day-6-k8s-cluster.md
  - domain-1-architecture-f

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 6: K8s 架构深化 + 集群配置authors:
- name: KUDIG Team
  role: contributor---

# Day 6: K8s 架构深化 + 集群配置

```yaml
---
id: LEARN-ONE-MONTH-W1-DAY6
topic: kubernetes
type: hands-on-guide
tags: [kubernetes, deployment, service, rolling-update, rollback, api-version, hands-on, week-1]  - "Deployment 完整配置怎么写"
  - "滚动更新怎么配置"
  - "回滚怎么做"
  - "Service 怎么暴露应用"  - Deployment
  - RollingUpdate
  - maxSurge
  - maxUnavailable
  - Rollback
  - RevisionHistoryLimit
  - Service
  - ClusterIP
  - NodePort
  - Endpoints
  - kubectl apply
  - 声明式管理  - sre
  - ops-engineer
  - developer
related_domains:
  - domain-01-cluster-fundamentals
  - domain-02-workloads-applications
related_topics:
  - kubernetes
  - deployment
  - service
  - rollout  - domain-11-production-operations/topic-learn/public-training/one-month/week-1-foundation/day-5-k8s-architecture.md
  - domain-02-workloads-applications/02-deploym

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 7: 周复习 + 综合实践authors:
- name: KUDIG Team
  role: contributor---

# Day 7: 周复习 + 综合实践

```yaml
---
id: LEARN-ONE-MONTH-W1-DAY7
topic: kubernetes
type: hands-on-guide
tags: [review, practice, kind, deployment, service, kubectl, troubleshooting, hands-on, week-1]  - "K8s 综合实践项目"
  - "Kind 集群怎么创建"
  - "Deployment 完整部署流程"
  - "声明式管理怎么理解"  - 综合实践
  - kind
  - 集群搭建
  - kubectl
  - 声明式管理
  - 故障排查
  - 产出文档
  - 滚动更新
  - 回滚
  - 资源清单  - sre
  - ops-engineer
  - developer
related_domains:
  - domain-01-cluster-fundamentals
  - domain-10-troubleshooting-diagnostics
related_topics:
  - kubernetes
  - kubectl
  - deployment
  - troubleshooting  - domain-11-production-operations/topic-learn/public-training/one-month/week-1-foundation/checkpoint.md
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p1-k8s-cluster-setup.md
---
```

> **学习时间**: 4-5 小时 | **主题**: Week 1 总结与实践项目

---

## 概述

今天是第一周的收官

> *（内容已精简，完整内容请参阅源文件）*

---

### Week 2: 核心技术构建期 (Days 8-14)

# Week 2: 核心技术构建期 (Days 8-14)

```yaml
---
id: LEARN-ONE-MONTH-W2-README
topic: kubernetes
type: guide
tags: [week-2, control-plane, workloads, networking, storage, kubernetes, one-month]  - "K8s 控制平面组件有哪些"
  - "Deployment StatefulSet DaemonSet 区别"
  - "K8s 网络栈包括什么"
  - "PV/PVC/StorageClass 关系"
  - "HPA 自动扩缩容原理"  - 控制平面
  - etcd
  - API Server
  - Scheduler
  - Controller Manager
  - 工作负载
  - Deployment
  - StatefulSet
  - 网络
  - CNI
  - Service
  - Ingress
  - 存储
  - PV
  - PVC
  - StorageClass
  - CSI  - sre
  - ops-engineer
  - developer
related_domains:
  - domain-01-cluster-fundamentals
  - domain-02-workloads-applications
  - domain-03-networking-traffic
  - domain-04-storage-data
related_topics:
  - kubernetes
  - control-plane
  - workloads
  - networking
  - storage  - domain-11-production-operations/topic-learn/public-training/one-month/week-1-foundation/README.md
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-devops-toolchain/README.md
---
```

## 概述

第二周是整个学习计划中技术密度最高的一周。在第一周的地基之上，本周将深入 K8s 的四大核心技术领域：**控制平面**、**工作负载**、**网络栈**和**存储体系**。这四个领域覆盖了 K8s 日常运维中 80% 以上的工作内容，是成为合格的 K8s 运维工程师必须跨越的技术门槛。

本周的学习理念是"理解原理 + 动手实践"。每个主题都包含理论讲解和对应的实操任务，确保你不仅能"知道是什么"，更能"知道为什么"和"知道怎么做"。

### 学习目标

- 深入理解控制平面各组件（etcd、API Server、Scheduler、Controller Manager）的工作机制
- 掌握所有主要工作负载类型（Deployment、StatefulSet、DaemonSet、Job、CronJob）及生产级配置模式
- 掌握 K8s 网络栈的完整体系（CNI、Servi

> *（内容已精简，完整内容请参阅源文件）*

---

### Week 2 Checkpoint: 自测检验authors:
- name: KUDIG Team
  role: contributor---

# Week 2 Checkpoint: 自测检验

```yaml
---
id: LEARN-ONE-MONTH-W2-CHECKPOINT
topic: kubernetes
type: checkpoint
tags: [checkpoint, self-test, week-2, deployment, statefulset, service, ingress, pvc, hpa, networkpolicy]  - "K8s Week 2 自测题"
  - "Deployment 滚动更新题"
  - "StatefulSet vs Deployment 区别"
  - "Service 转发原理"
  - "PV/PVC 动态供应流程"  - 自测
  - checkpoint
  - 概念理解
  - 命令实操
  - 场景分析
  - 综合设计
  - 评分标准
  - 薄弱点
  - 知识点速查  - sre
  - ops-engineer
  - developer
related_domains:
  - domain-02-workloads-applications
  - domain-03-networking-traffic
  - domain-04-storage-data
  - domain-10-troubleshooting-diagnostics
related_topics:
  - workloads
  - networking
  - storage
  - troubleshooting  - domain-11-production-operations/topic-learn/public-training/one-month/week-2-core-tech/README.md
  - domain-11-production-operations/topic-learn/public-training/one-month/week-2-core-

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 10: 工作负载 - Deployment + StatefulSet + DaemonSetauthors:
- name: KUDIG Team
  role: contributor---
# Day 10: 工作负载 - Deployment + StatefulSet + DaemonSet

```yaml
---
id: LEARN-ONE-MONTH-W2-DAY10
topic: kubernetes
type: hands-on-guide
tags: [deployment, statefulset, daemonset, replicaset, rolling-update, rollback, hands-on, week-2]  - "Deployment 滚动更新怎么配置"
  - "StatefulSet 和 Deployment 区别"
  - "DaemonSet 什么场景用"
  - "maxSurge/maxUnavailable 怎么设置"  - Deployment
  - StatefulSet
  - DaemonSet
  - ReplicaSet
  - RollingUpdate
  - maxSurge
  - maxUnavailable
  - Rollback
  - revisionHistoryLimit
  - Headless Service
  - volumeClaimTemplate
  - topologyKey
  - nodeSelector  - sre
  - ops-engineer
  - developer
related_domains:
  - domain-02-workloads-applications
  -

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 11: 工作负载 - Pod 生命周期 + 资源管理 + HPAauthors:
- name: KUDIG Team
  role: contributor---

# Day 11: 工作负载 - Pod 生命周期 + 资源管理 + HPA

```yaml
---
id: LEARN-ONE-MONTH-W2-DAY11
topic: kubernetes
type: hands-on-guide
tags: [pod, lifecycle, probe, resources, qos, hpa, vpa, autoscaling, hands-on, week-2]  - "Pod 生命周期怎么理解"
  - "Liveness/Readiness 探针怎么配"
  - "resources requests/limits 怎么设置"
  - "QoS 等级是什么"
  - "HPA 怎么配置"  - Pod Lifecycle
  - Init Container
  - PostStart
  - PreStop
  - LivenessProbe
  - ReadinessProbe
  - StartupProbe
  - resources
  - requests
  - limits
  - QoS
  - Guaranteed
  - Burstable
  - BestEffort
  - HPA
  - HorizontalPodAutoscaler
  - autoscaling  - sre
  - ops-engineer
  - developer
related_domains:
  - domain-02-workloads-applications
  - domain-10-troubleshooting-diagnostics
related_topics:
  - workloads
  - pod
  -

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 12: 网络栈 - CNI + Service + DNSauthors:
- name: KUDIG Team
  role: contributor---
# Day 12: 网络栈 - CNI + Service + DNS

```yaml
---
id: LEARN-ONE-MONTH-W2-DAY12
topic: kubernetes
type: hands-on-guide
tags: [cni, service, dns, coredns, iptables, ipvs, networking, hands-on, week-2]  - "K8s 网络模型是什么"
  - "Service 四种类型怎么选"
  - "CoreDNS 怎么工作"
  - "iptables vs IPVS 区别"  - CNI
  - Container Network Interface
  - Service
  - ClusterIP
  - NodePort
  - LoadBalancer
  - DNS
  - CoreDNS
  - kube-proxy
  - iptables
  - IPVS
  - Endpoints
  - Service Discovery  - sre
  - ops-engineer
  - developer
related_domains:
  - domain-03-networking-traffic
  - domain-10-troubleshooting-diagnostics
related_topics:
  - networking
  - service
  - dns
  - cni  - domain-11-production-operations/topic-learn/public-training/one-month/week-2-core-tech/day-13-networking-2.md
  - d

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 13: 网络栈 - Ingress + NetworkPolicyauthors:
- name: KUDIG Team
  role: contributor---

# Day 13: 网络栈 - Ingress + NetworkPolicy

```yaml
---
id: LEARN-ONE-MONTH-W2-DAY13
topic: kubernetes
type: hands-on-guide
tags: [ingress, networkpolicy, nginx-ingress, tls, hostname, path, routing, hands-on, week-2]  - "Ingress 怎么配置"
  - "Ingress Controller 怎么选"
  - "TLS 证书怎么配"
  - "NetworkPolicy 怎么写"
  - "L7 路由怎么实现"  - Ingress
  - Ingress Controller
  - Nginx Ingress
  - TLS
  - hostname
  - path
  - rewrite
  - canary
  - NetworkPolicy
  - egress
  - ingress
  - podSelector
  - namespaceSelector
  - ipBlock  - sre
  - ops-engineer
  - developer
related_domains:
  - domain-03-networking-traffic
  - domain-10-troubleshooting-diagnostics
related_topics:
  - networking
  - ingress
  - networkpolicy
  - tls  - t

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 14: 存储体系 + 综合实践authors:
- name: KUDIG Team
  role: contributor---

# Day 14: 存储体系 + 综合实践

```yaml
---
id: LEARN-ONE-MONTH-W2-DAY14
topic: kubernetes
type: hands-on-guide
tags: [pv, pvc, storageclass, dynamic-provisioning, statefulset, csi, hands-on, week-2]  - "PV/PVC 静态供应怎么配置"
  - "StorageClass 动态供应怎么用"
  - "NFS 存储怎么配置"
  - "StatefulSet 存储怎么管理"  - PersistentVolume
  - PersistentVolumeClaim
  - StorageClass
  - Dynamic Provisioning
  - CSI
  - NFS
  - volumeClaimTemplate
  - reclaimPolicy
  - accessModes
  - WaitForFirstConsumer  - sre
  - ops-engineer
related_domains:
  - domain-04-storage-data
  - domain-10-troubleshooting-diagnostics
related_topics:
  - storage
  - pv
  - pvc
  - statefulset  - domain-11-production-operations/topic-learn/public-training/one-month/week-2-core-tech/day-12-networking-1.md
  - domain-04-storage-data/04-storageclass-dynamic-provisioning.md
---
```

> 

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 8: 控制平面 - etcd + API Serverauthors:
- name: KUDIG Team
  role: contributor---

# Day 8: 控制平面 - etcd + API Server

```yaml
---
id: LEARN-ONE-MONTH-W2-DAY8
topic: kubernetes
type: hands-on-guide
tags: [etcd, apiserver, control-plane, raft, authentication, authorization, admission, hands-on, week-2]  - "etcd 集群怎么工作"
  - "Raft 协议是什么"
  - "API Server 请求处理链"
  - "认证授权准入控制怎么配"
  - "etcd 备份恢复怎么做"  - etcd
  - API Server
  - Control Plane
  - Raft
  - Leader
  - Follower
  - Authentication
  - Authorization
  - Admission Control
  - LimitRanger
  - ResourceQuota
  - etcdctl
  - snapshot
  - 备份恢复  - sre
  - ops-engineer
related_domains:
  - domain-01-cluster-fundamentals
  - domain-10-troubleshooting-diagnostics
related_topics:
  - control-plane
  - etcd
  - apiserver
  - authentication  - domain-11-production-operations/topic-learn/public-training/one-month/week-2-c

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 9: 控制平面 - Scheduler + Controller Managerauthors:
- name: KUDIG Team
  role: contributor---
# Day 9: 控制平面 - Scheduler + Controller Manager

```yaml
---
id: LEARN-ONE-MONTH-W2-DAY9
topic: kubernetes
type: hands-on-guide
tags: [scheduler, controller-manager, affinity, taint, toleration, nodeelector, hands-on, week-2]  - "Scheduler 调度算法是什么"
  - "Filter/Score 阶段怎么做"
  - "nodeSelector/nodeAffinity 怎么用"
  - "Taint/Toleration 区别"
  - "Controller Manager 工作原理"  - Scheduler
  - Filter
  - Score
  - Scheduling
  - nodeSelector
  - nodeAffinity
  - podAffinity
  - podAntiAffinity
  - Taint
  - Toleration
  - Controller Manager
  - Reconcile
  - 调度
  - 亲和性
  - 污点  - sre
  - ops-engineer
related_domains:
  - domain-01-cluster-fundamentals
  - domain-12-t

> *（内容已精简，完整内容请参阅源文件）*

---

### Week 3: 运维作战能力期 (Days 15-21)

---  - kubernetes Week 3 学习路径
  - K8s 运维能力建设
  - 安全可观测性排障学习
  - 故障排查方法论入门  - Week 3
  - 安全体系
  - 可观测性
  - 故障排查
  - FTA
  - FEBM
  - 运维
  - 学习路径  - sre-engineer
  - devops-engineer
  - platform-engineer
related_domains:
  - domain-05-security-compliance
  - domain-06-observability
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-2-core-technologies/README
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-15-security-1
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/README
---

# Week 3: 运维作战能力期 (Days 15-21)

## 概述

第三周是从"学习知识"向"实战能力"转变的关键阶段。前两周你建立了 K8s 架构认知和核心技术基础，本周将聚焦于三大运维核心能力：**安全合规**、**可观测性**和**故障排查**。

在生产环境中，运维工程师的日常工作不仅仅是部署应用，更重要的是确保系统的安全、稳定和可观测。安全体系是底线——一次安全事件可能导致数据泄露和业务中断；可观测性是眼睛——没有监控和日志，你就像在黑暗中操作；故障排查能力是手术刀——当问题发生时，需要快速定位和解决。

### 学习目标

- 建立完整的 K8s 安全合规体系认知（认证、授权、Pod 安全标准、密钥管理）
- 构建覆盖 Metrics、Logs、Traces、Alerting 的完整可观测性体系
- 掌握基于 FTA/FEBM 方法的结构化故障排查方法论
- **产出**: 监控告警配置 + 故障排查手册

---

## 核心概念详解

### K8s 安全体系全景

Kubernetes 的安全模型建立在三个核心机制之上：**认证（Authentication）**、**授权（Authorization）** 和 **准入控制（Admission Control）**。

**认证** 回答"你是谁"的问题。K8s 支持多种认证方式：X.509 客户端证书、ServiceAccount Token、OIDC（OpenID Connect）、Webhook Token 认证等。在 ACK 集群中，常用的是客户端证书（kubeconfig 中内置的证书）和 ServiceAccount Token（Pod 中自动挂载的 Token）。从 Kubernetes 1.24 开始，ServiceAccoun

> *（内容已精简，完整内容请参阅源文件）*

---

### Week 3 Checkpoint: 自测检验authors:
- name: KUDIG Team
  role: contributor---

---  - kubernetes Week 3 自测
  - K8s 运维能力自测
  - 故障排查知识点检验
  - 安全监控自测题  - 自测
  - checkpoint
  - Week 3
  - 检验
  - RBAC
  - Prometheus
  - 故障排查  - sre-engineer
  - devops-engineer
  - platform-engineer
related_domains:
  - domain-05-security-compliance
  - domain-06-observability
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-15-security-1
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-19-troubleshooting-methodology
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-21-platform-ops
---

# Week 3 Checkpoint: 自测检验

> 完成本周学习后，请独立完成以下自测题。

---

## 概述

Week 3 是运维作战能力建设的关键阶段，涵盖了安全合规体系、可观测性构建、故障排查方法论和平台运维实践四大核心领域。本周学习内容是日常运维工作的基础，掌握这些知识意味着你已经具备了独立处理大多数生产运维问题的能力。

本自测从概念理解、命令实操和场景分析三个维度全面检验你对 We

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 15: 安全体系 - RBAC + 认证授权authors:
- name: KUDIG Team
  role: contributor---

---  - kubernetes RBAC 配置
  - K8s 认证授权体系
  - ServiceAccount 管理
  - RBAC 权限设计  - RBAC
  - 认证
  - 授权
  - ServiceAccount
  - Role
  - ClusterRole
  - 权限控制  - sre-engineer
  - devops-engineer
  - platform-engineer
  - security-engineer
related_domains:
  - domain-05-security-compliance
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-16-security-2
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-19-troubleshooting-methodology
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-21-platform-ops
---

# Day 15: 安全体系 - RBAC + 认证授权

## 概述

今天进入 K8s 安全体系的学习。安全是生产环境的底线——一个配置不当的 K8s 集群可能面临权限滥用、数据泄露、服务中断等严重风险。在所有安全机制中，**RBAC（Role-Based Access Control）** 是最基础也最重要的，它控制了"谁可以在集群中做什么"。

理解 K8s 的认证授权体系，就像理解一栋大楼的门禁系统：认证（Authentication）确认

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 16: 安全体系 - Pod 安全 + 密钥管理authors:
- name: KUDIG Team
  role: contributor---

---  - kubernetes Pod Security Standards
  - K8s Secret 管理最佳实践
  - Kyverno 策略引擎
  - Pod 安全上下文配置  - Pod Security
  - PSS
  - Secret
  - SecurityContext
  - Kyverno
  - 密钥管理
  - Pod 安全标准  - sre-engineer
  - devops-engineer
  - platform-engineer
  - security-engineer
related_domains:
  - domain-05-security-compliance
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-15-security-1
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-21-platform-ops
  - domain-10-troubleshooting-diagnostics/topic-fta/04-fta-core-principles
---

# Day 16: 安全体系 - Pod 安全 + 密钥管理

> **学习时间**: 4-5 小时 | **主题**: Pod 安全标准与 Secret 管理

---

## 概述

Pod 安全和密钥管理是 Kubernetes 安全体系的两大核心支柱。Pod 安全标准（Pod Security Standards, PSS）定义了不同安全级别下的 Pod 安全策略，从无限制的 Privileged 到最严格的

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 17: 可观测性 - 监控 + Prometheusauthors:
- name: KUDIG Team
  role: contributor---

---  - kubernetes Prometheus 监控
  - K8s 可观测性架构
  - Prometheus 查询语言 PromQL
  - Grafana Dashboard 配置  - Prometheus
  - Grafana
  - 监控
  - 可观测性
  - Metrics
  - PromQL
  - Alertmanager
  - kube-prometheus-stack  - sre-engineer
  - devops-engineer
  - platform-engineer
related_domains:
  - domain-06-observability
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-18-observability-2
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-21-platform-ops
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p3-observability-fault-drill
---

# Day 17: 可观测性 - 监控 + Prometheus

## 概述

今天进入可观测性体系的学习。可观测性（Observability）是现代运维的核心能力，它由三大支柱组成：**Metrics（指标）**、**Logs（日志）**和**Traces（分布式追踪）**。今天聚焦于

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 18: 可观测性 - 日志 + 分布式追踪authors:
- name: KUDIG Team
  role: contributor---

---  - kubernetes 日志聚合方案
  - Loki 日志系统
  - ELK 企业日志
  - 分布式链路追踪
  - Alertmanager 告警路由  - Loki
  - ELK
  - 日志
  - LogQL
  - 分布式追踪
  - Trace
  - Alertmanager
  - 可观测性  - sre-engineer
  - devops-engineer
  - platform-engineer
related_domains:
  - domain-06-observability
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-17-observability-1
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/day-22-enterprise-monitoring
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p3-observability-fault-drill
---

# Day 18: 可观测性 - 日志 + 分布式追踪

> **学习时间**: 4-5 小时 | **主题**: 日志聚合与链路追踪

---

## 概述

可观测性（Observability）是现代云原生运维的三大支柱之一，而日志和分布式追踪是其中最关键的两个组成部分。在 Kubernetes 生产环境中，应用运行在大量动态变化的 Pod 中，传统的 SSH 登录查看日志的方式已经完全不可行。

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 19: 故障排查方法论 (关键日)authors:
- name: KUDIG Team
  role: contributor---

---  - kubernetes 故障排查方法论 FTA
  - FEBM 取证循证方法
  - k8s 故障树分析
  - 结构化排障流程  - FTA
  - FEBM
  - 故障树
  - 取证循证
  - 故障排查
  - 故障树分析
  - 根因分析  - sre-engineer
  - devops-engineer
  - platform-engineer
related_domains:
  - domain-10-troubleshooting-diagnostics
  - topic-fta
  - topic-febm
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-20-troubleshooting-practice
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-21-platform-ops
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p3-observability-fault-drill
---

# Day 19: 故障排查方法论 (关键日)

> **学习时间**: 4-5 小时 | **主题**: FTA/FEBM 结构化故障排查

---

## 概述

故障排查是运维工程师最核心的能力。传统的"试错法"效率低下且容易引入新问题。今天你将学习两种结构化的故障排查方法论：FTA（故障树分析）和 

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 20: 故障排查实战authors:
- name: KUDIG Team
  role: contributor---
---  - kubernetes 故障排查实战练习
  - k8s 常见故障注入和排除
  - kubectl 故障排查命令练习
  - ImagePullBackOff CrashLoopBackOff OOMKilled 排查  - 故障排查
  - 实战
  - ImagePullBackOff
  - CrashLoopBackOff
  - OOMKilled
  - PVC Pending
  - 故障注入
  - 故障演练  - sre-engineer
  - devops-engineer
  - platform-engineer
related_domains:
  - domain-10-troubleshooting-diagnostics
  - topic-fta
  - topic-febm
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-19-troubleshooting-methodology
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-21-platform-ops
---

# Day 20: 故障排查实战

> **学习时间**: 4-5 小时 | **主题**: 构造并排查常见问题

---

## 今日目标

- [ ] 实战排查 5 类常见问题
- [ ] 熟练使用排障工具链
- [ ] 建立排障肌肉记忆

---

## 理论学习 (2h)

### 必读文档

1. **Pod 综合排障**
   - 文件: `../../domain-10-troubleshooting-diagnostics/08-pod-comprehensive-troubleshooting.md`

2. **Service

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 21: 平台运维 + 综合实践authors:
- name: KUDIG Team
  role: contributor---

---  - kubernetes 平台运维知识点
  - 集群生命周期管理备份恢复
  - kube-prometheus-stack 监控部署
  - k8s 综合运维实践  - 平台运维
  - 集群生命周期
  - 备份恢复
  - 监控
  - Prometheus
  - Alertmanager
  - 故障演练
  - 运维实践  - sre-engineer
  - devops-engineer
  - platform-engineer
related_domains:
  - domain-07-platform-engineering
  - domain-06-observability
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p3-observability-fault-drill
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-19-troubleshooting-methodology
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-20-troubleshooting-practice
---

# Day 21: 平台运维 + 综合实践

> **学习时间**: 4-5 小时 | **主题**: Week 3 总结与实践项目

---

## 概述

今天是 Week 3 的最后一天，将整合本周所学的安全体系、可观测性和故障排查方法论，完成一个综合实践项目。你将搭建一套完整的可观测性体系（监控 + 日志 + 告警

> *（内容已精简，完整内容请参阅源文件）*

---

### Week 4: 企业级进阶期 (Days 22-28)

---  - Kubernetes 企业级运维
  - GitOps 持续部署
  - 生产事故响应
  - SRE 能力建设  - Week 4
  - 企业级
  - GitOps
  - ArgoCD
  - 变更管理
  - 事故响应
  - 容量规划
  - SLO
  - 学习路径  - sre-engineer
  - devops-engineer
  - platform-engineer
related_domains:
  - domain-20-enterprise-monitoring-alerting
  - domain-08-release-change-management
  - domain-05-security-compliance
  - domain-11-production-operations
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/README
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p4-gitops-pipeline
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p5-graduation-project
---

# Week 4: 企业级进阶期 (Days 22-28)

## 概述

第四周是整个一个月学习计划的收官阶段，聚焦于企业级场景下的 Kubernetes 运维能力建设。经过前三周的学习，你已经掌握了 K8s 的架构基础、核心技术（工作负载、网络、存储）以及运维安全体系。本周将在此基础上，深入企业级监控告警平台、GitOps 持续部署、云原生安全合规以及生产事故响应等高级主题。

本周的学习目标是帮助你从"能操作"跃迁到"能设计"和"能决策"。在真实的生产环境中，运维工程师面临的挑战远不止于执行 kubectl 命令，而是需要具备系统化的监控体系设计能力、规范化的变更管理流程、以及结构化的问题响应方法论。

### 学习目标

- 掌握企业级监控告警平台（Prometheus + Thanos + Grafana）的架构设计与部署实践
- 理解 GitOps 理念并能够使用 ArgoCD 实现声明式持续部署
- 深入云原生安全体系，掌握策略引擎（Kyverno）、Secret 管理（Vault）与零信任架构
- 学会运用 FTA（故障树分析）和 FEBM（取证循证方法）解决复杂生产问题
- 建立变更管理、事故响应、容量规划等生产运维标准流程
- **产出**: GitOps 部署流水线 + 生产事故响应 Playbook

---

## 核心概念详解

### 企业级监控体系的演进

在 Week 3 中你学习了 Prometheus 的基础用法。本周将视野扩展到企业级场景。单集群的 Prometheus 可以满足小规模需求，但当企业拥有多个集群、数千个节点时，监控架构需要全面升级。

**Thanos** 是目前最流

> *（内容已精简，完整内容请参阅源文件）*

---

### Week 4 Checkpoint: 终极自测authors:
- name: KUDIG Team
  role: contributor---

---  - kubernetes 综合自测
  - K8s 终极检验
  - 毕业自测题
  - 企业级运维能力评估  - 自测
  - checkpoint
  - Week 4
  - 终极
  - 毕业
  - 综合评估
  - SLO
  - GitOps
  - ArgoCD  - sre-engineer
  - devops-engineer
  - platform-engineer
related_domains:
  - domain-20-enterprise-monitoring-alerting
  - domain-08-release-change-management
  - domain-05-security-compliance
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p4-gitops-pipeline
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p5-graduation-project
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/day-23-logging-gitops
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/day-25-production-best-practices
---

# Week 4 Checkpoint: 终极自测

## 概述

本文档是整个一个月学习计划的终极自测。它涵盖了四个星期的核心知识点，从基础概念到企业级实践，帮助你评估学习成果并发现薄弱环节。

自测规则：独立完成，不查阅资料。每道题先写下

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 22: 企业监控 - Prometheus 企业级 + Grafanaauthors:
- name: KUDIG Team
  role: contributor---

---  - Prometheus 企业级监控架构
  - Thanos 跨集群监控
  - Grafana 企业级配置
  - SLO/SLI 体系设计  - Thanos
  - SLO
  - SLI
  - 企业监控
  - Prometheus
  - Grafana
  - 错误预算
  - 黄金信号  - sre-engineer
  - devops-engineer
  - platform-engineer
related_domains:
  - domain-20-enterprise-monitoring-alerting
  - domain-06-observability
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/day-23-logging-gitops
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-17-observability-1
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p3-observability-fault-drill
---

# Day 22: 企业监控 - Prometheus 企业级 + Grafana

## 概述

今天进入企业级监控体系的学习。在前面的课程中，你已经掌握了 Prometheus 的基础用法——部署、配置

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 23: 企业日志 + GitOpsauthors:
- name: KUDIG Team
  role: contributor---

---  - ELK 企业日志架构
  - ArgoCD GitOps 实践
  - Kubernetes Kustomize
  - 多环境配置管理  - ELK
  - Loki
  - GitOps
  - ArgoCD
  - Kustomize
  - 多环境
  - 持续部署
  - 声明式  - sre-engineer
  - devops-engineer
  - platform-engineer
related_domains:
  - domain-21-logging-management-analytics
  - domain-08-release-change-management
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/day-22-enterprise-monitoring
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/day-24-security-compliance
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p4-gitops-pipeline
---

# Day 23: 企业日志 + GitOps

> **学习时间**: 4-5 小时 | **主题**: ELK 日志 + ArgoCD GitOps

---

## 概述

企业级日志管理和 GitOps 是现代 Kubernetes 运维的两个关键能力。日志管理帮助你在分布式环境中快速定位问题，而 GitOps 则将基础设施和应用配置的管理标准化、自动化，确保所有变更都有迹可循、可审计、可回滚。

本课程将深入

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 24: 云原生安全 + 合规authors:
- name: KUDIG Team
  role: contributor---

---  - Kubernetes 安全加固
  - Kyverno 策略引擎
  - 零信任安全架构
  - Secret 管理工具  - 云原生安全
  - Kyverno
  - 零信任
  - Sealed Secrets
  - Vault
  - 安全审计
  - 合规
  - 纵深防御  - sre-engineer
  - security-engineer
  - platform-engineer
related_domains:
  - domain-05-security-compliance
  - domain-05-security-compliance
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-15-security-1
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-16-security-2
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/day-25-production-best-practices
---

# Day 24: 云原生安全 + 合规

## 概述

今天深入学习云原生安全体系。在前面的课程中，你已经学习了 RBAC 权限控制和安全基础概念。今天将视角提升到企业级安全：如何使用策略引擎（Kyverno）实施安全策略？如何安全管理 Secret？如何构建零信任安全架构？

云原生安全与传统安全的最大区别在于：容器是短暂的、动态调度的、可能跨越多个节点和可用区。传统的基于边界的防护模型不再适用，需要在每个层面（代码、镜像、运行时、编排、基础设施）都实施安全控制，

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 25: 生产运维最佳实践authors:
- name: KUDIG Team
  role: contributor---

---  - Kubernetes 变更管理
  - 生产事故响应流程
  - 容量规划预测
  - SRE 最佳实践  - 变更管理
  - 事故响应
  - 容量规划
  - MTTR
  - MTTD
  - Runbook
  - SOP
  - 生产运维  - sre-engineer
  - devops-engineer
  - platform-engineer
related_domains:
  - domain-11-production-operations
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/day-24-security-compliance
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/day-26-fta-febm-deep
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/day-28-final-project
---

# Day 25: 生产运维最佳实践

> **学习时间**: 4-5 小时 | **主题**: 变更管理与事故响应

---

## 概述

生产运维的核心目标是在保障业务稳定性的前提下，持续高效地交付价值。变更管理和事故响应是生产运维中两个最关键的流程——据统计，超过 70% 的生产事故由变更引发，而完善的事故响应机制可以将 MTTR（平均恢复时间）缩短 50% 以上。

本课程将系统性地介绍生产架构设计原则、变更管理流程（ITIL/ITSM 标准）、事故响应处理机制、以及容量规划预测方法。你将学习如何制定标准化的变更管理 SOP，如何编写事故响应 Runbo

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 26: FTA/FEBM 专题深化authors:
- name: KUDIG Team
  role: contributor---

---  - Kubernetes 故障树分析进阶
  - FEBM 取证循证方法深化
  - AI Agent 运维模式
  - K8s 问题全景树  - FTA
  - FEBM
  - 故障树
  - 取证循证
  - AI Agent
  - 故障诊断
  - 根因分析
  - 问题全景树  - sre-engineer
  - devops-engineer
  - platform-engineer
related_domains:
  - domain-10-troubleshooting-diagnostics
  - topic-fta
  - topic-febm
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-19-troubleshooting-methodology
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/day-25-production-best-practices
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p3-observability-fault-drill
---

# Day 26: FTA/FEBM 专题深化

> **学习时间**: 4-5 小时 | **主题**: 故障诊断方法论进阶

---

## 概述

FTA（故障树分析，Fault Tree Analysis）和 FEBM（取证循证方法，Forensic Evidence-Based Method）是两种互补的故障诊断方法论。FTA 提供系统化的问题原因分析框架，帮助你从顶层事件出

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 27: 扩展生态 + 高级主题authors:
- name: KUDIG Team
  role: contributor---

---  - Kubernetes CRD 开发
  - Helm Charts 管理
  - Operator 模式
  - K8s 扩展生态  - CRD
  - Helm
  - Operator
  - Operator SDK
  - Kubebuilder
  - Kustomize
  - Chart
  - 扩展生态  - sre-engineer
  - platform-engineer
  - developer
related_domains:
  - domain-15-specialized-tech
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/day-23-logging-gitops
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/day-28-final-project
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p4-gitops-pipeline
---

# Day 27: 扩展生态 + 高级主题

## 概述

今天学习 K8s 的扩展生态，包括三个核心技术：**CRD（Custom Resource Definition）**、**Helm** 和 **Operator 模式**。这三者构成了 K8s 扩展性的基础，让你能够定义自定义资源、打包和分发应用、以及自动化复杂运维操作。

K8s 的核心设计理念之一就是可扩展性。K8s 内置了 Pod、Service、Deployment 等标准资源类型，但在实际使用中，你可能需要定义自己的资源类型（如 Database、Certificate、Bac

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 28: 综合复习 + 毕业项目authors:
- name: KUDIG Team
  role: contributor---

---  - Kubernetes 毕业项目
  - K8s 综合实践
  - 费曼复述法
  - 知识图谱绘制  - 毕业项目
  - 综合复习
  - P5
  - 费曼复述
  - 知识图谱
  - 一个月学习总结
  - 综合实践  - sre-engineer
  - devops-engineer
  - platform-engineer
related_domains:
  - domain-01-cluster-fundamentals
  - domain-11-production-operations
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p5-graduation-project
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/checkpoint
  - domain-11-production-operations/topic-learn/public-training/one-month/README
---

# Day 28: 综合复习 + 毕业项目

## 概述

今天是整个一个月学习计划的最后一天，也是最重要的一天。前三周你学习了大量的知识点——Docker 基础、Linux 运维、K8s 架构、工作负载管理、网络存储、安全体系、监控告警、GitOps、故障排查方法论。今天是检验你是否真正掌握这些知识的时刻。

今天的核心活动是完成毕业综合实践项目（P5），它要求你从头搭建一个生产级的 K8s 平台，涵盖应用部署、网络配置、存储管理、安全加固、监控告警、日志采集和 GitOps 流水线。这个项目是对一个月学习成果的全面检验。

### 学习目标

- 通过费曼复述法巩固整月学习内容
- 完成毕业综合实践项目，检验综合能力
- 通过终

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 8: K8s RBAC 权限配置实操authors:
- name: KUDIG Team
  role: contributor---
# Day 8: K8s RBAC 权限配置实操

> **日期**: Week 2 Day 1 | **主题**: RBAC 权限模型与配置实践 | **版本**: K8s 1.28-1.33

---

## 1. RBAC 核心概念

### 1.1 四大资源对象

| 对象 | 作用域 | 说明 |
|------|--------|------|
| `Role` | Namespace | 授权特定 namespace 内的资源操作 |
| `ClusterRole` | 集群级 | 授权集群范围的资源或非资源路径（如 `/healthz`） |
| `RoleBinding` | Namespace | 将 Role/ClusterRole 绑定到用户/组/SA |
| `ClusterRoleBinding` | 集群级 | 将 ClusterRole 绑定到集群范围的主体 |

### 1.2 API 主体（Subject）类型

```yaml
subjects:
  - kind: User      # 外部用户（如 LDAP 集成）
    name: jane@example.com
    apiGroup: ""
  - kind: Group     # 用户组
    name: frontend-team
    apiGroup: ""
  - kind: ServiceAccount  # 服务账号
    name: ci-builder
    namespace: ci-system
    apiGroup: ""
```

### 1.3 规则语法（Rules）

```yaml
rules:
  - apiGroups: [""]           # "" = core API group (Pod/Service/ConfigMap)
    resources: ["pods", "services"]
    verbs: ["get", "list"]
  - apiGroups: ["apps"]
    resources: ["deployments"]
    verbs: ["*"]
  - nonResourceURLs: ["/healthz", "/version"

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 9: K8s 审计日志配置与分析实操authors:
- name: KUDIG Team
  role: contributor---
# Day 9: K8s 审计日志配置与分析实操

> **日期**: Week 2 Day 2 | **主题**: 审计日志配置与深度分析 | **版本**: K8s 1.28-1.33

---

## 1. 审计策略概述

### 1.1 审计生命周期

```
请求 → API Server → 认证 → 授权 → 准入控制 → 审计日志记录 → 处理请求
```

### 1.2 审计阶段 (Stage)

| Stage | 时机 | 用途 |
|-------|------|------|
| `RequestReceived` | 收到请求时 | 记录原始请求 |
| `ResponseStarted` | 响应开始发送时 | 记录长时间运行请求的初始响应 |
| `ResponseComplete` | 响应发送完成 | 记录最终响应状态 |
| `Panic` | 服务器 panic 时 | 记录紧急状态（不常用） |

### 1.3 审计级别 (Level)

| Level | 说明 | 适用场景 |
|-------|------|---------|
| `None` | 不记录 | 排除噪音 |
| `Metadata` | 仅记录元数据（user, timestamp, resource） | 大多数操作 |
| `RequestResponse` | 记录元数据 + 请求体 + 响应体 | 变更操作 |
| `Request` | 仅记录请求体（不含响应） | 特殊情况 |

---

## 2. 配置审计策略

### 2.1 创建审计策略文件

```bash
cat > audit-policy.yaml <<'EOF'
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
  # 不记录只读请求（get/list/watch）
  - level: Metadata
    verbs: ["get", "list", "watch"]
    resources:
      - group: ""
        resources: ["pods", "services"

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 10: K8s 集群监控体系搭建实操authors:
- name: KUDIG Team
  role: contributor---

# Day 10: K8s 集群监控体系搭建实操

> **日期**: Week 2 Day 3 | **主题**: 监控体系搭建与告警配置 | **版本**: K8s 1.28-1.33

---

## 1. 监控架构概述

### 1.1 三层监控指标

| 层 | 指标 | 采集工具 | 说明 |
|---|------|---------|------|
| 基础设施层 | CPU/内存/磁盘/网络 | node_exporter | 节点级别资源使用 |
| Kubernetes 层 | Pod/Deployment/Node 状态 | kube-state-metrics | K8s 对象状态 |
| 应用层 | 业务指标（QPS/Latency/Error） | 应用自暴露 | Pod 内应用 metrics |

### 1.2 监控组件清单

```
Prometheus Operator (监控中枢)
  ├── node_exporter (节点指标)
  ├── kube-state-metrics (K8s 对象状态)
  ├── cAdvisor (容器资源)
  ├── blackbox_exporter (探测)
  └── alertmanager (告警)
```

---

## 2. 部署 Prometheus Operator

### 2.1 使用 kube-prometheus-stack

```bash
# 添加 Prometheus Community Helm repo
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo update

# 安装 kube-prometheus-stack（生产推荐配置）
helm install prometheus prometheus-community/kube-prometheus-stack \
  --namespace monitoring \
  --create-namespace \
  --set prome

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 11: K8s 安全风险识别与防护实操authors:
- name: KUDIG Team
  role: contributor---

# Day 11: K8s 安全风险识别与防护实操

> **日期**: Week 2 Day 4 | **主题**: 安全风险评估与最佳实践 | **版本**: K8s 1.28-1.33

---

## 1. 安全风险分类

### 1.1 K8s 安全层级模型

```
┌─────────────────────────────────────────┐
│           云厂商层（物理安全）           │
├─────────────────────────────────────────┤
│           集群层（RBAC/网络策略）        │
├─────────────────────────────────────────┤
│           节点层（OS/运行时/网络）       │
├─────────────────────────────────────────┤
│           Pod 层（SecurityContext/PSP）  │
├─────────────────────────────────────────┤
│           应用层（镜像/密钥/数据）       │
└─────────────────────────────────────────┘
```

### 1.2 五大攻击面

| 攻击面 | 风险 | 防护措施 |
|--------|------|---------|
| API Server | 未授权访问、提权 | RBAC + 审计日志 + 认证 |
| etcd | 数据泄露 | TLS + 网络隔离 + 加密 |
| Kubelet | 容器逃逸 | RBAC + 静态 Pod + PSP |
| Container Runtime | 权限过大 | 最小化 capabilities |
| 网络 | 横向移动 | NetworkPolicy + CNI 隔离 |

---

## 2. 身份与访问风险（RBAC）

### 2.1 高危权限组合检测

**检查 system:masters 组滥用**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度

```bash
# 查找所有 s

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 15: Node 节点基础实操authors:
- name: KUDIG Team
  role: contributor---
---  - kubernetes 节点管理
  - kubectl cordon drain uncordon
  - 节点状态 NotReady 排查
  - Pod 调度到特定节点  - 节点
  - cordon
  - drain
  - uncordon
  - 污点
  - 标签
  - 调度
  - NotReady
  - node  - sre-engineer
  - ops-engineer
  - platform-engineer
related_domains:
  - domain-10-troubleshooting-diagnostics
  - domain-07-platform-engineering
related_topics:
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-16-node-advanced/01-node-advanced-hands-on
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-17-nodepool/01-nodepool-basics-hands-on
---

# Day 15: Node 节点基础实操

> **日期**: Week 3 Day 1 | **主题**: 节点概念、状态与管理操作 | **版本**: K8s 1.28-1.33

---

## 1. 节点核心概念

### 1.1 节点状态

| 状态 | 含义 | 处理方式 |
|------|------|---------|
| `Ready` | 节点健康，Pod 可调度 | 正常 |
| `NotReady` | kubelet 无法上报心跳 | 检查 kubelet/网络 |
| `Sched

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 16: Node 节点进阶实操authors:
- name: KUDIG Team
  role: contributor---

---  - kubernetes 节点池扩缩容
  - Cluster Autoscaler 配置
  - Pod 亲和性反亲和性
  - 拓扑分布约束  - 节点池
  - node-pool
  - autoscaler
  - 亲和性
  - topology
  - 调度
  - 拓扑
  - PodAntiAffinity  - sre-engineer
  - ops-engineer
  - platform-engineer
related_domains:
  - domain-10-troubleshooting-diagnostics
  - domain-07-platform-engineering
related_topics:
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-15-node-basics/01-node-basics-hands-on
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-17-nodepool/01-nodepool-basics-hands-on
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-18-nodepool-advanced/01-nodepool-advanced-hands-on
---

# Day 16: Node 节点进阶实操

> **日期**: Week 3 Day 2 | **主题**: 节点维护、标签与调度约束 | **版本**: K8s 1.28-1.33

---

## 1. 节点池（Node Pool）概念

### 1.1 节点池 vs 手动节点管理

| 维度 | 手动管理 | 节点池 |
|------|---

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 17: 节点池基础实操authors:
- name: KUDIG Team
  role: contributor---

---  - kubernetes 节点池创建
  - 新节点加入集群
  - 节点池扩缩容
  - 工作负载调度到节点池  - 节点池
  - node-pool
  - kubeadm
  - 扩缩容
  - 调度
  - 标签
  - 污点  - sre-engineer
  - ops-engineer
  - platform-engineer
related_domains:
  - domain-07-platform-engineering
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-16-node-advanced/01-node-advanced-hands-on
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-18-nodepool-advanced/01-nodepool-advanced-hands-on
---

# Day 17: 节点池基础实操

> **日期**: Week 3 Day 3 | **主题**: 节点池概念与创建配置 | **版本**: K8s 1.28-1.33

---

## 1. 节点池核心概念

### 1.1 节点池架构

```
集群
├── 系统节点池 (system)
│   └── 运行 kube-system Pod（3 节点，高可用）
├── 通用计算节点池 (general-compute)
│   └── 运行无状态业务应用（按需扩缩）
├── GPU 计算节点池 (gpu-compute)
│   └── 运行 ML/AI 工作负载（预留实例）
└── 内存优化节点池 (memory-optimized)
    └── 运行大数据/缓存工作负载
```

#

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 18: 节点池进阶实操authors:
- name: KUDIG Team
  role: contributor---

---  - Kubernetes 节点池弹性伸缩
  - Cluster Autoscaler 配置
  - 节点池生命周期管理
  - PDB Pod 中断预算  - 节点池
  - Cluster Autoscaler
  - 弹性伸缩
  - 节点池升级
  - PDB
  - 成本优化
  - Spot 实例  - sre-engineer
  - ops-engineer
  - platform-engineer
related_domains:
  - domain-07-platform-engineering
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-17-nodepool/01-nodepool-basics-hands-on
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-20-pod-advanced/01-pod-advanced-hands-on
---

# Day 18: 节点池进阶实操

> **日期**: Week 3 Day 4 | **主题**: 节点池扩缩容与生命周期管理 | **版本**: K8s 1.28-1.33

---

## 1. 节点池弹性伸缩

### 1.1 Cluster Autoscaler 原理

```
触发条件: Pod 无法调度（Pending）
    ↓
检测: 调度失败原因 = 资源不足
    ↓
决策: 计算需要新增节点数量
    ↓
执行: 调用云厂商 API 创建节点
    ↓
等待: 节点 Ready → 调度器分配 Pod
    ↓
冷却: scale-down-delay 避免频繁扩缩
```

### 1

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 19: Pod 容器组基础实操authors:
- name: KUDIG Team
  role: contributor---
---  - Kubernetes Pod 生命周期
  - Pod 探针配置
  - CrashLoopBackOff 排查
  - Pod QoS 等级  - Pod
  - 生命周期
  - 探针
  - livenessProbe
  - readinessProbe
  - CrashLoopBackOff
  - ImagePullBackOff
  - QoS
  - Init 容器  - sre-engineer
  - ops-engineer
  - developer
  - platform-engineer
related_domains:
  - domain-10-troubleshooting-diagnostics
  - domain-02-workloads-applications
related_topics:
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-20-pod-advanced/01-pod-advanced-hands-on
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-15-node-basics/01-node-basics-hands-on
---

# Day 19: Pod 容器组基础实操

> **日期**: Week 3 Day 5 | **主题**: Pod 生命周期与基本操作 | **版本**: K8s 1.28-1.33

---

## 1. Pod 生命周期

### 1.1 Pod 状态与 Conditions

```bash
# 查看 Pod 完整状态
kubectl get pod <pod-name> -o yaml | grep -A20 "status:"

# 查看 Pod Conditions
kubectl get pod <pod-name> -o jsonpath='{.st

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 20: Pod 容器组进阶实操authors:
- name: KUDIG Team
  role: contributor---

---  - Kubernetes Pod 调度深度配置
  - Pod 亲和性反亲和性
  - 拓扑分布约束配置
  - 探针与资源配置  - Pod
  - 调度
  - 亲和性
  - topology
  - 拓扑
  - 探针
  - livenessProbe
  - readinessProbe
  - PDB
  - PriorityClass  - sre-engineer
  - ops-engineer
  - platform-engineer
related_domains:
  - domain-10-troubleshooting-diagnostics
  - domain-02-workloads-applications
related_topics:
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-19-pod-basics/01-pod-basics-hands-on
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-16-node-advanced/01-node-advanced-hands-on
---

# Day 20: Pod 容器组进阶实操

> **日期**: Week 3 Day 6 | **主题**: Pod 调度、探针与资源配置 | **版本**: K8s 1.28-1.33

---

## 1. Pod 调度深度配置

### 1.1 nodeSelector（节点选择器）

```yaml
# 调度到特定标签的节点
apiVersion: v1
kind: Pod
metadata:
  name: gpu-workload
spec:
  nodeSelector:
    gpu: "true"
    zone: us-eas

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 21: K8s 组件运维实操authors:
- name: KUDIG Team
  role: contributor---

---  - kubernetes 控制平面组件运维
  - API Server 故障排查
  - etcd 备份恢复
  - 证书管理与续期  - 控制平面
  - API Server
  - Scheduler
  - Controller Manager
  - etcd
  - kubelet
  - 证书
  - 组件运维  - sre-engineer
  - ops-engineer
  - platform-engineer
related_domains:
  - domain-07-platform-engineering
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-15-node-basics/01-node-basics-hands-on
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-16-node-advanced/01-node-advanced-hands-on
---

# Day 21: K8s 组件运维实操

> **日期**: Week 3 Day 7 | **主题**: 核心组件状态检查与故障处理 | **版本**: K8s 1.28-1.33

---

## 1. 控制平面组件概述

### 1.1 组件清单

| 组件 | 进程名 | 默认端口 | 关键文件 |
|------|--------|---------|---------|
| kube-apiserver | kube-apiserver | 6443 | /etc/kubernetes/manif

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 22: Service 基础实操authors:
- name: KUDIG Team
  role: contributor---
# Day 22: Service 基础实操

> **日期**: Week 4 Day 1 | **主题**: Service 类型与配置实践 | **版本**: K8s 1.28-1.33

---

## 1. Service 核心概念

### 1.1 Service 类型

| 类型 | ClusterIP | 适用场景 |
|------|-----------|---------|
| `ClusterIP` | 集群内部 IP | 内部服务间调用 |
| `NodePort` | <nodeIP>:<port> | 开发/测试环境 |
| `LoadBalancer` | 外部负载均衡 IP | 生产环境（配合云厂商） |
| `ExternalName` | CNAME 映射 | 访问外部服务 |

### 1.2 Service 工作原理

```
Pod A → Service (10.96.0.1) → Endpoints (Pod B:8080, Pod C:8080)
                        ↑
                   kube-proxy (iptables/ipvs)
```

---

## 2. 创建 Service

### 2.1 ClusterIP Service

```yaml
# 基本 ClusterIP
apiVersion: v1
kind: Service
metadata:
  name: backend-svc
  namespace: production
spec:
  type: ClusterIP
  selector:
    app: backend
  ports:
    - name: http
      port: 80
      targetPort: 8080
    - name: grpc
      port: 50051
      targetPort: 50051
```

### 2.2 NodePort Service

```yaml
apiVersion: v1
kind: Service

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 23: Ingress 实操authors:
- name: KUDIG Team
  role: contributor---

# Day 23: Ingress 实操

> **日期**: Week 4 Day 2 | **主题**: Ingress 路由规则与控制器配置 | **版本**: K8s 1.28-1.33

---

## 1. Ingress 核心概念

### 1.1 Ingress 架构

```
客户端 → Ingress Controller (Nginx/Traefik/Envoy) → Service → Pod
              ↓
         Ingress Resource (路由规则)
```

### 1.2 Ingress 控制器类型

| 控制器 | 特点 | 适用场景 |
|--------|------|---------|
| NGINX Ingress Controller | 功能丰富，性能高 | 生产环境 |
| Traefik | 支持 Let's Encrypt 自动证书 | 内部服务 |
| Ambassador | 基于 Envoy，支持 canary | API Gateway |
| GKE Ingress | GCP 原生集成 | GCP 环境 |

---

## 2. 安装 Ingress Controller

### 2.1 NGINX Ingress Controller

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 方式 1: Helm 安装
helm repo add ingress-nginx https://kubernetes.github.io/ingress-nginx
helm repo update
helm install nginx-ingress ingress-nginx/ingress-nginx \
  --namespace ingress-nginx \
  --create-namespace \
  --set controller.replicaCount=2 \
  --set controller.service.type=LoadBalancer

# 方式 2: 手动部署
kubectl apply -f https://raw.githubuserconte

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 24: Terway 网络实操authors:
- name: KUDIG Team
  role: contributor---
# Day 24: Terway 网络实操

> **日期**: Week 4 Day 3 | **主题**: Terway CNI 架构与配置 | **版本**: K8s 1.28-1.33

---

## 1. Terway 核心概念

### 1.1 Terway vs Flannel

| 维度 | Terway | Flannel |
|------|--------|---------|
| 网络模型 | ENI + Trunk ENI | VXLAN / IPIP |
| Pod 数量 | 高密度（200+ Pod/节点） | 中等（100-150 Pod/节点） |
| Pod IP 来源 | 云厂商 ENI | overlay 网络 |
| 性能 | 高（原生网络） | 中等（隧道开销） |
| 安全组支持 | 完全支持 | 不支持 |

### 1.2 Terway 架构

```
Pod → Terway CNI → Veth Pair → Host Bridge → ENI (云网络)
                    ↓
              Terway Agent (daemonset)
                    ↓
              Metadata Service (获取 ENI 信息)
```

---

## 2. Terway 安装与配置

### 2.1 安装 Terway

```bash
# Alibaba Cloud ACK
# 集群创建时选择 Terway 网络插件

# 自建集群手动安装
git clone https://github.com/Alibaba/terway.git
cd terway/deploy
kubectl apply -f terway.yaml

# 或使用 Helm
helm install terway -n kube-system ./charts/terway
```

### 2.2 Terway 配置

```yaml
# /etc/terway/config.json
{
  "version": "2",
  "TerwaySubnetCIDR": "10.4

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 25: Flannel 网络实操authors:
- name: KUDIG Team
  role: contributor---
# Day 25: Flannel 网络实操

> **日期**: Week 4 Day 4 | **主题**: Flannel 网络模型与故障排查 | **版本**: K8s 1.28-1.33

---

## 1. Flannel 核心概念

### 1.1 Flannel 模式

| 模式 | 说明 | 适用场景 |
|------|------|---------|
| VXLAN | 二层隧道（推荐） | 跨节点 Pod 通信 |
| host-gw | 主机网关（需二层可达） | 同机房低延迟 |
| IPIP | IP 隧道 | 跨网络通信 |
| WireGuard | 加密隧道 | 安全要求高 |

### 1.2 Flannel 架构

```
Pod A (10.244.1.2) → cni0 (10.244.1.1) → flannel.1 (VXLAN) → eth0 → Node B
                    ↓
              flanneld (daemonset)
                    ↓
              etcd (网络分配存储)
```

---

## 2. 安装 Flannel

### 2.1 kubeadm 集群安装 Flannel

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 安装 CNI 插件
kubectl apply -f https://raw.githubusercontent.com/flannel-io/flannel/master/Documentation/kube-flannel.yml

# 检查 Pod
kubectl get pods -n kube-flannel

# 确认节点有 flannel 接口
ip addr | grep flannel
```

### 2.2 自定义 CIDR

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
# 通过 kubeadm 配置 Pod CIDR
kubeadm init --pod-network-cidr=10.244.0.0/16

# 或通过 ConfigMap 修改
kubectl edit configmap -n kube-system

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 26: 存储卷创建与删除实操authors:
- name: KUDIG Team
  role: contributor---

# Day 26: 存储卷创建与删除实操

> **日期**: Week 4 Day 5 | **主题**: PV/PVC 创建与生命周期管理 | **版本**: K8s 1.28-1.33

---

## 1. 存储核心概念

### 1.1 存储对象关系

```
StorageClass (存储类)
    ↓ 定义存储类型和供给方式
PersistentVolume (PV) - 集群级存储资源
    ↓ 绑定关系
PersistentVolumeClaim (PVC) - Pod 申请存储的请求
    ↓ 挂载
Pod 使用 PVC
```

### 1.2 存储类型

| 类型 | 说明 | 示例 | 访问模式 |
|------|------|------|---------|
| Block | 块存储 | 云盘、LVM | RWO |
| File | 文件存储 | NFS、CIFS | RWO/ROX/RWX |
| Object | 对象存储 | S3、OSS | 需要特殊挂载 |

### 1.3 访问模式

| 模式 | 缩写 | 说明 | 适用存储 |
|------|------|------|---------|
| ReadWriteOnce | RWO | 单节点读写 | 云盘、块存储 |
| ReadOnlyMany | ROX | 多节点只读 | NFS |
| ReadWriteMany | RWX | 多节点读写 | NFS、NAS |
| ReadWriteOncePod | RWOP | 单 Pod 独占读写 | K8s 1.27+ |

### 1.4 回收策略

| 策略 | 说明 | 使用场景 |
|------|------|---------|
| Retain | 保留 PV 和数据 | 生产数据库 |
| Delete | 自动删除 PV 和底层存储 | 临时数据、缓存 |
| Recycle | 已废弃，用动态供给替代 | 不推荐 |

---

## 2. 创建 PV/PVC

### 2.1 Static Provisioning（静态）

```yaml
# 创建 PV
apiVersion: v1
kind: PersistentVolume
metadata:
  name: pv-hostpath
 

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 27: 存储卷挂载实操authors:
- name: KUDIG Team
  role: contributor---
# Day 27: 存储卷挂载实操

> **日期**: Week 4 Day 6 | **主题**: 存储挂载方式与最佳实践 | **版本**: K8s 1.28-1.33

---

## 1. 存储挂载类型

### 1.1 挂载类型对比

| 类型 | 说明 | 用途 |
|------|------|------|
| `emptyDir` | 临时存储，Pod 删除后丢失 | 临时缓存/共享内存 |
| `hostPath` | 宿主机目录 | 开发/单节点 |
| `persistentVolumeClaim` | PVC 持久化存储 | 生产环境有状态应用 |
| `configMap` | 配置文件挂载 | 应用配置 |
| `secret` | 密钥挂载 | 敏感信息 |
| `projected` | 多种资源投影 | 服务账号令牌等 |

---

## 2. emptyDir 实践

### 2.1 基本使用

```yaml
apiVersion: v1
kind of Pod
metadata:
  name: app-with-tmp
spec:
  containers:
    - name: app
      image: app:v1
      volumeMounts:
        - name: tmp-storage
          mountPath: /tmp
  volumes:
    - name: tmp-storage
      emptyDir:
        sizeLimit: 1Gi
        medium: Memory  # 使用内存存储（高性能）
```

### 2.2 共享存储（多容器）

```yaml
apiVersion: v1
kind of Pod
metadata:
  name: sidecar-pod
spec:
  containers:
    - name: main
      image: app:v1
      volumeMounts:
        - name: shared-data
          mountPath: /data
    - name: sidecar
      image: sidecar:v1
      volu

> *（内容已精简，完整内容请参阅源文件）*

---

### Day 28: 综合复习与实践authors:
- name: KUDIG Team
  role: contributor---

# Day 28: 综合复习与实践

> **日期**: Week 4 Day 7 | **主题**: 全流程实操与问题答疑 | **版本**: K8s 1.28-1.33

---

## 1. 综合实操项目

### 1.1 项目：部署 Web 应用完整栈

**要求**：使用所学知识，部署一个完整的 Web 应用栈

```
Frontend (Nginx) → Backend (Python API) → Database (MySQL) + Cache (Redis)
```

**步骤**：

```bash
# 1. 创建命名空间
kubectl create namespace production

# 2. 部署 MySQL StatefulSet
cat > mysql-statefulset.yaml <<'EOF'
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
  namespace: production
spec:
  selector:
    matchLabels:
      app: mysql
  serviceName: mysql-headless
  replicas: 1
  template:
    metadata:
      labels:
        app: mysql
    spec:
      containers:
        - name: mysql
          image: mysql:8.0
          env:
            - name: MYSQL_ROOT_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: mysql-secret
                  key: root-password
          volumeMounts:
            - name: data
              mountPath: /var/lib/mysql
  volumeClaimTemplat

> *（内容已精简，完整内容请参阅源文件）*

## Related

- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[entities/argocd.md|argocd]] — ArgoCD
- [[concepts/docker-architecture.md|docker-architecture]] — Docker Architecture and Container Runtime
- [[pod-lifecycle]] — Pod Lifecycle

```