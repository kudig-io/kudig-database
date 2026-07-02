---
title: SLA/SLO 定义模板与 Error Budget 管理
description: '定义 SLO as Code（Sloth/Pyrra）、SLO 与监控系统集成、Error Budget 计算与可视化及行业 SLO 基准参考'
summary: '定义 SLO as Code（Sloth/Pyrra）、SLO 与监控系统集成、Error Budget 计算与可视化及行业 SLO 基准参考'
category: production-operations
tags:
- production
- operations
- sla
- slo
- error-budget
- sloth
- pyrra
tier: critical
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- SLA/SLO 定义模板 是什么
- 如何 定义 SLO
- 如何 计算 Error Budget
trigger_keywords:
- sla
- slo
- error-budget
- sloth
- pyrra
- reliability
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

# SLA/SLO 定义模板与 Error Budget 管理

## 1. SLA/SLO/SLI 基础概念

### 1.1 术语定义

```
SLI (Service Level Indicator)
  定义: 衡量服务可靠性的具体指标
  示例: 请求成功率、延迟 P99、吞吐量

SLO (Service Level Objective)
  定义: SLI 的目标值，团队内部承诺
  示例: 99.9% 的请求在 200ms 内完成

SLA (Service Level Agreement)
  定义: 对外承诺，通常包含违约赔偿
  示例: 99.95% 可用性，否则退还 10% 费用

Error Budget
  定义: SLO 允许的错误量
  公式: Error Budget = 1 - SLO
  示例: SLO 99.9% → Error Budget = 0.1% → 每月 43.2 分钟
```

### 1.2 关系图

```
┌─────────────────────────────────────────────────────────┐
│                      SLA (对外承诺)                      │
│  ┌───────────────────────────────────────────────────┐  │
│  │                 SLO (内部目标)                     │  │
│  │  ┌─────────────────────────────────────────────┐  │  │
│  │  │           SLI (度量指标)                     │  │  │
│  │  │                                             │  │  │
│  │  │  - 可用性: 成功率 99.9%                      │  │  │
│  │  │  - 延迟: P99 < 200ms                        │  │  │
│  │  │  - 吞吐量: > 1000 req/s                     │  │  │
│  │  └─────────────────────────────────────────────┘  │  │
│  │                                                   │  │
│  │  Error Budget: 0.1% (每月 43.2 分钟)              │  │
│  └───────────────────────────────────────────────────┘  │
│                                                         │
│  SLA: 99.95% 可用性，否则赔偿                           │
└─────────────────────────────────────────────────────────┘
```

## 2. SLI 定义模板

### 2.1 可用性 SLI

```yaml
# 可用性 SLI 定义
sli:
  name: "API 可用性"
  description: "API 请求成功率"
  metric: |
    sum(rate(http_requests_total{code!~"5.."}[5m]))
    /
    sum(rate(http_requests_total[5m]))
  unit: "百分比"
  aggregation: "5 分钟滚动窗口"
  data_source: "Prometheus"
  labels:
    service: "api-gateway"
    environment: "production"
```

### 2.2 延迟 SLI

```yaml
# 延迟 SLI 定义
sli:
  name: "API 延迟"
  description: "API 响应时间 P99"
  metric: |
    histogram_quantile(0.99,
      sum(rate(http_request_duration_seconds_bucket[5m])) by (le)
    )
  unit: "秒"
  aggregation: "5 分钟窗口"
  data_source: "Prometheus"
  labels:
    service: "api-gateway"
    environment: "production"
```

### 2.3 吞吐量 SLI

```yaml
# 吞吐量 SLI 定义
sli:
  name: "API 吞吐量"
  description: "每秒请求数"
  metric: |
    sum(rate(http_requests_total[5m]))
  unit: "req/s"
  aggregation: "5 分钟平均"
  data_source: "Prometheus"
```

### 2.4 数据新鲜度 SLI

```yaml
# 数据新鲜度 SLI（批处理场景）
sli:
  name: "数据新鲜度"
  description: "数据更新延迟"
  metric: |
    time() - max(etl_last_success_timestamp)
  unit: "秒"
  aggregation: "即时值"
  data_source: "Prometheus"
```

