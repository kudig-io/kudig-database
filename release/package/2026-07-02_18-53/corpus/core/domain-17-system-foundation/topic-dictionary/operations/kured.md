---
title: Kured 节点重启
description: Kured（KUbernetes REboot Daemon）是开源的 K8s 节点重启守护进程，在节点需要重启时（如内核更新）安全地逐节点重启，确保工作负载的...
summary: Kured（KUbernetes REboot Daemon）是开源的 K8s 节点重启守护进程，在节点需要重启时（如内核更新）安全地逐节点重启，确保工作负载的...
category: dictionary
tags:
- k8s
- glossary
- operations
- node
- reboot
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kured 节点重启 是什么
- Kured 详解
trigger_keywords:
- Kured 节点重启
- Kured
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kured 节点重启（Kured）

## 概述

Kured（KUbernetes REboot Daemon）是开源的 K8s 节点重启守护进程，在节点需要重启时（如内核更新）安全地逐节点重启，确保工作负载的平滑迁移。

## 核心概念/原理

- **安全重启**：逐节点排空（cordon + drain）后重启
- **锁机制**：确保同一时间只有一个节点重启
- **社区成熟**：广泛使用的节点维护工具
- **轻量部署**：DaemonSet 方式运行

## 关键机制或特性

- 检测重启信号（`/var/run/reboot-required`）
- 节点排空（Pod 迁移）
- 节点重启
- 节点恢复（uncordon）
- 分布式锁（K8s Lock API）
- 时间窗口控制
- Prometheus 指标

## 使用场景与最佳实践

- 内核更新后的节点重启
- 安全补丁的自动化应用
- 节点维护的自动化编排
- 大规模集群的滚动重启
- OS 更新的自动化管理

## 参考链接

- https://kubereboot.github.io/kured/
- https://github.com/kubereboot/kured

## Related

- [[domain-17-system-foundation/topic-dictionary/operations/upgrade.md|升级]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/flatcar.md|Flatcar]]
- [[domain-17-system-foundation/topic-dictionary/operations/kubean.md|Kubean]]


<!-- risk-assessed -->
