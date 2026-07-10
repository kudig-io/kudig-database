---
title: K8sGPT AI 诊断助手
description: K8sGPT 是 CNCF Sandbox 项目，利用 AI/LLM 技术自动扫描 Kubernetes 集群中的问题并提供诊断建议，将复杂的
  K8s 故障排查...
summary: K8sGPT 是 CNCF Sandbox 项目，利用 AI/LLM 技术自动扫描 Kubernetes 集群中的问题并提供诊断建议，将复杂的 K8s
  故障排查...
category: dictionary
tags:
- k8s
- glossary
- operations
- ai
- diagnostics
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8sGPT AI 诊断助手 是什么
- K8sGPT 详解
trigger_keywords:
- K8sGPT AI 诊断助手
- K8sGPT
- dictionary
prerequisites:
- kubernetes
---



# K8sGPT AI 诊断助手（K8sGPT）

## 概述

K8sGPT 是 CNCF Sandbox 项目，利用 AI/LLM 技术自动扫描 Kubernetes 集群中的问题并提供诊断建议，将复杂的 K8s 故障排查简化为自然语言交互。

## 核心概念/原理

- **AI 驱动**：集成多种 LLM 后端（OpenAI/Azure/Local）进行智能诊断
- **自动扫描**：检测集群中的异常资源和问题
- **自然语言输出**：以人类可读的方式解释问题和解决方案
- **CNCF Sandbox**：活跃的 AI+K8s 社区

## 关键机制或特性

- `k8sgpt analyze` 扫描集群问题
- Analyzer 插件架构（Pod/Service/Ingress/PVC 等分析器）
- 多 LLM 后端支持（OpenAI、Azure OpenAI、LocalAI、Amazon Bedrock）
- Filter 机制（按命名空间/类型筛选）
- 自定义 AI Provider
- 与 Prometheus/Grafana 集成可视化

## 使用场景与最佳实践

- K8s 集群的快速健康检查
- 复杂问题的 AI 辅助诊断
- 运维新手的问题排查引导
- 日常巡检中的异常检测
- 故障根因分析的第一步

## 参考链接

- https://k8sgpt.ai/
- https://github.com/k8sgpt-ai/k8sgpt

## Related

- [[系统基础/topic-dictionary/operations/chaos-engineering.md|混沌工程]]
- [[系统基础/topic-dictionary/observability/prometheus.md|Prometheus]]
- [[系统基础/topic-dictionary/operations/k8up.md|K8up]]
