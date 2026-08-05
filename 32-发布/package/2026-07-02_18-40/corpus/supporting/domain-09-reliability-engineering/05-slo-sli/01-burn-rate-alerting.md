---
title: Burn Rate 告警与预算消耗监控
description: '# Burn Rate 告警与预算消耗监控'
summary: 'Burn Rate | 30天窗口耗尽时间 | 28天窗口耗尽时间 | 告警级别'
category: domain
tags:
- sre
- slo
- burn-rate
- alerting
- monitoring
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 35min
intent_queries:
- Burn Rate 告警与预算消耗监控 是什么
- 如何 Burn Rate 告警与预算消耗监控
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- Burn
- Rate
- 告警与预算消耗监控
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
- prometheus-basics
- alertmanager-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Burn Rate 告警与预算消耗监控

> Burn Rate 回答"以当前速度，错误预算将在多久后耗尽？"

## Burn Rate 方法论详解

### 什么是 Burn Rate

**Burn Rate（燃烧率）** 是 Google SRE Workbook 第 5 章提出的核心概念，用于衡量错误预算的消耗速度。它将复杂的错误预算管理简化为一个易于理解和监控的指标。

```
Burn Rate = 当前错误率 / SLO 允许的错误率

其中:
  当前错误率 = 错误请求数 / 总请求数（在特定时间窗口内）
  SLO 允许的错误率 = 1 - SLO

示例:
  SLO = 99.9% → 允许错误率 = 0.1% = 0.001
  过去 1 小时错误率 = 0.0144 (1.44%)
  
  Burn Rate = 0.0144 / 0.001 = 14.4x
  
  这意味着当前错误率是正常允许值的 14.4 倍
```

### Burn Rate 与耗尽时间的关系

```
Burn Rate 与预算耗尽时间的对应关系:

Burn Rate | 30天窗口耗尽时间 | 28天窗口耗尽时间 | 告警级别
----------|----------------|----------------|--------
1x        | 30 天          | 28 天          | 正常（基线）
2x        | 15 天          | 14 天          | 提醒
6x        | 5 天           | 4.7 天         | 警告
14.4x     | 2 天           | 1.9 天         | 严重
60x       | 12 小时        | 11.2 小时      | 紧急
720x      | 1 小时         | 56 分钟        | 灾难

计算公式:
  耗尽时间 = 评估窗口 / Burn Rate
  
  示例: Burn Rate = 14.4x, 窗口 = 30 天
  耗尽时间 = 30 / 14.4 = 2.08 天
```

### 为什么使用 Burn Rate

传统错误预算监控的问题：

```
❌ 传统方法: "本月已发生 5,000 次错误"
  → 不知道这是快还是慢
  → 不知道还剩多少时间
  → 告警阈值难以设定

✅ Burn Rate: "当前燃烧率 14.4x，预算将在 2 天内耗尽"
  → 直观的速度概念
  → 可预测的耗尽时间
  → 与业务节奏对齐的告警级别
```

### Burn Rate 的数学推导

```
设:
  S = SLO 目标值 (如 0.999)
  E = 允许错误率 = 1 - S (如 0.001)
  e(t) = 时间 t 内的实际错误率
  T = 评估窗口 (如 30 天)

Burn Rate R = e(t) / E

错误预算消耗比例:
  Consumed = (e(T) - E) / E

如果错误率保持恒定:
  预算将在 T/R 时间后耗尽
  
  验证: R = 1 时，T/R = T，正好用完窗口
        R = 2 时，T/R = T/2，一半时间用完预算
        R = 14.4 时，T/R = T/14.4 ≈ 2 天

多窗口验证:
  要使告警准确，短窗口的 Burn Rate 必须与长窗口一致
  
  即: e(short) / E = e(long) / E
  
  这要求问题在短窗口内有足够的"存在感"
```

## 多窗口多燃烧率告警规则

### Google SRE Workbook 推荐配置

Google SRE Workbook 第 5 章推荐的经典配置使用**两个燃烧率 × 两个时间窗口**的组合：

