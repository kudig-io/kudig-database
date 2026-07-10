---
title: Kured (KUbernetes REboot Daemon)
description: '## 概述'
summary: 'Kured (KUbernetes REboot Daemon) 是一个 Kubernetes 守护进程，用于在节点需要重启时安全地执行重启操作。它检测节点上的重启信号 (如 /var/run/reboot-required 文件)，协调节点重启以避免同时重启多个节点，并在重启前正确驱逐工作负载。'
category: entities
tags:
- k8s
- cncf
- orchestration
- kured
- prometheus
- grafana
- coredns
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kured (KUbernetes REboot Daemon) 是什么
- 如何 Kured (KUbernetes REboot Daemon)
trigger_keywords:
- Kured
- KUbernetes
- REboot
- Daemon
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Kured|Kured]] (KUbernetes REboot Daemon)

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Kured (KUbernetes REboot Daemon) 是一个 Kubernetes 守护进程，用于在节点需要重启时安全地执行重启操作。它检测节点上的重启信号 (如 /var/run/reboot-required 文件)，协调节点重启以避免同时重启多个节点，并在重启前正确驱逐工作负载。

## 核心能力

- **自动检测**: 检测系统重启需求信号
- **协调重启**: 一次只重启一个节点，避免服务中断
- **Cordon/Drain**: 重启前自动隔离和驱逐 Pod
- **时间窗口**: 支持配置允许重启的时间窗口
- **Prometheus 集成**: 暴露指标供监控
- **通知集成**: 支持 Slack、Teams 等通知

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **时间窗口**: 配置业务低峰期的重启窗口
- **告警集成**: 使用 Prometheus 告警阻止关键时期重启
- **通知配置**: 启用 Slack/Teams 通知及时了解状态
- **Pod 保护**: 使用 PodDisruptionBudget 保护关键应用
- **锁超时**: 设置合理的 lock-ttl 防止死锁
- **控制平面**: 谨慎处理控制平面节点的重启

## 架构定位

在 CNCF 生态中，kured 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[pod-lifecycle]]
- [[concepts/security-defense-depth.md|security-defense-depth]]

## Related

- [[notary-project]] — Notary Project
- [[coredns]] — CoreDNS
- [[contour]] — Contour
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kured
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
