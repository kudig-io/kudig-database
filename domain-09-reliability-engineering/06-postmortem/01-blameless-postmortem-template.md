---
title: 无责事后复盘模板
description: '| 14:35 | On-call 响应并开始排查 | 通过 PagerDuty |'
category: domain
tags:
- postmortem
- sre
- incident-management
- reliability
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 无责事后复盘模板 是什么
- 如何 无责事后复盘模板
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- 无责事后复盘模板
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
---

# 无责事后复盘模板

> **核心原则**: 事后复盘的目标是理解系统为何允许故障发生，而非追究谁犯了错误。每个人的决策在当时看来都是合理的。

## 模板结构

```markdown
# 事件复盘: [事件标题]

## 元信息
- 事件编号: INC-2026-001
- 日期: 2026-05-21
- 影响服务: order-service, payment-service
- 严重级别: P1 (严重)
- 持续时间: 23 分钟
- 复盘主持人: [SRE Lead]
- 参与者: [On-call, Dev Lead, QA Lead]

## 摘要 (Executive Summary)
[2-3 句话描述发生了什么、影响范围、持续时间]

## 事件时间线 (Timeline)

| 时间 | 事件 | 备注 |
|------|------|------|
| 14:32 | 告警触发: 订单服务错误率 > 5% | 自动告警 |
| 14:35 | On-call 响应并开始排查 | 通过 PagerDuty |
| 14:45 | 定位到数据库连接池耗尽 | 发现 max_connections = 100 |
| 14:50 | 临时扩容连接池 | 错误率开始下降 |
| 14:55 | 服务完全恢复 | 错误率 < 0.1% |

## 影响评估 (Impact Assessment)

- 受影响用户: ~12,000 人
- 失败订单: ~450 笔
- 收入影响: 约 ¥85,000
- 数据丢失: 无
- 合规影响: 无

## 根因分析 (Root Cause Analysis)

### 5 Whys

1. 为什么订单服务错误率升高?
   → 数据库连接池耗尽，新请求无法获取连接

2. 为什么连接池会耗尽?
   → 连接池配置 max_connections = 100，远低于实际需求

3. 为什么配置如此低?
   → 配置沿用开发环境默认值，未根据生产环境调整

4. 为什么生产环境未调整?
   → 上线检查清单缺少数据库连接池配置项

5. 为什么检查清单不完整?
   → 新服务上线流程未经过 SRE 评审

### 根因分类

- 直接原因: 数据库连接池配置不当
-  Contributing Factor: 缺乏生产环境配置审查流程
-  Contributing Factor: 连接池使用率监控缺失

## 经验教训 (Lessons Learned)

### 做得好的 (What Went Well)
- 告警及时，On-call 在 3 分钟内响应
- 快速定位根因并修复
- 客服团队及时发布状态更新

### 需要改进的 (What Went Wrong)
- 连接池配置未经审查
- 缺乏连接池使用率监控
- 上线流程缺少 SRE 把关

### 意外发现 (Where We Got Lucky)
- 故障发生在低峰期，影响用户较少
- 数据库本身未崩溃，只是连接拒绝

## 改进措施 (Action Items)

| 措施 | 负责人 | 截止日期 | 优先级 | 状态 |
|------|--------|---------|--------|------|
| 更新所有服务连接池配置 | @devops | 2026-05-28 | P0 | 待开始 |
| 添加连接池使用率监控 | @sre | 2026-05-25 | P0 | 待开始 |
| 更新上线检查清单 | @sre-lead | 2026-05-30 | P1 | 待开始 |
| SRE 评审所有新服务上线 | @sre-lead | 2026-06-01 | P1 | 待开始 |

## 无责声明 (Blameless Statement)

本复盘采用无责原则。所有参与者在当时都做出了基于可用信息的最佳决策。
问题的根源在于系统和流程，而非个人。

---
复盘完成日期: 2026-05-22
下次审查: 2026-06-22
```

## 无责文化的核心

```
❌ "张三配置错了连接池"
✅ "连接池配置流程缺少审查环节"

❌ "李四没有及时响应告警"
✅ "告警信息不够清晰，On-call 手册缺少该场景指导"

❌ "测试团队没有测出这个问题"
✅ "测试环境未模拟生产环境负载，缺乏压力测试"
```

## 相关

- [[domain-09-reliability-engineering/06-postmortem/02-postmortem-culture-guide]]