```
告警配置矩阵:

燃烧率  | 短窗口 | 长窗口  | 用途        | 通知方式
--------|--------|--------|------------|----------
14.4x   | 1h     | 1h      | 快速燃尽    | Page/电话
14.4x   | 5m     | 1h      | 极快燃尽    | Page/电话
6x      | 6h     | 3d      | 中速燃尽    | 工单/Slack
2x      | 3d     | 3d      | 慢速燃尽    | 邮件/日报

为什么需要长窗口?
  → 防止短窗口内的随机波动触发误告警
  → 确保问题是持续性的，不是瞬时的
  
为什么需要短窗口?
  → 快速检测严重问题
  → 提供及时的干预机会
```

### 快速燃尽 (Fast Burn) vs 慢速燃尽 (Slow Burn)

#### Fast Burn（快速燃尽）

```
特征:
  - 错误率在短时间内急剧上升
  - 通常在几分钟到几小时内耗尽大量预算
  - 一般由严重问题引起（部署 bug、基础设施问题）

示例:
  14:00 发布新版本 v2.5
  14:05 错误率从 0.01% 上升到 5%
  14:10 Burn Rate = 5% / 0.1% = 50x
  14:30 已消耗 30% 的月度预算
  
  如果没有快速告警，2 小时内预算将全部耗尽

应对:
  - 立即回滚或前滚修复
  - 启动事故响应流程
  - 快速止损优先于根因分析
```

#### Slow Burn（慢速燃尽）

```
特征:
  - 错误率在较长时间内略高于正常水平
  - 可能需要数天或数周才能耗尽预算
  - 一般由性能退化、资源泄漏、缓慢增长引起

示例:
  本周错误率从 0.01% 缓慢上升到 0.05%
  Burn Rate = 0.05% / 0.1% = 0.5x（低于 1，不触发告警）
  
  但 3 天后:
  错误率上升到 0.2%
  Burn Rate = 0.2% / 0.1% = 2x
  预算将在 15 天内耗尽
  
  这种缓慢恶化容易被忽视，但累积影响严重

应对:
  - 排期修复，不紧急但重要
  - 深入分析根因（通常是架构或代码问题）
  - 防止演变成快速燃尽
```

### 多窗口多燃烧率告警的数学原理

```
核心问题: 如何确保告警既不漏报也不误报？

使用两个窗口的 AND 条件:
  - 短窗口满足燃烧率: 确认当前确实有高错误率
  - 长窗口满足燃烧率: 确认这不是随机波动

示例配置: 14.4x burn rate, 短窗口 5m, 长窗口 1h

PromQL:
  (
    sum(rate(http_requests_total{status=~"5.."}[5m]))
    / sum(rate(http_requests_total[5m]))
    > 14.4 * 0.001
  )
  AND
  (
    sum(rate(http_requests_total{status=~"5.."}[1h]))
    / sum(rate(http_requests_total[1h]))
    > 14.4 * 0.001
  )

为什么有效:
  场景 A: 5 分钟突发（如 GC 暂停）
    → 短窗口触发，但长窗口不触发 → 不告警 ✅
    
  场景 B: 持续 1 小时问题
    → 两个窗口都触发 → 告警 ✅
    
  场景 C: 正常波动
    → 两个窗口都不触发 → 不告警 ✅
```

## 告警规则完整 YAML 配置

### [[Prometheus|Prometheus]] Alertmanager 完整配置

