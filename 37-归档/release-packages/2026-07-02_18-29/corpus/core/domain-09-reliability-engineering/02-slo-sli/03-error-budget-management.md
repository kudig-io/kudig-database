---
title: 错误预算管理
description: '# 错误预算管理'
summary: 'print(f"等价停机时间: {error_time_seconds} 秒 = {error_time_seconds/60:.1f} 分钟")'
category: domain
tags:
- sre
- slo
- error-budget
- reliability
- risk-management
- grafana
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 30min
intent_queries:
- 错误预算管理 是什么
- 如何 错误预算管理
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- 错误预算管理
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 错误预算管理

> **核心原则**: 错误预算是 SRE 的核心创新——它将"可靠性"从"不允许失败"转变为"允许可控的失败，但用完预算后必须停止冒险"。

## 错误预算基础

### 什么是错误预算

**错误预算** = 在 SLO 评估窗口内，允许的服务不可靠事件总量。

```
计算公式:
错误预算 = (1 - SLO) × 总事件数

示例:
  SLO: 99.9% 可用性
  窗口: 30 天
  总请求数: 10,000,000
  
  错误预算 = (1 - 0.999) × 10,000,000 = 10,000 次错误请求
  
  即: 30 天内允许最多 10,000 次请求失败
```

### 错误预算的意义

```
传统思维:              SRE 思维:
"零问题"               "可接受的问题率"
  ↓                      ↓
不敢发布               积极发布直到预算告警
  ↓                      ↓
发布堆积               小步快跑
  ↓                      ↓
大爆炸发布             风险可控
  ↓                      ↓
更大问题               持续改进
```

## 错误预算计算公式和示例

### 基础计算公式

```
错误预算公式:

基于请求数:
  Error Budget = (1 - SLO) × Total Requests

基于时间:
  Error Budget (seconds) = (1 - SLO) × Window (seconds)

基于燃烧率:
  Remaining Budget = 1 - (Actual Error Rate / (1 - SLO))
```

### 不同 SLO 的错误预算对照表

| SLO | 允许错误率 | 30天请求预算(1000万请求) | 30天时间预算 | 1天时间预算 |
|-----|----------|------------------------|------------|-----------|
| 99% | 1.0% | 100,000 次 | 7.2 小时 | 14.4 分钟 |
| 99.5% | 0.5% | 50,000 次 | 3.6 小时 | 7.2 分钟 |
| 99.9% | 0.1% | 10,000 次 | 43.2 分钟 | 86.4 秒 |
| 99.95% | 0.05% | 5,000 次 | 21.6 分钟 | 43.2 秒 |
| 99.99% | 0.01% | 1,000 次 | 4.32 分钟 | 8.64 秒 |
| 99.999% | 0.001% | 100 次 | 25.9 秒 | 0.86 秒 |

### 实际计算示例

**示例 1: 电商订单服务**

```
SLO: 99.9% 可用性
窗口: 30 天
日均请求: 500,000
30 天总请求: 15,000,000

错误预算 = (1 - 0.999) × 15,000,000 = 15,000 次错误

等价时间预算:
  30 天 = 2,592,000 秒
  时间预算 = 0.001 × 2,592,000 = 2,592 秒 = 43.2 分钟

场景分析:
  场景 A: 一次发布引入 bug，导致 2 小时持续 5% 错误率
    错误数 = 500,000 × 2h/24h × 5% = 2,083 次
    预算消耗 = 2,083 / 15,000 = 13.9%
    → 发布被接受，但需修复

  场景 B: 数据库问题，4 小时 100% 不可用
    错误数 = 500,000 × 4h/24h = 83,333 次
    预算消耗 = 83,333 / 15,000 = 555%
    → 严重超支，触发复盘和流程改进
```

**示例 2: 金融支付服务**

```
SLO: 99.99% 可用性
窗口: 30 天
日均交易: 1,000,000
30 天总交易: 30,000,000

错误预算 = (1 - 0.9999) × 30,000,000 = 3,000 次失败

时间预算 = 0.0001 × 2,592,000 = 259.2 秒 ≈ 4.3 分钟

关键约束:
  每月最多允许 3,000 笔交易失败
  每月最多允许 4.3 分钟不可用
  
  这意味着任何超过 30 秒的问题都会消耗大量预算
```

