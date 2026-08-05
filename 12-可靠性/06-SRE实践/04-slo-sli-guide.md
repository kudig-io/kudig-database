---
title: SLO、SLI 与错误预算实践指南
summary: SLO、SLI 与错误预算实践指南：SLO（服务等级目标）是可靠性工程的核心工具，它将抽象的"系统稳定"转化为可量化、可追踪的指标。通过 SLO
  和错误预算，团队可以在可靠性与创新速度之间找到平衡。
category: 可靠性
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

## SLO 实施完整流程

### 实施阶段

```
阶段 1: 服务识别与分级 (1-2 周)
  └─ 识别核心服务 → 定义服务等级 → 确定 SLO 负责人

阶段 2: SLI 定义与数据采集 (2-3 周)
  └─ 确定 SLI 指标 → 配置数据采集 → 验证数据质量

阶段 3: SLO 设定与对齐 (1-2 周)
  └─ 基于历史数据设定 → 与业务对齐 → 获得签字确认

阶段 4: 告警与仪表盘 (2-3 周)
  └─ 配置 Burn Rate 告警 → 构建 Dashboard → 验证告警有效性

阶段 5: 流程集成 (2-4 周)
  └─ 发布门控集成 → 错误预算策略 → 评审机制建立

阶段 6: 持续优化 (持续)
  └─ 月度回顾 → SLO 调整 → 成熟度提升
```

### 服务分级模板

| 服务等级 | 定义 | 示例 | 可用性 SLO | 延迟 SLO |
|---------|------|------|-----------|----------|
| **Tier 0** | 影响收入的核心服务 | 支付、下单 | 99.99% | P99 < 200ms |
| **Tier 1** | 影响用户体验的重要服务 | 搜索、推荐 | 99.9% | P99 < 500ms |
| **Tier 2** | 影响运营的内部服务 | 后台管理 | 99.5% | P99 < 2s |
| **Tier 3** | 可延迟的批处理服务 | 报表、ETL | 99% | P99 < 30s |

## 多窗口多 Burn Rate 告警

### Google SRE 告警策略

基于《SRE Workbook》的多窗口多 Burn Rate 告警，避免单一窗口导致的误报/漏报：

| 严重度 | Burn Rate | 短窗口 | 长窗口 | 响应时间 | 预算耗尽时间 |
|-------|-----------|-------|-------|---------|-------------|
| **紧急** | 14.4x | 5m | 1h | 立即 | 2 天 |
| **高** | 6x | 30m | 6h | 1 小时内 | 5 天 |
| **中** | 3x | 2h | 24h | 工作日内 | 10 天 |
| **低** | 1x | 6h | 3d | 下次评审 | 30 天 |

### 完整 PrometheusRule 配置

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: slo-burn-rate-alerts
  namespace: monitoring
spec:
  groups:
    - name: slo.burn_rate.rules
      rules:
        # 紧急: 14.4x burn rate (2天耗尽)
        - alert: SLO_BurnRate_Critical
          expr: |
            (
              sum(rate(http_requests_total{job="api-gateway", status=~"5.."}[5m]))
              /
              sum(rate(http_requests_total{job="api-gateway"}[5m]))
            ) > (14.4 * 0.001)
            and
            (
              sum(rate(http_requests_total{job="api-gateway", status=~"5.."}[1h]))
              /
              sum(rate(http_requests_total{job="api-gateway"}[1h]))
            ) > (14.4 * 0.001)
          for: 2m
          labels:
            severity: critical
            slo: api-gateway-availability
          annotations:
            summary: "🔥 SLO 紧急: API Gateway 错误率 Burn Rate 14.4x"
            description: "按当前速度，错误预算将在 2 天内耗尽"
            runbook: "https://runbooks.example.com/slo-critical"

        # 高: 6x burn rate (5天耗尽)
        - alert: SLO_BurnRate_High
          expr: |
            (
              sum(rate(http_requests_total{job="api-gateway", status=~"5.."}[30m]))
              /
              sum(rate(http_requests_total{job="api-gateway"}[30m]))
            ) > (6 * 0.001)
            and
            (
              sum(rate(http_requests_total{job="api-gateway", status=~"5.."}[6h]))
              /
              sum(rate(http_requests_total{job="api-gateway"}[6h]))
            ) > (6 * 0.001)
          for: 15m
          labels:
            severity: high
            slo: api-gateway-availability
          annotations:
            summary: "⚠️ SLO 高: API Gateway 错误率 Burn Rate 6x"

        # 中: 3x burn rate (10天耗尽)
        - alert: SLO_BurnRate_Medium
          expr: |
            (
              sum(rate(http_requests_total{job="api-gateway", status=~"5.."}[2h]))
              /
              sum(rate(http_requests_total{job="api-gateway"}[2h]))
            ) > (3 * 0.001)
            and
            (
              sum(rate(http_requests_total{job="api-gateway", status=~"5.."}[24h]))
              /
              sum(rate(http_requests_total{job="api-gateway"}[24h]))
            ) > (3 * 0.001)
          for: 1h
          labels:
            severity: medium
            slo: api-gateway-availability
          annotations:
            summary: "⚡ SLO 中: API Gateway 错误率 Burn Rate 3x"

        # 低: 1x burn rate (30天耗尽)
        - alert: SLO_BurnRate_Low
          expr: |
            (
              sum(rate(http_requests_total{job="api-gateway", status=~"5.."}[6h]))
              /
              sum(rate(http_requests_total{job="api-gateway"}[6h]))
            ) > (1 * 0.001)
            and
            (
              sum(rate(http_requests_total{job="api-gateway", status=~"5.."}[3d]))
              /
              sum(rate(http_requests_total{job="api-gateway"}[3d]))
            ) > (1 * 0.001)
          for: 6h
          labels:
            severity: low
            slo: api-gateway-availability
          annotations:
            summary: "📊 SLO 低: API Gateway 错误率持续偏高"
