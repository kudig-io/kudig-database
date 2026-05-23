---
title: 混沌工程与灾备演练的结合
description: → 管理层参与
category: synthesis
tags:
- chaos-engineering
- disaster-recovery
- game-day
- reliability
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 混沌工程与灾备演练的结合 是什么
- 如何 混沌工程与灾备演练的结合
trigger_keywords:
- 混沌工程与灾备演练的结合
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# 混沌工程与灾备演练的结合

## 分层验证体系

```
日常 (Daily):
  → 自动化混沌实验（小范围）
  → 验证自愈能力
  → 持续验证 SLO

周度 (Weekly):
  → 有计划的中等规模实验
  → 验证故障转移流程
  → 团队轮换 On-call 响应

月度 (Monthly):
  → 跨服务依赖故障实验
  → 验证灾难恢复手册

季度 (Quarterly):
  → 全面 GameDay
  → 生产环境大规模演练
  → 管理层参与
```

## GameDay 流程

```
1. 场景设定
   → "Region A 完全不可用"

2. 注入故障
   → Chaos Mesh 网络分区
   → 模拟 DNS 故障

3. 团队响应
   → 执行 DR Playbook
   → 流量切换到 Region B

4. 验证恢复
   → SLO 达标
   → 业务功能正常

5. 复盘改进
   → 更新 Playbook
   → 修复发现的问题
```

## 相关 Domain

- [[domain-09-reliability-engineering/05-chaos-engineering/01-chaos-engineering-overview]]
- [[domain-09-reliability-engineering/09-disaster-recovery-playbooks/01-dr-scenarios-catalog]]
- [[domain-09-reliability-engineering/08-performance-testing/02-chaos-load-integration]]
