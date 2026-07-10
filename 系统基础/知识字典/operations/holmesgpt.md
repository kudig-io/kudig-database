---
title: HolmesGPT AI 排障
description: HolmesGPT 是 Robusta 开源的 AI 辅助 Kubernetes 排障工具，利用 LLM 分析告警和日志，自动生成故障诊断报告和修复建议，是
  K...
summary: HolmesGPT 是 Robusta 开源的 AI 辅助 Kubernetes 排障工具，利用 LLM 分析告警和日志，自动生成故障诊断报告和修复建议，是
  K...
category: dictionary
tags:
- k8s
- glossary
- operations
- ai
- diagnostics
tier: supporting
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- HolmesGPT AI 排障 是什么
- HolmesGPT 详解
trigger_keywords:
- HolmesGPT AI 排障
- HolmesGPT
- dictionary
prerequisites:
- kubernetes
---



# HolmesGPT AI 排障（HolmesGPT）

## 概述

HolmesGPT 是 Robusta 开源的 AI 辅助 Kubernetes 排障工具，利用 LLM 分析告警和日志，自动生成故障诊断报告和修复建议，是 K8sGPT 的增强替代方案。

## 核心概念/原理

- **AI 排障**：利用 LLM 分析告警/日志/指标
- **多数据源**：集成 Prometheus/Grafana/Loki/Elasticsearch
- **Robusta 出品**：K8s 可观测性平台团队
- **自动诊断**：告警触发后自动分析根因

## 关键机制或特性

- 告警自动分析（Alert → Root Cause）
- 多数据源集成（Prometheus/Loki/ES）
- Runbook 自动执行
- 多 LLM 后端（OpenAI/Azure/Local）
- Slack/Teams 集成
- 历史事件学习
- 修复建议生成

## 使用场景与最佳实践

- On-Call 告警的快速诊断
- 复杂故障的 AI 辅助分析
- Runbook 的自动化执行
- 运维团队的 AI 助手
- 事件管理的效率提升

## 参考链接

- https://github.com/robusta-dev/holmesgpt
- https://home.robusta.dev/

## Related

- [[系统基础/知识字典/operations/k8sgpt.md|K8sGPT]]
- [[系统基础/知识字典/observability/prometheus.md|Prometheus]]
- [[系统基础/知识字典/observability/loki.md|Loki]]
