---
title: 有状态副本集
description: StatefulSet 是 Kubernetes 中管理有状态应用的工作负载控制器。与 Deployment 不同，StatefulSet
  为每个 Pod 提供...
summary: StatefulSet 是 Kubernetes 中管理有状态应用的工作负载控制器。与 Deployment 不同，StatefulSet 为每个
  Pod 提供...
category: dictionary
tags:
- k8s
- glossary
- statefulset
- workload
- storage
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 有状态副本集 是什么
- StatefulSet 详解
trigger_keywords:
- 有状态副本集
- StatefulSet
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 有状态副本集

> **英文名**: StatefulSet

## 概述

StatefulSet 是 Kubernetes 中管理有状态应用的工作负载控制器。与 Deployment 不同，StatefulSet 为每个 Pod 提供稳定的网络标识、存储和有序的部署/扩缩容/删除顺序。

## 核心概念/原理

### 核心特性

- **稳定的网络标识**：每个 Pod 有固定的名称（如 `mysql-0`, `mysql-1`）和对应的 Headless Service DNS。
- **稳定的存储**：每个 Pod 通过 VolumeClaimTemplate 绑定独立的 PVC，Pod 重启/重调度后仍保持绑定。
- **有序操作**：Pod 按序号顺序创建（0→N-1），逆序删除（N-1→0）。
- **有序更新**：RollingUpdate 从高序号向低序号逆序更新。

### 与 Deployment 的对比

| 特性 | Deployment | StatefulSet |
|------|-----------|-------------|
| Pod 标识 | 随机名称 | 固定有序名称 |
| 存储 | 共享或无 | 每 Pod 独立 PVC |
| 创建顺序 | 并行 | 有序（0→N-1） |
| 适用场景 | 无状态应用 | 有状态应用 |

## 关键机制或特性

- `podManagementPolicy: Parallel` 可让 Pod 并行创建/删除。
- `serviceName` 必须指向一个 Headless Service。
- 删除 StatefulSet 不会自动删除关联的 PVC（保护数据安全）。

## 使用场景与最佳实践

- 数据库（MySQL、PostgreSQL）、消息队列（Kafka）、分布式存储等使用 StatefulSet。
- 为每个 Pod 配置独立的 PVC 和 VolumeClaimTemplate。
- 使用 `partition` 字段实现金丝雀更新。
- 考虑使用 Operator 模式管理复杂的有状态应用生命周期。

## 参考链接

