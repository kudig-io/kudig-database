---
title: ACK/ACR/K8S 内部培训 1 个月学习计划
description: '- "集群管理"'
summary: '本学习计划为内部运维工程师和技术支持人员设计，覆盖 ACK（阿里云容器服务）、ACR（阿里云容器镜像服务）和 [[Kubernetes|Kubernetes]] 三大技术栈。通过 28 天的系统性学习，从基础概念到生产运维，逐步建立完整的云原生运维能力。'
category: learning
tags:
- k8s
- training
- hands-on
- flannel
- docker
- ingress
- rbac
- rag
- daemonset
- gpu
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- ACK/ACR/K8S 内部培训 1 个月学习计划 是什么
- 如何 ACK/ACR/K8S 内部培训 1 个月学习计划
trigger_keywords:
- ACK
- ACR
- K8S
- 内部培训
- 个月学习计划
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# ACK/ACR/K8S 内部培训 1 个月学习计划

```yaml
---
title: ACK/ACR/K8S 内部培训一个月学习计划
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "ACK ACR培训内容"
  - "阿里云Kubernetes培训"
  - "一个月学习计划"
  - "内部培训体系"
  - "SRE工程师培训"
trigger_keywords:
  - "ACK培训"
  - "ACR培训"
  - "阿里云容器"
  - "Kubernetes培训"
  - "一个月计划"
  - "内部培训"
  - "集群管理"
  - "安全认证"
reading_level: intermediate
audience:
  - 内部运维工程师
  - 技术支持人员
  - SRE工程师
estimated_read_time: 25min
related_domains:
  - 集群基础
  - 集群基础
  - 工作负载
  - 安全
  - 故障诊断
related_topics:
  - 生产运维/topic-learn/inner-training/README
  - 生产运维/topic-learn/inner-training/week-1-ack-acr-lifecycle
  - 生产运维/topic-learn/inner-training/week-2-security-monitoring
  - 生产运维/topic-learn/inner-training/week-3-node-workload
  - 生产运维/topic-learn/inner-training/week-4-network-storage
id: INNER-TRAINING-001
topic: training
type: training-plan
tags: [training, inner-training, ack, acr, k8s, month-1, k8s-1.28-1.33]
---
```

> **目标人群**: 内部运维工程师、技术支持人员 | **投入**: 4+ 小时/天 | **知识库**: kudig-database (668+ 篇)

---

## 概述

本学习计划为内部运维工程师和技术支持人员设计，覆盖 ACK（阿里云容器服务）、ACR（阿里云容器镜像服务）和 [[Kubernetes|Kubernetes]] 三大技术栈。通过 28 天的系统性学习，从基础概念到生产运维，逐步建立完整的云原生运维能力。

课程设计遵循知识依赖关系，从 ACK/ACR 服务基础开始，经过安全认证和节点管理，最终掌握网络和存储的核心技能。每个学习阶段有明确的产出目标和评估标准，通过每周的自测检验和实践项目确保学习效果。

**培训目标**: 完成培训后能够独立处理 ACK/ACR/K8S 日常运维工单，具备集群管理、安全配置、故障排查的基本能力。

---

## 快速导航

| 周次 | 主题 | 核心产出 | 目录 |
|------|------|---------|------|
| Week 1 | ACK/ACR 基础与集群生命周期 | 集群全生命周期操作能力 | [week-1-ack-acr-lifecycle/](./week-1-ack-acr-lifecycle/) |
| Week 2 | 安全认证与监控运维 | 安全体系 + 监控基础 | [week-2-security-monitoring/](./week-2-security-monitoring/) |
| Week 3 | 节点与工作负载管理 | 节点池 + Pod 运维能力 | [week-3-node-workload/](./week-3-node-workload/) |
| Week 4 | 网络与存储 | 网络 + 存储实操能力 | [week-4-network-storage/](./week-4-network-storage/) |
| - | 实践项目 | 5 个递进式项目 | [projects/](./projects/) |
| - | 补充资源 | 知识图谱模板 | [resources/](./resources/) |

---

## 整体学习路径

