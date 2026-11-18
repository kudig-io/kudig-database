---
title: Kubernetes 网络安全最佳实践 [security]
description: 生产环境 Kubernetes 网络安全配置的最佳实践指南
summary: 生产环境 Kubernetes 网络安全配置的最佳实践指南
category: best-practices/security
tags:
- kubernetes
- security
- network-policy
- mtls
- service-mesh
- istio
- cilium
- calico
- ingress
- networkpolicy
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- 网络工程师
- SRE
estimated_read_time: 20min
intent_queries:
- Kubernetes 网络安全 最佳实践
- 如何 配置 Kubernetes 网络策略
- Kubernetes 服务网格 安全
trigger_keywords:
- Kubernetes
- 网络安全
- 网络策略
- 服务网格
prerequisites:
- kubectl-basics
- service-mesh-basics
- ebpf-basics
- cilium-basics
- cni-basics
cross_refs:
- type: domain
  path: ../../网络/
  label: 网络知识域
- type: domain
  path: ../../网络/
  label: 服务网格知识域
- type: best-practice
  path: ./pod-security.md
  label: Pod安全最佳实践
---

> **内容已并入主场**：[[wiki/20-最佳实践/08-security/network-security.md|Kubernetes 网络安全最佳实践 [security]]]（本页为重排去重桩，正文见主场）