```

## SLO Dashboard 设计

### Grafana Dashboard 布局

```
┌─────────────────────────────────────────────────────────────────┐
│  SLO Overview - API Gateway                                     │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │
│  │ 可用性      │  │ 错误预算    │  │ Burn Rate   │           │
│  │ 99.95%      │  │ 剩余 68%    │  │ 1.2x        │           │
│  │ 目标 99.9%  │  │ 本月已用 32%│  │ 状态: 正常  │           │
│  └─────────────┘  └─────────────┘  └─────────────┘           │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ 错误率趋势 (30天)                                       │   │
│  │ [============================]                          │   │
│  │ SLO 目标线: 0.1%                                        │   │
│  └─────────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────────┤
│  ┌────────────────────────┐  ┌────────────────────────┐        │
│  │ 延迟分布 (P50/P95/P99) │  │ 错误预算消耗趋势       │        │
│  │ [====================] │  │ [====================] │        │
│  └────────────────────────┘  └────────────────────────┘        │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ 按服务分解的错误率                                       │   │
│  │ payment-api:    0.05% ✅                                │   │
│  │ order-service:  0.12% ⚠️                                │   │
│  │ user-service:   0.03% ✅                                │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

### 关键面板 PromQL

| 面板 | PromQL | 用途 |
|-----|--------|------|
| 当前可用性 | `1 - sum(rate(http_requests_total{status=~"5.."}[5m])) / sum(rate(http_requests_total[5m]))` | 实时可用性 |
| 错误预算剩余 | `1 - (sum(increase(http_requests_total{status=~"5.."}[30d])) / sum(increase(http_requests_total[30d]))) / 0.001` | 预算剩余比例 |
| Burn Rate (1h) | `sum(rate(http_requests_total{status=~"5.."}[1h])) / sum(rate(http_requests_total[1h])) / 0.001` | 1小时燃烧率 |
| P99 延迟 | `histogram_quantile(0.99, sum(rate(http_request_duration_seconds_bucket[5m])) by (le))` | 延迟分位值 |

## SLO 与发布决策集成

### 发布门控流程

```
发布请求 → 检查错误预算 → 预算 > 25%? → 允许发布
                              │
                              └─ 预算 < 25%? → 仅允许 Sev1 修复
                                             │
                                             └─ 预算 < 0%? → 全面冻结
```

### CI/CD 集成示例

