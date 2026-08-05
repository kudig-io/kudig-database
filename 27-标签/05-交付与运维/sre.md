---
title: sre
description: 站点可靠性工程标签枢纽 — 涵盖 SRE 原则、SLO/SLI/错误预算、事件管理、容量规划、混沌工程、 toil 消除与可靠性实践的完整知识索引
category: tag-index
tags:
- sre
- site-reliability-engineering
- reliability
- incident-management
- error-budget
tier: core
difficulty: intermediate-to-advanced
domain: reliability-engineering
k8s_versions: ["1.28", "1.30", "1.32", "1.34"]
created: '2026-07-21'
last_updated: '2026-07-21'
---

# sre Tag Hub

> 站点可靠性工程页面 — SRE 原则与实践、SLO/SLI/错误预算、事件管理、容量规划、混沌工程、Toil 消除、On-Call 运营等。

## 核心定义

**站点可靠性工程（Site Reliability Engineering, SRE）** 是由 Google 提出的运维方法论，将软件工程方法应用于基础设施和运维问题。其核心理念是：用自动化和工程化手段替代手动运维，在系统可靠性和迭代速度之间找到最优平衡点。

### SRE 核心原则

| 原则 | 描述 | 实践 |
|------|------|------|
| 拥抱风险 | 可靠性不是 100%，而是基于 SLO 的平衡 | 错误预算策略 |
| SLO 驱动 | 以服务水平目标指导所有决策 | SLO → 错误预算 → 发布决策 |
| 消除 Toil | 自动化重复性手动操作 | Toil 预算 < 50% 工作时间 |
| 分布式系统思维 | 假设一切都会失败 | 冗余、降级、熔断 |
| 可观测性 | 没有监控的系统是黑盒 | 指标 + 日志 + 追踪 |
| 事后无责 | 从失败中学习而非追责 | Blameless Postmortem |
| 渐进式变更 | 小步快跑，快速回滚 | 金丝雀、蓝绿、Feature Flag |

### SRE 与 DevOps 的关系

```
DevOps: 文化哲学 → 打破开发与运维的壁垒
SRE:    工程实践 → DevOps 的具体实现方式之一

SRE 是 DevOps 的一种实现 (SRE is a specific implementation of DevOps)
— Google SRE Book
```

## SLO/SLI/错误预算

- [[09-可观测性/06-SLO-SLI/01-slo-engineering-practice|SLO 工程实践]]
- [[09-可观测性/06-SLO-SLI/02-error-budget-policy|错误预算策略]]
- [[09-可观测性/06-SLO-SLI/03-sli-implementation-guide|SLI 实现指南]]
- [[09-可观测性/06-SLO-SLI/04-sli-definition-selection|SLI 定义选择]]
- [[09-可观测性/06-SLO-SLI/08-slo-sli-system|SLO/SLI 系统]]
- [[09-可观测性/06-SLO-SLI/09-slo-operations-guide|SLO 运营指南]]

### SLO 设计框架

| SLI 类型 | 度量 | 典型 SLO | 计算方式 |
|---------|------|----------|----------|
| 可用性 | 成功请求比例 | 99.9% | 成功请求 / 总请求 |
| 延迟 | 请求响应时间 | P99 < 200ms | 第 99 百分位延迟 |
| 吞吐量 | 每秒处理请求数 | > 1000 RPS | 时间窗口内请求数 |
| 正确性 | 数据一致性 | 99.99% | 正确响应 / 总响应 |
| 新鲜度 | 数据更新延迟 | < 5 分钟 | 最新数据时间戳差 |

## 事件管理 (Incident Management)

- [[13-生产运维/03-事件响应/05-incident-response-template|事故响应模板与流程]]
- [[13-生产运维/03-事件响应/01-escalation-matrix-severity-levels|升级矩阵与严重性级别]]
- [[13-生产运维/03-事件响应/02-war-room-coordination-procedures|作战室协调流程]]
- [[13-生产运维/03-事件响应/06-security-incident-response-playbook|安全事件响应 Playbook]]
- [[13-生产运维/03-事件响应/10-incident-response-handling|事件响应处理]]
- [[13-生产运维/03-事件响应/11-incident-response-runbook-template|事件响应 Runbook 模板]]

### 事件严重性分级

| 级别 | 名称 | 影响 | 响应时间 | 示例 |
|------|------|------|----------|------|
| SEV1 | 致命 | 核心服务完全不可用 | 5 分钟 | 全站宕机、数据丢失 |
| SEV2 | 严重 | 核心功能严重退化 | 15 分钟 | 支付失败率 > 10% |
| SEV3 | 一般 | 非核心功能异常 | 1 小时 | 推荐系统降级 |
| SEV4 | 轻微 | 用户体验轻微影响 | 4 小时 | UI 显示异常 |

## 容量规划 (Capacity Planning)

- [[13-生产运维/07-运维手册/03-capacity-planning-readiness|容量规划就绪]]
- [[12-可靠性/03-容量规划/01-capacity-planning-framework|容量规划方法论]]
- [[12-可靠性/03-容量规划/06-capacity-planning-forecasting|资源预测模型]]
- [[02-工作负载/01-核心工作负载/22-cluster-capacity-planning|集群容量规划]]

