---
title: 节点清理机制 — cleanup-node 源码分析 (topic-code-analysis)
description: 'description: 深入分析 kubeadm reset cleanup-node 阶段的源码实现，涵盖停止 kubelet 服务、卸载挂载点、移除容器、清理配置目录（pki/manifests/kubelet）、删除'
summary: 'description: 深入分析 kubeadm reset cleanup-node 阶段的源码实现，涵盖停止 kubelet 服务、卸载挂载点、移除容器、清理配置目录（pki/manifests/kubelet）、删除'
category: general
tags:
- reference
- etcd
- kubelet
- scheduler
- controller-manager
- containerd
- cri-o
- docker
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 节点清理机制 — cleanup-node 源码分析 是什么
- 如何 节点清理机制 — cleanup-node 源码分析
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 节点清理机制
- cleanup-node
- 源码分析
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- etcd-basics
---

> **内容已并入主场**：[[wiki/10-平台工程/06-代码分析/cluster-delete/04-cleanup.md|节点清理机制 — cleanup-node 源码分析 (topic-code-analysis)]]（本页为重排去重桩，正文见主场）
