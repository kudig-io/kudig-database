---
title: Pod 生命周期事件表
description: 深入解析 Pod 生命周期状态转换、Phase 与 Condition 机制、Init Container、Sidecar、容器重启策略与事件体系
summary: 深入解析 Pod 生命周期状态转换、Phase 与 Condition 机制、Init Container、Sidecar、容器重启策略与事件体系
category: 工作负载
tags:
- k8s
- pod
- lifecycle
- phase
- condition
- init-container
- sidecar
- container
- pdb
- job
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Pod 生命周期事件表 是什么
- 如何 Pod 生命周期事件表
- Kubernetes 4 workloads 最佳实践
trigger_keywords:
- Pod
- 生命周期事件表
- workloads
prerequisites:
- kubectl-basics
- pod-lifecycle
k8s_versions:
- '1.25'
- '1.26'
- '1.27'
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: fta
  path: ../故障诊断/FTA故障树/list/pod-fta.md
  label: '故障树: pod'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
related_docs:
- path: 10-workload-controllers-overview.md
  type: depth
  desc: 工作负载控制器详解
- path: ../工作负载/21-hpa-vpa-autoscaling.md
  type: depth
  desc: HPA/VPA 自动扩缩容
- path: ../故障诊断/FTA故障树/list/pod-fta.md
  type: fta
  desc: Pod 故障树
---

> **内容已并入主场**：[[wiki/02-工作负载/01-核心工作负载/11-pod-lifecycle-events.md|Pod 生命周期事件表]]（本页为重排去重桩，正文见主场）
