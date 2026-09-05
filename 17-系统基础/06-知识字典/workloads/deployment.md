---
title: Deployment
description: Deployment 是 Kubernetes 中管理无状态应用的核心工作负载控制器。它通过管理 ReplicaSet 来维护期望的 Pod
  副本数和版本，支持...
summary: Deployment 是 Kubernetes 中管理无状态应用的核心工作负载控制器。它通过管理 ReplicaSet 来维护期望的 Pod 副本数和版本，支持...
category: dictionary
tags:
- k8s
- glossary
- deployment
- workload
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Deployment 是什么
- Deployment 详解
trigger_keywords:
- Deployment
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Deployment

> **英文名**: Deployment

## 概述

Deployment 是 Kubernetes 中管理无状态应用的核心工作负载控制器。它通过管理 ReplicaSet 来维护期望的 Pod 副本数和版本，支持声明式更新、滚动发布和回滚。

## 核心概念/原理

### 核心能力

- **声明式更新**：修改 Pod 模板后，Deployment 自动执行滚动更新。
- **版本管理**：每次更新创建新的 ReplicaSet，保留历史记录支持回滚。
- **滚动更新策略**：通过 `maxSurge` 和 `maxUnavailable` 控制更新节奏。
- **扩缩容**：修改 `replicas` 字段即可调整副本数。

### 更新流程

```
修改 Pod 模板 → 创建新 ReplicaSet → 逐步增加新 Pod → 逐步减少旧 Pod → 完成更新
```

## 关键机制或特性

- `strategy.type: RollingUpdate` 是最常用的更新策略，保证零停机。
- `strategy.type: Recreate` 先停掉所有旧 Pod 再创建新 Pod，适用于不兼容版本升级。
- `revisionHistoryLimit` 控制保留的历史 ReplicaSet 数量（默认 10）。
- `minReadySeconds` 确保新 Pod 就绪后才继续更新。

## 使用场景与最佳实践

- 生产环境始终使用 Deployment 而非裸 ReplicaSet 管理应用。
- 设置合理的 `maxSurge` 和 `maxUnavailable`（推荐 25%/25%）。
- 使用 `kubectl rollout status` 监控更新进度。
- 配置 Pod 的反亲和性，确保副本分布在不同的节点/可用区。

## 参考链接