```yaml
# slo-burn-rate-alerts.yaml
groups:
  # ==================== 快速燃尽告警 (Fast Burn) ====================
  - name: slo_fast_burn
    interval: 30s
    rules:
      # CRITICAL: 14.4x burn rate — 预算将在 ~2 天内耗尽
      # 双窗口验证: 1h 短窗口 + 1h 长窗口（相同，用于快速检测持续问题）
      - alert: SLOFastBurnCritical
        expr: |
          (
            sum(rate(http_requests_total{status=~"5.."}[1h])) by (service, namespace)
            / sum(rate(http_requests_total[1h])) by (service, namespace)
          ) > 14.4 * 0.001
        for: 2m
        labels:
          severity: critical
          slo_window: "30d"
          burn_rate: "14.4x"
          team: "{{ $labels.service }}-team"
        annotations:
          summary: "{{ $labels.service }} 快速燃尽 — 预算将在 2 天内耗尽"
          description: |
            服务 {{ $labels.namespace }}/{{ $labels.service }} 的 1h 错误率为 {{ $value | humanizePercentage }}，
            超过 SLO 允许错误率的 14.4 倍。
            
            当前 Burn Rate: 14.4x
            预计预算耗尽: ~2 天
            SLO: 99.9% (允许错误率 0.1%)
            
            建议立即:
            1. 检查最近是否有发布
            2. 查看依赖服务状态
            3. 准备回滚方案
          dashboard: "https://grafana.example.com/d/slo-dashboard?var-service={{ $labels.service }}"
          runbook_url: "https://wiki.example.com/runbooks/slo-fast-burn"

      # CRITICAL: 60x burn rate — 预算将在 12 小时内耗尽
      # 用于检测灾难性问题
      - alert: SLOFastBurnDisaster
        expr: |
          (
            sum(rate(http_requests_total{status=~"5.."}[5m])) by (service, namespace)
            / sum(rate(http_requests_total[5m])) by (service, namespace)
          ) > 60 * 0.001
        for: 1m
        labels:
          severity: critical
          slo_window: "30d"
          burn_rate: "60x"
          team: "{{ $labels.service }}-team"
        annotations:
          summary: "{{ $labels.service }} 灾难性燃尽 — 预算将在 12 小时内耗尽"
          description: |
            服务 {{ $labels.namespace }}/{{ $labels.service }} 的 5m 错误率为 {{ $value | humanizePercentage }}，
            Burn Rate 达到 60x。
            
            预计预算耗尽: ~12 小时
            
            立即行动:
            1. 启动事故响应流程
            2. 检查是否可立即回滚
            3. 通知团队负责人
          dashboard: "https://grafana.example.com/d/slo-dashboard?var-service={{ $labels.service }}"

  # ==================== 中速燃尽告警 (Medium Burn) ====================
  - name: slo_medium_burn
    interval: 60s
    rules:
      # WARNING: 6x burn rate — 预算将在 ~5 天内耗尽
      # 双窗口验证: 6h 短窗口 + 3d 长窗口
      - alert: SLOMediumBurnWarning
        expr: |
          (
            (
              sum(rate(http_requests_total{status=~"5.."}[6h])) by (service, namespace)
              / sum(rate(http_requests_total[6h])) by (service, namespace)
            ) > 6 * 0.001
          )
          and
          (
            (
              sum(rate(http_requests_total{status=~"5.."}[3d])) by (service, namespace)
              / sum(rate(http_requests_total[3d])) by (service, namespace)
            ) > 6 * 0.001
          )
        for: 5m
        labels:
          severity: warning
          slo_window: "30d"
          burn_rate: "6x"
          team: "{{ $labels.service }}-team"
        annotations:
          summary: "{{ $labels.service }} 中速燃尽 — 预算将在 5 天内耗尽"
          description: |
            服务 {{ $labels.namespace }}/{{ $labels.service }}:
            - 6h 错误率: {{ $value | humanizePercentage }}
            - 3d 错误率同样超过阈值
            - Burn Rate: 6x
            - 预计预算耗尽: ~5 天
            
            建议:
            1. 排查最近是否有性能退化
            2. 检查资源使用趋势
            3. 安排修复计划
          dashboard: "https://grafana.example.com/d/slo-dashboard?var-service={{ $labels.service }}"

  # ==================== 慢速燃尽告警 (Slow Burn) ====================
  - name: slo_slow_burn
    interval: 300s
    rules:
      # INFO: 2x burn rate — 预算将在 ~15 天内耗尽
      # 使用 3d 窗口检测慢速趋势
      - alert: SLOSlowBurnInfo
        expr: |
          (
            sum(rate(http_requests_total{status=~"5.."}[3d])) by (service, namespace)
            / sum(rate(http_requests_total[3d])) by (service, namespace)
          ) > 2 * 0.001
        for: 15m
        labels:
          severity: info
          slo_window: "30d"
          burn_rate: "2x"
          team: "{{ $labels.service }}-team"
        annotations:
          summary: "{{ $labels.service }} 慢速燃尽 — 预算将在 15 天内耗尽"
          description: |
            服务 {{ $labels.namespace }}/{{ $labels.service }} 的 3d 错误率为 {{ $value | humanizePercentage }}，
            Burn Rate 为 2x。
            
            预计预算耗尽: ~15 天
            
            建议本周内安排排查:
            1. 查看错误日志趋势
            2. 检查最近变更
            3. 评估是否需要调整发布计划
          dashboard: "https://grafana.example.com/d/slo-dashboard?var-service={{ $labels.service }}"

  # ==================== 预算耗尽告警 ====================
  - name: slo_budget_exhausted
    interval: 300s
    rules:
      # 错误预算已耗尽或超支
      - alert: SLOBudgetExhausted
        expr: |
          (
            (
              sum(rate(http_requests_total{status=~"5.."}[30d])) by (service, namespace)
              / sum(rate(http_requests_total[30d])) by (service, namespace)
            ) - 0.001
          ) / 0.001 >= 1.0
        for: 5m
        labels:
          severity: critical
          slo_window: "30d"
          team: "{{ $labels.service }}-team"
        annotations:
          summary: "{{ $labels.service }} 错误预算已耗尽"
          description: |
            服务 {{ $labels.namespace }}/{{ $labels.service }} 的 30 天错误预算已完全耗尽。
            
            当前 30d 错误率: {{ $value | humanizePercentage }}
            允许错误率: 0.1%
            
            强制执行:
            1. 所有非紧急发布已冻结
            2. 启动事后复盘流程
            3. 技术负责人必须在 24h 内提交恢复计划

      # 错误预算即将耗尽（75%）
      - alert: SLOBudgetNearlyExhausted
        expr: |
          (
            (
              sum(rate(http_requests_total{status=~"5.."}[30d])) by (service, namespace)
              / sum(rate(http_requests_total[30d])) by (service, namespace)
            ) - 0.001
          ) / 0.001 >= 0.75
        for: 10m
        labels:
          severity: warning
          slo_window: "30d"
          team: "{{ $labels.service }}-team"
        annotations:
          summary: "{{ $labels.service }} 错误预算已消耗 75%"
          description: |
            服务 {{ $labels.namespace }}/{{ $labels.service }} 的 30 天错误预算已消耗超过 75%。
            
            建议:
            1. 暂停非紧急发布
            2. 评估是否需要调整 SLO
            3. 安排剩余预算的使用计划

  # ==================== 延迟类 SLO Burn Rate 告警 ====================
  - name: slo_latency_burn
    interval: 60s
    rules:
      # 延迟 SLO 的快速燃尽
      # 定义: 延迟超过 SLO 阈值的请求比例
      - alert: SLOLatencyFastBurn
        expr: |
          (
            sum(rate(http_request_duration_seconds_bucket{le="0.5"}[1h])) by (service, namespace)
            / sum(rate(http_request_duration_seconds_count[1h])) by (service, namespace)
          ) < 0.99
        for: 5m
        labels:
          severity: warning
          slo_type: "latency"
          team: "{{ $labels.service }}-team"
        annotations:
          summary: "{{ $labels.service }} P99 延迟 SLO 快速燃尽"
          description: |
            服务 {{ $labels.namespace }}/{{ $labels.service }} 的 1h P99 延迟达标率低于 99%。
            
            当前达标率: {{ $value | humanizePercentage }}
            SLO: P99 < 500ms
            
            建议检查:
            1. 数据库查询性能
            2. 缓存命中率
            3. 下游服务延迟

  # ==================== 基础设施 Burn Rate 告警 ====================
  - name: slo_infrastructure_burn
    interval: 30s
    rules:
      # API Server 错误率燃尽
      - alert: SLOApiserverBurnRate
        expr: |
          (
            sum(rate(apiserver_request_total{code=~"5.."}[5m]))
            / sum(rate(apiserver_request_total[5m]))
          ) > 10 * 0.001
        for: 2m
        labels:
          severity: critical
          component: "apiserver"
          team: "platform-team"
        annotations:
          summary: "API Server 错误率异常升高"
          description: |
            API Server 的 5m 错误率为 {{ $value | humanizePercentage }}。
            这可能是 etcd 问题或 apiserver 过载。
            
            立即检查:
            1. etcd 健康状态
            2. apiserver 资源使用
            3. 最近是否有大量 LIST 请求

      # etcd WAL fsync 延迟燃尽
      - alert: SLOEtcdDiskBurnRate
        expr: |
          histogram_quantile(0.99,
            sum(rate(etcd_disk_wal_fsync_duration_seconds_bucket[5m])) by (le)
          ) > 0.01
        for: 3m
        labels:
          severity: critical
          component: "etcd"
          team: "platform-team"
        annotations:
          summary: "etcd 磁盘 fsync 延迟超过 10ms"
          description: |
            etcd WAL fsync P99 延迟为 {{ $value }}s，超过 10ms 阈值。
            这可能导致 API Server 响应延迟和请求超时。
            
            检查:
            1. 磁盘 I/O 性能（是否使用 SSD）
            2. 是否有其他进程占用磁盘
            3. etcd 数据目录是否需要整理
```

