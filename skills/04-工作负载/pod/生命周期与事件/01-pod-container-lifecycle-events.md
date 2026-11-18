---
title: 02 - Pod 与容器生命周期事件
description: '**本文档全面覆盖 kubelet 产生的 Pod 和容器生命周期事件，包括容器创建、启动、终止、重启、沙箱管理等全流程事件，是生产环境排查
  Pod 启动失败、CrashLoopBackOff、驱逐等问题的核心参考。**'
summary: '**本文档全面覆盖 kubelet 产生的 Pod 和容器生命周期事件，包括容器创建、启动、终止、重启、沙箱管理等全流程事件，是生产环境排查 Pod
  启动失败、CrashLoopBackOff、驱逐等问题的核心参考。**'
category: kubernetes-events
tags:
- k8s
- events
- troubleshooting
- etcd
- kubelet
- scheduler
- prometheus
- cilium
- flannel
- calico
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 20min
intent_queries:
- Pod 与容器生命周期事件 是什么
- 如何 Pod 与容器生命周期事件
- Kubernetes 33 kubernetes events 最佳实践
trigger_keywords:
- Pod
- 与容器生命周期事件
- kubernetes
- events
prerequisites:
- kubectl-basics
- cloud-provider-basics
- prometheus-basics
- cilium-basics
- cni-basics
- etcd-basics
- logging-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: fta
  path: ../故障诊断/FTA故障树/list/pod-fta.md
  label: '故障树: pod'
lifecycle: reviewed
---

> **内容已并入主场**：[[wiki/17-系统基础/04-K8s事件/02-pod-container-lifecycle-events.md|02 - Pod 与容器生命周期事件]]（本页为重排去重桩，正文见主场）
