---
title: SLO 定义模板
description: 服务等级目标 (SLO) 标准定义模板
summary: SLO 定义模板 — 标准化的服务等级目标定义，覆盖 SLI、告警、错误预算
category: template
tags:
- slo
- sli
- template
- sre
- reliability
- monitoring
tier: supporting
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 服务负责人
- 产品经理
estimated_read_time: 5min
intent_queries:
- SLO 模板 是什么
- 如何定义 Kubernetes 服务等级目标
- SLI SLO 定义模板
- service level objective template
trigger_keywords:
- slo
- sli
- 服务等级目标
- 错误预算
- error-budget
- 模板
prerequisites:
- monitoring-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档为模板定义，不包含可执行命令。

# SLO 定义模板

> **模板版本**: 1.0 | **适用标准**: Google SRE Workbook + 实践经验 | **使用方式**: 复制此模板，替换所有 `[PLACEHOLDER]`

## 模板使用说明

1. 每个关键服务应定义至少一个 SLO
2. SLO 应由服务负责人和业务方共同商定
3. 错误预算耗尽时，开发应停止新功能上线，优先修复可靠性问题
4. SLO 应每季度回顾，根据实际表现调整

---

## 服务等级目标: [服务名称]

> **SLO ID**: SLO-[NN]
> **服务名称**: [服务名称]
> **服务负责人**: [Team/Owner]
> **最后更新**: [YYYY-MM-DD]
> **下次回顾**: [YYYY-MM-DD]

### 1. 服务描述

[简要描述服务的功能和重要性]

**服务层级**:
- [ ] Tier 0 — 关键路径 (影响所有用户)
- [ ] Tier 1 — 重要服务 (影响部分用户)
- [ ] Tier 2 — 辅助服务 (有替代方案)
- [ ] Tier 3 — 内部工具

**用户群体**: [描述服务的主要用户]
**依赖服务**: [列出上游和下游依赖]

### 2. 服务等级指标 (SLI)

本服务定义以下 SLI:

#### SLI-1: [可用性 / Availability]

| 属性 | 值 |
|------|-----|
| **指标名称** | [如: request_availability] |
| **定义** | 成功请求数 / 总请求数 |
| **成功定义** | HTTP 状态码 < 500 |
| **测量方式** | [Prometheus / 日志分析 / 自定义] |
| **PromQL** | `sum(rate(http_requests_total{status!~"5.."}[5m])) / sum(rate(http_requests_total[5m]))` |
| **测量窗口** | 滚动 30 天 |

#### SLI-2: [延迟 / Latency]

| 属性 | 值 |
|------|-----|
| **指标名称** | [如: request_latency_p99] |
| **定义** | 请求处理延迟 P99 百分位 |
| **测量方式** | Prometheus histogram |
| **PromQL** | `histogram_quantile(0.99, sum(rate(http_request_duration_seconds_bucket[5m])) by (le))` |
| **测量窗口** | 滚动 30 天 |

#### SLI-3: [自定义指标 — 如: 数据新鲜度]

| 属性 | 值 |
|------|-----|
| **指标名称** | [如: data_freshness] |
| **定义** | [描述] |
| **PromQL** | `[query]` |

### 3. 服务等级目标 (SLO)

| SLI | SLO 目标 | 测量窗口 | 错误预算 |
|-----|---------|---------|---------|
| 可用性 | ≥ [99.9]% | 30 天 | [0.1]% = [43.2] 分钟/月 |
| 延迟 P99 | ≤ [200] ms | 30 天 | — |
| [自定义] | [目标值] | [窗口] | [计算] |

**错误预算计算**:
- 月度总分钟数: 43,200 分钟 (30 天)
- 99.9% 可用性 → 允许 [43.2] 分钟的不可用时间/月
- 当前消耗: [X] 分钟 ([X]% of budget)

### 4. 告警规则

基于错误预算消耗速率设置多级告警:

```yaml
# Prometheus Alerting Rules
groups:
- name: [service]-slo
  rules:
  # 快速消耗告警 — 2小时内消耗 2% 的月度预算
  - alert: [Service]SLOBurnRateFast
    expr: |
      (
        sum(rate(http_requests_total{status=~"5.."}[1h]))
        / sum(rate(http_requests_total[1h]))
      ) > 14.4  # 14.4x = 2% budget in 1h
    for: 5m
    labels:
      severity: critical
      service: [service-name]
    annotations:
      summary: "[Service] SLO fast burn rate"
      description: "Consuming 2% of monthly error budget per hour"
      runbook_url: "[runbook-link]"
  
  # 慢速消耗告警 — 6小时内消耗 5% 的月度预算
  - alert: [Service]SLOBurnRateSlow
    expr: |
      (
        sum(rate(http_requests_total{status=~"5.."}[6h]))
        / sum(rate(http_requests_total[6h]))
      ) > 6  # 6x = 5% budget in 6h
    for: 30m
    labels:
      severity: warning
      service: [service-name]
    annotations:
      summary: "[Service] SLO slow burn rate"
      description: "Consuming 5% of monthly error budget per 6 hours"
```

### 5. 错误预算策略

当错误预算消耗达到以下阈值时:

| 消耗比例 | 状态 | 行动 |
|---------|------|------|
| 0-50% | 🟢 健康 | 正常开发 |
| 50-75% | 🟡 注意 | 开发需评估风险，增加测试 |
| 75-100% | 🟠 警告 | 暂停非关键变更，优先可靠性修复 |
| 100% | 🔴 超支 | 冻结新功能发布，全员投入可靠性 |

### 6. 仪表盘

[Dashboard 链接 — Grafana / 内部工具]

关键面板:
- [ ] SLI 趋势图 (30 天滚动窗口)
- [ ] 错误预算剩余量
- [ ] 延迟分位数图 (P50/P90/P99)
- [ ] 错误率按路径/状态码分解
- [ ] 告警历史

### 7. 回顾计划

- **频率**: 每季度
- **回顾内容**:
  - SLO 是否合理 (目标是否过松或过紧)
  - 错误预算消耗模式
  - 是否需要新增/移除 SLI
  - 告警是否有效 (噪声/遗漏)
- **参与者**: 服务负责人、SRE、业务方

### 8. 相关文档

- [[10-平台工程/02-运维/04-monitoring-alerting-system|监控告警体系]] — 告警设计参考
- [[31-脚本/templates/runbook-template|Runbook 模板]] — 告警联动 Runbook
- [Google SRE Workbook — SLO 章节]
- [服务架构文档链接]

---

## 版本历史

| 版本 | 日期 | 变更 | 作者 |
|------|------|------|------|
| v1.0 | [YYYY-MM-DD] | 初始 SLO 定义 | [作者] |

<!-- risk-assessed -->
