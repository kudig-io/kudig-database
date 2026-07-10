---
title: ChaosBlade 混沌工程
description: ChaosBlade 是阿里巴巴开源的混沌工程工具，支持对 Java/C++/Node.js 应用和 Kubernetes/Docker/物理机环境的故障注入，...
summary: ChaosBlade 是阿里巴巴开源的混沌工程工具，支持对 Java/C++/Node.js 应用和 Kubernetes/Docker/物理机环境的故障注入，...
category: dictionary
tags:
- k8s
- glossary
- operations
- chaos-engineering
- alibaba
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- ChaosBlade 混沌工程 是什么
- ChaosBlade 详解
trigger_keywords:
- ChaosBlade 混沌工程
- ChaosBlade
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# ChaosBlade 混沌工程（ChaosBlade）

## 概述

ChaosBlade 是阿里巴巴开源的混沌工程工具，支持对 Java/C++/Node.js 应用和 Kubernetes/Docker/物理机环境的故障注入，是国内最广泛使用的混沌工程框架之一。

## 核心概念/原理

- **多平台**：K8s/Docker/物理机/云环境
- **多语言**：Java/C++/Node.js/Go 应用级故障注入
- **阿里开源**：经过双11大规模验证
- **CNCF Landscape**：混沌工程领域代表项目

## 关键机制或特性

- `blade create` 创建故障实验
- Pod/Container/Node/Network/Process/JVM 故障类型
- 应用级故障（方法延迟/异常/返回值修改）
- 文件系统故障（读写延迟/磁盘满）
- 网络故障（延迟/丢包/DNS 异常）
- ChaosBlade Operator（K8s CRD 管理）
- 实验自动恢复

## 使用场景与最佳实践

- 微服务的弹性验证
- 生产环境的故障演练
- 数据库/中间件的故障注入
- Java 应用的方法级故障模拟
- 双11前的全链路压测和故障演练

## 参考链接

- https://chaosblade.io/
- https://github.com/chaosblade-io/chaosblade

## Related

- [[domain-17-system-foundation/知识字典/operations/chaos-mesh.md|Chaos Mesh]]
- [[domain-17-system-foundation/知识字典/operations/litmus.md|LitmusChaos]]
- [[domain-17-system-foundation/知识字典/operations/krkn.md|Krkn]]


<!-- risk-assessed -->
