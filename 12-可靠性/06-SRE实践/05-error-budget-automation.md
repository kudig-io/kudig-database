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

## 完整 OPA 策略包

### ConstraintTemplate

```yaml
apiVersion: templates.gatekeeper.sh/v1
kind: ConstraintTemplate
metadata:
  name: k8serrorbudget
spec:
  crd:
    spec:
      names:
        kind: K8sErrorBudget
      validation:
        openAPIV3Schema:
          type: object
          properties:
            budgetThreshold:
              type: number
              description: "预算阈值，低于此值拒绝部署"
            allowedLabels:
              type: array
              items:
                type: string
              description: "允许绕过的标签"
  targets:
    - target: admission.k8s.gatekeeper.sh
      rego: |
        package k8serrorbudget

        import data.errorbudget.remaining

        violation[{"msg": msg}] {
          input.review.kind.kind == "Deployment"
          remaining < input.parameters.budgetThreshold
          not is_allowed(input)
          msg := sprintf("错误预算不足 (%.2f%%). 部署被策略拒绝.", [remaining * 100])
        }

        violation[{"msg": msg}] {
          input.review.kind.kind == "Deployment"
          remaining >= input.parameters.budgetThreshold
          remaining < 0.25
          not is_critical_fix(input)
          msg := sprintf("错误预算紧张 (%.2f%%). 仅允许 Sev1 修复.", [remaining * 100])
        }

        is_allowed(review) {
          review.review.object.metadata.labels["break-glass"] == "true"
        }

        is_critical_fix(review) {
          review.review.object.metadata.labels["fix-type"] == "sev1"
        }
```

### Constraint 实例

```yaml
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sErrorBudget
metadata:
  name: error-budget-policy
spec:
  enforcementAction: deny  # 先 dryrun 观察，再切 deny
  match:
    kinds:
      - apiGroups: ["apps"]
        kinds: ["Deployment"]
    namespaces:
      - production
      - staging
  parameters:
    budgetThreshold: 0.0
    allowedLabels:
      - break-glass
      - fix-type
```

## Argo Rollouts 集成

### 分析模板集成错误预算

```yaml
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: error-budget-check
spec:
  args:
    - name: service-name
  metrics:
    - name: error-budget-remaining
      interval: 5m
      successCondition: result[0] > 0.25
      failureLimit: 1
      provider:
        prometheus:
          address: http://prometheus:9090
          query: |
            1 - (
              sum(rate(http_requests_total{job="{{args.service-name}}",code=~"5.."}[30d]))
              /
              sum(rate(http_requests_total{job="{{args.service-name}}"}[30d]))
            ) / (1 - 0.999)
---
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: api-service
spec:
  strategy:
    canary:
      steps:
        - setWeight: 10
        - analysis:
            templates:
              - templateName: error-budget-check
            args:
              - name: service-name
                value: api-service
        - setWeight: 50
        - pause: {duration: 10m}
        - setWeight: 100
```

## 多团队预算分配

### 预算分配策略

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: error-budget-allocation
  namespace: sre
data:
  allocation.yaml: |
    # 全局错误预算: 100%
    # 分配策略: 按服务关键度 + 历史表现
    
    global_budget: 1.0  # 100% = 30 天 * 0.1% = 43.2 分钟
    
    teams:
      - name: platform-team
        services: [api-gateway, auth-service]
        allocation: 0.4  # 40% 预算
        slo_target: 0.999
        
      - name: product-team
        services: [order-service, payment-service]
        allocation: 0.35  # 35% 预算
        slo_target: 0.999
        
      - name: growth-team
        services: [recommendation, notification]
        allocation: 0.25  # 25% 预算
        slo_target: 0.995
    
    rules:
      # 团队预算耗尽时
      - condition: team_budget_exhausted
        actions:
          - freeze_deployments
          - notify_team_lead
          - create_incident
      
      # 团队预算剩余 > 50% 时
      - condition: team_budget_healthy
        actions:
          - allow_experiments
          - allow_feature_flags
```

### 团队预算监控

```promql
# 团队级别错误预算剩余
sum by (team) (
  1 - (
    sum by (team) (rate(http_requests_total{code=~"5.."}[30d]))
    /
    sum by (team) (rate(http_requests_total[30d]))
  )
) / (1 - slo_target)

# 团队预算消耗速率
deriv(
  sum by (team) (
    1 - (
      sum by (team) (rate(http_requests_total{code=~"5.."}[1d]))
      /
      sum by (team) (rate(http_requests_total[1d]))
    )
  )[7d:1h]
)
```

## 预算报告自动化

### 周报生成脚本

```bash
#!/bin/bash
# 🟢 低风险：错误预算周报生成
set -euo pipefail

