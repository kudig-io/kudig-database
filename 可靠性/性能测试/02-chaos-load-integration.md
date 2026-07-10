---
title: 混沌测试与负载测试集成
description: → 准备回滚方案
summary: → 准备回滚方案
category: domain
tags:
- chaos-engineering
- load-testing
- integration
- reliability
- prometheus
- grafana
tier: supporting
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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
   → 在无问题情况下运行负载测试
   → 记录正常性能指标

3. 执行 (GameDay)
   → 启动负载测试
   → 注入问题
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

- [[可靠性/混沌工程/03-chaos-experiment-design.md|03 chaos experiment design]]
- [[可靠性/性能测试/01-load-testing-methodology.md|01 load testing methodology]]


<!-- risk-assessed -->