```
Week 1: ACK/ACR 基础       Week 2: 安全认证与监控    Week 3: 节点与工作负载    Week 4: 网络与存储
├─ ACK/ACR 管控 SR        ├─ RBAC 权限配置          ├─ Node 节点管理          ├─ Service 基础
├─ ACK SDK & API          ├─ RAM 账号集成           ├─ 节点池管理             ├─ Ingress 路由
├─ ACK/ACR 控制台         ├─ 漏洞 & 风险防范        ├─ Pod 容器组管理         ├─ Terway/Flannel CNI
├─ 集群创建               ├─ 集群审计               ├─ K8S 组件运维           ├─ 存储卷管理
├─ 集群删除               ├─ 集群监控                                        ├─ 综合复习
└─ 集群升级/证书           └─ 配额 & License
    |                         |                         |                         |
    v                         v                         v                         v
 产出: 集群全生命周期      产出: 安全体系+监控基础   产出: 节点池+Pod运维能力  产出: 网络+存储实操能力
```

### 每周学习内容

**Week 1 (Day 1-7)**: ACK/ACR 服务架构理解 → SDK/API 操作 → 控制台使用 → 集群创建 → 集群删除 → 集群升级 → 证书管理

**Week 2 (Day 8-14)**: RBAC 权限配置 → RAM 集成 → 漏洞管理 → 安全最佳实践 → 审计日志 → 集群监控 → 配额管理

**Week 3 (Day 15-21)**: 节点基础 → 节点进阶(标签/污点) → 节点池基础 → 节点池进阶(扩缩容) → Pod 基础 → Pod 进阶 → 组件运维

**Week 4 (Day 22-28)**: Service → Ingress → Terway → Flannel → 存储卷创建/删除 → 存储卷挂载 → 综合复习

---

## 知识依赖关系

```
ACK/ACR 管控层 ──> ACK SDK/API ──> 控制台操作
       │
       v
集群生命周期 (创建/删除/升级/证书)
       │
       v
安全认证 (RBAC + RAM) ──> 漏洞 & 风险 ──> 审计 & 监控
       │
       v
节点管理 (Node + NodePool) ──> 工作负载 (Pod + 组件)
       │
       v
网络 (Service + Ingress + CNI) ──> 存储 (PV/PVC)
```

### 关键依赖说明

| 学习阶段 | 依赖的前置知识 | 为什么依赖 |
|----------|--------------|-----------|
| 集群创建 | ACK 架构理解 | 需要理解参数含义才能正确配置 |
| RBAC 配置 | RAM 集成 | ACK 使用两层权限模型 |
| 审计日志 | 监控基础 | 审计日志存储在 SLS 中 |
| 节点池管理 | 节点基础 | 节点池是节点的集合管理 |
| Service/Ingress | 网络基础 | 需要理解 K8s 网络模型 |
| 存储管理 | CSI 插件 | 存储通过 CSI 插件实现 |

---

## 学习方法论

### 1. 费曼学习法 (每日)
每天学完一个模块后，用自己的语言向"虚拟初学者"复述，检测理解漏洞。

### 2. 间隔重复 (每周)
- 每周第一天用 15 分钟回顾上周关键概念
- 每周末复习本周 10 个核心术语

### 3. 主动回忆 (每节)
先合上文档，尝试回答: "这个功能做什么？它和哪些服务交互？出问题了怎么排查？"

### 4. 实践优先原则
理论文档读完后，立刻动手复现。每天 4 小时中: 理论 <= 1.5h，实践 >= 2.5h

### 5. 结构化记录
每个主题学完后，产出一张思维导图或笔记摘要，形成个人知识图谱。

### 6. SR 驱动学习
结合实际处理的 SR（Service Request）巩固所学知识，从实践中学习效果最佳。

---

## 每周目标与产出

| 周次 | 核心产出 | 完成评估标准 |
|------|----------|--------------|
| Week 1 | 独立完成集群全生命周期操作 | 能通过控制台/SDK/API 三种方式完成集群创建、升级、删除 |
| Week 2 | 安全体系配置 + 监控基础搭建 | 能配置 RBAC 和 RAM 集成，搭建基础监控告警 |
| Week 3 | 节点池运维 + Pod 问题排查 | 能管理节点池扩缩容，独立排查 Pod 常见问题 |
| Week 4 | 网络和存储配置 + 综合实操 | 能配置 Service/Ingress/CNI，管理存储卷 |