### Alertmanager 路由配置

```yaml
# alertmanager-config.yaml
global:
  smtp_smarthost: 'smtp.example.com:587'
  smtp_from: 'alerts@example.com'
  slack_api_url: 'https://hooks.slack.com/services/XXX/YYY/ZZZ'
  pagerduty_url: 'https://events.pagerduty.com/v2/enqueue'

route:
  group_by: ['alertname', 'service', 'namespace']
  group_wait: 30s
  group_interval: 5m
  repeat_interval: 4h
  receiver: default
  routes:
    # SLO 相关告警路由
    - matchers:
      - severity="critical"
      receiver: slo-critical
      group_wait: 10s
      repeat_interval: 30m
      continue: true
    - matchers:
      - severity="warning"
      receiver: slo-warning
      group_wait: 1m
      repeat_interval: 2h
      continue: true
    - matchers:
      - severity="info"
      receiver: slo-info
      group_wait: 5m
      repeat_interval: 24h
receivers:
  - name: default
    slack_configs:
      - channel: '#alerts'
        send_resolved: true

  - name: slo-critical
    pagerduty_configs:
      - routing_key: '<PAGERDUTY-SERVICE-KEY>'
        severity: critical
        description: '{{ .GroupLabels.service }}: {{ .CommonAnnotations.summary }}'
    slack_configs:
      - channel: '#slo-critical'
        send_resolved: true
        title: '🚨 SLO CRITICAL: {{ .GroupLabels.service }}'
        text: |
          {{ range .Alerts }}
          *Service:* {{ .Labels.service }}
          *Burn Rate:* {{ .Labels.burn_rate }}
          *Summary:* {{ .Annotations.summary }}
          *Dashboard:* {{ .Annotations.dashboard }}
          {{ end }}

  - name: slo-warning
    slack_configs:
      - channel: '#slo-alerts'
        send_resolved: true
        title: '⚠️ SLO WARNING: {{ .GroupLabels.service }}'
        text: |
          {{ range .Alerts }}
          *Service:* {{ .Labels.service }}
          *Burn Rate:* {{ .Labels.burn_rate }}
          *Summary:* {{ .Annotations.summary }}
          *Dashboard:* {{ .Annotations.dashboard }}
          {{ end }}

  - name: slo-info
    email_configs:
      - to: 'sre-team@example.com'
        subject: 'SLO Info: {{ .GroupLabels.service }}'
        body: |
          {{ range .Alerts }}
          Service: {{ .Labels.service }}
          Summary: {{ .Annotations.summary }}
          {{ end }}
```

