---
title: SLO 与监控系统的深度集成
description: '# SLO 与监控系统的深度集成'
summary: '# SLO 与监控系统的深度集成'
category: synthesis
tags:
- slo
- monitoring
- observability
- reliability
- alerting
- prometheus
- grafana
- ingress
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SLO 与监控系统的深度集成 是什么
- 如何 SLO 与监控系统的深度集成
trigger_keywords:
- SLO
- 与监控系统的深度集成
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
relationships:
- target: '[[20-最佳实践/01-best-practices/observability/monitoring.md]]'
  type: related_to
- target: '[[17-系统基础/06-知识字典/observability/observability.md]]'
  type: related_to
- target: '[[17-系统基础/05-速查卡/gitops.md]]'
  type: related_to
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# SLO 与监控系统的深度集成

## 概述

SLO（Service Level Objective）与监控系统的深度集成，是将可靠性目标从文档转化为可自动执行的工程实践。通过将 SLI（Service Level Indicator）指标标准化、错误预算自动计算、多窗口告警分级和 CI/CD 发布门控串联，SLO 不再是事后报告，而是驱动发布决策和告警策略的实时信号。

## 架构概览

```
用户请求 → Ingress → Service → Pod
                ↓
         Prometheus (SLI 指标采集)
                ↓
    ┌─────────────────────────────┐
    │   SLO 计算引擎               │
    │  - 错误预算 (Error Budget)   │
    │  - Burn Rate (消耗速率)      │
    │  - 多窗口聚合                │
    └─────────────────────────────┘
                ↓
    ┌─────────────────────────────┐
    │   Grafana 看板               │
    │   - SLO 达标率可视化         │
    │   - 错误预算趋势             │
    │                              │
    │   Alertmanager 告警          │
    │   - Fast Burn → Page         │
    │   - Slow Burn → Ticket       │
    │                              │
    │   CI/CD 发布门控             │
    │   - 预算不足时阻止发布        │
    └─────────────────────────────┘
```

## 集成要点

### 1. SLI 指标标准化

SLI 是 SLO 的量化基础，必须统一定义和命名：

```yaml
# SLI 定义标准（通过 Prometheus Recording Rules）
groups:
  - name: slo-metrics
    rules:
      # 可用性 SLI: 成功请求比例
      - record: slo:availability:ratio_rate5m
        expr: |
          sum(rate(http_requests_total{status!~"5.."}[5m])) by (service)
          /
          sum(rate(http_requests_total[5m])) by (service)

      # 延迟 SLI: P99 < 200ms 的请求比例
      - record: slo:latency_p99:ratio_rate5m
        expr: |
          sum(rate(http_request_duration_seconds_bucket{le="0.2"}[5m])) by (service)
          /
          sum(rate(http_request_duration_seconds_count[5m])) by (service)
```

统一标签规范：`service`, `slo_name`, `window`

### 2. 错误预算自动计算

```yaml
# 错误预算计算（假设 SLO = 99.9%）
groups:
  - name: error-budget
    rules:
      # 30 天错误预算消耗
      - record: slo:error_budget:remaining:30d
        expr: |
          1 - (
            (1 - slo:availability:ratio_rate30d) / 0.001
          )

      # Burn Rate: 当前错误消耗速率
      - record: slo:burn_rate:5m
        expr: |
          (1 - slo:availability:ratio_rate5m) / 0.001
```

### 3. 多窗口告警分级

Google SRE 推荐的多窗口 Burn Rate 告警策略：

