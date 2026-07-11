---
title: SLO 仪表盘设计
description: 用 Grafana 设计 SLO/SLI/错误预算可视化仪表盘，含面板布局、PromQL 与告警阈值
summary: 七面板标准布局覆盖 SLI 趋势、错误预算燃烧率、延迟热力图与请求量，附可复用 PromQL
category: reliability
tags:
- slo
- sli
- reliability
- grafana
- observability
- prometheus
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
audience:
- SRE
- 架构师
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# SLO 仪表盘设计

> **设计哲学**：仪表盘不是"把所有指标堆上去"，而是让一个值班工程师在 30 秒内回答三个问题——**服务健康吗？错误预算烧得多快？要不要现在动手？** 每多一个无关面板，这三个问题的答案就被稀释一分。

## 标准七面板布局

```
┌─────────────────────────────────────────────────────────────┐
│  ① SLO 概览 Stat (大数字)   ② 30 天 SLI 趋势线               │
│  达成率 / 预算剩余           绿/黄/红着色                     │
├─────────────────────────────────────────────────────────────┤
│  ③ 错误预算燃烧率 (多窗口)  ④ 延迟分位线 (P50/P95/P99)        │
│  1h / 6h / 1d / 3d 双阈值                                    │
├─────────────────────────────────────────────────────────────┤
│  ⑤ 请求量 RPS (堆叠)        ⑥ 延迟热力图 (P99 by bucket)     │
│  按 status code 着色                                          │
├─────────────────────────────────────────────────────────────┤
│  ⑦ 事件标注 Timeline (annotations 叠加)                       │
└─────────────────────────────────────────────────────────────┘
```

## 面板 PromQL 速查

### ① SLO 概览 Stat

```promql
# SLI 达成率（可用性）
1 - (
  sum(rate(http_requests_total{job="api",code=~"5.."}[30d]))
  /
  sum(rate(http_requests_total{job="api"}[30d]))
)
```

```promql
# 错误预算剩余 %
1 - (
  sum(rate(http_requests_total{job="api",code=~"5.."}[30d]))
  / sum(rate(http_requests_total{job="api"}[30d]))
) / (1 - 0.999)
```

阈值着色：`> 0.5` 绿、`0.25–0.5` 黄、`< 0.25` 红。

### ③ 错误预算燃烧率（多窗口告警核心）

```promql
# 1h vs 5m 燃烧率（快速突发）
(
  sum(rate(http_requests_total{code=~"5.."}[1h]))
  / sum(rate(http_requests_total[1h]))
) / (1 - 0.999)

# 6h vs 30m 燃烧率（中速）
# 3d vs 6h 燃烧率（慢速）
```

经典多窗口告警：`burn_1h > 14.4 AND burn_5m > 14.4` → P1 页面（30 天预算将在 2 小时内耗尽）。

### ④ 延迟分位（直方图）

```promql
# P99 延迟
histogram_quantile(0.99,
  sum by (le) (rate(http_request_duration_seconds_bucket[5m]))
)
```

### ⑥ 延迟热力图

用 Grafana `Heatmap` 面板 + Prometheus `http_request_duration_seconds_bucket`，按 `le` bucket 渲染。比折线图更早发现尾部延迟漂移。

## 仪表盘 JSON 骨架

```json
{
  "title": "SLO: API Service",
  "tags": ["slo", "sre"],
  "templating": {
    "list": [{
      "name": "service",
      "type": "query",
      "datasource": "Prometheus",
      "query": "label_values(http_requests_total, job)"
    }]
  },
  "panels": [
    {
      "type": "stat", "title": "错误预算剩余",
      "gridPos": {"h": 4, "w": 6, "x": 0, "y": 0},
      "fieldConfig": {
        "defaults": {
          "thresholds": {
            "mode": "absolute",
            "steps": [
              {"color": "red", "value": null},
              {"color": "yellow", "value": 0.25},
              {"color": "green", "value": 0.5}
            ]
          },
          "unit": "percentunit",
          "mappings": []
        }
      },
      "targets": [{"expr": "error_budget_remaining"}]
    }
  ]
}
```

## 关键设计原则

1. **一个 SLO 一个仪表盘**：不要把 10 个服务的 SLO 塞进一张图，用变量 `$service` 切换。
2. **错误预算永远在最上方**：它是决策依据，不是辅助指标。
3. **用 30 天滚动窗口**：与 SLO 定义窗口一致，避免"日历月重置"错觉。
4. **叠加 annotations**：发布、扩缩容、混沌实验、Incident 全部标注到 timeline，做归因。
5. **告警阈值与面板阈值一致**：仪表盘变红的瞬间就应该是 PagerDuty 响铃的瞬间。

## Terraform / GitOps 管理

```hcl
resource "grafana_dashboard" "slo_api" {
  folder       = "SRE"
  config_json  = file("dashboards/slo-api.json")
  message      = "chore: update SLO dashboard"
}
```

仪表盘纳入 Git 仓库，PR 评审变更，避免"谁的 Grafana 谁改"导致的标准漂移。

## 相关

- [[可靠性/SRE实践/05-error-budget-automation.md|05 error budget automation]]
- [[可靠性/03-slo-sli-guide.md|03 slo sli guide]]
- [[可靠性/99-production-readiness-operations-guide.md|99 production readiness operations guide]]

<!-- risk-assessed -->
