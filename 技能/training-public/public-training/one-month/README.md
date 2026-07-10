---
title: Kubernetes 生产运维 1 个月学习计划
description: '# Kubernetes 生产运维 1 个月学习计划'
summary: '本学习计划旨在帮助运维工程师在一个月内从 Kubernetes 入门级提升到全栈运维能力。课程设计遵循"理论 + 实践"的黄金比例（40% 理论 : 60% 实践），每天 4-5 小时的学习时间，通过 28 天的系统性学习，覆盖从集群搭建到生产运维的完整知识体系。'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- scheduler
- prometheus
- jaeger
- helm
- argocd
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 生产运维 1 个月学习计划 是什么
- 如何 Kubernetes 生产运维 1 个月学习计划
trigger_keywords:
- Kubernetes
- 生产运维
- 个月学习计划
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- prometheus-basics
- gitops-basics
- etcd-basics
- gpu-scheduling-basics
- policy-basics
- logging-basics
- tracing-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Kubernetes|Kubernetes]] 生产运维 1 个月学习计划

```yaml
---
title: Kubernetes 生产运维一个月学习计划
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Kubernetes运维学习计划"
  - "一个月学习路径"
  - "云原生工程师培训"
  - "全栈运维课程"
trigger_keywords:
  - "K8s学习"
  - "一个月计划"
  - "云原生工程师"
  - "全栈运维"
  - "Docker"
  - "Kubernetes"
  - "监控排障"
  - "GitOps"
reading_level: intermediate
audience:
  - 运维工程师
  - sre工程师
  - devops工程师
estimated_read_time: 25min
related_domains:
  - 集群基础
  - 集群基础
  - 工作负载
  - 网络
  - 故障诊断
related_topics:
  - 生产运维/topic-learn/public-training/README
  - 生产运维/topic-learn/quick-start
  - 故障诊断
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
| Week 2 | 核心技术构建期 | 生产级应用 YAML 编排 | [week-2-core-tech/](./week-2-core-tech/) |
| Week 3 | 运维作战能力期 | 监控大盘 + 排障手册 | [week-3-operations/](./week-3-operations/) |
| Week 4 | 企业级进阶期 | GitOps 流水线 + Playbook | [week-4-enterprise/](./week-4-enterprise/) |
| - | 实践项目 | 5 个递进式项目 | [projects/](./projects/) |
| - | 补充资源 | 知识图谱模板 | [resources/](./resources/) |

---

## 整体学习路径

```
# 🟢 低风险：只读/信息收集，通常无副作用
Week 1: 地基建设期     Week 2: 核心技术构建期    Week 3: 运维作战能力期    Week 4: 企业级进阶期
├─ Docker 基础        ├─ 控制平面精读           ├─ 安全合规体系           ├─ 企业监控/日志平台
├─ Linux 基础         ├─ 工作负载深潜           ├─ 可观测性构建           ├─ GitOps & CI/CD
├─ K8s 架构全貌       ├─ 网络栈精通             ├─ 故障排查方法论         ├─ FTA/FEBM 专题
└─ kubectl 实战       └─ 存储体系               └─ 平台运维实践           └─ 生产最佳实践综合
    |                     |                         |                         |
    v                     v                         v                         v
 产出: K8s 集群搭建    产出: 生产级应用编排      产出: 监控大盘+排障手册   产出: GitOps 流水线
