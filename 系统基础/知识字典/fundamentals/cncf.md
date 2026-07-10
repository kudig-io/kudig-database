---
title: 云原生计算基金会
description: CNCF（Cloud Native Computing Foundation，云原生计算基金会）是 Linux Foundation 旗下的开源基金会，致力于推...
summary: CNCF（Cloud Native Computing Foundation，云原生计算基金会）是 Linux Foundation 旗下的开源基金会，致力于推...
category: dictionary
tags:
- k8s
- glossary
- cncf
- cloud-native
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 云原生计算基金会 是什么
- CNCF (Cloud Native Computing Foundation) 详解
trigger_keywords:
- 云原生计算基金会
- CNCF (Cloud Native Computing Foundation)
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 云原生计算基金会

> **英文名**: CNCF (Cloud Native Computing Foundation)

## 概述

CNCF（Cloud Native Computing Foundation，云原生计算基金会）是 Linux Foundation 旗下的开源基金会，致力于推动云原生计算的普及。它托管了 Kubernetes、Prometheus、Envoy 等核心云原生项目。

## 核心概念/原理

### 项目成熟度

CNCF 项目分为三个成熟度级别：

| 级别 | 含义 | 代表项目 |
|------|------|---------|
| **Sandbox** | 实验阶段 | 早期探索性项目 |
| **Incubating** | 孵化阶段 | 活跃开发，社区增长中 |
| **Graduated** | 毕业阶段 | 生产就绪，广泛采用 |

### 毕业项目（部分）

- **Kubernetes**：容器编排平台。
- **Prometheus**：监控系统。
- **Envoy**：服务代理。
- **CoreDNS**：DNS 服务。
- **containerd**：容器运行时。
- **Fluentd**：日志收集。
- **Jaeger**：分布式追踪。
- **Vitess**：MySQL 水平扩展。

## 关键机制或特性

- CNCF 成立于 2015 年，由 Google 捐赠 Kubernetes 项目而发起。
- 截至 2026 年，CNCF 托管 200+ 个开源项目。
- CNCF Landscape 是了解云原生生态的权威参考。
- TOC（Technical Oversight Committee）负责项目评审和技术方向。

## 使用场景与最佳实践

- 技术选型时参考 CNCF 项目成熟度级别。
- 关注 CNCF 毕业项目，优先用于生产环境。
- 使用 CNCF Landscape 了解云原生生态全景。
- 关注新兴项目（如 eBPF、Wasm 相关项目）。

## 参考链接

- [CNCF (Cloud Native Computing Foundation) - Official Documentation](https://www.cncf.io/)

## Related

- [[系统基础/知识字典/workloads/pod.md|Pod]]
- [[系统基础/知识字典/fundamentals/container.md|Container]]
- [[系统基础/知识字典/fundamentals/node.md|Node]]
- [[系统基础/知识字典/fundamentals/namespace.md|Namespace]]
- [[系统基础/知识字典/fundamentals/cluster.md|Cluster]]


<!-- risk-assessed -->
