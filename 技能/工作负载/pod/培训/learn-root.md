---
title: Kubernetes 培训：Root
description: '**专题**: topic-learn'
summary: '**专题**: topic-learn'
category: skills
tags:
- k8s
- learn
- training
- root
- hpa
- statefulset
- ingress
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 培训：Root 是什么
- 如何 Kubernetes 培训：Root
trigger_keywords:
- Kubernetes
- 培训：Root
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




### topic-learn MOC

# topic-learn MOC

> **MOC 版本**: 1.0
> **专题**: topic-learn
> **文档数量**: 123 篇
> **最后更新**: 2026-05-21
> **用途**: 本专题的导航入口，汇总所有相关文档

---

## 专题概述

学习计划 — 系统学习路径与考核

### 专题定位

| 维度 | 说明 |
|---|---|
| **专题** | topic-learn |
| **文档数量** | 123 篇（展示前 50 篇） |
| **难度分布** | 入门 17 / 进阶 2 / 高级 0 / 专家 0 |

---

## 文档清单

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|
| 1 | [[实体/kubernetes.md|kubernetes]] | 入门 | tutorial, k8s, training | 5min |
| 2 | 第二课：Pod - K8s 的最小调度单元 | 入门 | tutorial, Pod, 容器组 | 5min |
| 3 | 第三课：Deployment - 应用部署管理器 | 入门 | tutorial, deployment, Deployment | 5min |
| 4 | 第四课：Service - 让应用可以被访问 | 入门 | tutorial, Service, 服务 | 5min |
| 5 | 第五课：Ingress - 外部 HTTP/HTTPS 访问 | 入门 | tutorial, Ingress, 入口 | 5min |
| 6 | 第六课：ConfigMap 和 Secret - 配置管理 | 入门 | tutorial, configuration, k8s | 5min |
| 7 | 第七课：Namespace 与资源隔离 | 入门 | tutorial, k8s, training | 5min |
| 8 | 第八课：存储 - PV 和 PVC | 入门 | tutorial, k8s, training | 5min |
| 9 | 第九课：HPA - 自动伸缩 | 入门 | tutorial, k8s, training | 5min |
| 10 | 10-health-check — 第14课：StatefulSet - 有状态应用管理
- [[实体/statefulset.md|statefulset]] — StatefulSet
- [[deployment]] — Deployment
- [[kubernetes]] — Kubernetes (CNCF Graduated)

## 学习路径指南

### K8s 学习路线图

```
入门 (1-2周)
├── 核心概念: Pod, Deployment, Service
├── 基本操作: kubectl 常用命令
└── 实验: 部署第一个应用

进阶 (1-2月)
├── 网络: CNI, Service, Ingress
├── 存储: PV, PVC, StorageClass
├── 配置: ConfigMap, Secret
└── 实验: 完整应用部署

高级 (3-6月)
├── 调度: 亲和性, 污点, 优先级
├── 安全: RBAC, PSS, NetworkPolicy
├── 运维: 监控, 备份, 升级
└── 实验: 生产级集群运维

专家 (6月+)
├── 源码: 控制器模式, 调度器
├── 扩展: CRD, Operator, Webhook
├── 架构: 多集群, 服务网格
└── 实验: 自定义 Operator 开发
```

### 学习资源推荐

| 类型 | 资源 | 适用阶段 |
|---|---|---|
| 官方文档 | kubernetes.io/docs | 全阶段 |
| 书籍 | 《Kubernetes in Action》 | 入门-进阶 |
| 实验 | Killercoda, Play with K8s | 入门-进阶 |
| 源码 | kubernetes/kubernetes | 高级-专家 |

## 面试要点

1. **Q：如何高效学习 K8s？**
   A：理论+实践结合、从简单到复杂、多动手实验、阅读源码、参与社区。

2. **Q：K8s 学习的关键里程碑？**
   A：能部署应用→能排查故障→能设计架构→能开发扩展→能贡献社区。

3. **Q：如何保持技术更新？**
   A：关注 Release Notes、参与社区、阅读源码、实践新技术、分享总结。

## See Also

- [[技能/learn-oncall-quick-qa.md|learn-oncall-quick-qa]]
- [[技能/learn-public-training.md|learn-public-training]]
- [[技能/manage-persistent-storage.md|manage-persistent-storage]]
- [[技能/monitor-kubernetes-metrics.md|monitor-kubernetes-metrics]]


<!-- risk-assessed -->
