---
title: 告警与 SLO 监控工程
description: '# 告警与 SLO 监控工程'
category: dictionary
tags:
- k8s
- glossary
- terminology
- prometheus
- grafana
- hpa
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 告警与 SLO 监控工程 是什么
- 如何 告警与 SLO 监控工程
trigger_keywords:
- 告警与
- SLO
- 监控工程
- dictionary
title_en: Alerting
---


# 告警与 SLO 监控工程

## 概述

有效的告警系统不仅仅是"出问题时通知人"，而是要在**用户受到影响之前**准确捕捉异常信号，同时避免告警疲劳。2026 年的最佳实践将 **SLO（Service Level Objective）** 作为告警设计的核心锚点，通过 **Multi-window Multi-burn-rate** 策略实现高信噪比的告警体系。Prometheus + Alertmanager 仍是 Kubernetes 环境的主流组合，但越来越多的组织开始引入 **Cortex、Thanos、VictoriaMetrics** 来解决大规模集群的指标存储和全局查询问题。

## 核心概念/原理

### 1. SLI / SLO / SLA 回顾

- **SLI（Service Level Indicator）**：可量化的服务健康指标，如请求延迟、错误率、吞吐量
- **SLO（Service Level Objective）**：SLI 的目标值，如"99.9% 的请求在 200ms 内完成"
- **SLA（Service Level Agreement）**：对外承诺的可用性合同，违反后通常伴随经济赔偿
- **Error Budget**：在 SLO 周期内允许的错误"预算"，如 99.9% 的 SLO 意味着每月有约 43 分钟的 Error Budget

### 2. 告警设计的黄金法则

根据 Google SRE 和 2026 年行业共识：
1. **每个告警都应对应一个需要立即采取行动的异常状态**
2. **告警应基于症状（Symptom-based），而非原因（Cause-based）**
3. **减少非紧急和低价值的告警页面（Page）**
4. **使用 SLO 推导告警阈值，而非拍脑袋设定固定数值**
5. **高优先级告警必须通过多窗口验证，避免瞬时抖动误报**

### 3. Multi-window Multi-burn-rate 告警

这是 SLO 告警的终极形态，通过观察 Error Budget 的**燃烧速度**来决定是否触发告警：

| Burn Rate | 观察窗口 | Error Budget 消耗 | 告警紧迫度 |
|-----------|----------|-------------------|------------|
| **14.4x** | 1 小时 + 5 分钟 | 2% in 1 hour | 🔴 P1 - 立即处理 |
| **6x** | 6 小时 + 30 分钟 | 5% in 6 hours | 🟠 P2 - 2 小时内处理 |
| **2x** | 3 天 + 6 小时 | 10% in 3 days | 🟡 P3 - 次日处理 |

> 例如：每月 Error Budget 为 43 分钟。如果系统在 1 小时内燃烧了 2% 的 Budget（约 52 分钟等效），说明故障极其严重，必须立即响应。

### 4. Prometheus Alertmanager

**Prometheus** 负责采集指标和触发告警规则，**Alertmanager** 负责：
- **去重（Deduplication）**：同一问题的多个告警合并为一条通知
- **分组（Grouping）**：将相关告警按标签聚合发送
- **路由（Routing）**：根据告警标签路由到不同的接收渠道
- **抑制（Inhibition）**：高优先级告警触发时抑制低优先级关联告警
- **静默（Silencing）**：在计划内维护窗口期静默特定告警

```yaml
# Multi-window burn-rate 告警规则示例
groups:
  - name: slo-alerts
    rules:
      - alert: HighErrorRateFastBurn
        expr: |
          (
            sum(rate(http_requests_total{status=~"5.."}[1h]))
            /
            sum(rate(http_requests_total[1h]))
          ) > 14.4 * (1 - 0.999)
        for: 5m
        labels:
          severity: p1
        annotations:
          summary: "Fast error budget burn detected"
```

## 关键机制或特性

### 指标采集架构演进

| 方案 | 特点 | 适用规模 |
|------|------|----------|
| **Prometheus 单实例** | 简单、独立 | < 100 节点 |
| **Prometheus Federation** | 分层聚合 | 中等规模 |
| **Thanos** | 全局查询、对象存储长期保留 | 大规模、多集群 |
| **Cortex** | 多租户、水平扩展 | SaaS 平台、超大规模 |
| **VictoriaMetrics** | 高性能、低成本 | 所有规模 |

### 告警模板与升级策略

- **通知渠道多样化**：PagerDuty / Opsgenie（P1）、Slack / 钉钉（P2/P3）、邮件（非紧急）
- **升级（Escalation）**：若 15 分钟内未有人确认告警，自动升级给二线值班经理
- **Runbook 链接**：每条告警必须附带对应故障排查手册的链接

### SLO 仪表盘设计

使用 **Grafana** 构建 SLO 仪表盘时应包含：
- **当前 SLO 达成率**：如 99.95%（本月累计）
- **Error Budget 剩余量**：以进度条形式可视化
- **Burn Rate 趋势图**：展示不同时间窗口的燃烧速度
- **SLI 分解图**：按区域、版本、端点细分的错误率和延迟热力图

## 使用场景

1. **电商平台大促保障**：设置 P1 告警监控支付接口的 1 小时 burn rate，确保能在数分钟内发现并止损
2. **多集群统一监控**：使用 Thanos Query 聚合 10 个地域集群的 Prometheus 数据，统一计算全局 SLO
3. **降低告警疲劳**：通过 Alertmanager 抑制"CPU 高"在已知批处理窗口期间的误报，仅保留真正的容量告警
4. **SLO 评审会议**：每周基于 Grafana SLO Dashboard 评审哪些服务消耗了过多 Error Budget，驱动工程改进
5. **自动扩容触发**：将 HPA 和 Cluster Autoscaler 的指标与 SLO 告警关联，实现容量问题的自动化响应

## 最佳实践/注意事项

- **所有 P1 告警必须是 Actionable**：如果收到告警后不知道该做什么，说明告警设计失败
- **避免 Cause-based 告警**：不要对"CPU 高"直接发 P1，而应对"请求延迟超过 SLO"发告警
- **设置合理的 for 持续时间**：绝大多数告警应至少持续 5 分钟才触发，避免瞬时抖动
- **Alertmanager 配置测试**：任何路由或静默规则变更都应在 staging 环境验证
- **定期审查告警质量**：每月统计每个告警的 MTTR 和误报率，淘汰低质量告警
- **Error Budget 政策**：当团队消耗了 50% 以上的 Budget 时，应暂停非必要的发布和功能变更
- **告警与混沌工程结合**：通过 Chaos Engineering 验证告警是否能在真实故障时及时、准确地触发
- **文档化每个告警**：每个告警规则都应在注释中包含：业务影响、排查步骤、预期响应动作

## 参考链接

- [Google SRE - Alerting on SLOs](https://sre.google/workbook/alerting-on-slos/)
- [Prometheus Alerting Best Practices](https://prometheus.io/docs/practices/alerting/)
- [Thanos Documentation](https://thanos.io/)
- [Cortex Documentation](https://cortexmetrics.io/docs/)
- [VictoriaMetrics Documentation](https://docs.victoriametrics.com/)
