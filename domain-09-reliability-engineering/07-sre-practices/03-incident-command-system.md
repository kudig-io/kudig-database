---
title: 事故指挥系统
description: '| **Operations Lead** | 技术执行，故障排查和修复 |'
category: domain
tags:
- sre
- incident-management
- on-call
- response
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 事故指挥系统 是什么
- 如何 事故指挥系统
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- 事故指挥系统
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
---

# 事故指挥系统

## ICS 核心角色

| 角色 | 职责 |
|------|------|
| **Incident Commander (IC)** | 总指挥官，决策和协调 |
| **Operations Lead** | 技术执行，故障排查和修复 |
| **Communications Lead** | 对外沟通，状态页更新 |
| **Scribe** | 记录时间线和决策 |

## 事故严重级别

| 级别 | 定义 | 响应时间 | 通知范围 |
|------|------|---------|---------|
| **P0** | 服务完全不可用 | 5 分钟 | VP + 全员 |
| **P1** | 核心功能严重受损 | 15 分钟 | 总监 + 相关团队 |
| **P2** | 部分功能受影响 | 1 小时 | 团队负责人 |
| **P3** | 轻微影响 | 4 小时 | On-call |

## 通信模板

```
【事故通告】INC-2026-001
状态: 🔴 进行中
影响: 订单服务延迟增加，约 30% 用户受影响
开始: 14:32 UTC
当前: 已定位根因，正在修复
ETA: 15:30 UTC
联系: #incident-2026-001
```

## 相关

- [[domain-09-reliability-engineering/06-postmortem/01-blameless-postmortem-template]]
