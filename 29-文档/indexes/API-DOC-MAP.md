---
title: KUDIG API → 文档映射
description: '| `Pod` | v1 | 最小部署单元 | [[02-工作负载/01-核心工作负载/11-pod-lifecycle-events.md|11
  pod lifecycle events]] | [[19-故障诊断/06-FTA故障树/list/pod-fta.md|pod
  fta]] |'
summary: '| `Pod` | v1 | 最小部署单元 | [[02-工作负载/01-核心工作负载/11-pod-lifecycle-events.md|11
  pod lifecycle events]] | [[19-故障诊断/06-FTA故障树/list/pod-fta.md...'
category: general
tags:
- k8s
- etcd
- hpa
- statefulset
- daemonset
- job
- cronjob
- ingress
- gateway
- rbac
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG API → 文档映射 是什么
- 如何 KUDIG API → 文档映射
trigger_keywords:
- KUDIG
- API
- 文档映射
prerequisites:
- kubectl-basics
- etcd-basics
- tls-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: KUDIG API → 文档映射
description: KUDIG API → 文档映射
category: docs
tags:
- k8s
- api
- mapping
relationships:
- target: "[[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]]"
  type: related_to
- target: "[[26-技能/04-工作负载/pod/方法论/Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]"
  type: related_to
- target: "[[22-概念/08-可靠性与运维/Symptom-SOP-RootCause Mapping.md|Symptom-SOP-RootCause Mapping]]"
  type: related_to
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- DevOps
estimated_read_time: 10min
last_updated: 2026-05
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'

tier: peripheral
---
# KUDIG API → 文档映射

> 创建时间: 2026-05-20
> 用途: 为 Agent 建立 Kubernetes API 资源类型到文档的映射

---

## 核心资源类型映射

| 资源类型 | API 版本 | 用途 | 参考文档 | FTA |
|---|---|---|---|---|
| `Pod` | v1 | 最小部署单元 | [[02-工作负载/01-核心工作负载/11-pod-lifecycle-events.md|11 pod lifecycle events]] | [[19-故障诊断/06-FTA故障树/list/pod-fta.md|pod fta]] |
| `Deployment` | apps/v1 | 无状态应用 | 工作负载/02-deployment-production-patterns | [[19-故障诊断/06-FTA故障树/list/deployment-fta.md|deployment fta]] |
| `StatefulSet` | apps/v1 | 有状态应用 | [[02-工作负载/01-核心工作负载/03-statefulset-advanced-operations.md|03 statefulset advanced operations]] | [[19-故障诊断/06-FTA故障树/list/statefulset-fta.md|statefulset fta]] |
| `DaemonSet` | apps/v1 | 节点级守护进程 | [[02-工作负载/01-核心工作负载/04-daemonset-management.md|04 daemonset management]] | - |
| `Job` | batch/v1 | 一次性任务 | [[02-工作负载/01-核心工作负载/05-job-cronjob-advanced.md|05 job cronjob advanced]] | - |
| `CronJob` | batch/v1 | 定时任务 | [[02-工作负载/01-核心工作负载/05-job-cronjob-advanced.md|05 job cronjob advanced]] | - |
| `Service` | v1 | 服务发现和负载均衡 | [[05-网络/README.md|README]] | [[19-故障诊断/06-FTA故障树/list/service-fta.md|service fta]] |
| `Ingress` | networking.k8s.io/v1 | HTTP/HTTPS 路由 | [[05-网络/README.md|README]] | [[19-故障诊断/06-FTA故障树/list/ingress-fta.md|ingress fta]] |
| `ConfigMap` | v1 | 配置管理 | [[03-清单模式/README.md|README]] | - |
| `Secret` | v1 | 敏感信息 | [[08-安全/README.md|README]] | [[19-故障诊断/06-FTA故障树/list/rbac-fta.md|rbac fta]] |
| `PersistentVolume` | v1 | 持久化存储 | [[06-存储/README.md|README]] | [[19-故障诊断/06-FTA故障树/list/csi-fta.md|csi fta]] |
| `PersistentVolumeClaim` | v1 | 存储请求 | [[06-存储/README.md|README]] | [[19-故障诊断/06-FTA故障树/list/csi-fta.md|csi fta]] |
| `StorageClass` | storage.k8s.io/v1 | 动态存储供给 | [[06-存储/README.md|README]] | [[19-故障诊断/06-FTA故障树/list/csi-fta.md|csi fta]] |
| `Namespace` | v1 | 资源隔离 | [[08-安全/README.md|README]] | - |
| `Role/RoleBinding` | rbac.authorization.k8s.io/v1 | RBAC 权限 | [[08-安全/README.md|README]] | - |
| `ClusterRole` | rbac.authorization.k8s.io/v1 | 集群级权限 | [[08-安全/README.md|README]] | - |
| `ServiceAccount` | v1 | 服务身份 | [[08-安全/README.md|README]] | - |
| `NetworkPolicy` | networking.k8s.io/v1 | 网络隔离 | [[05-网络/README.md|README]] | [[19-故障诊断/06-FTA故障树/list/networkpolicy-fta.md|networkpolicy fta]] |
| `HorizontalPodAutoscaler` | autoscaling/v2 | 水平自动伸缩 | 工作负载/02-deployment-production-patterns | [[19-故障诊断/06-FTA故障树/list/hpa-fta.md|hpa fta]] |
| `CustomResourceDefinition` | apiextensions.k8s.io/v1 | 自定义资源 | [[16-专项技术/README.md|README]] | [[19-故障诊断/06-FTA故障树/list/crd-operator-fta.md|crd operator fta]] |

## 控制平面资源

| 资源类型 | 用途 | 参考文档 | FTA |
|---|---|---|---|
| `etcd` (非 K8s 资源) | 集群数据存储 | 集群基础/11-etcd-deep-dive | [[19-故障诊断/06-FTA故障树/list/etcd-fta.md|etcd fta]] |
| `APIService` | API 聚合 | [[16-专项技术/README.md|README]] | - |
| `ValidatingWebhookConfiguration` | 准入验证 | [[08-安全/README.md|README]] | [[19-故障诊断/06-FTA故障树/list/webhook-admission-fta.md|webhook admission fta]] |
| `MutatingWebhookConfiguration` | 准入变异 | [[08-安全/README.md|README]] | [[19-故障诊断/06-FTA故障树/list/webhook-admission-fta.md|webhook admission fta]] |

## 扩展资源

| 资源类型 | 用途 | 参考文档 |
|---|---|---|
| `Gateway` (Gateway API) | 下一代网关 | [[05-网络/README.md|README]] |
| `HTTPRoute` | HTTP 路由 | [[05-网络/README.md|README]] |
| `Certificate` (cert-manager) | TLS 证书管理 | [[08-安全/README.md|README]] |
| `Issuer` (cert-manager) | 证书签发者 | [[08-安全/README.md|README]] |

---

*本文档是 API 映射的权威来源，新增资源类型时应注册。*

---

## Related

- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]]
- [[26-技能/04-工作负载/pod/方法论/Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]
- [[22-概念/08-可靠性与运维/Symptom-SOP-RootCause Mapping.md|Symptom-SOP-RootCause Mapping]]


<!-- risk-assessed -->
