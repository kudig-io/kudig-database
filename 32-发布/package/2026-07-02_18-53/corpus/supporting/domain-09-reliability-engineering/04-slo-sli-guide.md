---
title: SLO、SLI 与错误预算实践指南
summary: SLO、SLI 与错误预算实践指南：SLO（服务等级目标）是可靠性工程的核心工具，它将抽象的"系统稳定"转化为可量化、可追踪的指标。通过 SLO
  和错误预算，团队可以在可靠性与创新速度之间找到平衡。
category: domain-09
tags:
- domain-09
- SLO
- SLI
- SLA
- 可靠性
- 错误预算
- 监控
- visibility/public
tier: supporting
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---



# SLO、SLI 与错误预算实践指南

## 概述

SLO（服务等级目标）是可靠性工程的核心工具，它将抽象的"系统稳定"转化为可量化、可追踪的指标。通过 SLO 和错误预算，团队可以在可靠性与创新速度之间找到平衡。

## 核心概念定义

### SLI（Service Level Indicator）

服务等级指标，是可量化的可靠性度量：
- **可用性**：服务成功响应的比例（如 99.9%）
- **延迟**：请求响应时间（如 P99 < 500ms）
- **错误率**：失败请求占比（如 < 0.1%）
- **吞吐量**：单位时间处理的请求量（如 10000 QPS）

### SLO（Service Level Objective）

服务等级目标，是 SLI 的目标值：
- 定义：在特定时间窗口内，SLI 应达到的阈值
- 示例："月度可用性 SLO 为 99.9%"
- 作用：为团队提供明确的可靠性目标

### SLA（Service Level Agreement）

服务等级协议，是面向客户的承诺：
- SLA 通常比 SLO 更严格（如 SLO 99.9%，SLA 99.5%）
- 违反 SLA 通常有商务赔偿条款
- SLO 是内部目标，SLA 是外部承诺

## Kubernetes 场景的典型 SLI

| SLI 类型 | 度量方式 | K8s 中的实现 | 典型目标 |
|---|---|---|---|
| 可用性 | 成功请求数 / 总请求数 | Ingress/Service 监控 | 99.9% |
| 延迟 | 请求响应时间分位值 | Prometheus histogram | P99 < 1s |
| 错误率 | 5xx 响应占比 | 应用指标/ Ingress 日志 | < 0.1% |
| 吞吐量 | 每秒请求数 | Prometheus rate() | 按业务定义 |
| Pod 就绪时间 | 调度到 Ready 的耗时 | Kubelet metrics | < 60s |

## SLO 设定方法

### 基于历史数据

1. 收集过去 30 天的实际 SLI 数据
2. 取 P95 分位值作为初始 SLO（确保 95% 时间能达标）
3. 逐步收紧目标，观察团队响应能力

### 考虑业务容忍度

| 业务类型 | 可用性 SLO | 延迟 SLO | 说明 |
|---|---|---|---|
| 电商核心交易 | 99.99% | P99 < 200ms | 低容忍度 |
| 内部管理系统 | 99.5% | P99 < 3s | 中等容忍度 |
| 数据分析平台 | 99% | P99 < 30s | 高容忍度 |

### 避免过度承诺

- SLO 不是越高越好，99.999% 的可用性成本可能是 99.9% 的 10 倍
- 未经验证的 SLO 会导致团队疲于奔命
- 建议从宽松目标开始，逐步迭代优化

## 错误预算（Error Budget）

### 概念

错误预算 = 100% - SLO，表示允许的错误配额：

| SLO | 月度错误预算 | 年化允许停机时间 |
|---|---|---|
| 99% | 7.2 小时 | 87.6 小时 |
| 99.9% | 43.2 分钟 | 8.76 小时 |
| 99.99% | 4.32 分钟 | 52.6 分钟 |

### 错误预算的作用

- **平衡创新与可靠性**：错误预算未耗尽时，允许发布新功能
- **变更决策依据**：错误预算耗尽时，冻结非紧急变更
- **优先级排序**：高错误预算消耗的服务应优先投入可靠性改进

### 错误预算消耗监控

```promql
# 过去 7 天错误预算消耗比例
(
  sum(rate(http_requests_total{status=~"5.."}[7d]))
  /
  sum(rate(http_requests_total[7d]))
)
/
(1 - 0.999)   # 对应 99.9% SLO
```

## 基于 SLO 的告警（Burn Rate）

### Burn Rate 告警

Burn Rate 表示错误预算的消耗速度：

| Burn Rate | 含义 | 响应时间 |
|---|---|---|
| 1x | 按当前速度将在周期末刚好耗尽预算 | 页面通知 |
| 2x | 将在半个周期内耗尽 | 低优先级告警 |
| 14.4x | 将在 2 天内耗尽月度预算 | 高优先级告警 |
| 72x | 将在 10 小时内耗尽月度预算 | 紧急告警 |

### 告警规则示例

```yaml
# 高 Burn Rate 告警（14.4x，2天窗口）
- alert: HighErrorRate
  expr: |
    (
      sum(rate(http_requests_total{status=~"5.."}[2h]))
      /
      sum(rate(http_requests_total[2h]))
    ) > 14.4 * (1 - 0.999)
  for: 5m
  labels:
    severity: critical
```

## 远程顾问指导要点

帮助客户定义合理的 SLO，需遵循以下步骤：

1. **现状摸底**：收集现有监控数据，了解真实的可用性、延迟分布
2. **业务对齐**：与产品经理确认各服务对业务的影响程度，避免技术团队单方面设定
3. **渐进式设定**：首月目标建议取历史数据的 P90，后续每月收紧 0.1%
4. **工具落地**：协助客户配置 Prometheus 规则，建立 Burn Rate 告警
5. **定期回顾**：每月组织 SLO 回顾会议，分析错误预算消耗原因

> 远程顾问应避免替客户拍脑袋定 SLO，而应提供方法论和工具，引导客户基于数据和业务实际做出决策。

## 相关链接

- [[observability-stack-evolution]] — 可观测性技术栈演进
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-10-troubleshooting-diagnostics/02-infrastructure-troubleshooting/04-monitoring-alerting-troubleshooting|monitoring-alerting-troubleshooting]] — 监控告警问题排查
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-09-reliability-engineering/05-slo-sli/index|chaos-engineering-guide]] — 混沌工程实践
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-09-reliability-engineering/05-slo-sli/index|capacity-planning-guide]] — 容量规划指南

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