### 基于请求数的计算

```python
# 错误预算计算器（基于请求数）
def calculate_error_budget(slo: float, total_requests: int) -> int:
    """
    计算错误预算
    
    Args:
        slo: SLO 目标值（如 0.999）
        total_requests: 评估窗口内的总请求数
    
    Returns:
        允许的错误请求数
    """
    error_rate = 1 - slo
    error_budget = int(error_rate * total_requests)
    return error_budget

# 示例
slo = 0.999       # 99.9%
total = 10_000_000  # 1000 万请求

budget = calculate_error_budget(slo, total)
print(f"错误预算: {budget:,} 次请求")  # 10,000 次

# 等价时间表示
window_days = 30
total_seconds = window_days * 24 * 3600
error_time_seconds = int(error_rate * total_seconds)
print(f"等价停机时间: {error_time_seconds} 秒 = {error_time_seconds/60:.1f} 分钟")
```

### 基于时间的计算

```python
# 错误预算计算器（基于时间）
def calculate_downtime_budget(slo: float, window_days: int) -> dict:
    """
    计算基于时间的错误预算
    
    Returns:
        不同时间粒度的允许停机时间
    """
    error_rate = 1 - slo
    total_seconds = window_days * 24 * 3600
    budget_seconds = error_rate * total_seconds
    
    return {
        "per_year": budget_seconds * (365 / window_days),
        "per_quarter": budget_seconds * (90 / window_days),
        "per_month": budget_seconds,
        "per_week": budget_seconds * (7 / window_days),
        "per_day": budget_seconds / window_days,
    }

# 不同 SLO 的年度停机预算
for slo in [0.99, 0.999, 0.9999]:
    budgets = calculate_downtime_budget(slo, 30)
    print(f"SLO {slo*100:.2f}%: 年停机预算 = {budgets['per_year']/3600:.2f} 小时")

# 输出:
# SLO 99.00%: 年停机预算 = 87.60 小时
# SLO 99.90%: 年停机预算 = 8.76 小时
# SLO 99.99%: 年停机预算 = 0.88 小时
```

## 预算消耗速率监控 PromQL

### 实时预算消耗计算

```promql
# 当前已消耗的错误预算比例
# 公式: 已消耗错误数 / 总错误预算

(
  (
    sum(rate(http_requests_total{status=~"5.."}[30d]))
    / sum(rate(http_requests_total[30d]))
  ) - (1 - 0.999)  # 减去 SLO 允许的错误率
)
/ (1 - 0.999)  # 除以总错误预算

# 结果解释:
# > 1.0:  预算已超支
# 0.75-1.0: 预算即将耗尽
# 0.5-0.75: 预算消耗过半
# < 0.5:   预算充足
```

### 预算消耗速率（每小时消耗）

```promql
# 过去 1 小时的错误率
sum(rate(http_requests_total{status=~"5.."}[1h]))
/
sum(rate(http_requests_total[1h]))

# 当前燃烧率（当前错误率 / SLO 允许错误率）
(
  sum(rate(http_requests_total{status=~"5.."}[1h]))
  / sum(rate(http_requests_total[1h]))
)
/
(1 - 0.999)

# 如果燃烧率为 14.4x:
# 意味着当前错误率是正常允许值的 14.4 倍
# 将在 30/14.4 = 2.08 天内耗尽预算
```

### 预算预计耗尽时间

```promql
# 基于当前燃烧率预测预算耗尽时间（小时）
(
  1 - (
    (
      sum(rate(http_requests_total{status=~"5.."}[30d]))
      / sum(rate(http_requests_total[30d]))
    ) - 0.001
  ) / 0.001
)
*
(30 * 24)
/
(
  (
    sum(rate(http_requests_total{status=~"5.."}[1h]))
    / sum(rate(http_requests_total[1h]))
  )
  /
  0.001
)

# 简化版本：基于过去 24h 燃烧率预测剩余天数
(
  1 - (
    (
      sum(rate(http_requests_total{status=~"5.."}[30d]))
      / sum(rate(http_requests_total[30d]))
    ) - 0.001
  ) / 0.001
) * 30
/
(
  (
    sum(rate(http_requests_total{status=~"5.."}[1d]))
    / sum(rate(http_requests_total[1d]))
  )
  /
  0.001
)
```