```
### 每周详细内容

| 周次 | Day | 主题 | 核心技能 | 对应 Domain |
|------|-----|------|---------|-------------|
| **W1** | Day 1 | Docker 容器基础 | 镜像构建、容器运行 | Domain 13 |
| | Day 2 | Linux 基础回顾 | Namespace/Cgroup | Domain 14 |
| | Day 3 | K8s 架构全览 | 控制面 + 数据面 | Domain 1 |
| | Day 4 | kubectl 实战 | 常用命令、资源操作 | Domain 1 |
| | Day 5 | Pod 深入理解 | 生命周期、探针 | Domain 4 |
| | Day 6 | Deployment 管理 | 滚动更新、回滚 | Domain 4 |
| | Day 7 | 项目 P1: 集群搭建 | kind/minikube 实操 | 综合 |
| **W2** | Day 8 | [[etcd|etcd]] + API Server | Raft、请求链路 | Domain 3 |
| | Day 9 | Scheduler + Controller | 调度算法、控制循环 | Domain 3 |
| | Day 10 | [[Service|Service]] 网络 | 四种类型、Endpoint | Domain 5 |
| | Day 11 | Ingress 路由 | Nginx Ingress 配置 | Domain 5 |
| | Day 12 | 存储基础 | PV/PVC/StorageClass | Domain 6 |
| | Day 13 | 有状态应用 | StatefulSet 实践 | Domain 4 |
| | Day 14 | 项目 P2: 应用编排 | 多层应用全栈部署 | 综合 |
| **W3** | Day 15 | RBAC 安全 | Role/ClusterRole | Domain 7 |
| | Day 16 | Pod 安全 + Secret | PSS、Kyverno | Domain 7 |
| | Day 17 | Prometheus 监控 | 部署、PromQL | Domain 8 |
| | Day 18 | 日志 + 追踪 | Loki、Jaeger | Domain 8 |
| | Day 19 | 故障排查基础 | 排障方法论 | Domain 12 |
| | Day 20 | FTA + FEBM | 故障树、取证分析 | topic-fta |
| | Day 21 | 项目 P3: 监控排障 | 监控 + 故障演练 | 综合 |
| **W4** | Day 22 | 平台运维基础 | 节点维护、组件管理 | Domain 9 |
| | Day 23 | ELK + GitOps | ArgoCD 实践 | Domain 21/23 |
| | Day 24 | Helm 包管理 | Chart 开发、Repo | Domain 22 |
| | Day 25 | 生产最佳实践 | 变更管理、事故响应 | Domain 18 |
| | Day 26 | FTA/FEBM 深化 | 复杂故障分析 | topic-fta |
| | Day 27 | 项目 P4: GitOps | ArgoCD 多环境 | 综合 |
| | Day 28 | 项目 P5: 毕业 | 综合实践 | 综合 |

---

## 知识依赖关系

```
# 🟢 低风险：只读/信息收集，通常无副作用
Domain13(Docker) ─┐
Domain14(Linux)  ─┼─> Domain1(架构) ─> Domain3(控制平面) ─> Domain9(平台运维)
Domain15(网络基础)┘       │                 │                     │
                          v                 v                     v
                     Domain4(工作负载)   Domain5(网络)      Domain12(故障排查)
                          │              Domain6(存储)           │
                          v                 │                    v
                     Domain7(安全)  <───────┘             故障诊断/topic-fta/febm
                     Domain8(可观测性)
                          │
                          v
                 Domain18-33(企业级专题)