## 3. SLO 定义模板

### 3.1 SLO YAML 模板（Sloth 格式）

```yaml
# slo-definitions.yaml
apiVersion: sloth.slok.dev/v1
kind: PrometheusServiceLevel
metadata:
  name: api-gateway-slo
  namespace: production
spec:
  service: "api-gateway"
  labels:
    team: "platform"
    tier: "critical"
  slos:
    # 可用性 SLO
    - name: "availability"
      objective: 99.9
      description: "API 请求成功率"
      sli:
        events:
          error_query: |
            sum(rate(http_requests_total{
              service="api-gateway",
              code=~"5.."
            }[{{.window}}]))
          total_query: |
            sum(rate(http_requests_total{
              service="api-gateway"
            }[{{.window}}]))
      alerting:
        name: "APIGatewayHighErrorRate"
        labels:
          severity: critical
        annotations:
          summary: "API Gateway 错误率超过 SLO"
        page_alert:
          labels:
            severity: page
        ticket_alert:
          labels:
            severity: ticket

    # 延迟 SLO
    - name: "latency"
      objective: 99.0
      description: "P99 延迟 < 200ms"
      sli:
        events:
          error_query: |
            sum(rate(http_request_duration_seconds_count{
              service="api-gateway",
              le="0.2"
            }[{{.window}}]))
          total_query: |
            sum(rate(http_request_duration_seconds_count{
              service="api-gateway"
            }[{{.window}}]))
      alerting:
        name: "APIGatewayHighLatency"
        labels:
          severity: warning
```

### 3.2 多窗口多燃烧率告警

```yaml
# Sloth 自动生成的多燃烧率告警
# 适用于 99.9% SLO

# 1 小时窗口，14.4x 燃烧率 → Page（5 分钟内消耗 1 天预算）
- alert: SLOBurnRateHigh
  expr: |
    (
      sum(rate(http_requests_total{code=~"5.."}[1h]))
      / sum(rate(http_requests_total[1h]))
    ) > (1 - 0.999) * 14.4
  for: 5m
  labels:
    severity: page

# 6 小时窗口，6x 燃烧率 → Ticket（30 分钟内消耗 1 天预算）
- alert: SLOBurnRateMedium
  expr: |
    (
      sum(rate(http_requests_total{code=~"5.."}[6h]))
      / sum(rate(http_requests_total[6h]))
    ) > (1 - 0.999) * 6
  for: 30m
  labels:
    severity: ticket

# 3 天窗口，1x 燃烧率 → 信息（6 小时内消耗 1 天预算）
- alert: SLOBurnRateLow
  expr: |
    (
      sum(rate(http_requests_total{code=~"5.."}[3d]))
      / sum(rate(http_requests_total[3d]))
    ) > (1 - 0.999) * 1
  for: 6h
  labels:
    severity: info
```

## 4. SLO as Code 工具

### 4.1 Sloth 配置

```bash
# 安装 Sloth
brew install slok/sloth/sloth

# 从 YAML 生成 Prometheus Rules
sloth generate -i slo-definitions.yaml -o generated-rules.yaml

# 输出包含:
# 1. SLI 记录规则
# 2. Error Budget 记录规则
# 3. 多窗口多燃烧率告警规则
```

### 4.2 Pyrra 配置

```yaml
# Pyrra SLO 定义
apiVersion: pyrra.dev/v1alpha1
kind: ServiceLevelObjective
metadata:
  name: api-availability
  namespace: monitoring
spec:
  target: "99.9"
  window: "30d"
  serviceLevelIndicator:
    indicator:
      ratio:
        errors:
          metric: http_requests_total{code=~"5.."}
        total:
          metric: http_requests_total
  description: "API 请求成功率 99.9%"
```

### 4.3 Pyrra vs Sloth 对比

| 特性 | Sloth | Pyrra |
|------|-------|-------|
| 配置格式 | YAML + Go模板 | CRD (Kubernetes) |
| 输出 | Prometheus Rules YAML | Prometheus Rules + UI |
| 多窗口告警 | 内置 | 内置 |
| Error Budget UI | 无（需自建） | 内置 Web UI |
| 适用场景 | GitOps、纯配置 | Kubernetes 原生 |