### 多服务预算消耗总览

```promql
# 所有服务的预算消耗比例
(
  (
    sum(rate(http_requests_total{status=~"5.."}[30d])) by (service)
    / sum(rate(http_requests_total[30d])) by (service)
  ) - 0.001
) / 0.001

# 预算消耗 Top 5 的服务
topk(5,
  (
    (
      sum(rate(http_requests_total{status=~"5.."}[30d])) by (service)
      / sum(rate(http_requests_total[30d])) by (service)
    ) - 0.001
  ) / 0.001
)
```

### 预算消耗看板

```yaml
# Grafana Dashboard 关键面板
panels:
  - title: "错误预算消耗率 (30天)"
    type: gauge
    query: |
      clamp_min(
        (
          (sum(rate(http_requests_total{status=~"5.."}[30d])) 
           / sum(rate(http_requests_total[30d])))
          - 0.001
        ) / 0.001,
        0
      )
    thresholds:
      - value: 0.25  color: green   # 充足
      - value: 0.50  color: yellow  # 过半
      - value: 0.75  color: orange  # 告警
      - value: 1.00  color: red     # 耗尽

  - title: "预算预计耗尽时间"
    type: stat
    query: |
      # 基于当前 Burn Rate 预测预算耗尽时间
      30d - (
        (
          (sum(rate(http_requests_total{status=~"5.."}[30d]))
           / sum(rate(http_requests_total[30d])))
          - 0.001
        ) / 0.001 * 30d
      )
```

## 预算耗尽时的自动熔断机制

### 熔断机制设计原则

当错误预算消耗达到特定阈值时，自动触发保护措施，防止进一步消耗预算。

```
熔断阈值设计:

预算剩余 100% → 正常发布，正常运维
预算剩余 75%  → 触发 warning，增加监控频率
预算剩余 50%  → 触发 page，非紧急发布需审批
预算剩余 25%  → 发布冻结（紧急修复除外）
预算剩余 0%   → 完全冻结，所有变更禁止
预算超支      → 触发事后复盘，SLO 评审
```

### [[Kubernetes|Kubernetes]] 自动熔断实现

```yaml
# error-budget-policy.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: error-budget-policy
  namespace: sre-system
data:
  policy.json: |
    {
      "service": "order-service",
      "slo": 0.999,
      "window_days": 30,
      "thresholds": [
        {
          "budget_remaining": 0.75,
          "action": "notify",
          "channels": ["slack#sre-alerts"],
          "message": "错误预算已消耗 25%，请关注"
        },
        {
          "budget_remaining": 0.50,
          "action": "page",
          "channels": ["pagerduty"],
          "message": "错误预算已消耗 50%，非紧急发布需审批"
        },
        {
          "budget_remaining": 0.25,
          "action": "freeze_release",
          "channels": ["slack#releases", "pagerduty"],
          "message": "错误预算已消耗 75%，发布已冻结"
        },
        {
          "budget_remaining": 0.00,
          "action": "emergency_freeze",
          "channels": ["slack#incidents", "pagerduty", "email:vp@example.com"],
          "message": "错误预算已耗尽！禁止所有变更，启动复盘"
        }
      ]
    }
```

### 基于 OPA/Gatekeeper 的发布门控

```yaml
# release-freeze-policy.rego
package kubernetes.admission

import future.keywords.if
import future.keywords.in

# 错误预算状态（由外部系统注入到 ConfigMap）
budget_status := data.configmaps["sre-system"]["error-budget-status"].data

# 拒绝非紧急部署当预算低于 25%
deny[msg] if {
  input.request.kind.kind == "Deployment"
  input.request.operation == "CREATE"
  
  service := input.request.object.metadata.labels["app"]
  budget_remaining := to_number(budget_status[service])
  
  budget_remaining < 0.25
  
  # 检查是否为紧急修复（带 emergency 注解）
  not input.request.object.metadata.annotations["release-type"] == "emergency"
  
  msg := sprintf(
    "发布被拒绝: 服务 %s 的错误预算仅剩 %.1f%%，低于 25%% 阈值。" +
    "如需紧急修复，请添加 annotation 'release-type: emergency' 并获得审批。",
    [service, budget_remaining * 100]
  )
}
```

