---
title: K8s 运维实战培训（四周体系）
description: '# K8s 运维实战培训（四周体系）'
summary: '本培训体系为 K8s 运维工程师设计，覆盖从零基础到独立处理 oncall 工单的完整学习路径。培训周期 28 天（四周），采用每日主题学习 + 实操练习的形式，结合理论知识与真实场景演练，确保学以致用。'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- kubelet
- scheduler
- prometheus
- grafana
- flannel
- statefulset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8s 运维实战培训（四周体系） 是什么
- 如何 K8s 运维实战培训（四周体系）
trigger_keywords:
- K8s
- 运维实战培训
- 四周体系
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8s 运维实战培训（四周体系）

```yaml
---
title: K8s 运维实战培训（四周体系）
last_updated: 2026-05-18
difficulty: beginner
intent_queries:
  - "K8s培训课程"
  - "四周学习路径"
  - "实操教程索引"
  - "SRE工程师培训"
  - "Kubernetes认证准备"
trigger_keywords:
  - "K8s培训"
  - "四周学习"
  - "实操培训"
  - "运维工程师"
  - "集群管理"
  - "安全监控"
  - "故障排查"
  - "云原生运维"
reading_level: beginner
audience:
  - sre工程师
  - ops工程师
  - 运维工程师
estimated_read_time: 15min
related_domains:
  - 集群基础
  - 工作负载
  - 故障诊断
related_topics:
  - 生产运维/topic-learn/quick-start
  - 故障诊断/topic-skills/assessment/k8s-fundamentals-quiz
  - 系统基础/topic-dictionary/k8s-glossary
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
| Day 1 | ACK/ACR 管控架构（云厂商） | - |
| Day 2 | K8s SDK & API（云厂商） | - |
| Day 3 | 控制台与功能（云厂商） | - |
| Day 4 | K8s 新建集群 | - |
| Day 5 | K8s 集群删除 | - |
| Day 6 | K8s 集群升级 | - |
| Day 7 | K8s 集群证书 | - |

**本周产出**: 能够独立完成集群创建、升级、删除全流程操作

---

## Week 2: 安全监控运维（进阶）

> **目标**: 建立集群安全体系与监控运维能力

| Day | 主题 | 文件 |
|:---:|------|------|
| Day 8 | K8s RBAC 权限配置实操 | [day-08-rbac/01-rbac-hands-on.md](./day-08-rbac/01-rbac-hands-on.md) |
| Day 9 | K8s 审计日志配置与分析实操 | [day-09-audit-logs/01-audit-logs-hands-on.md](./day-09-audit-logs/01-audit-logs-hands-on.md) |
| Day 10 | K8s 集群监控体系搭建实操 | [day-10-cluster-monitoring/01-cluster-monitoring-hands-on.md](./day-10-cluster-monitoring/01-cluster-monitoring-hands-on.md) |
| Day 11 | K8s 安全风险识别与防护实操 | [day-11-risk-assessment/01-risk-assessment-hands-on.md](./day-11-risk-assessment/01-risk-assessment-hands-on.md) |

**本周产出**: 能够配置集群 RBAC 权限、识别安全风险、搭建基础监控

---

## Week 3: 节点与工作负载管理（进阶）

> **目标**: 精通节点管理与工作负载运维

| Day | 主题 | 文件 |
|:---:|------|------|
| Day 15 | Node 节点基础实操 | [day-15-node-basics/01-node-basics-hands-on.md](./day-15-node-basics/01-node-basics-hands-on.md) |
| Day 16 | Node 节点进阶实操 | [day-16-node-advanced/01-node-advanced-hands-on.md](./day-16-node-advanced/01-node-advanced-hands-on.md) |
| Day 17 | 节点池基础实操 | [day-17-nodepool/01-nodepool-basics-hands-on.md](./day-17-nodepool/01-nodepool-basics-hands-on.md) |
| Day 18 | 节点池进阶实操 | [day-18-nodepool-advanced/01-nodepool-advanced-hands-on.md](./day-18-nodepool-advanced/01-nodepool-advanced-hands-on.md) |
| Day 19 | Pod 容器组基础实操 | [day-19-pod-basics/01-pod-basics-hands-on.md](./day-19-pod-basics/01-pod-basics-hands-on.md) |
| Day 20 | Pod 容器组进阶实操 | [day-20-pod-advanced/01-pod-advanced-hands-on.md](./day-20-pod-advanced/01-pod-advanced-hands-on.md) |
| Day 21 | K8s 组件运维实操 | [day-21-component-ops/01-component-ops-hands-on.md](./day-21-component-ops/01-component-ops-hands-on.md) |

**本周产出**: 能够管理节点池、排查 Pod 问题、维护 K8s 核心组件

---

## Week 4: 网络与存储（进阶）

> **目标**: 掌握集群网络架构与存储管理

| Day | 主题 | 文件 |
|:---:|------|------|
| Day 22 | [[Service|Service]] 基础实操 | [day-22-service-basics/01-service-basics-hands-on.md](./day-22-service-basics/01-service-basics-hands-on.md) |
| Day 23 | [[Ingress|Ingress]] 实操 | [day-23-ingress/01-ingress-hands-on.md](./day-23-ingress/01-ingress-hands-on.md) |
| Day 24 | Terway 网络实操 | [day-24-terway/01-terway-hands-on.md](./day-24-terway/01-terway-hands-on.md) |
| Day 25 | Flannel 网络实操 | [day-25-flannel/01-flannel-hands-on.md](./day-25-flannel/01-flannel-hands-on.md) |
| Day 26 | 存储卷创建与删除实操 | [day-26-pvc-create/01-pvc-create-hands-on.md](./day-26-pvc-create/01-pvc-create-hands-on.md) |
| Day 27 | 存储卷挂载实操 | [day-27-pvc-mount/01-pvc-mount-hands-on.md](./day-27-pvc-mount/01-pvc-mount-hands-on.md) |
| Day 28 | 综合复习与实践 | [day-28-comprehensive-review/01-comprehensive-review.md](./day-28-comprehensive-review/01-comprehensive-review.md) |

**本周产出**: 能够配置 Service/Ingress、排查网络问题、管理存储卷

---

## 每日学习节奏

```
09:00 - 11:00  理论学习 (必读文档 + 阅读要点)
11:00 - 11:15  休息
11:15 - 12:00  实践任务 (上半场)
12:00 - 13:00  午餐
13:00 - 15:30  实践任务 (下半场)
15:30 - 16:00  费曼复述 (用自己的语言回答问题)
16:00 - 16:30  今日检验 (对照检验清单)
```

---

## 培训主题索引

| 类别 | 包含主题 |
|------|---------|
| 集群生命周期 | 新建集群、集群删除、集群升级、集群证书 |
| 安全认证 | RBAC、审计日志、漏洞与风险点 |
| 监控运维 | 集群监控、告警配置、Prometheus/Grafana |
| 节点管理 | Node 节点、节点池、cordon/drain/uncordon |
| 工作负载 | Pod 容器组、Deployment、StatefulSet、探针 |
| 网络 | Service、Ingress、Terway、Flannel、NetworkPolicy |
| 存储 | PV/PVC、StorageClass、emptyDir、hostPath |
| 组件运维 | API Server、Scheduler、Controller Manager、etcd、kubelet |

---

## 技能等级定义

| 等级 | 定义 | 能力描述 |
|------|------|---------|
| L1 入门 | 了解概念 | 能说出概念定义，了解基本原理 |
| L2 理解 | 理解原理 | 能解释工作原理，理解组件关系 |
| L3 应用 | 独立操作 | 能独立完成操作，处理常见问题 |
| L4 精通 | 深度掌握 | 能排查复杂问题，设计优化方案 |

培训完成后目标等级:

| 能力域 | 目标等级 |
|--------|---------|
| 集群生命周期管理 | L3 |
| 安全与权限管理 | L3 |
| 监控与告警 | L3 |
| 节点与工作负载 | L3 |
| 网络与存储 | L3 |
| 故障排查 | L2-L3 |

---

## 配套资源

| 资源 | 路径 | 用途 |
|------|------|------|
| oncall 速查卡 | `P1-5-oncall-quick-reference-card.md` | 值班快速参考 |
| kubectl 场景速查卡 | `系统基础/topic-cheat-sheet/kubectl-scene-cheatsheet.md` | 命令场景对照 |
| 新人上手路径 | `生产运维/topic-learn/quick-start/` | 快速入门指南 |
| 考核评估体系 | `故障诊断/topic-skills/assessment/` | 技能评估工具 |
| K8s 术语表 | `系统基础/topic-dictionary/k8s-glossary.md` | 术语查询 |
| 故障排查手册 | `故障诊断/` | 故障排查参考 |
| 内部培训教案 | `生产运维/topic-learn/inner-training/` | 详细教案 (阿里云版本) |
| 公开培训教案 | `生产运维/topic-learn/public-training/` | 通用版本教案 |

---

## 学习建议

### 新手 (无 K8s 经验)

```
1. 按顺序完成 Week 1-4
2. 每天不要跳过费曼复述
3. 每周 checkpoint 必须达到 60% 以上再继续
4. 综合项目独立完成后再看参考答案
```

### 有经验 (6 个月以上 K8s 经验)

```
1. 快速浏览 Week 1-2，重点做 checkpoint
2. 重点学习 Week 3-4 的进阶内容
3. 关注故障排查和最佳实践部分
4. 完成所有综合项目
```

### 阿里云 ACK 用户

```
1. 优先使用 inner-training 路径
2. Day 1-7 涵盖 ACK/ACR 特有内容
3. 关注 aliyun CLI 和 API 操作
4. 注意 ACK 与开源 K8s 的差异
```

---

```yaml
---
id: TRAINING-INDEX-001
topic: training
type: index
tags: [training, learning-path, week-1-4, hands-on, k8s-1.28-1.33]
intent_queries:
  - "K8s 培训课程"
  - "四周学习路径"
  - "实操教程索引"
difficulty: beginner
target_roles: [sre, ops-engineer]
related:
  - 生产运维/topic-learn/quick-start/README.md
  - 故障诊断/topic-skills/assessment/k8s-fundamentals-quiz.md
  - 系统基础/topic-dictionary/k8s-glossary.md
---
```

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- 网络 MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[AI基础设施/01-ai-infra/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[AI基础设施/01-ai-infra/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[平台工程/operate/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[生态参考/topic-index/cluster-index.md|Cluster 集群知识图谱索引]]
- [[生态参考/topic-index/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/topic-index/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/topic-index/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[生态参考/topic-index/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
