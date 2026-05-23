---
title: K8sGPT (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- platform
- k8sgpt
- prometheus
- grafana
- job
- cronjob
- ingress
- networkpolicy
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8sGPT 是什么
- 如何 K8sGPT
trigger_keywords:
- K8sGPT
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
created: "2026-05-23"
---

# K8sGPT

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Go

## 概述

K8sGPT 是一款 AI 驱动的 Kubernetes 诊断工具，利用大语言模型 (LLM) 自动分析集群问题并提供人类可读的解释和建议。它扫描 Kubernetes 集群中的问题，结合 AI 能力生成诊断报告，帮助 SRE 和开发者快速定位和解决问题。

## 核心能力

- **多 LLM 支持**: OpenAI、Azure OpenAI、Anthropic、LocalAI、Ollama
- **问题扫描**: 自动检测 Pod、[[Service|Service]]、[[Ingress|Ingress]] 等资源问题
- **AI 解释**: 使用 LLM 生成问题原因和解决方案
- **多语言输出**: 支持中文、英文等多种语言
- **Operator 模式**: 作为 Kubernetes Operator 持续监控
- **自定义分析器**: 扩展内置分析能力

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **后端选择**: 生产环境建议使用 GPT-4 或本地部署的模型
- **缓存策略**: 启用缓存减少 API 调用成本
- **过滤优化**: 根据需要启用特定分析器
- **定时运行**: 配合 CronJob 定期生成诊断报告
- **敏感信息**: 注意 API Key 安全存储
- **成本控制**: 监控 AI API 调用量

## 架构定位

在 CNCF 生态中，k8sgpt 属于 **Platform** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana|prometheus-grafana]]
- [[entities/trivy|trivy]]
- [[entities/networkpolicy|networkpolicy]]
- [[deployment]]
- [[entities/crd-custom-resources|crd-custom-resources]]

## Related

- [[spire]] — SPIRE
- [[akri]] — Akri
- [[entities/cncf-edge-ai|cncf-edge-ai]] — CNCF 边缘计算与 AI/ML 项目全景
- [[confidential-containers]] — Confidential Containers (CoCo)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- k8sgpt
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
