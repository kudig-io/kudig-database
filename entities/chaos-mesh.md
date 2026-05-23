---
title: Chaos Mesh [entities]
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- orchestration
- chaos-mesh
- scheduler
- crd
- operator
- ebpf
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Chaos Mesh 是什么
- 如何 Chaos Mesh
trigger_keywords:
- Chaos
- Mesh
prerequisites:
- kubectl-basics
- ebpf-basics
created: "2026-05-23"
---

# Chaos Mesh

> **CNCF 状态**: Incubating | **类别**: Orchestration | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，chaos-mesh 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/observability-pillars|observability-pillars]]
- [[pod-lifecycle]]
- [[entities/kube-scheduler|kube-scheduler]]

## Related

- [[journal/digest-2026-05-21|digest-2026-05-21]] — Wiki 全量知识库摘要 — 2026-05-21
- [[references/k8s-advanced-ecosystem|k8s-advanced-ecosystem]] — 硬件知识体系、CNCF 全景生态与 eBPF 平台工程
- [[skills/Agent Orchestration Patterns|[[Agent Orchestration Patterns for FTA|Agent Orchestration Patterns]]]] — Agent Orchestration Patterns for FTA
- observability.md|cncf-observability]] — CNCF 可观测性项目全景
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- chaos-mesh
- [[entities/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
