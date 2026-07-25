---
title: Cloud Custodian 云治理
description: Cloud Custodian（c7n）是 CNCF Sandbox 项目，多云环境的统一治理引擎，通过声明式 YAML 策略管理云资源的合规性、安全和成本优化...
summary: Cloud Custodian（c7n）是 CNCF Sandbox 项目，多云环境的统一治理引擎，通过声明式 YAML 策略管理云资源的合规性、安全和成本优化...
category: dictionary
tags:
- k8s
- glossary
- operations
- cloud
- governance
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cloud Custodian 云治理 是什么
- Cloud Custodian 详解
trigger_keywords:
- Cloud Custodian 云治理
- Cloud Custodian
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Cloud Custodian 云治理（Cloud Custodian）

## 概述

Cloud Custodian（c7n）是 CNCF Sandbox 项目，多云环境的统一治理引擎，通过声明式 YAML 策略管理云资源的合规性、安全和成本优化，支持 AWS/Azure/GCP。

## 核心概念/原理

- **多云治理**：统一管理 AWS/Azure/GCP 的资源策略
- **声明式策略**：YAML 定义资源过滤和操作
- **CNCF Sandbox**：Capital One 主导
- **事件驱动**：响应云事件自动执行策略

## 关键机制或特性

- Policy YAML 定义治理规则
- Filters 资源过滤（标签/年龄/大小/成本等）
- Actions 资源操作（停止/终止/通知/标记）
- 支持 Cron 和事件触发
- 多账户/多区域管理
- 输出到 S3/CloudWatch/SQS
- c7n-org 多账户编排

## 使用场景与最佳实践

- 云资源的合规性检查
- 闲置资源的自动清理
- 安全配置的统一审计
- 成本优化（自动关闭非工作时段资源）
- 标签策略的强制执行

## 参考链接

- https://cloudcustodian.io/
- https://github.com/cloud-custodian/cloud-custodian

## Related

- [[domain-17-system-foundation/知识字典/security/opa.md|OPA]]
- [[domain-17-system-foundation/知识字典/observability/opencost.md|OpenCost]]
- [[domain-17-system-foundation/知识字典/platform-engineering/crossplane.md|Crossplane]]


<!-- risk-assessed -->
