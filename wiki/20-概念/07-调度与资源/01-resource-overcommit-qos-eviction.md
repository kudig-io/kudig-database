---
sources:
- "集群基础/设计原则/21-resource-overcommit-qos-eviction.md"
title: 资源超额订阅 (Overcommit) 与 QoS 驱逐联动
summary: 系统化解析 Kubernetes 资源 overcommit 模型、可压缩/不可压缩资源差异、QoS 与节点驱逐的联动机制。
category: concepts
tags:
- overcommit
- qos
- resource-management
- eviction
- cpu-management
- memory-pressure
tier: core
created: 2026-07-23
updated: 2026-07-23
last_updated: 2026-07
status: stable
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 容量规划
estimated_read_time: 25min
intent_queries:
- Kubernetes overcommit 是什么
- QoS 三类如何判定
- CPU 和内存 overcommit 有何不同
- 节点驱逐如何与 QoS 联动
trigger_keywords:
- overcommit
- 超额订阅
- 超卖
- QoS
- 驱逐
- 压缩
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **内容已并入主场**：[[wiki/01-集群基础与系统/02-设计原则/22-resource-overcommit-qos-eviction.md|资源超额订阅 (Overcommit) 与 QoS 驱逐联动]]（本页为重排去重桩，正文见主场）