## 混沌工程 (Chaos Engineering)

- [[12-可靠性/04-混沌工程/01-chaos-engineering-overview|混沌工程原则]]
- [[12-可靠性/04-混沌工程/02-chaos-mesh-deployment|Chaos Mesh 实践]]
- [[12-可靠性/04-混沌工程/04-litmus-practices|LitmusChaos 实验]]
- [[25-研究/04-可靠性与运维/chaos-engineering-practice|混沌工程实践研究]]
- [[24-综合/06-可靠性与成本/chaos-engineering-sre-resilience|混沌工程与 SRE 韧性]]

## 灾难恢复 (Disaster Recovery)

- [[12-可靠性/02-灾难恢复/13-disaster-recovery-bc-runbook-v1|灾备 BC Runbook v1]]
- [[12-可靠性/02-灾难恢复/25-disaster-recovery-bc-runbook-v2|灾备 BC Runbook v2]]
- [[12-可靠性/02-灾难恢复/18-disaster-recovery-drills|灾备演练]]
- [[12-可靠性/02-灾难恢复/19-storage-backend-failure-playbook|存储后端故障 Playbook]]

## 事后复盘 (Postmortem)

- [[12-可靠性/05-事后复盘/01-blameless-postmortem-template|无责事后复盘指南]]
- [[12-可靠性/05-事后复盘/03-incident-postmortem-template|事后复盘模板]]
- [[12-可靠性/05-事后复盘/02-postmortem-culture-guide|事件回顾流程]]

## On-Call 运营

- [[13-生产运维/07-运维手册/01-production-sre-daily-ops|生产环境日常巡检与值班手册]]
- [[13-生产运维/03-事件响应/04-on-call-playbook|值班手册与告警响应]]
- [[13-生产运维/07-运维手册/07-change-freeze-policy|变更冻结策略]]

### On-Call 最佳实践

| 实践 | 描述 |
|------|------|
| 告警可操作 | 每条告警必须有对应的 Runbook |
| 告警分级 | P1 电话 / P2 短信 / P3 邮件 |
| 轮转合理 | 每人每周不超过 1 次 On-Call |
| 交接规范 | 换班时同步未关闭事件 |
| 告警降噪 | 消除告警疲劳，目标 < 5 条/班次 |
| 事后回顾 | 每次 On-Call 后记录改进项 |

## SRE 关键指标 (Four Golden Signals)

| 信号 | 度量 | 工具 |
|------|------|------|
| 延迟 (Latency) | 请求响应时间分布 | Prometheus histogram |
| 流量 (Traffic) | 系统负载量 (QPS/并发) | Prometheus rate |
| 错误 (Errors) | 失败请求比率 | Prometheus error rate |
| 饱和度 (Saturation) | 资源使用率 | node_exporter / cAdvisor |

## 概念 (Concepts)

- [[22-概念/sre-principles|SRE 原则]]
- [[22-概念/08-可靠性与运维/slo-error-budget-framework|错误预算]]
- [[22-概念/toil-elimination|Toil 消除]]
- [[22-概念/08-可靠性与运维/incident-management-patterns|事件管理]]
- [[22-概念/capacity-planning|容量规划]]

## 实体 (Entities)

- [[23-实体/15-参考与索引/k8s-production-operations|Kubernetes Production Operations]]

## SRE 实践全景

### SRE 核心原则

| 原则 | 说明 |
|---|---|
| SLO 驱动 | 以服务水平目标为导向 |
| 消除 Toil | 自动化重复性工作 |
| 拥抱风险 | Error Budget 平衡创新与稳定 |
| 可观测性 | 监控/日志/追踪三大支柱 |

### SRE 工具链

| 类别 | 工具 |
|---|---|
| 监控 | Prometheus, Grafana, Datadog |
| 日志 | ELK, Loki, Fluentd |
| 追踪 | Jaeger, Zipkin, Tempo |
| 告警 | Alertmanager, PagerDuty |
| 混沌 | Chaos Mesh, Litmus |

## 面试要点

1. **Q：SRE 与 DevOps 的区别？**
   A：DevOps：文化/理念。SRE：具体实践/工程方法。SRE 是 DevOps 的一种实现。

2. **Q：SLO/SLI/Error Budget 的关系？**
   A：SLI：指标(成功率/延迟)。SLO：目标(99.9%)。Error Budget：1-SLO，允许失败的余量。

3. **Q：如何消除 Toil？**
   A：识别重复工作→自动化→平台化→持续优化。目标：Toil < 50% 工作时间。

## Related Tags

- [[27-标签/01-核心平台/k8s|k8s — Kubernetes 核心]]
- [[27-标签/05-交付与运维/reliability|reliability — 可靠性工程]]
- [[27-标签/05-交付与运维/production|production — 生产运营]]
- [[27-标签/04-可观测性/observability|observability — 可观测性]]
- [[27-标签/04-可观测性/troubleshooting|troubleshooting — 故障诊断]]
- [[27-标签/05-交付与运维/platform-engineering|platform-engineering — 平台工程]]
- [[27-标签/07-参考与最佳实践/best-practices|best-practices — 最佳实践]]
