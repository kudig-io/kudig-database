---
title: Kueue [entities]
description: 'Kubernetes 原生作业队列系统，为批处理与 AI/ML 工作负载提供配额管理与作业排队能力'
summary: 'Kueue 是 Kubernetes SIG-Scheduling 子项目，提供作业级队列、配额借用、多集群分发（MultiKueue）等能力，是 AI/ML 批量调度的核心组件之一。'
category: entities
tags:
- k8s
- kueue
- scheduler
- batch
- queue
- ai-ml
- gang-scheduling
tier: supporting
created: '2026-07-27'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
estimated_read_time: 6min
intent_queries:
- Kueue 是什么
- Kueue 与 Volcano 有什么区别
- 如何用 Kueue 管理批处理作业配额
trigger_keywords:
- Kueue
- ClusterQueue
- LocalQueue
- MultiKueue
prerequisites:
- kubectl-basics
---

# Kueue

> **归属**: Kubernetes SIG-Scheduling 子项目 | **类别**: Orchestration / Batch | **主要语言**: Go

## 概述

Kueue 是 Kubernetes 官方 SIG-Scheduling 维护的作业排队系统（Job Queueing）。它不替换 kube-scheduler，而是工作在其上游：决定"作业何时可以被准入（admit）并占用配额"，准入后再交由默认调度器完成 Pod 放置。这一分层设计使 Kueue 与 K8s 原生 Job、JobSet、RayJob、PyTorchJob 等上层工作负载天然兼容，是 AI/ML 训练与批处理场景下配额治理的主流选择。

## Key Features（核心能力）

- **ClusterQueue / LocalQueue 两级队列**：集群级配额池 + 命名空间级提交入口
- **Cohort 配额借用**：同一 cohort 内的队列可临时借用彼此的空闲配额
- **Gang Admission**：整组资源齐备才准入，避免部分 Pod 长期 Pending 死锁
- **抢占（Preemption）**：高优先级/回收借用配额时可抢占低优先级作业
- **MultiKueue**：多集群作业分发，作业提交到管理集群、在最先满足配额的工作集群执行
- **TopologyAwareScheduling**：结合拓扑感知放置，优化 RDMA/NVLink 亲和

## 架构与工作机制

```
用户 Job (suspend=true)
   │  指定 kueue.x-k8s.io/queue-name
   ▼
LocalQueue (namespace 级)
   ▼
ClusterQueue (集群级配额: CPU/Memory/GPU flavor)
   │  配额满足 → unsuspend, 注入 nodeSelector/toleration
   ▼
kube-scheduler 完成 Pod 放置
```

Kueue 通过 Job 的 `spec.suspend` 字段实现"先排队后运行"：作业创建时被挂起，准入后由 Kueue 置为运行。ResourceFlavor 抽象异构资源（如 A100 与 H100 GPU 池），配额按 flavor 维度定义。

## 快速上手

```yaml
apiVersion: kueue.x-k8s.io/v1beta1
kind: ClusterQueue
metadata:
  name: team-a-cq
spec:
  namespaceSelector: {}
  resourceGroups:
  - coveredResources: ["cpu", "memory", "nvidia.com/gpu"]
    flavors:
    - name: default-flavor
      resources:
      - name: "cpu"
        nominalQuota: 100
      - name: "memory"
        nominalQuota: 512Gi
      - name: "nvidia.com/gpu"
        nominalQuota: 16
---
apiVersion: kueue.x-k8s.io/v1beta1
kind: LocalQueue
metadata:
  name: team-a
  namespace: ml-team-a
spec:
  clusterQueue: team-a-cq
```

作业提交时添加标签 `kueue.x-k8s.io/queue-name: team-a` 即可进入排队。

## 与 Volcano / YuniKorn 对比

| 维度 | Kueue | [[23-实体/09-编排调度/volcano|Volcano]] | YuniKorn |
|------|-------|---------|----------|
| 定位 | 作业排队/配额准入 | 完整批调度器（替换调度器） | 完整批调度器 |
| 侵入性 | 低（复用 kube-scheduler） | 中（自带 scheduler） | 中 |
| Gang 语义 | 准入层 Gang Admission | 调度层 Gang Scheduling | 调度层 Gang |
| 多集群 | MultiKueue 原生支持 | 无原生支持 | 无原生支持 |
| 社区归属 | Kubernetes SIG 官方 | CNCF 孵化 | Apache |

选型建议：K8s 原生栈、需要多集群分发选 Kueue；需要复杂调度插件（如 binpack、SLA、拓扑）且可接受替换调度器选 Volcano。两者也可组合（Kueue 管配额、Volcano 管放置）。

## 生产运维要点

- 🟢 查看队列状态：`kubectl get clusterqueue,localqueue -A`；`kubectl describe clusterqueue <cq>` 检查 `Admitted Workloads` 与 `Pending Workloads`
- 🟡 配额调整会触发借用回收与抢占，变更前评估在跑作业影响
- 🔴 删除 ClusterQueue 会导致其下所有排队作业无法准入，需先迁移 LocalQueue
- 作业长期 Pending 排查顺序：LocalQueue→ClusterQueue 配额余量 → ResourceFlavor 节点标签匹配 → 集群实际可分配资源

## 相关阅读

- [[15-AI基础设施/05-K8s-AI基础设施/08-batch-scheduling-kueue-yunikorn|Kueue 与 YuniKorn 批量调度实践]]
- [[23-实体/09-编排调度/volcano|Volcano]]
- [[22-概念/07-调度与资源/gang-scheduling|Gang Scheduling 概念]]
