---
title: 节点删除流程 — kubectl delete node 源码分析 (topic-code-analysis)
description: '## 概述'
summary: 'Node 对象、Node Lifecycle Controller 响应、kubeadm reset 重置、Taint Manager 驱逐以及 etcd 成员移除等关键环节。'
category: general
tags:
- reference
- etcd
- apiserver
- kubelet
- flannel
- containerd
- docker
- statefulset
- daemonset
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 节点删除流程 — kubectl delete node 源码分析 是什么
- 如何 节点删除流程 — kubectl delete node 源码分析
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 节点删除流程
- kubectl
- delete
- node
- 源码分析
- platform
- engineering
- code
prerequisites:
- kubectl-basics
- platform-engineering-basics
- etcd-basics
---

> **内容已并入主场**：[[wiki/10-平台工程/06-代码分析/cluster-delete/03-delete-node.md|节点删除流程 — kubectl delete node 源码分析 (topic-code-analysis)]]（本页为重排去重桩，正文见主场）