## 实战案例：某服务 30 天 SLO 的 Burn Rate 告警配置

### 场景设定

```yaml
# 服务: payment-service
# SLO: 99.9% 可用性（允许错误率 0.1%）
# 评估窗口: 30 天
# 日均请求: 2,000,000
# 月度总请求: 60,000,000
# 月度错误预算: 60,000 次

# 服务依赖:
#   payment-service → payment-gateway (第三方)
#                 → redis-cluster (缓存)
#                 → postgres-primary (数据库)
```

### 完整告警规则

```yaml
# payment-service-slo-alerts.yaml
groups:
  - name: payment_service_slo
    interval: 30s
    rules:
      # ========== FAST BURN: 14.4x ==========
      # 1h 窗口，双条件验证
      - alert: PaymentServiceFastBurn
        expr: |
          (
            (
              sum(rate(http_requests_total{service="payment-service",status=~"5.."}[1h]))
              / sum(rate(http_requests_total{service="payment-service"}[1h]))
            ) > 14.4 * 0.001
          )
          and
          (
            (
              sum(rate(http_requests_total{service="payment-service",status=~"5.."}[5m]))
              / sum(rate(http_requests_total{service="payment-service"}[5m]))
            ) > 14.4 * 0.001
          )
        for: 2m
        labels:
          severity: critical
          service: payment-service
          burn_rate: "14.4x"
          team: payments-team
        annotations:
          summary: "支付服务快速燃尽 — 2 天内耗尽预算"
          description: |
            支付服务当前错误率 {{ $value | humanizePercentage }}，
            Burn Rate 14.4x，预算将在 2 天内耗尽。
            
            当前 1h 错误率已超过 SLO 允许值的 14.4 倍。
            
            立即行动:
            1. 检查 payment-gateway 状态
            2. 查看最近是否有发布
            3. 准备回滚到上一个稳定版本
            
            Dashboard: https://grafana.example.com/d/payment-slo
            Runbook: https://wiki.example.com/runbooks/payment-fast-burn
          runbook_url: "https://wiki.example.com/runbooks/payment-fast-burn"

      # ========== FAST BURN: 60x (灾难级) ==========
      - alert: PaymentServiceDisasterBurn
        expr: |
          (
            sum(rate(http_requests_total{service="payment-service",status=~"5.."}[5m]))
            / sum(rate(http_requests_total{service="payment-service"}[5m]))
          ) > 60 * 0.001
        for: 1m
        labels:
          severity: critical
          service: payment-service
          burn_rate: "60x"
          team: payments-team
        annotations:
          summary: "支付服务灾难级燃尽 — 12 小时内耗尽预算"
          description: |
            🚨 紧急！支付服务 Burn Rate 达到 60x！
            
            当前 5m 错误率: {{ $value | humanizePercentage }}
            预算将在 ~12 小时内完全耗尽。
            
            立即执行:
            1. 启动 P0 事故响应
            2. 检查 payment-gateway 可用性
            3. 如果无法快速修复，立即回滚
            4. 通知支付业务负责人

      # ========== MEDIUM BURN: 6x ==========
      - alert: PaymentServiceMediumBurn
        expr: |
          (
            (
              sum(rate(http_requests_total{service="payment-service",status=~"5.."}[6h]))
              / sum(rate(http_requests_total{service="payment-service"}[6h]))
            ) > 6 * 0.001
          )
          and
          (
            (
              sum(rate(http_requests_total{service="payment-service",status=~"5.."}[3d]))
              / sum(rate(http_requests_total{service="payment-service"}[3d]))
            ) > 6 * 0.001
          )
        for: 5m
        labels:
          severity: warning
          service: payment-service
          burn_rate: "6x"
          team: payments-team
        annotations:
          summary: "支付服务中速燃尽 — 5 天内耗尽预算"
          description: |
            支付服务 Burn Rate 6x，预算将在 5 天内耗尽。
            
            6h 错误率: {{ $value | humanizePercentage }}
            3d 错误率同样超过阈值。
            
            建议今天内:
            1. 检查数据库连接池状态
            2. 查看 Redis 缓存命中率
            3. 检查 payment-gateway 的延迟趋势
            4. 安排修复排期

      # ========== SLOW BURN: 2x ==========
      - alert: PaymentServiceSlowBurn
        expr: |
          (
            sum(rate(http_requests_total{service="payment-service",status=~"5.."}[3d]))
            / sum(rate(http_requests_total{service="payment-service"}[3d]))
          ) > 2 * 0.001
        for: 15m
        labels:
          severity: info
          service: payment-service
          burn_rate: "2x"
          team: payments-team
        annotations:
          summary: "支付服务慢速燃尽 — 15 天内耗尽预算"
          description: |
            支付服务 Burn Rate 2x，预算将在 15 天内耗尽。
            
            3d 错误率: {{ $value | humanizePercentage }}
            
            建议本周内:
            1. 分析错误日志模式
            2. 检查是否有资源泄漏
            3. 评估发布计划是否需要调整

      # ========== 预算耗尽告警 ==========
      - alert: PaymentServiceBudgetExhausted
        expr: |
          (
            (
              sum(rate(http_requests_total{service="payment-service",status=~"5.."}[30d]))
              / sum(rate(http_requests_total{service="payment-service"}[30d]))
            ) - 0.001
          ) / 0.001 >= 1.0
        for: 5m
        labels:
          severity: critical
          service: payment-service
          team: payments-team
        annotations:
          summary: "支付服务错误预算已耗尽"
          description: |
            支付服务 30 天错误预算已完全耗尽。
            
            当前 30d 错误率: {{ $value | humanizePercentage }}
            
            强制执行:
            1. ❌ 所有非紧急发布已冻结
            2. 📋 启动无责事后复盘
            3. 👤 技术负责人 24h 内提交恢复计划
            4. 📊 复盘会议安排在 48h 内

      # ========== 预算 75% 告警 ==========
      - alert: PaymentServiceBudget75Percent
        expr: |
          (
            (
              sum(rate(http_requests_total{service="payment-service",status=~"5.."}[30d]))
              / sum(rate(http_requests_total{service="payment-service"}[30d]))
            ) - 0.001
          ) / 0.001 >= 0.75
        for: 10m
        labels:
          severity: warning
          service: payment-service
          team: payments-team
        annotations:
          summary: "支付服务错误预算已消耗 75%"
          description: |
            支付服务 30 天错误预算已消耗超过 75%。
            
            剩余预算不足 25%，请:
            1. 暂停非关键发布
            2. 重新评估本月发布计划
            3. 关注任何可能消耗预算的变更

      # ========== 依赖服务 Burn Rate 监控 ==========
      - alert: PaymentServiceDependencyBurn
        expr: |
          (
            sum(rate(redis_commands_total{service="payment-service",status="failed"}[1h]))
            / sum(rate(redis_commands_total{service="payment-service"}[1h]))
          ) > 10 * 0.001
        for: 3m
        labels:
          severity: warning
          service: payment-service
          dependency: redis
          team: payments-team
        annotations:
          summary: "支付服务 Redis 依赖错误率升高"
          description: |
            支付服务到 Redis 的 1h 错误率: {{ $value | humanizePercentage }}
            这可能影响支付缓存和会话状态。
            
            检查:
            1. Redis Cluster 节点状态
            2. 网络连接质量
            3. 内存使用是否接近上限
```