### 自动化熔断控制器

```python
# error_budget_controller.py
# 运行在集群中，定期评估错误预算并执行熔断动作

import requests
import json
from datetime import datetime, timedelta

PROMETHEUS_URL = "http://prometheus.monitoring.svc:9090"
SLO_CONFIG = {
    "order-service": {"slo": 0.999, "window_days": 30},
    "payment-service": {"slo": 0.9999, "window_days": 30},
    "user-service": {"slo": 0.995, "window_days": 30},
}

THRESHOLDS = [
    (0.75, "notify"),
    (0.50, "page"),
    (0.25, "freeze_release"),
    (0.00, "emergency_freeze"),
]

def query_prometheus(query: str) -> float:
    """执行 PromQL 查询"""
    resp = requests.get(
        f"{PROMETHEUS_URL}/api/v1/query",
        params={"query": query}
    )
    resp.raise_for_status()
    data = resp.json()
    if data["data"]["result"]:
        return float(data["data"]["result"][0]["value"][1])
    return 0.0

def calculate_budget_consumed(service: str, slo: float) -> float:
    """计算已消耗的错误预算比例"""
    query = f'''
    (
      (
        sum(rate(http_requests_total{{service="{service}",status=~"5.."}}[30d]))
        / sum(rate(http_requests_total{{service="{service}"}}[30d]))
      ) - {1 - slo}
    ) / {1 - slo}
    '''
    result = query_prometheus(query)
    return max(0.0, result)

def take_action(service: str, budget_remaining: float, action: str):
    """执行熔断动作"""
    actions = {
        "notify": lambda: send_slack(f"{service}: 预算剩余 {budget_remaining:.1%}"),
        "page": lambda: trigger_pagerduty(service, budget_remaining),
        "freeze_release": lambda: freeze_release_pipeline(service),
        "emergency_freeze": lambda: emergency_lockdown(service),
    }
    
    if action in actions:
        actions[action]()
        log_action(service, action, budget_remaining)

def evaluate_all_services():
    """评估所有服务的错误预算状态"""
    for service, config in SLO_CONFIG.items():
        consumed = calculate_budget_consumed(service, config["slo"])
        remaining = 1.0 - consumed
        
        for threshold, action in THRESHOLDS:
            if remaining <= threshold:
                take_action(service, remaining, action)
                break

# 由 CronJob 每小时执行
if __name__ == "__main__":
    evaluate_all_services()
```

### Kubernetes CronJob 部署

```yaml
# error-budget-controller-cronjob.yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: error-budget-controller
  namespace: sre-system
spec:
  schedule: "0 * * * *"  # 每小时执行
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: error-budget-controller
          containers:
            - name: controller
              image: sre/error-budget-controller:v1.0
              env:
                - name: PROMETHEUS_URL
                  value: "http://prometheus.monitoring.svc:9090"
                - name: SLACK_WEBHOOK
                  valueFrom:
                    secretKeyRef:
                      name: sre-secrets
                      key: slack-webhook
          restartPolicy: OnFailure
```

## 错误预算与发布频率的关联模型

### 发布风险模型

每次发布都引入一定的风险，错误预算决定了你能承受多少次发布。

```
基本假设:
  - 每次发布引入错误率上升的概率: p
  - 发布导致的平均错误率上升: Δe
  - 发布导致的平均持续时间: t
  - 发布期间请求量: R

单次发布期望消耗预算 = p × Δe × t × R

示例:
  p = 20% (每 5 次发布有 1 次出问题)
  Δe = 5% (出问题时的错误率)
  t = 30 分钟 = 1800 秒
  R = 1000 req/s
  
  单次发布期望消耗 = 0.2 × 0.05 × 1800 × 1000 = 18,000 次错误
  
  如果月度预算为 10,000 次:
  可承受发布次数 = 10,000 / 18,000 ≈ 0.55 次/月
  
  → 这意味着当前发布质量下，每月最多发布 0-1 次！
  → 需要降低发布风险（更好的测试、金丝雀发布）
```

### 发布频率与错误预算的数学关系

