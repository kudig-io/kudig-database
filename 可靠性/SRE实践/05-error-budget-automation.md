---
title: 错误预算自动化执行
description: 当错误预算耗尽时自动冻结发布、限制流量与告警的自动化执行方案
summary: 通过 Prometheus 查询 + OPA 策略 + 发布门控实现错误预算耗尽即自动冻结发布的闭环
category: reliability
tags:
- slo
- sli
- reliability
- error-budget
- automation
- opa
- release-gate
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

# 错误预算自动化执行

> **核心原则**：错误预算策略只有"被自动执行"才有效。靠人记的规则会在冲刺压力下被绕过——把规则写进 CI 流水线与集群准入控制，让系统自己说"不"。

## 自动化闭环

```
  ┌──────────────┐    SLI 指标     ┌──────────────┐
  │  Prometheus  │ ──────────────▶ │  规则引擎     │
  │  + SLO 计算   │                 │ (OPA/Rego)   │
  └──────────────┘                 └──────┬───────┘
                                          │ budget_state
                  ┌───────────────────────┼───────────────────┐
                  ▼                       ▼                   ▼
          ┌──────────────┐       ┌──────────────┐     ┌──────────────┐
          │ 发布门控 CI    │       │ 准入控制器    │     │ 告警/Slack   │
          │ (freeze)     │       │ (reject)     │     │ (notify)     │
          └──────────────┘       └──────────────┘     └──────────────┘
```

## 1. 用 PromQL 计算预算剩余比例

```promql
# 30 天滚动窗口的 SLI 达成率
sum(rate(http_requests_total{code!~"5.."}[30d]))
/
sum(rate(http_requests_total[30d]))
```

```promql
# 错误预算剩余百分比（SLO=99.9%）
1 -
(
  (1 -
    sum(rate(http_requests_total{code=~"5.."}[30d]))
    /
    sum(rate(http_requests_total[30d]))
  ) / 0.999
)
```

把结果写入 `ErrorBudgetRemaining` 指标并暴露给策略引擎。

## 2. 发布门控：GitHub Actions 检查

```yaml
# .github/workflows/slo-gate.yml
name: SLO Release Gate
on: { pull_request: { branches: [main] } }
jobs:
  budget-check:
    runs-on: ubuntu-latest
    steps:
      - name: Query error budget
        id: budget
        run: |
          REMAINING=$(curl -sG "$PROM_URL/api/v1/query" \
            --data-urlencode 'query=error_budget_remaining' \
            | jq -r '.data.result[0].value[1] // 0')
          echo "remaining=$REMAINING" >> $GITHUB_OUTPUT
        env:
          PROM_URL: ${{ secrets.PROM_URL }}
      - name: Freeze on exhausted budget
        if: ${{ steps.budget.outputs.remaining < 0.25 }}
        run: |
          echo "::error::Error budget < 25% ($REMAINING). Deploy frozen."
          exit 1
```

## 3. 准入控制：OPA / Gatekeeper

```rego
# rego/error_budget.rego
package k8s.errorbudget

import data.errorbudget.remaining

deny[msg] {
  input.review.kind.kind == "Deployment"
  remaining < 0.0
  msg := sprintf("Error budget exhausted (%v). Deploy frozen by policy.", [remaining])
}

deny[msg] {
  input.review.kind.kind == "Deployment"
  remaining >= 0.0
  remaining < 0.25
  not is_critical_fix(input)
  msg := sprintf("Error budget critical (%v). Only Sev1 fixes allowed.", [remaining])
}

is_critical_fix(x) { x.review.object.metadata.labels.fix-type == "sev1" }
```

🟡 **中危变更** — Gatekeeper 策略默认 `dryrun`，观察 1 周再切 `enforce`：

```bash
kubectl apply -f constraint.yaml
# 验证不会误杀后：
kubectl annotate constraint error-budget-deploy \
  enforcementAction=deny --overwrite
```

## 4. 预算恢复自动化

当 `ErrorBudgetRemaining` 回到 >50%，自动解除冻结并通知：

```yaml
# PrometheusRule 自动恢复告警
- alert: ErrorBudgetRecovered
  expr: error_budget_remaining > 0.5
  for: 1h
  annotations:
    summary: "错误预算已恢复至 {{ $value | humanizePercentage }}，发布限制解除"
```

## 决策矩阵（策略可配置）

| 预算剩余 | CI 门控 | 准入控制 | 告警级别 |
|---------|--------|---------|---------|
| > 50% | 放行 | 放行 | 无 |
| 25–50% | 放行 + 标签 | 仅 Sev1 | P3 |
| 0–25% | 仅 Sev1 | 仅 Sev1 | P2 |
| < 0% | 全冻结 | 全拒绝 | P1 + 自动开 Incident |

## 常见陷阱

1. **窗口太短**：用 30 天滚动窗口而非日历月，避免月初"预算重置"错觉。
2. **只挡新增不挡扩缩容**：把 `Scale`/`Rollout` 子资源也纳入 Gatekeeper 审计。
3. **没有逃生阀**：保留 `break-glass` Label，Sev1 修复可用人工 override，但每次 override 自动开审计工单。
4. **指标缺失时静默放行**：策略缺数据应默认**拒绝**而非通过。

## 相关

- [[可靠性/SRE实践/02-release-gate-slo-based.md|02 release gate slo based]]
- [[可靠性/03-slo-sli-guide.md|03 slo sli guide]]
- [[可靠性/04-error-budget-policy-template.md|04 error budget policy template]]

<!-- risk-assessed -->
