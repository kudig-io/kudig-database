---
title: 29 - 原地 Pod 资源调整 (In-Place Pod Resize)
description: 1. [特性概述](#1-特性概述)
summary: 1. [特性概述](#1-特性概述)
category: control-plane
tags:
- k8s
- control-plane
- etcd
- apiserver
- scheduler
- controller-manager
- kubelet
- prometheus
- containerd
- cri-o
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 原地 Pod 资源调整 (In-Place Pod Resize) 是什么
- 如何 原地 Pod 资源调整 (In-Place Pod Resize)
- Kubernetes 3 control plane 最佳实践
trigger_keywords:
- 原地
- Pod
- 资源调整
- In-Place
- Pod
- Resize
- control
- plane
prerequisites:
- kubectl-basics
- kubernetes-concepts
- prometheus-basics
- gpu-scheduling-basics
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
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../工作负载/
  label: '相关知识域: 工作负载'
- type: domain
  path: ../网络/
  label: '相关知识域: 网络'
- type: domain
  path: ../存储/
  label: '相关知识域: 存储'
- type: domain
  path: ../安全/
  label: '相关知识域: 安全'
- type: fta
  path: ../故障诊断/FTA故障树/list/pod-fta.md
  label: '故障树: pod'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
---

> **内容已并入主场**：[[wiki/01-集群基础/03-控制平面/30-in-place-pod-resize.md|29 - 原地 Pod 资源调整 (In-Place Pod Resize)]]（本页为重排去重桩，正文见主场）
