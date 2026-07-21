---
title: SLO & SLI Engineering
description: SLO/SLI 知识域 — 服务等级目标设计、错误预算策略、SLI 实现方法论、SLO 驱动运维
category: subdomain
tags:
- slo
- sli
- error-budget
- reliability
- sre
tier: core
created: '2026-07-02'
last_updated: '2026-07-21'
---
# SLO & SLI 工程实践

> 以 SLO 为核心的可靠性工程体系，用错误预算驱动发布决策。

## 核心概念

| 概念 | 定义 | 典型目标 |
|------|------|----------|
| SLI | 服务等级指标，量化服务质量的度量 | 延迟 P99 < 200ms |
| SLO | 服务等级目标，SLI 的目标阈值 | 可用性 ≥ 99.9% |
| SLA | 服务等级协议，含违约赔偿条款 | 合同约束 |
| 错误预算 | 1 - SLO，允许的最大不可靠度 | 99.9% → 43.2min/月 |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[可观测性/SLO-SLI/01-slo-engineering-practice.md\|SLO 工程实践]] | SLO 设计方法论与落地 | intermediate |
| [[可观测性/SLO-SLI/02-error-budget-policy.md\|错误预算策略]] | 预算耗尽时的发布冻结机制 | advanced |
| [[可观测性/SLO-SLI/03-sli-implementation-guide.md\|SLI 实现指南]] | 从指标到 SLI 的技术实现 | intermediate |
| [[可观测性/SLO-SLI/18-slo-sli-system.md\|SLO/SLI 系统]] | 企业级 SLO 平台架构 | advanced |

## SLO 设计检查清单

- [ ] 识别关键用户旅程（Critical User Journeys）
- [ ] 为每个 CUJ 定义 SLI（可用性/延迟/吞吐）
- [ ] 设定合理 SLO（非 100%，平衡可靠性与迭代速度）
- [ ] 建立错误预算消耗告警（多窗口多燃烧率）
- [ ] 制定预算耗尽策略（冻结发布/回滚/降级）
- [ ] 定期 SLO Review（季度回顾调整目标）

## Related

- [[可观测性/指标/index.md|指标 Metrics]]
- [[可观测性/告警/index.md|告警 Alerting]]
- [[可靠性/index.md|可靠性 Reliability]]