---

## 实践项目清单

| # | 项目名称 | 周 | 详情 |
|---|----------|---|------|
| P1 | ACK 集群全生命周期管理 | Week 1 | [p1-ack-cluster-lifecycle.md](./projects/p1-ack-cluster-lifecycle.md) |
| P2 | 安全认证与监控体系搭建 | Week 2 | [p2-security-monitoring-setup.md](./projects/p2-security-monitoring-setup.md) |
| P3 | 节点与工作负载运维实战 | Week 3 | [p3-node-workload-management.md](./projects/p3-node-workload-management.md) |
| P4 | 网络与存储综合实践 | Week 4 | [p4-network-storage-practice.md](./projects/p4-network-storage-practice.md) |
| P5 | 毕业综合实践项目 | Week 4 | [p5-graduation-project.md](./projects/p5-graduation-project.md) |

---

## 关键文件索引

### ACK/ACR 核心文档
- `../../云厂商/04-alicloud-ack/alicloud-ack-overview.md`
- `../../云厂商/04-alicloud-ack/service-ack-practical-guide.md`
- `../../云厂商/04-alicloud-ack/243-ack-ram-authorization.md`

### 集群架构与组件
- `../../集群基础/01-kubernetes-architecture-overview.md`
- `../../集群基础/02-core-components-deep-dive.md`

### 故障排查体系
- `../../故障诊断/` (42篇)
- `../../故障诊断/topic-structural-trouble-shooting/README.md`

### 速查手册
- `../../系统基础/topic-cheat-sheet/k8s.md`
- `../../容器运行时/99-docker-commands-reference.md`
- `../../系统基础/99-linux-commands-reference.md`

---

## 每日时间分配建议

| 环节 | 时间 | 内容 |
|------|------|------|
| **理论阅读** | 1.5h | 阅读必读文档，记录关键概念 |
| **实践操作** | 2-2.5h | 完成 day 文件中的实践任务 |
| **费曼复述** | 0.5h | 用自己的语言复述核心概念 |
| **记录总结** | 0.5h | 更新个人知识图谱 |

---

## 如何使用本学习计划

1. **按周顺序学习**: 从 Week 1 开始，按 Day 1 -> Day 7 顺序推进
2. **每日任务**: 每个 day 文件包含理论阅读、实践任务、费曼复述三个环节
3. **周末检验**: 每周末完成 `checkpoint.md` 中的自测题
4. **项目驱动**: 每周末完成一个实践项目，巩固所学知识
5. **记录成长**: 在 `resources/knowledge-map.md` 中记录个人知识图谱
6. **结合 SR**: 在实际工作中处理 SR 时，对照学习内容加深理解

---

## 要点总结

- **4 周路径**: ACK/ACR 基础 → 安全监控 → 节点工作负载 → 网络存储
- **每天 4-5 小时**: 理论 <= 1.5h，实践 >= 2.5h，费曼复述 0.5h
- **5 个项目**: 每周末一个递进式实践项目
- **668+ 篇知识库**: 按需深入，结合 day 文件中的必读文档
- **SR 驱动**: 在实际工单处理中巩固所学知识
- **两层权限**: RAM (云平台) + RBAC (集群内) 是 ACK 安全的基础

---

## 延伸阅读

- [ACK 产品文档](https://help.aliyun.com/product/85222.html)
- [ACR 产品文档](https://help.aliyun.com/product/60716.html)
- [Kubernetes 官方文档](https://kubernetes.io/docs/home/)
- [阿里云容器服务最佳实践](https://help.aliyun.com/document_detail/2627792.html)

开始你的 ACK/ACR/K8S 内部培训之旅吧!

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- networking|发布说明索引 — 网络]] — Cross-reference
- 网络 MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[AI基础设施/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[AI基础设施/基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[平台工程/运维/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[生态参考/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[生态参考/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