```
变量定义:
  B = 月度错误预算
  f = 发布频率 (次/月)
  r = 单次发布引入的平均错误数
  c = 其他原因（非发布）导致的月均错误数

约束条件:
  f × r + c ≤ B

求解最大发布频率:
  f ≤ (B - c) / r

示例:
  B = 10,000 次 (SLO 99.9%, 1000 万请求/月)
  c = 2,000 次 (基础设施、依赖问题等)
  r = 1,500 次 (改进后的发布质量)
  
  f ≤ (10,000 - 2,000) / 1,500 = 5.33
  
  → 每月最多可发布 5 次
  → 如果团队需要每周发布，需要进一步降低 r
```

### 发布策略与预算消耗矩阵

| 发布策略 | 风险等级 | 单次预算消耗估计 | 适用预算状态 |
|---------|---------|----------------|------------|
| **大爆炸发布** | 极高 | 30-50% 预算 | ❌ 永远不推荐 |
| **蓝绿发布** | 中 | 5-10% 预算 | ✅ 预算 > 50% |
| **金丝雀 5% → 100%** | 低 | 2-5% 预算 | ✅ 预算 > 25% |
| **金丝雀 1% → 5% → 25% → 100%** | 很低 | 1-3% 预算 | ✅ 任何状态 |
| **特性开关 (Feature Flag)** | 极低 | < 1% 预算 | ✅ 任何状态 |
| **紧急热修复** | 高 | 10-20% 预算 | ⚠️ 仅紧急 |

### 发布频率优化策略

```
当错误预算紧张时，如何保持发布频率?

策略 1: 降低单次发布风险
  → 增加自动化测试覆盖率
  → 强制代码审查（至少 2 人）
  → 引入金丝雀发布（最小 1% 流量）
  → 自动回滚（SLO 下降时自动触发）

策略 2: 分流发布风险
  → 使用特性开关，新功能默认关闭
  → 灰度发布（按用户、地域、设备）
  → A/B 测试框架隔离实验流量

策略 3: 增加错误预算
  → 提升 SLO（需要架构改进）
  → 减少非发布错误（提升基础设施稳定性）
  → 扩展评估窗口（从 30 天到 90 天，平滑波动）

策略 4: 分离发布类型
  → 功能发布: 消耗错误预算
  → 安全修复: 不消耗预算（或单独预算池）
  → 配置变更: 低风险，快速回滚
```

### 发布日历与预算规划

```
月度发布规划示例 (SLO 99.9%, 预算 10,000 次):

Week 1:
  计划: 金丝雀发布 v2.1 (新功能)
  预留预算: 2,000 次
  实际消耗: 500 次 (顺利)
  剩余预算: 9,500 次

Week 2:
  计划: 金丝雀发布 v2.2 (性能优化)
  预留预算: 2,000 次
  实际消耗: 3,500 次 (引入回归 bug，快速回滚)
  剩余预算: 6,000 次
  → 触发 yellow 告警

Week 3:
  原计划: 发布 v2.3
  调整: 改为仅 bug 修复，禁止新功能
  实际消耗: 200 次
  剩余预算: 5,800 次

Week 4:
  计划: 回顾和修复 Week 2 的问题
  发布冻结，除非紧急修复
  实际消耗: 100 次
  剩余预算: 5,700 次 (57%)

月度总结:
  预算使用率: 43%
  发布次数: 3 次（原计划 4 次）
  关键教训: v2.2 缺少集成测试，下月增加
```

## 基于错误预算的发布决策

### 发布门控矩阵

```
错误预算状态    发布策略
─────────────────────────────────────────
> 75% 剩余     正常发布，无需额外审批
50-75% 剩余    正常发布，加强监控
25-50% 剩余    仅发布关键修复，禁止新功能
< 25% 剩余     发布冻结，仅紧急修复
预算已耗尽     完全冻结，问题修复除外
─────────────────────────────────────────
```

### 发布决策流程

```
开发团队申请发布
    ↓
检查错误预算状态
    ↓
├── 预算充足 (> 50%)
│   └── 自动批准，标准发布流程
│
├── 预算紧张 (25-50%)
│   └── 需要 SRE 审批
│   └── 要求: 发布计划 + 回滚方案 + 加强监控
│
└── 预算耗尽 (< 25%)
    └── 发布冻结
    └── 例外流程: 技术总监 + VP 双签
    └── 要求: 业务影响评估 + 无替代方案说明
```