### 模拟演练：不同问题场景下的告警触发

```
场景 1: 发布引入 bug，导致 5% 错误率持续 30 分钟
─────────────────────────────────────────────────

14:00 发布 v2.5
14:05 错误率上升到 5%
14:06 PaymentServiceDisasterBurn (60x) 触发
  → Burn Rate = 5% / 0.1% = 50x (接近 60x 阈值)
  → 1 分钟内连续触发，PagerDuty 通知 on-call
  
14:07 PaymentServiceFastBurn (14.4x) 触发
  → 1h 窗口和 5m 窗口都满足条件
  → Slack #slo-critical 发送告警

14:08 on-call 开始回滚
14:15 回滚完成，错误率下降到 0.01%
14:16 PaymentServiceDisasterBurn 恢复
14:17 PaymentServiceFastBurn 恢复

预算消耗:
  问题期间请求: 2,000,000 × (30/1440) = 41,667 次
  错误请求: 41,667 × 5% = 2,083 次
  预算消耗: 2,083 / 60,000 = 3.5%
  
  → 一次快速止损避免了 50% 以上的预算消耗

─────────────────────────────────────────────────

场景 2: 数据库连接池泄漏，错误率从 0.01% 缓慢上升到 0.15%
─────────────────────────────────────────────────

Day 1: 错误率 0.01% (正常)
Day 3: 错误率上升到 0.05%
  → Burn Rate = 0.05% / 0.1% = 0.5x (不触发)
  
Day 5: 错误率上升到 0.12%
  → Burn Rate = 0.12% / 0.1% = 1.2x (不触发)
  
Day 7: 错误率上升到 0.22%
  → 3d 窗口错误率 = 0.22%
  → Burn Rate = 0.22% / 0.1% = 2.2x
  → PaymentServiceSlowBurn (2x) 触发
  → Slack #slo-alerts 发送 info 级别告警
  
Day 8: 开发团队排查，发现连接池泄漏
Day 9: 修复部署，错误率回到 0.01%
Day 12: PaymentServiceSlowBurn 恢复

预算消耗:
  7 天错误总数 ≈ 14,000,000 × 0.15% ≈ 21,000 次
  预算消耗: 21,000 / 60,000 = 35%
  
  → Slow Burn 告警及时提醒，避免了预算耗尽

─────────────────────────────────────────────────

场景 3: 第三方支付网关间歇性问题，错误率波动
─────────────────────────────────────────────────

10:00 网关问题，错误率 2%，持续 10 分钟
10:10 网关恢复，错误率正常

10:00-10:10:
  5m 窗口错误率 = 2%
  Burn Rate = 2% / 0.1% = 20x
  
  但 1h 窗口错误率 = 2% × (10/60) ≈ 0.33%
  Burn Rate (1h) = 0.33% / 0.1% = 3.3x
  
  → PaymentServiceDisasterBurn 可能触发（5m 窗口 > 60x? 20x < 60x，不触发）
  → PaymentServiceFastBurn 不触发（1h 窗口 3.3x < 14.4x）
  → 只有短暂的波动，不触发持续告警 ✅

预算消耗:
  10 分钟错误 ≈ 2,000,000 × (10/1440) × 2% ≈ 278 次
  预算消耗: 278 / 60,000 = 0.46%
  
  → 短暂问题，预算影响很小
```