## 5. Error Budget 计算

### 5.1 Error Budget 计算公式

```
Error Budget 计算:

月度 Error Budget（分钟）= (1 - SLO) × 月度总分钟数

SLO 99.9%:
  Error Budget = 0.001 × 43,200 = 43.2 分钟/月

SLO 99.95%:
  Error Budget = 0.0005 × 43,200 = 21.6 分钟/月

SLO 99.99%:
  Error Budget = 0.0001 × 43,200 = 4.32 分钟/月

SLO 99.999%:
  Error Budget = 0.00001 × 43,200 = 0.432 分钟/月 = 25.9 秒/月
```

### 5.2 Error Budget 记录规则

```yaml
# Prometheus 记录规则
groups:
  - name: error_budget
    interval: 1m
    rules:
      # SLI 值
      - record: sli:value
        expr: |
          1 - (
            sum(rate(http_requests_total{code=~"5.."}[30d]))
            / sum(rate(http_requests_total[30d]))
          )

      # Error Budget 剩余比例
      - record: error_budget:remaining_ratio
        expr: |
          (
            1 - (
              sum(rate(http_requests_total{code=~"5.."}[30d]))
              / sum(rate(http_requests_total[30d]))
            )
            - 0.999  # SLO 目标
          ) / (1 - 0.999)

      # Error Budget 剩余分钟数
      - record: error_budget:remaining_minutes
        expr: |
          error_budget:remaining_ratio * 43200 * (1 - 0.999)

      # Error Budget 消耗速率（每天）
      - record: error_budget:burn_rate_daily
        expr: |
          1 - (
            1 - (
              sum(rate(http_requests_total{code=~"5.."}[1d]))
              / sum(rate(http_requests_total[1d]))
            )
            / (1 - 0.999)
          )

      # Error Budget 预计耗尽时间
      - record: error_budget:exhaustion_days
        expr: |
          error_budget:remaining_ratio / error_budget:burn_rate_daily
          and error_budget:burn_rate_daily > 0
```

### 5.3 Error Budget 策略

```
Error Budget 使用策略:

Error Budget 充足 (> 50%):
  - 允许进行风险变更
  - 可以安排实验和测试
  - 发布频率可以更高

Error Budget 适中 (20-50%):
  - 谨慎进行变更
  - 增加变更审批层级
  - 优先修复稳定性问题

Error Budget 紧张 (< 20%):
  - 冻结非关键变更
  - 全力投入稳定性修复
  - 提高告警响应优先级

Error Budget 耗尽 (≤ 0%):
  - 完全冻结变更
  - 启动稳定性专项
  - 管理层介入
```

## 6. Error Budget 可视化

### 6.1 Grafana Dashboard

```json
{
  "dashboard": {
    "title": "SLO & Error Budget Dashboard",
    "panels": [
      {
        "title": "当前 SLI 值",
        "type": "gauge",
        "targets": [
          {
            "expr": "sli:value{service=\"api-gateway\"}",
            "legendFormat": "SLI"
          }
        ],
        "fieldConfig": {
          "defaults": {
            "min": 0.99,
            "max": 1,
            "thresholds": {
              "steps": [
                { "value": 0.999, "color": "red" },
                { "value": 0.9995, "color": "yellow" },
                { "value": 0.9999, "color": "green" }
              ]
            }
          }
        }
      },
      {
        "title": "Error Budget 剩余",
        "type": "stat",
        "targets": [
          {
            "expr": "error_budget:remaining_ratio{service=\"api-gateway\"} * 100",
            "legendFormat": "剩余 %"
          }
        ]
      },
      {
        "title": "Error Budget 趋势",
        "type": "timeseries",
        "targets": [
          {
            "expr": "error_budget:remaining_ratio{service=\"api-gateway\"} * 100",
            "legendFormat": "剩余 %"
          }
        ]
      }
    ]
  }
}
```

### 6.2 Pyrra 内置 UI