- [Deployment - Official Documentation](https://kubernetes.io/docs/concepts/workloads/controllers/deployment/)

## Related

[[23-实体/02-K8s核心组件/deployment.md|Deployment]]


<!-- risk-assessed -->
- [[02-工作负载/01-核心工作负载/19-scheduler-configuration|调度器配置与优化]]
- [[02-工作负载/01-核心工作负载/10-workload-controllers-overview|工作负载控制器详解]]
- [[02-工作负载/01-核心工作负载/11-pod-lifecycle-events|Pod 生命周期事件表]]
- [[22-概念/08-可靠性与运维/Symptom-SOP-RootCause-Mapping|Symptom-SOP-RootCause Mapping]]
- [[22-概念/08-可靠性与运维/Structural-Troubleshooting-Framework|Structural Troubleshooting Framework]]
- [[22-概念/11-交叉分析/Deployment-×-PVC|Deployment × PVC]]
- [[22-概念/11-交叉分析/Deployment-×-Service|Deployment × Service]]
- [[22-概念/11-交叉分析/Pod 生命周期 × Secret 管理|Pod 生命周期 × Secret 管理]]
- [[22-概念/11-交叉分析/apiserver-×-Deployment|apiserver × Deployment]]
- [[22-概念/11-交叉分析/控制器模式 × Deployment|控制器模式 × Deployment]]
- [[22-概念/11-交叉分析/Deployment-×-Ingress|Deployment × Ingress]]
- [[22-概念/11-交叉分析/etcd-×-Deployment|etcd × Deployment]]
- [[22-概念/11-交叉分析/Deployment × Secret 管理|[[deployment]] × Secret 管理]]
- [[22-概念/11-交叉分析/Deployment-×-RBAC|Deployment × RBAC]]
- [[22-概念/11-交叉分析/Deployment-×-NetworkPolicy|Deployment × NetworkPolicy]]
- [[22-概念/12-研究/ai-agent-README|AI Agent 工程专题]]
- [[22-概念/12-研究/ai-agent-MOC|02-ai-agents MOC]]
- [[23-实体/08-交付与制品/porter|Porter (entities)]]
- [[23-实体/09-编排调度/metal3-io|Metal3]]
- [[23-实体/15-参考与索引/workloads-terms|K8s 工作负载术语参考]]
- [[23-实体/15-参考与索引/fundamentals-terms|K8s 基础概念术语参考]]
- [[23-实体/15-参考与索引/release-notes-kubernetes|发布说明索引 — Kubernetes]]
- [[23-实体/15-参考与索引/kubectl Scenario Quick Reference|kubectl Scenario Quick Reference]]
- [[23-实体/15-参考与索引/k8s-control-plane-deep-dive|控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI]]
- [[23-实体/15-参考与索引/k8s-ai-agent-engineering|AI Agent 工程：RAG、多 Agent 编排、安全护栏与生产部署]]
- [[23-实体/15-参考与索引/KUDIG Scenario Taxonomy|KUDIG Scenario Taxonomy]]
- [[23-实体/15-参考与索引/k8s-supply-chain-yaml-cheatsheet|供应链安全、YAML 配置清单与速查表]]
- [[26-技能/01-集群运维/kubeadm/kubeadm-cluster-lifecycle|kubeadm 集群创建生命周期]]
- [[26-技能/02-控制面/scheduler/培训/learn-15-scheduling-basics|第15课：调度与亲和性]]
- [[26-技能/04-工作负载/daemonset/培训/learn-13-daemonset-basics|第13课：DaemonSet 与节点守护]]
- [[26-技能/04-工作负载/deployment/deployment-canary-and-bluegreen|金丝雀与蓝绿发布]]
- [[26-技能/04-工作负载/deployment/deployment-workload-selection|工作负载控制器选型]]
- [[26-技能/04-工作负载/deployment/deployment-rolling-update|Deployment 滚动更新策略]]
- [[26-技能/04-工作负载/deployment/培训/learn-03-deployment-basics|第三课：Deployment - 应用部署管理器]]
- [[26-技能/04-工作负载/hpa-vpa/培训/learn-09-hpa-basics|第九课：HPA - 自动伸缩]]
- [[26-技能/04-工作负载/job-cronjob/培训/learn-11-job-cronjob|第九课：Job 和 CronJob - 任务调度]]
- [[26-技能/04-工作负载/pod/培训/learn-01-day-one-checklist|Day 1: 新人首日检查清单]]
- [[26-技能/04-工作负载/pod/培训/learn-01-what-is-kubernetes|第一课：Kubernetes 入门]]
- [[26-技能/04-工作负载/pod/培训/learn-06-configmap-secret|第六课：ConfigMap 和 Secret - 配置管理]]
- [[26-技能/04-工作负载/pod/培训/learn-10-health-check|第八课：健康检查 - Probe 详解]]
- [[26-技能/04-工作负载/pod/培训/learn-lecturer-persona|K8S 讲师角色设定与场景规范]]
- [[26-技能/04-工作负载/pod/培训/learn-root|Kubernetes 培训：Root]]
- [[26-技能/04-工作负载/pod/培训/learn-02-pod-basics|第二课：Pod - K8s 的最小调度单元]]
- [[26-技能/04-工作负载/pod/培训/learn-04-debug-tools-setup|Day 4: 调试工具全家桶安装]]
- [[26-技能/04-工作负载/pod/培训/learn-12-common-problems|第十课：常见问题排查]]
- [[26-技能/04-工作负载/pod/培训/learn-analogy-dictionary|K8S 概念类比词典]]
- [[26-技能/04-工作负载/pod/培训/测验/assessment-k8s-fundamentals-quiz|K8S Fundamentals Quiz]]
- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles|FTA Methodology and Core Principles]]
- [[26-技能/04-工作负载/pod/概念原理/Pod生命周期×Secret管理|Pod 生命周期 × Secret 管理]]
- [[26-技能/04-工作负载/pod/生命周期与事件/02-pod-lifecycle-events|Pod 生命周期事件表]]
- [[26-技能/04-工作负载/statefulset/培训/learn-14-statefulset-basics|第14课：StatefulSet - 有状态应用管理]]
- [[26-技能/05-网络/service/培训/learn-04-service-basics|第四课：Service - 让应用可以被访问]]
- [[26-技能/07-安全/resource-quota/培训/learn-07-namespace-resource-quota|第七课：Namespace 与资源隔离]]
