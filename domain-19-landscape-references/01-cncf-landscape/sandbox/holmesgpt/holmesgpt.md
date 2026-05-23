---
title: HolmesGPT [entities]
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- platform
- holmesgpt
- prometheus
- grafana
- helm
- rbac
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- HolmesGPT 是什么
- 如何 HolmesGPT
trigger_keywords:
- HolmesGPT
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
created: "2026-05-23"
---

# HolmesGPT

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Python

## 概述

HolmesGPT 是一个基于大语言模型（LLM）的 Kubernetes 故障排查助手，能够自动分析集群告警和事件，执行运维调查流程，提供根因分析（RCA）和修复建议。它将 AI 推理能力与 Kubernetes 原生工具（kubectl、Helm 等）结合，实现从告警到根因定位的自动化排查闭环。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **RBAC 最小权限**: 生产环境中限制 Holmes 的 ServiceAccount 权限，仅授予只读权限
- **Runbook 积累**: 将团队运维经验编写为 Runbook，提高排查准确率
- **LLM 选择**: 复杂排查场景使用 GPT-4 级别模型，简单场景可用轻量模型降低成本
- **敏感数据**: 注意 LLM 调用会将集群信息发送到外部 API，确保不泄露敏感数据
- **Slack 集成**: 将 Holmes 分析结果推送到告警频道，加速 On-Call 响应

## 架构定位

在 CNCF 生态中，holmesgpt 属于 **Platform** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[concepts/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[kubeelasti]] — [[entities/kubeelasti.md|KubeElastic]]
- [[xregistry]] — xRegistry
- [[carvel]] — Carvel
- [[helm]] — Helm
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- holmesgpt
- observability|CNCF 可观测性项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/ai-gpu-index|AI / GPU 基础设施知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
