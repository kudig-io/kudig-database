---
title: Progressive Delivery
description: 渐进式交付知识域 — Argo Rollouts、Flagger、金丝雀分析、蓝绿部署、A/B 测试、流量镜像
summary: 渐进式交付子目录索引，涵盖 Argo Rollouts 控制器、Flagger 自动化金丝雀、金丝雀分析指标、流量管理策略
category: subdomain
tags:
- progressive-delivery
- canary
- argo-rollouts
- flagger
- blue-green
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
---
# 渐进式交付 Progressive Delivery

> 通过自动化金丝雀分析、蓝绿部署、A/B 测试降低发布风险。

## 核心概念

渐进式交付（Progressive Delivery）是持续交付的演进，核心思想：
- **渐进式流量转移**：逐步将流量从旧版本转移到新版本
- **自动化指标分析**：基于 Prometheus/Datadog 指标自动判断发布健康度
- **自动回滚**：指标异常时自动回滚，无需人工干预

## 文件索引

| 文件 | 内容 | 难度 |
|------|------|------|
| [[发布变更/Progressive-Delivery/01-argo-rollouts-deep-dive.md\|01-argo-rollouts-deep-dive]] | Argo Rollouts 架构与生产实践 | advanced |
| [[发布变更/Progressive-Delivery/02-canary-analysis-patterns.md\|02-canary-analysis-patterns]] | 金丝雀分析模式与指标设计 | advanced |

## 工具对比

| 工具 | 类型 | 流量管理 | 指标分析 | 适用场景 |
|------|------|----------|----------|----------|
| Argo Rollouts | K8s Controller | Istio/Nginx/ALB | Prometheus/Datadog/CloudWatch | 通用 K8s |
| Flagger | K8s Operator | Istio/Linkerd/App Mesh | Prometheus/Datadog | Service Mesh 环境 |
| Dapr | 运行时 Sidecar | Dapr Traffic | 自定义 | 多语言微服务 |

## Related

- [[发布变更/GitOps/09-argo-rollouts-progressive-delivery.md|Argo Rollouts 渐进式交付]]
- [[发布变更/GitOps/11-flagger-automated-canary.md|Flagger 自动化金丝雀]]
- [[发布变更/部署方案/index.md|部署方案]]