```yaml
# GitHub Actions 发布门控
name: Release Gate
on:
  pull_request:
    types: [opened, synchronize]

jobs:
  slo-gate:
    runs-on: ubuntu-latest
    steps:
      - name: Check Error Budget
        run: |
          # 查询错误预算剩余
          BUDGET_REMAINING=$(curl -s 'http://prometheus:9090/api/v1/query?query=1-(sum(increase(http_requests_total{status=~"5.."}[30d]))/sum(increase(http_requests_total[30d])))/0.001' | jq -r '.data.result[0].value[1]')
          
          echo "错误预算剩余: ${BUDGET_REMAINING}%"
          
          if (( $(echo "$BUDGET_REMAINING < 0" | bc -l) )); then
            echo "❌ 错误预算已耗尽，禁止发布"
            exit 1
          elif (( $(echo "$BUDGET_REMAINING < 25" | bc -l) )); then
            # 检查是否为 Sev1 修复
            if [[ "$PR_TITLE" != *"[Sev1]"* ]]; then
              echo "⚠️ 错误预算 < 25%，仅允许 Sev1 修复"
              exit 1
            fi
          fi
          echo "✅ 发布门控通过"
```

## SLO 自动化治理

### SLO 即代码 (SLO-as-Code)

```yaml
# SLO 定义文件 (slo.yaml)
apiVersion: slo.kudig.io/v1
kind: ServiceLevelObjective
metadata:
  name: api-gateway-availability
  namespace: production
  labels:
    service: api-gateway
    tier: "0"
spec:
  description: "API Gateway 可用性 SLO"
  service: api-gateway
  indicator:
    type: availability
    goodEvents:
      metric: http_requests_total
      matchers:
        status: "!~5.."
    totalEvents:
      metric: http_requests_total
  objective: 99.9  # 99.9% 可用性
  window: 30d      # 30 天滚动窗口
  alerts:
    - name: fast-burn
      burnRate: 14.4
      shortWindow: 5m
      longWindow: 1h
      severity: critical
    - name: slow-burn
      burnRate: 3
      shortWindow: 2h
      longWindow: 24h
      severity: medium
```

### SLO 治理 CronJob

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: slo-governance-report
  namespace: monitoring
spec:
  schedule: "0 9 * * 1"  # 每周一 9:00
  jobTemplate:
    spec:
      template:
        spec:
          restartPolicy: OnFailure
          containers:
            - name: reporter
              image: bitnami/kubectl:latest
              command:
                - /bin/sh
                - -c
                - |
                  echo "=== SLO 周度治理报告 $(date) ==="
                  
                  # 1. 各服务 SLO 达成率
                  echo "[1] SLO 达成率:"
                  for slo in api-gateway payment-service order-service; do
                    AVAILABILITY=$(curl -s "http://prometheus:9090/api/v1/query?query=1-sum(rate(http_requests_total{job=\"$slo\",status=~'5..'}[7d]))/sum(rate(http_requests_total{job=\"$slo\"}[7d]))" | jq -r '.data.result[0].value[1]')
                    echo "  $slo: ${AVAILABILITY}"
                  done
                  
                  # 2. 错误预算消耗 Top 3
                  echo "[2] 错误预算消耗 Top 3:"
                  # 查询逻辑...
                  
                  # 3. 发送报告到 Slack
                  curl -X POST -H 'Content-type: application/json' \
                    --data "{\"text\":\"📊 SLO 周报已生成\"}" \
                    $SLACK_WEBHOOK
```

## 常见反模式与修复

| 反模式 | 症状 | 修复方案 |
|-------|------|----------|
| **SLO 过高** | 团队疑于奔命，无法发布新功能 | 基于历史数据重新设定，从 P90 开始 |
| **SLO 过低** | 经常达标但用户投诉多 | 收紧 SLO，对齐用户期望 |
| **无 SLI 数据** | 无法计算 SLO 达成率 | 先部署监控，采集 30 天数据 |
| **告警疲劳** | Burn Rate 告警太多被忽略 | 使用多窗口策略，减少误报 |
| **SLO 无主** | 无人关注 SLO 达成情况 | 明确指定 SLO Owner |
| **只看不用** | SLO 只是仪表盘装饰 | 集成到发布门控和绩效评估 |
| **单点指标** | 只看可用性，忽略延迟 | 多维度 SLI：可用性+延迟+错误率 |

## 相关链接

- [[observability-stack-evolution]] — 可观测性技术栈演进
- [[19-故障诊断/03-基础设施排障/06-monitoring-alerting-troubleshooting.md|monitoring-alerting-troubleshooting]] — 监控告警问题排查
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/skills/training-lecturer/11-workloads/index|chaos-engineering-guide]] — 混沌工程实践
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/skills/training-lecturer/11-workloads/index|capacity-planning-guide]] — 容量规划指南

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
