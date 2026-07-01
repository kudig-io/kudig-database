---
title: Cozystack 云操作系统
description: 'Cozystack 是开源的 Kubernetes 云操作系统，在 K8s 之上提供完整的 PaaS 能力（VM/数据库/存储/K8s-as-a-Service...'
category: dictionary
tags:
- k8s
- glossary
- platform-engineering
- cloud
- paas
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cozystack 云操作系统 是什么
- Cozystack 详解
trigger_keywords:
- Cozystack 云操作系统
- Cozystack
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# Cozystack 云操作系统（Cozystack）

## 概述

Cozystack 是开源的 Kubernetes 云操作系统，在 K8s 之上提供完整的 PaaS 能力（VM/数据库/存储/K8s-as-a-Service），通过统一 API 管理多种基础设施服务。

## 核心概念/原理

- **云操作系统**：在 K8s 上构建完整的云平台
- **多服务**：VM/DB/存储/K8s 集群的统一管理
- **开源**：完全开源的 PaaS 方案
- **API 驱动**：统一的 RESTful API 管理所有服务

## 关键机制或特性

- Tenant CRD 多租户管理
- 虚拟化管理（KubeVirt 集成）
- 数据库服务（PostgreSQL/MySQL/Redis）
- 对象存储和块存储
- Kubernetes-as-a-Service
- 计费和使用计量
- Cozystack Dashboard

## 使用场景与最佳实践

- 私有云/混合云的 PaaS 建设
- 企业内部的基础设施服务平台
- IDC 的云服务化转型
- 开发和测试环境的自助服务
- 多租户的云平台运营

## 参考链接

- https://cozystack.io/
- https://github.com/cozystack/cozystack

## Related

- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/kubevirt.md|KubeVirt]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/rancher.md|Rancher]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/backstage.md|Backstage]]