- [StatefulSet - Official Documentation](https://kubernetes.io/docs/concepts/workloads/controllers/statefulset/)

## Related

- [[17-系统基础/06-知识字典/workloads/pod.md|Pod]]
- [[17-系统基础/06-知识字典/workloads/deployment.md|Deployment]]
- [[17-系统基础/06-知识字典/workloads/daemonset.md|Daemonset]]
- [[17-系统基础/06-知识字典/workloads/replicaset.md|Replicaset]]
- [[17-系统基础/06-知识字典/workloads/job.md|Job]]


<!-- risk-assessed -->
- [[01-集群基础/02-设计原则/03-declarative-api-pattern|02 - 声明式 API 与面向终态设计 (Declarative API)]]
- [[02-工作负载/01-核心工作负载/README-old|Domain-4: Kubernetes工作负载]]
- [[02-工作负载/01-核心工作负载/23-resource-management|16 - 资源管理表]]
- [[04-应用模式/02-行业架构/04-im-rtc-architecture|实时通信 (IM / RTC) Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/03-cms-architecture|内容管理系统 (CMS) Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/93-digital-twin-factory|数字孪生工厂架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/54-social-gaming-metaverse|社交游戏与元宇宙社交架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/31-instant-retail|即时零售架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/70-ecny-cbdc|数字人民币架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/42-secondhand-circular|二手交易与循环经济架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/26-aviation-travel|航空出行架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/43-enterprise-im|企业即时通讯架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/25-quantitative-trading|证券量化交易架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/48-vocational-edtech|职业教育培训架构设计 — 阿里云视角]]
- [[09-可观测性/00-总览/04-elastic-stack-enterprise-observability|Elastic Stack企业级可观测性平台深度实践]]
- [[09-可观测性/02-日志/07-splunk-enterprise-siem|Splunk企业级日志分析与安全智能平台深度实践]]
- [[11-发布变更/07-迁移方案/06-stateful-services-migration|06 - 有状态服务迁移 [migration]]]
- [[17-系统基础/04-K8s事件/09-job-cronjob-batch-events|09 - Job 与 CronJob 批处理事件]]
- [[17-系统基础/04-K8s事件/12-autoscaling-events|12 - 自动扩缩容事件 (HPA / VPA / Cluster Autoscaler)]]
- [[17-系统基础/04-K8s事件/08-statefulset-daemonset-events|08 - StatefulSet 与 DaemonSet 控制器事件]]
- [[17-系统基础/04-K8s事件/14-namespace-resource-gc-events|14 - Namespace、资源管理与垃圾回收事件]]
- [[17-系统基础/06-知识字典/fundamentals/kubernetes-self-healing|Kubernetes Self-Healing（Kubernetes 自愈能力）]]
- [[17-系统基础/06-知识字典/fundamentals/objects-in-kubernetes|Kubernetes 中的对象]]
- [[17-系统基础/06-知识字典/operations/node-shutdowns|节点关闭（Node Shutdowns）]]
- [[17-系统基础/06-知识字典/storage/dynamic-volume-provisioning|Dynamic Volume Provisioning（动态卷供给）]]
- [[17-系统基础/06-知识字典/storage/node-specific-volume-limits|Node-specific Volume Limits（节点特定卷限制）]]
- [[17-系统基础/06-知识字典/workloads/horizontal-pod-autoscaling|Horizontal Pod Autoscaling]]
- [[17-系统基础/06-知识字典/workloads/managing-workloads|Managing Workloads]]
- [[17-系统基础/06-知识字典/workloads/workload-management|Workload Management]]
- [[17-系统基础/06-知识字典/workloads/pod-hostname|Pod Hostname]]
- [[19-故障诊断/02-资源排障/13-statefulset-troubleshooting|StatefulSet 故障排查]]
- [[19-故障诊断/02-资源排障/07-ingress-troubleshooting|15 - Ingress 故障排查 (Ingress Troubleshooting)]]
- [[20-最佳实践/04-migration/06-stateful-services-migration|06 - 有状态服务迁移 [migration]]]
- [[22-概念/11-交叉分析/etcd-×-StatefulSet|etcd × StatefulSet]]
- [[22-概念/11-交叉分析/StatefulSet-×-Service|StatefulSet × Service]]
- [[22-概念/11-交叉分析/apiserver-×-StatefulSet|apiserver × StatefulSet]]
- [[22-概念/11-交叉分析/StatefulSet-×-Ingress|StatefulSet × Ingress]]
- [[22-概念/11-交叉分析/StatefulSet-×-NetworkPolicy|StatefulSet × NetworkPolicy]]
- [[22-概念/11-交叉分析/StatefulSet-×-PVC|StatefulSet × PVC]]
- [[22-概念/11-交叉分析/StatefulSet-×-RBAC|StatefulSet × RBAC]]
- [[26-技能/01-集群运维/migration/06-stateful-services-migration|06 - 有状态服务迁移 [migration]]]
- [[26-技能/02-控制面/scheduler/培训/learn-15-scheduling-basics|第15课：调度与亲和性]]
- [[26-技能/04-工作负载/hpa-vpa/培训/learn-09-hpa-basics|第九课：HPA - 自动伸缩]]
- [[26-技能/04-工作负载/job-cronjob/培训/learn-11-job-cronjob|第九课：Job 和 CronJob - 任务调度]]
- [[26-技能/04-工作负载/pod/资源与自动扩缩/字典-horizontal-pod-autoscaling|Horizontal Pod Autoscaling]]
- [[26-技能/04-工作负载/pod/配置与字典/pod-hostname|Pod Hostname]]
- [[26-技能/04-工作负载/statefulset/statefulset-fta|StatefulSet 异常故障树分析 (skills)]]
- [[26-技能/04-工作负载/statefulset/skill-21-statefulset-failure|StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation (skills)]]
- [[26-技能/04-工作负载/statefulset/培训/learn-14-statefulset-basics|第14课：StatefulSet - 有状态应用管理]]
- [[26-技能/05-网络/service/培训/kubernetes-service-presentation|Kubernetes Service 全栈进阶培训 (从入门到专家) [topic-presentations]]]