### 错误预算与发布节奏

```
场景: SLO 99.9%，月度错误预算 10,000 次错误

月初: 预算充足
  → 按计划发布新功能 v2.1
  → 发布引入 2,000 次错误（预算剩余 80%）

月中: 预算过半
  → 发布 v2.2 需要更多审批
  → 发布引入 3,000 次错误（预算剩余 50%）
  → ⚠️ 触发告警，加强监控

月末: 预算紧张
  → 原计划发布 v2.3
  → ❌ 发布冻结
  → 转向优化和修复已知问题
  → 下个月预算重置后再评估
```

## 多窗口预算监控

### 短期 vs 长期窗口

```
窗口类型      用途                敏感度
─────────────────────────────────────────
1 小时       检测突发问题          高
1 天         检测日级模式          中
7 天         检测周级趋势          低
30 天        SLO 评估标准          基准
─────────────────────────────────────────
```

### 多窗口预算看板

```promql
# 1 小时窗口（检测突发）
sum(rate(http_requests_total{status=~"5.."}[1h]))
/ sum(rate(http_requests_total[1h]))

# 1 天窗口（日级监控）
sum(rate(http_requests_total{status=~"5.."}[1d]))
/ sum(rate(http_requests_total[1d]))

# 7 天窗口（周级趋势）
sum(rate(http_requests_total{status=~"5.."}[7d]))
/ sum(rate(http_requests_total[7d]))

# 30 天窗口（SLO 评估）
sum(rate(http_requests_total{status=~"5.."}[30d]))
/ sum(rate(http_requests_total[30d]))
```

## 错误预算恢复策略

### 预算耗尽后的恢复路径

```
阶段 1: 紧急止损（0-2 小时）
  - 停止所有非紧急发布
  - 执行已知有效的回滚方案
  - 启动战时指挥中心

阶段 2: 根因修复（2-24 小时）
  - 确定问题根因
  - 实施修复（回滚、配置调整、扩容等）
  - 验证修复效果

阶段 3: 预算恢复评估（24-72 小时）
  - 计算实际消耗的错误数
  - 评估是否需要调整 SLO
  - 制定预防措施

阶段 4: 组织复盘（1-2 周）
  - 无责事后复盘
  - 更新运行手册
  - 优化监控和告警
```

### 预算"借贷"机制

```
概念: 允许在当前窗口超支，但需要在后续窗口补偿

示例:
  1 月预算: 10,000 次错误
  1 月实际: 15,000 次错误（超支 5,000）
  
  处理:
  - 2 月预算调整为: 10,000 - 5,000 = 5,000
  - 连续 2 个月超支则触发 SLO 评审

警告: 此机制应谨慎使用，避免长期累积债务
```

## 跨服务错误预算

### 依赖服务的预算影响

```
服务拓扑:
  订单服务 (SLO 99.9%)
    ├── 支付服务 (SLO 99.95%)
    ├── 库存服务 (SLO 99.9%)
    └── 物流服务 (SLO 99.5%)

问题: 物流服务的 99.5% 会拖累订单服务

计算:
  订单服务理论最大可用性 = 99.95% × 99.9% × 99.5%
                      ≈ 99.35%

结论: 订单服务 SLO 99.9% 理论上不可达！

解决: 要么提升物流服务，要么调整订单服务 SLO
```

### 预算分配模型

```
用户请求链路:
  用户 → 网关(99.99%) → 订单(99.9%) → 支付(99.95%) → 银行(99.99%)

预算分配:
  网关: 0.01% 错误预算
  订单: 0.1% 错误预算
  支付: 0.05% 错误预算
  银行: 0.01% 错误预算

链路总预算:
  1 - (0.9999 × 0.999 × 0.9995 × 0.9999) ≈ 0.17%
  
对应可用性: 99.83%
```

## 相关

- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-09-reliability-engineering/02-slo-sli/02-slo-implementation-guide|02 slo implementation guide]] — SLO 设定与实施指南
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-09-reliability-engineering/05-slo-sli/01-burn-rate-alerting|04 burn rate alerting]] — Burn Rate 告警


<!-- risk-assessed -->
