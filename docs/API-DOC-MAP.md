---
title: KUDIG API → 文档映射
description: '| `Pod` | v1 | 最小部署单元 | [[domain-02-workloads-applications/00-core-workloads/11-pod-lifecycle-events]] |
  [[domain-10-troubleshooting-diagnostics/topic-fta/list/pod-fta]] |'
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
created: "2026-05-23"
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
| `Pod` | v1 | 最小部署单元 | [[domain-02-workloads-applications/00-core-workloads/11-pod-lifecycle-events]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/pod-fta]] |
| `Deployment` | apps/v1 | 无状态应用 | domain-02-workloads-applications/02-deployment-production-patterns | [[domain-10-troubleshooting-diagnostics/topic-fta/list/deployment-fta]] |
| `StatefulSet` | apps/v1 | 有状态应用 | [[domain-02-workloads-applications/00-core-workloads/03-statefulset-advanced-operations]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/statefulset-fta]] |
| `DaemonSet` | apps/v1 | 节点级守护进程 | [[domain-02-workloads-applications/00-core-workloads/04-daemonset-management]] | - |
| `Job` | batch/v1 | 一次性任务 | [[domain-02-workloads-applications/00-core-workloads/05-job-cronjob-advanced]] | - |
| `CronJob` | batch/v1 | 定时任务 | [[domain-02-workloads-applications/00-core-workloads/05-job-cronjob-advanced]] | - |
| `Service` | v1 | 服务发现和负载均衡 | [[domain-03-networking-traffic/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/service-fta]] |
| `Ingress` | networking.k8s.io/v1 | HTTP/HTTPS 路由 | [[domain-03-networking-traffic/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/ingress-fta]] |
| `ConfigMap` | v1 | 配置管理 | [[domain-18-manifests-patterns/README]] | - |
| `Secret` | v1 | 敏感信息 | [[domain-05-security-compliance/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/rbac-fta]] |
| `PersistentVolume` | v1 | 持久化存储 | [[domain-04-storage-data/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/csi-fta]] |
| `PersistentVolumeClaim` | v1 | 存储请求 | [[domain-04-storage-data/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/csi-fta]] |
| `StorageClass` | storage.k8s.io/v1 | 动态存储供给 | [[domain-04-storage-data/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/csi-fta]] |
| `Namespace` | v1 | 资源隔离 | [[domain-05-security-compliance/README]] | - |
| `Role/RoleBinding` | rbac.authorization.k8s.io/v1 | RBAC 权限 | [[domain-05-security-compliance/README]] | - |
| `ClusterRole` | rbac.authorization.k8s.io/v1 | 集群级权限 | [[domain-05-security-compliance/README]] | - |
| `ServiceAccount` | v1 | 服务身份 | [[domain-05-security-compliance/README]] | - |
| `NetworkPolicy` | networking.k8s.io/v1 | 网络隔离 | [[domain-03-networking-traffic/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/networkpolicy-fta]] |
| `HorizontalPodAutoscaler` | autoscaling/v2 | 水平自动伸缩 | domain-02-workloads-applications/02-deployment-production-patterns | [[domain-10-troubleshooting-diagnostics/topic-fta/list/hpa-fta]] |
| `CustomResourceDefinition` | apiextensions.k8s.io/v1 | 自定义资源 | [[domain-15-specialized-tech/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/crd-operator-fta]] |

## 控制平面资源

| 资源类型 | 用途 | 参考文档 | FTA |
|---|---|---|---|
| `etcd` (非 K8s 资源) | 集群数据存储 | domain-01-cluster-fundamentals/11-etcd-deep-dive | [[domain-10-troubleshooting-diagnostics/topic-fta/list/etcd-fta]] |
| `APIService` | API 聚合 | [[domain-15-specialized-tech/README]] | - |
| `ValidatingWebhookConfiguration` | 准入验证 | [[domain-05-security-compliance/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/webhook-admission-fta]] |
| `MutatingWebhookConfiguration` | 准入变异 | [[domain-05-security-compliance/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/webhook-admission-fta]] |

## 扩展资源

| 资源类型 | 用途 | 参考文档 |
|---|---|---|
| `Gateway` (Gateway API) | 下一代网关 | [[domain-03-networking-traffic/README]] |
| `HTTPRoute` | HTTP 路由 | [[domain-03-networking-traffic/README]] |
| `Certificate` (cert-manager) | TLS 证书管理 | [[domain-05-security-compliance/README]] |
| `Issuer` (cert-manager) | 证书签发者 | [[domain-05-security-compliance/README]] |

---

*本文档是 API 映射的权威来源，新增资源类型时应注册。*

---

## Related

- [[skills/FTA Diagnostic Execution Engine|FTA Diagnostic Execution Engine]]
- [[skills/Kubernetes FTA Top Events Index|Kubernetes FTA Top Events Index]]
- [[concepts/Symptom-SOP-RootCause Mapping|Symptom-SOP-RootCause Mapping]]