### 故障注入验证 ([[domain-17-system-foundation/知识字典/operations/chaos-engineering.md|Chaos Engineering]])

```yaml
# burn-rate-chaos-experiment.yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: NetworkChaos
metadata:
  name: payment-service-latency
  namespace: chaos-testing
spec:
  action: delay
  mode: one
  selector:
    labelSelectors:
      app: payment-service
  delay:
    latency: "500ms"
    correlation: "100"
    jitter: "0ms"
  duration: "10m"
  
  # 预期结果:
  # - 延迟增加导致超时，错误率上升
  # - 5m 窗口 Burn Rate 应触发告警
  # - 验证告警在 2m 内到达 PagerDuty
  # - 验证 Grafana Dashboard 正确显示燃烧率
---
apiVersion: chaos-mesh.org/v1alpha1
kind: PodChaos
metadata:
  name: redis-pod-failure
  namespace: chaos-testing
spec:
  action: pod-failure
  mode: fixed-percent
  value: "50"
  selector:
    labelSelectors:
      app: redis
  duration: "15m"
  
  # 预期结果:
  # - 50% Redis 节点问题
  # - 支付服务缓存命中率下降
  # - 错误率上升，触发 Dependency Burn 告警
```

### Burn Rate 告警效果度量

```promql
# 度量告警有效性

# 1. 告警触发到恢复的平均时间 (MTTR)
(
  sum(avg_over_time(ALERTS{alertstate="firing",alertname=~"SLO.*Burn"}[30d]))
  / count(count_over_time(ALERTS{alertstate="firing",alertname=~"SLO.*Burn"}[30d]))
)

# 2. 告警覆盖的预算消耗比例
# （即告警触发时已经消耗的预算 / 总消耗预算）

# 3. 误告警率
# 告警触发但 1h 内自动恢复的比例
(
  count(ALERTS{alertstate="firing",alertname=~"SLO.*Burn"} unless ALERTS{alertstate="firing",alertname=~"SLO.*Burn"} offset 1h)
  / count(ALERTS{alertstate="firing",alertname=~"SLO.*Burn"})
)
```

