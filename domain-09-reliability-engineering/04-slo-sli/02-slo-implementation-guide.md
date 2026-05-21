---
title: SLO 设定与实施指南
description: '# SLO 设定与实施指南'
category: domain
tags:
- sre
- slo
- sli
- reliability
- implementation
- prometheus
- grafana
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SLO 设定与实施指南 是什么
- 如何 SLO 设定与实施指南
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- SLO
- 设定与实施指南
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
- prometheus-basics
- monitoring-basics
---

# SLO 设定与实施指南

> **核心原则**: SLO 不是越高越好，而是基于业务需求、用户期望和技术成本的平衡。未达成的 SLO 比没有 SLO 更糟糕。

## SLO 基础概念

### SLO vs SLA vs SLI 的关系

```
SLI (指标) ──衡量──> SLO (目标) ──承诺──> SLA (合同)
  │                    │                  │
  可用性 99.9%      目标 99.9%         赔付 < 99.9%
  延迟 P99<200ms    目标 P99<200ms     赔付 P99>200ms
```

| 概念 | 定义 | 受众 | 违约后果 |
|------|------|------|---------|
| **SLI** | 测量指标 | 工程师 | 无 |
| **SLO** | 内部目标 | 团队/部门 | 内部流程触发（如停止发布） |
| **SLA** | 对外合同 | 客户 | 经济赔偿/服务积分 |

**关键区别**: SLO 可以比 SLA 更严格，为 SLA 提供缓冲空间。

```
SLA: 99.9% 可用性（年停机 8.76 小时）
SLO: 99.95% 可用性（年停机 4.38 小时）
  → SLO 比 SLA 严格 2 倍，提供 4.38 小时缓冲
```

## 可用性等级对照表

### 9 个等级及业务含义

| 等级 | 可用性 | 年停机时间 | 月停机时间 | 适用场景 |
|------|--------|-----------|-----------|---------|
| **1 个 9** | 90% | 36.5 天 | 73 小时 | 内部测试环境 |
| **2 个 9** | 99% | 3.65 天 | 7.3 小时 | 内部工具、非关键系统 |
| **3 个 9** | 99.9% | 8.76 小时 | 43.8 分钟 | 一般业务系统 |
| **4 个 9** | 99.99% | 52.6 分钟 | 4.38 分钟 | 支付、核心交易 |
| **5 个 9** | 99.999% | 5.26 分钟 | 26.3 秒 | 金融核心、电信 |
| **6 个 9** | 99.9999% | 31.5 秒 | 2.63 秒 | 军事、航空航天 |
| **7 个 9** | 99.99999% | 3.15 秒 | 0.26 秒 | 极端关键系统 |

### 可用性成本曲线

```
可用性      成本        复杂度
99.9%  ────────────────── 基础高可用
         ↑
99.95% ────────────────── 多可用区
         ↑↑
99.99% ────────────────── 全球多活
         ↑↑↑
99.999%────────────────── 异地多活+自动故障转移
         ↑↑↑↑
99.9999%───────────────── 理论上难以实现
```

**经验法则**: 每增加一个 9，成本增加 10 倍。

## SLO 设定方法论

### Step 1: 识别关键用户旅程 (CUJ)

```
方法: 从用户角度描述完整的使用场景

示例: 电商平台的"下单支付"旅程
1. 用户浏览商品 → 商品服务
2. 添加购物车 → 购物车服务
3. 提交订单 → 订单服务
4. 发起支付 → 支付网关
5. 支付结果通知 → 回调服务
6. 订单状态更新 → 订单服务
7. 用户查看订单 → 订单查询服务
```

### Step 2: 识别 SLI

为每个关键步骤选择合适的 SLI（参考 [[domain-09-reliability-engineering/04-slo-sli/01-sli-definition-selection]]）：

```
步骤 3: 提交订单
  SLI: 订单创建成功率
  SLI: 订单创建 P99 延迟

步骤 4: 发起支付
  SLI: 支付请求成功率
  SLI: 支付请求 P99 延迟

步骤 5: 支付结果通知
  SLI: 回调处理成功率
  SLI: 回调处理延迟
```

### Step 3: 基于历史数据设定初始 SLO

```
收集过去 30-90 天的 SLI 数据：

订单创建成功率:
  过去 30 天平均: 99.87%
  过去 30 天最低: 99.65%
  过去 30 天 P99: 99.97%

初始 SLO 建议:
  保守: 99.85%（基于历史平均）
  合理: 99.90%（略低于历史平均，留有余量）
  激进: 99.95%（需要改进才能达成）

推荐: 从 99.90% 开始，运行 1-2 个季度后调整
```

### Step 4: 验证 SLO 可行性

```
可行性检查清单:

□ 当前系统能否在不重大改造的情况下达成？
□ 依赖服务（数据库、缓存、第三方）的可用性是否支持？
□ 团队是否有能力在 SLO 告警时快速响应？
□ 错误预算是否合理（见下节）？
□ 业务方是否理解并接受对应的成本？
```

