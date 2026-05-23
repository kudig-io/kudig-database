---
title: Kagent
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- platform
- kagent
- prometheus
- grafana
- rbac
- crd
- operator
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kagent 是什么
- 如何 Kagent
trigger_keywords:
- Kagent
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
created: "2026-05-23"
---

# Kagent

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Python, Go

## 概述

Kagent 是一个 Kubernetes 原生的 AI Agent 平台，使开发者能够在 Kubernetes 上构建、部署和管理 AI Agent。它基于 AutoGen 框架，通过 CRD 声明式定义 AI Agent 的工具集、模型配置和对话流程，将 AI Agent 作为 Kubernetes 资源进行管理。Kagent 内置了丰富的 Kubernetes 运维工具，使 AI Ag...

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **最小权限**: 仅赋予 Agent 必要的工具和 RBAC 权限，避免操作过度
- **只读优先**: 排查类 Agent 默认只授予只读工具（get/describe/logs）
- **审计日志**: 启用 Agent 操作审计，记录所有工具调用和结果
- **模型温度**: 运维类 Agent 使用低 temperature (0.1)，确保输出稳定可靠
- **人工确认**: 对于删除、重启等破坏性操作，配置需要人工确认的审批流

## 架构定位

在 CNCF 生态中，kagent 属于 **Platform** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana|prometheus-grafana]]
- [[entities/crd-custom-resources|crd-custom-resources]]
- [[operator-pattern]]
- [[concepts/secrets-management|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[volcano]] — Volcano
- [[bpfman]] — bpfman
- [[in-toto]] — in-toto
- [[grpc]] — gRPC
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kagent
- [[entities/cncf-edge-ai|[[CNCF 边缘计算与 AI/ML 项目全景|CNCF 边缘计算与 AI/ML 项目全景]]]] — Cross-reference
- index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