```bash
# 安装 Pyrra
kubectl apply -f https://github.com/pyrra-dev/pyrra/releases/download/v0.7.0/manifests.yaml

# 访问 Pyrra UI
kubectl port-forward svc/pyrra 9099:9099 -n monitoring

# UI 功能:
# - 所有 SLO 概览
# - Error Budget 剩余可视化
# - 历史趋势
# - 告警状态
```

## 7. 行业 SLO 基准参考

### 7.1 常见 SLO 水平

| 可用性 | 年度停机 | 月度停机 | 适用场景 |
|--------|---------|---------|---------|
| 99% | 3.65 天 | 7.2 小时 | 内部工具、开发环境 |
| 99.9% | 8.76 小时 | 43.2 分钟 | 一般 B2B 服务 |
| 99.95% | 4.38 小时 | 21.6 分钟 | 重要业务服务 |
| 99.99% | 52.6 分钟 | 4.32 分钟 | 关键金融/支付 |
| 99.999% | 5.26 分钟 | 25.9 秒 | 电信核心网 |

### 7.2 各行业参考 SLO

```yaml
# 行业 SLO 基准
industry_slos:
  ecommerce:
    availability: 99.95%
    latency_p99: 300ms
    latency_p95: 100ms
    error_rate: < 0.1%

  fintech:
    availability: 99.99%
    latency_p99: 100ms
    latency_p95: 50ms
    error_rate: < 0.01%

  saas_b2b:
    availability: 99.9%
    latency_p99: 500ms
    latency_p95: 200ms
    error_rate: < 0.5%

  internal_tools:
    availability: 99%
    latency_p99: 1s
    latency_p95: 500ms
    error_rate: < 1%

  iot_telemetry:
    availability: 99.9%
    data_freshness: < 5min
    data_loss: < 0.1%
```

### 7.3 SLO 设定指南

```
SLO 设定步骤:

Step 1: 识别关键用户旅程
  - 用户注册/登录
  - 核心业务操作（下单/支付）
  - 数据查询/展示

Step 2: 定义 SLI
  - 选择最能反映用户体验的指标
  - 确定数据来源和采集方式

Step 3: 分析历史数据
  - 查看过去 3-6 个月的 SLI 分布
  - 识别 P50、P90、P99、P999 值

Step 4: 设定 SLO
  - 基于历史数据，选择合理的初始目标
  - 留出改进空间（不要一开始就设太高）

Step 5: 设定 Error Budget 策略
  - 定义 Error Budget 消耗时的响应措施
  - 与产品/工程团队对齐

Step 6: 持续迭代
  - 每季度 Review SLO
  - 根据业务变化调整
```

## 8. SLO 与变更管理集成

### 8.1 变更窗口决策

```python
# change_approval.py
def should_approve_change(service, change_risk):
    """基于 Error Budget 决策变更"""
    error_budget = get_error_budget_remaining(service)

    if error_budget > 0.5:
        return {"approved": True, "reason": "Error Budget 充足"}
    elif error_budget > 0.2:
        if change_risk == "low":
            return {"approved": True, "reason": "低风险变更，Error Budget 适中"}
        else:
            return {"approved": False, "reason": "高风险变更，Error Budget 不足"}
    elif error_budget > 0:
        return {"approved": False, "reason": "Error Budget 紧张，仅允许紧急修复"}
    else:
        return {"approved": False, "reason": "Error Budget 耗尽，冻结所有变更"}
```

### 8.2 发布节奏调整

```
基于 Error Budget 的发布节奏:

Error Budget > 50%:
  - 每日发布
  - 自动化发布
  - 最小审批

Error Budget 20-50%:
  - 隔日发布
  - 需要 QA 验证
  - Team Lead 审批

Error Budget < 20%:
  - 每周发布
  - 完整回归测试
  - Manager 审批

Error Budget ≤ 0%:
  - 冻结发布
  - 仅允许 P0 修复
  - VP 审批
```

---

*本文档提供 SLA/SLO 定义的完整模板和 Error Budget 管理机制。每个服务都应定义明确的 SLO，并将 Error Budget 纳入变更管理决策。*
