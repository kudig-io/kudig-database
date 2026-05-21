---
title: 错误预算管理
description: '# 错误预算管理'
category: domain
tags:
- sre
- slo
- error-budget
- reliability
- risk-management
- grafana
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
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
"零故障"               "可接受的故障率"
  ↓                      ↓
不敢发布               积极发布直到预算告警
  ↓                      ↓
发布堆积               小步快跑
  ↓                      ↓
大爆炸发布             风险可控
  ↓                      ↓
更大故障               持续改进
```

## 错误预算计算

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

## 错误预算消耗监控

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

### 预算消耗看板

```yaml
# Grafana Dashboard 关键面板
panels:
  - title: "错误预算消耗率 (30天)"
    type: gauge
    query: |
      (
        (sum(rate(http_requests_total{status=~"5.."}[30d])) 
         / sum(rate(http_requests_total[30d])))
        - 0.001
      ) / 0.001
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

## 基于错误预算的发布决策

### 发布门控矩阵

```
错误预算状态    发布策略
─────────────────────────────────────────
> 75% 剩余     正常发布，无需额外审批
50-75% 剩余    正常发布，加强监控
25-50% 剩余    仅发布关键修复，禁止新功能
< 25% 剩余     发布冻结，仅紧急修复
预算已耗尽     完全冻结，故障修复除外
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
1 小时       检测突发故障          高
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
  - 确定故障根因
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

- [[domain-09-reliability-engineering/04-slo-sli/02-slo-implementation-guide]] — SLO 设定与实施指南
