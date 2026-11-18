---
title: Terway CRD 资源操作
description: '## 概述'
summary: 'Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]'
category: entities
tags:
- k8s
- networking
- terway
- cni
- alicloud
- cilium
- networkpolicy
- crd
- ebpf
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Terway CRD 资源操作 是什么
- 如何 Terway CRD 资源操作
trigger_keywords:
- Terway
- CRD
- 资源操作
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
---

> **内容已并入主场**：[[wiki/05-网络/06-Terway/03b-crd-operations.md|03b - Terway CRD 深度操作指南 (CRD Operations Deep Dive)]]（本页为重排去重桩，正文见主场）