## Burn Rate 告警调优指南

### 初始配置建议

```
第一次配置 Burn Rate 告警时，建议:

1. 先观察 2-4 周不告警
   → 收集基线数据
   → 了解正常波动范围
   → 确定误告警的源头

2. 从宽松的阈值开始
   → Fast Burn: 从 10x 开始，逐步调整到 14.4x
   → Slow Burn: 从 1.5x 开始，逐步调整到 2x

3. 使用"告警但不通知"模式
   → 配置告警规则
   → 但不发送通知（仅记录）
   → 验证告警准确性后再启用通知

4. 持续优化
   → 每周回顾告警触发情况
   → 记录每个告警的处置结果
   → 调整阈值减少误报/漏报
```

### 常见调优问题

| 问题 | 现象 | 解决方案 |
|------|------|---------|
| **误告警过多** | 正常波动触发告警 | 增加 `for` 持续时间；使用更长窗口；提高阈值 |
| **告警过晚** | 预算耗尽后才告警 | 降低阈值；缩短 `for` 时间；增加更多窗口 |
| **无法检测慢燃尽** | 预算缓慢耗尽无告警 | 添加 2x Slow Burn 告警；降低 Slow Burn 窗口 |
| **依赖问题未覆盖** | 下游问题未触发告警 | 添加依赖服务 Burn Rate 监控 |
| **延迟 SLO 未监控** | 只监控了可用性 | 添加延迟类 Burn Rate 规则 |

## 相关

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-09-reliability-engineering/02-slo-sli/03-error-budget-management|03 error budget management]] — 错误预算管理
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-09-reliability-engineering/04-sre-practices/01-release-gate-slo-based|02 release gate slo based]] — 基于 SLO 的发布门控
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-09-reliability-engineering/02-slo-sli/02-slo-implementation-guide|02 slo implementation guide]] — SLO 设定与实施指南


<!-- risk-assessed -->
