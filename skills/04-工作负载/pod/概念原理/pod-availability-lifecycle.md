---
title: Pod 可用性与生命周期生产模式
description: 生产级 Pod 可用性保障：探针设计、PDB、优雅终止、就绪门控与滚动更新策略
summary: 生产级 Pod 可用性保障：探针设计、PDB、优雅终止、就绪门控与滚动更新策略，含排障速查与可落地清单。
category: application-patterns
tags:
- pod
- availability
- probes
- pdb
- lifecycle
- production
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 应用开发者
estimated_read_time: 18min
intent_queries:
- Pod 可用性生产模式是什么
- 如何设计生产级探针与 PDB
trigger_keywords:
- 探针
- PDB
- 优雅终止
- 滚动更新
- 可用性
prerequisites:
- kubectl-basics
- pod-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **内容已并入主场**：[[wiki/04-应用模式/03-生产模式/pod-availability-lifecycle.md|Pod 可用性与生命周期生产模式]]（本页为重排去重桩，正文见主场）
