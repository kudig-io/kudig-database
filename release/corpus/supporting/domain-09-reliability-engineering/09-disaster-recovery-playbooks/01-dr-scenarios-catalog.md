---
title: 灾备场景目录
description: '| 区域级网络中断 | 整个 Region | 30 分钟 | 0 |'
summary: '| 区域级网络中断 | 整个 Region | 30 分钟 | 0 |'
category: domain
tags:
- disaster-recovery
- dr
- scenarios
- sre
- etcd
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 灾备场景目录 是什么
- 如何 灾备场景目录
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- 灾备场景目录
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
- etcd-basics
---



# 灾备场景目录

## 场景分类

### 基础设施层

| 场景 | 影响范围 | RTO 目标 | RPO 目标 |
|------|---------|---------|---------|
| 单可用区问题 | 1 个 AZ | 5 分钟 | 0 |
| 区域级网络中断 | 整个 Region | 30 分钟 | 0 |
| [[Kubernetes|Kubernetes]] 控制面问题 | 集群管理 | 15 分钟 | 0 |
|  [[etcd|etcd]] 数据损坏 | 集群状态 | 30 分钟 | 5 分钟 |

### 应用层

| 场景 | 影响范围 | RTO 目标 | RPO 目标 |
|------|---------|---------|---------|
| 核心服务级联问题 | 多个服务 | 10 分钟 | 0 |
| 数据库主节点问题 | 数据服务 | 5 分钟 | 0 |
| 缓存集群完全失效 | 读性能 | 15 分钟 | 0 |
| 消息队列堆积 | 异步处理 | 30 分钟 | 5 分钟 |

### 外部依赖

| 场景 | 影响范围 | RTO 目标 | RPO 目标 |
|------|---------|---------|---------|
| 第三方支付服务中断 | 交易功能 | 60 分钟 | 0 |
| CDN 问题 | 静态资源 | 10 分钟 | 0 |
| DNS 服务商问题 | 全站访问 | 15 分钟 | 0 |
| 云厂商 API 限流 | 自动化运维 | 30 分钟 | 0 |

## 相关

- domain-09-reliability-engineering/02-disaster-recovery/README.md
