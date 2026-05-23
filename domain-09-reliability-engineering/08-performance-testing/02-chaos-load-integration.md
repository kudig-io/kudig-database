---
title: 混沌测试与负载测试集成
description: → 准备回滚方案
category: domain
tags:
- chaos-engineering
- load-testing
- integration
- reliability
- prometheus
- grafana
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 混沌测试与负载测试集成 是什么
- 如何 混沌测试与负载测试集成
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- 混沌测试与负载测试集成
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
- prometheus-basics
- monitoring-basics
created: "2026-05-23"
---

# 混沌测试与负载测试集成

## GameDay 概念

**GameDay** = 有组织的、可控的生产环境实验，验证系统在真实负载下的韧性。

## GameDay 流程

```
1. 规划 (2 周前)
   → 确定实验场景
   → 设定成功/失败标准
   → 准备回滚方案

2. 基线测量 (1 周前)
   → 在无故障情况下运行负载测试
   → 记录正常性能指标

3. 执行 (GameDay)
   → 启动负载测试
   → 注入故障
   → 实时监控 SLO

4. 验证
   → SLO 是否达标？
   → 自动恢复是否生效？

5. 复盘
   → 记录发现
   → 制定改进措施
```

## 集成架构

```
┌─────────────┐     ┌─────────────┐
│  Load Test  │     │  Chaos Test │
│   (k6)      │     │ (Chaos Mesh)│
└──────┬──────┘     └──────┬──────┘
       │                   │
       └─────────┬─────────┘
                 ▼
        ┌─────────────────┐
        │  Target System  │
        │  (Production)   │
        └────────┬────────┘
                 ▼
        ┌─────────────────┐
        │   Observability │
        │ (Prometheus/    │
        │  Grafana/SLO)   │
        └─────────────────┘
```

## 相关

- [[domain-09-reliability-engineering/05-chaos-engineering/03-chaos-experiment-design]]
- [[domain-09-reliability-engineering/08-performance-testing/01-load-testing-methodology]]