```
### Domain 学习优先级

| 优先级 | Domain | 说明 | 学习周次 |
|--------|--------|------|---------|
| **P0** | Domain 1: 架构基础 | 必须最先掌握 | Week 1 |
| **P0** | Domain 4: 工作负载 | 最常用的资源类型 | Week 1-2 |
| **P1** | Domain 3: 控制平面 | 理解 K8s 运作机制 | Week 2 |
| **P1** | Domain 5: 网络 | Service/Ingress/CNI | Week 2 |
| **P1** | Domain 6: 存储 | PV/PVC/CSI | Week 2 |
| **P2** | Domain 7: 安全 | RBAC/PSS/Secret | Week 3 |
| **P2** | Domain 8: 可观测性 | Prometheus/日志 | Week 3 |
| **P2** | Domain 12: 故障排查 | 排障方法论 | Week 3 |
| **P3** | Domain 18: 生产运维 | 最佳实践 | Week 4 |
| **P3** | Domain 23: GitOps | ArgoCD/CI-CD | Week 4 |

---

## 学习方法论

### 1. 费曼学习法 (每日)
每天学完一个模块后，用自己的语言向"虚拟初学者"复述，检测理解漏洞。

### 2. 间隔重复 (每周)
- 每周第一天用 15 分钟回顾上周关键概念
- 每周末复习本周 10 个核心术语

### 3. 主动回忆 (每节)
先合上文档，尝试回答: "这个组件做什么？它和哪些组件交互？出问题了怎么排查？"

### 4. 实践优先原则
理论文档读完后，立刻动手复现。每天 4 小时中: 理论 <= 1.5h，实践 >= 2.5h

### 5. 结构化记录
每个 Domain 学完后，产出一张思维导图或 README 摘要，形成个人知识图谱。

### 6. 项目驱动学习
每个周末完成一个实践项目，将本周所学知识串联应用。

---

## 每周目标与产出

| 周次 | 核心产出 | 完成评估标准 |
|------|----------|--------------|
| Week 1 | K8s 集群环境 + 架构图 | 能独立搭建集群，能解释所有组件职责 |
| Week 2 | 生产级应用 YAML 编排 | 能完整部署含网络/存储的多层应用 |
| Week 3 | 监控告警体系 + 排障手册 | 能独立构建监控栈，30分钟内定位问题 |
| Week 4 | GitOps 流水线 + Playbook | 任何变更都通过 Git PR 触发部署，有文档化 SOP |

---

## 实践项目清单

| # | 项目名称 | 周 | 难度 | 详情 | 核心技能 |
|---|----------|---|------|------|---------|
| P1 | 从零搭建 K8s 集群 | Week 1 | ★★☆ | [p1-k8s-cluster-setup.md](./projects/p1-k8s-cluster-setup.md) | kind, kubectl, Deployment |
| P2 | 生产级应用全栈编排 | Week 2 | ★★★ | [p2-production-app-orchestration.md](./projects/p2-production-app-orchestration.md) | Service, Ingress, PVC |
| P3 | 可观测性体系 + 故障演练 | Week 3 | ★★★ | [p3-observability-fault-drill.md](./projects/p3-observability-fault-drill.md) | Prometheus, 排障 |
| P4 | GitOps 流水线 | Week 4 | ★★★ | [p4-gitops-pipeline.md](./projects/p4-gitops-pipeline.md) | ArgoCD, Kustomize |
| P5 | 毕业综合实践项目 | Week 4 | ★★★★ | [p5-graduation-project.md](./projects/p5-graduation-project.md) | 全栈综合 |

---

## 关键文件索引

### 核心架构文档
- `../集群基础/01-kubernetes-architecture-overview.md`
- `../集群基础/02-core-components-deep-dive.md`

### 故障排查体系
- `../故障诊断/topic-fta/23-fta-production-quick-start.md`
- `../故障诊断/topic-febm/08-febm-production-quick-start.md`
- `../故障诊断/` (42篇)

### 生产运维实践
- `../生产运维/23-incident-response-handling.md`
- `../生产运维/22-change-management-process.md`

### 速查手册
- `../系统基础/topic-cheat-sheet/k8s.md`
- `../容器运行时/99-docker-commands-reference.md`
- `../系统基础/99-linux-commands-reference.md`

---

## 如何使用本学习计划

1. **按周顺序学习**: 从 Week 1 开始，按 Day 1 -> Day 7 顺序推进
2. **每日任务**: 每个 day 文件包含理论阅读、实践任务、费曼复述三个环节
3. **周末检验**: 每周末完成 `checkpoint.md` 中的自测题
4. **项目驱动**: 每周末完成一个实践项目，巩固所学知识
5. **记录成长**: 在 `resources/knowledge-map.md` 中记录个人知识图谱

---

## 要点总结

- **4 周学习路径**: 地基建设 → 核心技术 → 运维能力 → 企业级实践
- **每天 4 小时**: 理论 <= 1.5h，实践 >= 2.5h
- **5 个递进式项目**: 从集群搭建到 GitOps 流水线
- **知识库 668+ 篇文档**: 按需深入，不需要全部读完
- **费曼学习法**: 每天用自己的语言复述核心概念
- **产出导向**: 每周有明确的可验证产出

---

## 延伸阅读

- [Kubernetes 官方文档](https://kubernetes.io/docs/home/)
- [Kubernetes the Hard Way](https://github.com/kelseyhightower/kubernetes-the-hard-way)
- [Google SRE Book](https://sre.google/sre-book/table-of-contents/)
- [CNCF 云原生全景图](https://landscape.cncf.io/)

开始你的 Kubernetes 全栈运维之旅吧!

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[实体/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- 网络 MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[概念/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[概念/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[AI基础设施/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[AI基础设施/基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[技能/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[技能/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[平台工程/运维/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[实体/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[生态参考/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[生态参考/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