REPORT_DATE=$(date +%Y-%m-%d)
OUTPUT_FILE="/tmp/error-budget-report-$REPORT_DATE.md"

echo "=== 生成错误预算周报 ==="

cat > $OUTPUT_FILE <<EOF
# 错误预算周报

**报告日期**: $REPORT_DATE
**报告周期**: $(date -v-7d +%Y-%m-%d 2>/dev/null || date -d "-7 days" +%Y-%m-%d) ~ $REPORT_DATE

## 总体状态

| 服务 | SLO 目标 | 当前达成率 | 预算剩余 | 状态 |
|-----|---------|-----------|---------|------|
EOF

# 查询各服务状态
for service in api-gateway order-service payment-service user-service; do
  ACHIEVEMENT=$(curl -sG "http://prometheus:9090/api/v1/query" \
    --data-urlencode "query=1 - (sum(rate(http_requests_total{job=\"$service\",code=~\"5..\"}[7d])) / sum(rate(http_requests_total{job=\"$service\"}[7d])))" \
    | jq -r '.data.result[0].value[1] // "N/A"')
  
  BUDGET=$(curl -sG "http://prometheus:9090/api/v1/query" \
    --data-urlencode "query=error_budget_remaining{job=\"$service\"}" \
    | jq -r '.data.result[0].value[1] // "N/A"')
  
  STATUS="🟢"
  if (( $(echo "$BUDGET < 0.25" | bc -l 2>/dev/null || echo 0) )); then
    STATUS="🔴"
  elif (( $(echo "$BUDGET < 0.5" | bc -l 2>/dev/null || echo 0) )); then
    STATUS="🟡"
  fi
  
  echo "| $service | 99.9% | $ACHIEVEMENT | $BUDGET | $STATUS |" >> $OUTPUT_FILE
done

cat >> $OUTPUT_FILE <<EOF

## 本周事件

| 日期 | 服务 | 影响 | 预算消耗 |
|-----|------|------|----------|
| | | | |

## 建议

- 

---
*本报告由自动化脚本生成*
EOF

echo "报告已生成: $OUTPUT_FILE"
cat $OUTPUT_FILE
```

### 定期报告 CronJob

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: error-budget-report
  namespace: sre
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
                  /scripts/generate-report.sh
                  # 发送到 Slack
                  curl -X POST -H 'Content-type: application/json' \
                    --data @/tmp/report.json \
                    $SLACK_WEBHOOK
              volumeMounts:
                - name: scripts
                  mountPath: /scripts
          volumes:
            - name: scripts
              configMap:
                name: report-scripts
                defaultMode: 0755
```

## 测试与验证

### 策略测试

```bash
#!/bin/bash
# 🟢 低风险：错误预算策略测试
set -euo pipefail

echo "=== 错误预算策略测试 ==="

# 1. 测试正常部署 (预算充足)
echo "[1] 测试正常部署..."
kubectl apply -f test-deployment.yaml --dry-run=server

# 2. 模拟预算耗尽
echo "[2] 模拟预算耗尽..."
# 临时修改 Prometheus 指标 (仅测试环境)

# 3. 验证拒绝
echo "[3] 验证拒绝..."
if kubectl apply -f test-deployment.yaml --dry-run=server 2>&1 | grep -q "denied"; then
  echo "✓ 策略生效，部署被拒绝"
else
  echo "✗ 策略未生效"
fi

# 4. 测试 break-glass
echo "[4] 测试 break-glass..."
kubectl apply -f test-deployment-break-glass.yaml --dry-run=server

echo "=== 测试完成 ==="
```

### 策略审计日志

```yaml
# 审计策略触发记录
apiVersion: v1
kind: ConfigMap
metadata:
  name: error-budget-audit
  namespace: sre
data:
  audit.log: |
    2026-07-11T10:00:00Z DENY deployment/api-service budget=0.15 user=alice
    2026-07-11T10:05:00Z ALLOW deployment/api-service budget=0.15 user=bob labels=fix-type=sev1
    2026-07-11T11:00:00Z ALLOW deployment/user-service budget=0.65 user=carol
```

## 相关

- [[12-可靠性/06-SRE实践/02-release-gate-slo-based.md|02 release gate slo based]]
- [[12-可靠性/06-SRE实践/03-slo-sli-guide.md|03 slo sli guide]]
- [[12-可靠性/06-SRE实践/04-error-budget-policy-template.md|04 error budget policy template]]

<!-- risk-assessed -->