### Step 5: 获得组织共识

```
SLO 需要多方共识:

产品经理: 用户能接受多大的错误率？
  → "支付失败率超过 0.1% 会严重影响用户信任"

开发团队: 技术上能否达成？成本如何？
  → "需要增加异地多活，成本增加 300%"

运维团队: 监控和告警能否覆盖？
  → "现有监控可以覆盖，需要新增 3 个告警规则"

管理层: 投入产出比是否合理？
  → "增加 300% 成本减少 50% 支付失败，ROI 不划算"

最终决策: 保持 99.9%，聚焦优化现有架构
```

## SLO 实施路径

### 阶段 1: 测量（Month 1-2）

```
目标: 建立 SLI 测量能力，不设定 SLO

任务:
1. 部署/配置监控（Prometheus、Grafana）
2. 为关键服务配置 SLI 指标
3. 建立 SLI 数据看板
4. 收集至少 30 天历史数据

输出:
- SLI 测量看板
- 历史数据基线报告
```

### 阶段 2: 设定（Month 2-3）

```
目标: 基于历史数据设定初始 SLO

任务:
1. 分析历史数据分布
2. 与相关团队讨论并设定 SLO
3. 文档化 SLO 定义
4. 建立错误预算计算

输出:
- SLO 文档
- 错误预算看板
```

### 阶段 3: 执行（Month 3-6）

```
目标: 按 SLO 管理发布和运维

任务:
1. 建立 SLO 告警规则
2. 将 SLO 纳入发布评审
3. 定期（每周）审查 SLO 达成情况
4. 错误预算耗尽时触发发布冻结

输出:
- SLO 告警规则
- SLO 周报
- 发布冻结机制
```

### 阶段 4: 优化（Month 6+）

```
目标: 持续优化 SLO 体系

任务:
1. 根据实际达成情况调整 SLO
2. 优化告警阈值和 Burn Rate
3. 扩展 SLO 覆盖范围
4. 将 SLO 纳入团队绩效考核

输出:
- 优化后的 SLO
- 扩展的 SLO 覆盖
```

## SLO 文档模板

```yaml
# SLO 定义文档模板
slo_id: ORDER-SVC-001
service: order-service
cuj: 用户提交订单
version: 1.0
created: 2026-05-21
owner: order-team@sre.example.com

slis:
  - name: order_creation_success_rate
    description: 订单创建成功率
    measurement: |
      sum(rate(order_created_total{status="success"}[5m])) /
      sum(rate(order_created_total[5m]))
    slo_target: 0.999
    window: 30d
    
  - name: order_creation_latency
    description: 订单创建 P99 延迟
    measurement: |
      histogram_quantile(0.99,
        sum(rate(order_creation_duration_seconds_bucket[5m])) by (le)
      )
    slo_target: 0.5  # 500ms
    window: 30d

error_budget:
  calculation: (1 - SLO) × 总请求数
  budget_30d: 0.1% of total requests
  burn_rate_alerts:
    - rate: 2x   # 将在 15 天内耗尽预算
      severity: warning
    - rate: 14.4x # 将在 2 天内耗尽预算
      severity: critical

escalation:
  budget_remaining_50%: 团队负责人通知
  budget_remaining_25%: 发布冻结，技术负责人介入
  budget_remaining_0%:  紧急复盘，VP 级别通报
```

## 常见 SLO 设定错误

### 错误 1: SLO 过高

```
问题: 设定 99.999% 可用性，但团队无能力达成
后果:
  - 持续告警疲劳
  - 团队士气下降
  - 错误预算永远为负，失去管理意义

解决: 从可达到的水平开始，逐步提升
```

### 错误 2: SLO 过低

```
问题: 设定 99% 可用性，实际系统已达 99.9%
后果:
  - 错误预算永远花不完
  - 失去改进动力
  - 用户实际体验远超 SLO 承诺

解决: SLO 应略低于当前能力，提供改进空间
```

### 错误 3: SLO 过多

```
问题: 为每个 API 端点设定 SLO
后果:
  - 管理复杂度爆炸
  - 无法聚焦关键问题

解决: 只为关键用户旅程设定 SLO（建议每个服务 2-5 个）
```

### 错误 4: 忽略依赖

```
问题: 订单服务 SLO 99.99%，但数据库 SLO 只有 99.9%
后果:
  - 订单服务 SLO 理论上无法达成
  - 错误预算被依赖服务消耗

解决: 确保依赖服务的 SLO 优于或等于上层服务
```

## 相关

- [[domain-09-reliability-engineering/04-slo-sli/01-sli-definition-selection]] — SLI 定义与选择方法论
- [[domain-09-reliability-engineering/04-slo-sli/03-error-budget-management]] — 错误预算管理
- [[domain-06-observability/06-slo-sli/18-slo-sli-system]] — SLO/SLI 体系概述
