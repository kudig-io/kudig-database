---
title: 混沌工程概述与原则
description: '# 混沌工程概述与原则'
category: domain
tags:
- chaos-engineering
- reliability
- sre
- testing
- scheduler
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 混沌工程概述与原则 是什么
- 如何 混沌工程概述与原则
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- 混沌工程概述与原则
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
created: "2026-05-23"
---

# 混沌工程概述与原则

> **定义**: 混沌工程是在分布式系统上进行实验的学科，目的是建立对系统抵御生产环境中失控条件能力的信心。

## 五大原则

### 1. 建立稳态假设 (Build a Hypothesis around Steady State Behavior)

```
稳态: 系统正常运行的可度量行为

示例假设:
  "当单个可用区故障时，订单服务的 P99 延迟增加不超过 50%"
  "当 30% 的 Pod 被随机终止时，API 错误率不超过 1%"
```

### 2. 引入真实世界事件 (Vary Real-world Events)

```
真实故障类型:
├── 基础设施层
│   ├── 节点故障 (Node failure)
│   ├── 网络分区 (Network partition)
│   └── 磁盘故障 (Disk failure)
├── Kubernetes 层
│   ├── Pod 随机终止 (Pod kill)
│   ├── 调度器故障 (Scheduler failure)
│   └── API Server 延迟 (API latency)
├── 应用层
│   ├── 依赖服务超时 (Dependency timeout)
│   ├── 数据库连接池耗尽 (Connection pool exhaust)
│   └── 内存泄漏 (Memory leak)
└── 运维层
    ├── 配置错误 (Config error)
    ├── 证书过期 (Certificate expiry)
    └── 人为操作失误 (Human error)
```

### 3. 生产环境运行 (Run Experiments in Production)

```
为什么必须在生产环境?
  - 测试环境 ≠ 生产环境（流量模式、数据规模、配置差异）
  - 只有在生产环境才能验证真实用户影响

安全措施:
  - 爆炸半径控制（见原则 5）
  - 快速回滚机制
  - 降级开关 (Kill switch)
```

### 4. 自动化持续执行 (Automate Experiments to Run Continuously)

```
手动执行 → 半自动 → 全自动

全自动混沌工程流水线:
  CI/CD → 部署 → 自动混沌实验 → 验证 SLO → 通过/回滚
```

### 5. 最小化爆炸半径 (Minimize Blast [[Radius|Radius]])

```
爆炸半径控制手段:
  - 只对特定用户/流量执行实验
  - 时间窗口限制（低峰期）
  - 快速终止机制
  - 金丝雀范围（1% → 5% → 25%）
```

## 混沌工程成熟度模型

| 级别 | 特征 | 工具 |
|------|------|------|
| **1. 萌芽** | 随机故障注入 | 手动 kubectl delete pod |
| **2. 基础** | 有计划的人工实验 | [[Chaos Mesh|Chaos Mesh]] Dashboard |
| **3. 中级** | 自动化实验，事后分析 | [[Litmus|Litmus]] + CI/CD 集成 |
| **4. 高级** | 生产环境持续运行，自动回滚 | Gremlin / 自研平台 |
| **5. 专家** | 智能故障预测，AI 驱动 | 智能混沌平台 |

## 相关

- deployment]]
- [[domain-09-reliability-engineering/05-chaos-engineering/03-chaos-experiment-design]]
