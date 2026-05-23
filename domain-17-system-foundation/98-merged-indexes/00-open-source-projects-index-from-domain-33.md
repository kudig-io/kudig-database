---
title: Domain-33 K8s 事件 — 开源项目索引
description: '| **EventRouter** | 事件路由到日志系统 | Heptio/VMware | v1.0.0 | 1k+ | Apache-2.0 |'
category: kubernetes-events
tags:
- k8s
- events
- troubleshooting
- prometheus
- opa
- falco
- statefulset
- daemonset
- job
- cronjob
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- Domain-33 K8s 事件 — 开源项目索引 是什么
- 如何 Domain-33 K8s 事件 — 开源项目索引
- Kubernetes 33 kubernetes events 最佳实践
trigger_keywords:
- Domain-33
- K8s
- 事件
- 开源项目索引
- kubernetes
- events
prerequisites:
- kubectl-basics
- cloud-provider-basics
- prometheus-basics
- policy-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
created: "2026-05-23"
---

# Domain-33 K8s 事件 — 开源项目索引

> **最后更新**: 2026-04-24

---

## 核心项目

| 项目 | 作用 | 归属 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Kubernetes Events** | 原生事件系统 | K8s 核心 | v1.33.0 | - | Apache-2.0 |
| **Event Exporter** | 事件导出到各种后端 | OpsGenie | v1.0.0 | 1k+ | Apache-2.0 |
| **kube-eventer** | K8s 事件收集与告警 | 阿里云 | v1.2.0 | 1k+ | Apache-2.0 |
| **Sloop** | K8s 历史状态与事件查看 | Salesforce | v1.0.0 | 2k+ | Apache-2.0 |
| **kubectl-event** | 增强事件查看插件 | 社区 | v0.1.0 | 200+ | Apache-2.0 |
| **EventRouter** | 事件路由到日志系统 | Heptio/VMware | v1.0.0 | 1k+ | Apache-2.0 |
| **Kubernetes Event Exporter** | 通用事件导出 | Resmo | v1.7.0 | 1.5k+ | Apache-2.0 |
| **Prometheus K8s Event Exporter** | 事件转 Prometheus 指标 | 社区 | v1.0.0 | 300+ | Apache-2.0 |
| **Falco** | 安全事件检测 | CNCF Graduated | v0.41.0 | 7.5k+ | Apache-2.0 |
| **Kyverno** | 策略违规事件 | CNCF Graduated | v1.14.0 | 5.5k+ | Apache-2.0 |
| **Policy Reporter** | Kyverno/OPA 策略结果展示 | Kyverno | v3.0.0 | 500+ | Apache-2.0 |
| **Komodor** | K8s 变更追踪与事件关联 | Komodor | SaaS | - | 商业 |

---

## 参考链接

- [K8s 事件文档](https://kubernetes.io/docs/reference/kubernetes-api/cluster-resources/event-v1/)
- [Sloop GitHub](https://github.com/salesforce/sloop)
- [Kubernetes Event Exporter](https://github.com/resmoio/kubernetes-event-exporter)

---

## Obsidian 相关文档

- domain-33-kubernetes-events MOC
- [[domain-17-system-foundation/README|Domain-33: Kubernetes Events 全域事件大全]]
- 01 - Kubernetes 事件系统架构与 API 参考
- 02 - Pod 与容器生命周期事件
- 03 - 镜像拉取事件
- 04 - 探针与健康检查事件
- 05 - 调度与抢占事件
- 06 - 节点生命周期与状态事件
- 07 - Deployment 与 ReplicaSet 控制器事件
- 08 - StatefulSet 与 DaemonSet 控制器事件
- 09 - Job 与 CronJob 批处理事件
- 10 - Service 与网络事件