```yaml
groups:
  - name: slo-alerts
    rules:
      # Fast Burn: 14.4x burn rate → 紧急 Page
      # 1h 内消耗 2% 的月预算
      - alert: SLOFastBurn
        expr: |
          (
            max(slo:burn_rate:5m) by (service) > 14.4
            and
            max(slo:burn_rate:1h) by (service) > 14.4
          )
        for: 2m
        labels:
          severity: page                # 触发 PagerDuty
          slo_alert: fast_burn

      # Slow Burn: 6x burn rate → 创建 Ticket
      # 6h 内消耗 5% 的月预算
      - alert: SLOSlowBurn
        expr: |
          (
            max(slo:burn_rate:30m) by (service) > 6
            and
            max(slo:burn_rate:6h) by (service) > 6
          )
        for: 15m
        labels:
          severity: ticket              # 创建工单
          slo_alert: slow_burn
```

### 4. 发布门控

```yaml
# CI/CD 发布门控：SLO 预算不足时阻止发布
# ArgoCD PreSync Hook
apiVersion: batch/v1
kind: Job
metadata:
  name: slo-budget-check
  annotations:
    argocd.argoproj.io/hook: PreSync
spec:
  template:
    spec:
      containers:
        - name: slo-check
          image: slo-gate:latest
          env:
            - name: SERVICE
              value: order-service
            - name: MIN_BUDGET_PCT
              value: "20"              # 预算剩余 < 20% 时阻止发布
          command:
            - /bin/sh
            - -c
            - |
              budget=$(curl -s prometheus:9090/api/v1/query \
                --data-urlencode 'query=slo:error_budget:remaining:30d{service="order-service"}' \
                | jq '.data.result[0].value[1] | tonumber')
              if [ $(echo "$budget < $MIN_BUDGET_PCT" | bc) -eq 1 ]; then
                echo "SLO 预算不足 ($budget%)，发布被阻止"
                exit 1
              fi
```

## 最佳实践

- **从用户视角定义 SLI**：SLI 应反映用户可感知的服务质量（如"成功请求比例"和"P99 延迟"），而非内部技术指标（如 CPU 利用率）
- **设置合理的 SLO 目标**：99.9% 是常见起点，但不要盲目追求更多 9——每增加一个 9 意味着 10 倍的工程投入
- **使用多窗口 Burn Rate 告警**：单一阈值告警会导致误报或漏报——多窗口策略（5m+1h, 30m+6h, 6h+3d）兼顾灵敏度和稳定性
- **错误预算驱动发布决策**：预算充足时可以激进发布（快速迭代），预算紧张时冻结发布（保守稳定）——用工程化方式平衡速度和稳定性
- **定期审视 SLO 目标**：每季度 review SLO 目标是否仍匹配业务需求，用户期望提升时应适当提高 SLO

## 常见陷阱

- **SLI 定义与用户体验脱节**：如果 SLI 仅测量技术指标（如 Pod CPU）而非用户可感知的请求成功率/延迟，SLO 就失去了意义
- **告警过多导致疲劳**：没有多窗口策略的 SLO 告警会产生大量噪音——遵循 Google SRE 的多窗口告警实践
- **错误预算未纳入发布流程**：SLO 仅作为事后报告而非发布门控——需要将错误预算检查集成到 CI/CD 流水线

## 相关 Domain

- [[09-可观测性/06-SLO-SLI/01-sli-definition-selection.md|01 sli definition selection]]
- [[17-系统基础/06-知识字典/observability/observability.md|observability]]/02-metrics/02-[[20-最佳实践/01-best-practices/observability/monitoring.md|monitoring]]-metrics-system]]
- 发布变更/01-[[17-系统基础/05-速查卡/gitops.md|gitops]]/01-gitops-principles

## 相关页面

- [[22-概念/09-平台与发布/gitops-release-gate.md|GitOps 发布门控]] — SLO 驱动的发布安全
- [[22-概念/06-可观测性/prometheus-argocd-monitoring.md|Prometheus 与 ArgoCD 监控]] — 监控栈 GitOps
- [[22-概念/09-平台与发布/platform-engineering-sre.md|平台工程与 SRE]] — SRE 实践

## Related

- [[17-系统基础/05-速查卡/git.md|Git 速查卡]]


<!-- risk-assessed -->
