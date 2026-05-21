---
title: KUDIG API → 文档映射
description: '| `Pod` | v1 | 最小部署单元 | [[domain-02-workloads-applications/00-core-workloads/11-pod-lifecycle-events.md]] |
  [[domain-10-troubleshooting-diagnostics/topic-fta/list/pod-fta.md]] |'
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

---
title: KUDIG API → 文档映射
description: KUDIG API → 文档映射
category: docs
tags:
- k8s
- api
- mapping
relationships:
- target: '[[skills/FTA Diagnostic Execution Engine|FTA Diagnostic Execution Engine]]'
  type: related_to
- target: '[[skills/Kubernetes FTA Top Events Index|Kubernetes FTA Top Events Index]]'
  type: related_to
- target: '[[concepts/Symptom-SOP-RootCause Mapping|Symptom-SOP-RootCause Mapping]]'
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

tier: peripheral---

# KUDIG API → 文档映射

> 创建时间: 2026-05-20
> 用途: 为 Agent 建立 Kubernetes API 资源类型到文档的映射

---

## 核心资源类型映射

| 资源类型 | API 版本 | 用途 | 参考文档 | FTA |
|---|---|---|---|---|
| `Pod` | v1 | 最小部署单元 | [[domain-02-workloads-applications/00-core-workloads/11-pod-lifecycle-events.md]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/pod-fta.md]] |
| `Deployment` | apps/v1 | 无状态应用 | [[domain-02-workloads-applications/02-deployment-production-patterns.md]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/deployment-fta.md]] |
| `StatefulSet` | apps/v1 | 有状态应用 | [[domain-02-workloads-applications/00-core-workloads/03-statefulset-advanced-operations.md]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/statefulset-fta.md]] |
| `DaemonSet` | apps/v1 | 节点级守护进程 | [[domain-02-workloads-applications/00-core-workloads/04-daemonset-management.md]] | - |
| `Job` | batch/v1 | 一次性任务 | [[domain-02-workloads-applications/00-core-workloads/05-job-cronjob-advanced.md]] | - |
| `CronJob` | batch/v1 | 定时任务 | [[domain-02-workloads-applications/00-core-workloads/05-job-cronjob-advanced.md]] | - |
| `Service` | v1 | 服务发现和负载均衡 | [[domain-03-networking-traffic/README.md]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/service-fta.md]] |
| `Ingress` | networking.k8s.io/v1 | HTTP/HTTPS 路由 | [[domain-03-networking-traffic/README.md]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/ingress-fta.md]] |
| `ConfigMap` | v1 | 配置管理 | [[domain-18-manifests-patterns/README.md]] | - |
| `Secret` | v1 | 敏感信息 | [[domain-05-security-compliance/README.md]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/rbac-fta.md]] |
| `PersistentVolume` | v1 | 持久化存储 | [[domain-04-storage-data/README.md]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/csi-fta.md]] |
| `PersistentVolumeClaim` | v1 | 存储请求 | [[domain-04-storage-data/README.md]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/csi-fta.md]] |
| `StorageClass` | storage.k8s.io/v1 | 动态存储供给 | [[domain-04-storage-data/README.md]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/csi-fta.md]] |
| `Namespace` | v1 | 资源隔离 | [[domain-05-security-compliance/README.md]] | - |
| `Role/RoleBinding` | rbac.authorization.k8s.io/v1 | RBAC 权限 | [[domain-05-security-compliance/README.md]] | - |
| `ClusterRole` | rbac.authorization.k8s.io/v1 | 集群级权限 | [[domain-05-security-compliance/README.md]] | - |
| `ServiceAccount` | v1 | 服务身份 | [[domain-05-security-compliance/README.md]] | - |
| `NetworkPolicy` | networking.k8s.io/v1 | 网络隔离 | [[domain-03-networking-traffic/README.md]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/networkpolicy-fta.md]] |
| `HorizontalPodAutoscaler` | autoscaling/v2 | 水平自动伸缩 | [[domain-02-workloads-applications/02-deployment-production-patterns.md]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/hpa-fta.md]] |
| `CustomResourceDefinition` | apiextensions.k8s.io/v1 | 自定义资源 | [[domain-15-specialized-tech/README.md]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/crd-operator-fta.md]] |

## 控制平面资源

| 资源类型 | 用途 | 参考文档 | FTA |
|---|---|---|---|
| `etcd` (非 K8s 资源) | 集群数据存储 | [[domain-01-cluster-fundamentals/11-etcd-deep-dive.md]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/etcd-fta.md]] |
| `APIService` | API 聚合 | [[domain-15-specialized-tech/README.md]] | - |
| `ValidatingWebhookConfiguration` | 准入验证 | [[domain-05-security-compliance/README.md]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/webhook-admission-fta.md]] |
| `MutatingWebhookConfiguration` | 准入变异 | [[domain-05-security-compliance/README.md]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/webhook-admission-fta.md]] |

## 扩展资源

| 资源类型 | 用途 | 参考文档 |
|---|---|---|
| `Gateway` (Gateway API) | 下一代网关 | [[domain-03-networking-traffic/README.md]] |
| `HTTPRoute` | HTTP 路由 | [[domain-03-networking-traffic/README.md]] |
| `Certificate` (cert-manager) | TLS 证书管理 | [[domain-05-security-compliance/README.md]] |
| `Issuer` (cert-manager) | 证书签发者 | [[domain-05-security-compliance/README.md]] |

---

*本文档是 API 映射的权威来源，新增资源类型时应注册。*

---

## Related

- [[skills/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]]
- [[skills/Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]
- [[concepts/Symptom-SOP-RootCause Mapping.md|Symptom-SOP-RootCause Mapping]]
