---
title: Alerting & On-Call
description: 告警知识域 — Alertmanager 配置、告警疲劳治理、On-Call 集成、告警路由与抑制、监控 Playbook
category: subdomain
tags:
- alerting
- alertmanager
- on-call
- pagerduty
- alert-fatigue
tier: core
created: '2026-07-02'
last_updated: '2026-07-21'
---
# 告警与 On-Call Alerting

> 构建可操作的告警体系，消除告警疲劳，确保每个告警都有明确的响应流程。

## 告警分级模型

| 级别 | 响应时间 | 示例 | 通知渠道 |
|------|----------|------|----------|
| P1 Critical | 5min | 服务不可用、数据丢失 | 电话 + 短信 |
| P2 High | 15min | 性能严重降级、部分功能不可用 | 即时消息 |
| P3 Medium | 1h | 非关键服务异常 | 工单 |
| P4 Low | 下个工作日 | 容量预警、证书即将过期 | 邮件 |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[可观测性/告警/01-alertmanager-deep-configuration.md\|Alertmanager 深度配置]] | 路由/抑制/静默/分组 | advanced |
| [[可观测性/告警/02-pagerduty-opsgenie-integration.md\|PagerDuty/OpsGenie 集成]] | On-Call 平台对接 | intermediate |
| [[可观测性/告警/03-alert-fatigue-reduction-strategies.md\|告警疲劳治理]] | 降噪/合并/自动化策略 | advanced |
| [[可观测性/告警/05-alerting-management.md\|告警管理]] | 告警生命周期管理 | intermediate |
| [[可观测性/告警/06-monitoring-alerting-practice.md\|监控告警实践]] | 生产环境告警最佳实践 | intermediate |
| [[可观测性/告警/21-monitoring-playbooks.md\|监控 Playbook]] | 告警响应操作手册 | advanced |

## 告警质量检查清单

- [ ] 每个告警关联 Runbook/Playbook 链接
- [ ] 告警基于 SLO 燃烧率而非单一阈值
- [ ] 配置合理的 `for` 持续时间避免抨动
- [ ] 使用 inhibition_rules 抑制级联告警
- [ ] 定期审计告警有效性（删除无人响应的告警）
- [ ] 告警可操作（Actionable）—— 收到即知如何处理

## Related

- [[可观测性/SLO-SLI/index.md|SLO & SLI]]
- [[可观测性/指标/index.md|指标 Metrics]]
- [[可靠性/index.md|可靠性 Reliability]]
