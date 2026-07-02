---
title: Flagger
description: Flagger 是 Weaveworks 开源的渐进式发布工具，自动化金丝雀发布、A/B 测试和蓝绿部署。它集成 Prometheus、Istio、Linker...
summary: Flagger 是 Weaveworks 开源的渐进式发布工具，自动化金丝雀发布、A/B 测试和蓝绿部署。它集成 Prometheus、Istio、Linker...
category: dictionary
tags:
- k8s
- glossary
- flagger
- canary
- progressive-delivery
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Flagger 是什么
- Flagger 详解
trigger_keywords:
- Flagger
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Flagger

> **英文名**: Flagger

## 概述

Flagger 是 Weaveworks 开源的渐进式发布工具，自动化金丝雀发布、A/B 测试和蓝绿部署。它集成 Prometheus、Istio、Linkerd 等，基于指标分析自动推进或回滚发布。

## 核心概念/原理

### 核心概念

- **Canary**：渐进式发布的 CRD 定义。
- **Traffic Management**：通过 Istio/Linkerd/Nginx 控制流量比例。
- **Metrics Analysis**：基于 Prometheus 指标自动判断发布是否健康。
- **Webhooks**：自定义的准入/通知/确认钩子。

### 金丝雀流程

```
1% 流量 → 指标分析 → 5% → 分析 → 10% → ... → 100%（完成）
                                ↓ 异常
                          自动回滚到 0%
```

## 关键机制或特性

- **自动推进**：根据错误率和延迟自动增加流量比例。
- **自动回滚**：指标超过阈值时自动回滚。
- **多种流量管理**：支持 Istio、Linkerd、Nginx、Traefik、Gateway API。
- **A/B Testing**：基于 Header 的流量路由。
- 支持 Slack/Teams/Discord 通知。

## 使用场景与最佳实践

- 配合 Istio/Linkerd 使用 Flagger 实现自动化金丝雀发布。
- 定义关键的 SLI 指标（错误率、延迟）作为发布门禁。
- 配置合理的 step 和 interval 控制发布速度。
- 使用 Webhook 集成手动确认步骤。
- 配合 Argo CD 实现 GitOps + 渐进式发布的完整流水线。

## 参考链接

- [Flagger Official](https://flagger.app/)

## Related

- [[domain-17-system-foundation/topic-dictionary/operations/argo.md|Argo]]
- [[domain-17-system-foundation/topic-dictionary/operations/rolling-update.md|Rolling Update]]
- [[domain-17-system-foundation/topic-dictionary/networking/istio.md|Istio]]
- [[domain-17-system-foundation/topic-dictionary/networking/linkerd.md|Linkerd]]
- [[domain-17-system-foundation/topic-dictionary/observability/prometheus.md|Prometheus]]


<!-- risk-assessed -->
