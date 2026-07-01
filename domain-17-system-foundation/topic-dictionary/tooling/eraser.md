---
title: Eraser 镜像清理
description: Eraser 是微软开源的 CNCF Sandbox 项目，自动清理 Kubernetes 节点上未使用的容器镜像，释放磁盘空间，支持基于漏洞扫描结果的自动镜像...
summary: Eraser 是微软开源的 CNCF Sandbox 项目，自动清理 Kubernetes 节点上未使用的容器镜像，释放磁盘空间，支持基于漏洞扫描结果的自动镜像...
category: dictionary
tags:
- k8s
- glossary
- tooling
- operations
- cleanup
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Eraser 镜像清理 是什么
- Eraser 详解
trigger_keywords:
- Eraser 镜像清理
- Eraser
- dictionary
prerequisites:
- kubernetes
---



# Eraser 镜像清理（Eraser）

## 概述

Eraser 是微软开源的 CNCF Sandbox 项目，自动清理 Kubernetes 节点上未使用的容器镜像，释放磁盘空间，支持基于漏洞扫描结果的自动镜像删除。

## 核心概念/原理

- **自动清理**：定时清理节点上未被使用的镜像
- **漏洞驱动**：基于漏洞扫描结果删除有问题镜像
- **CNCF Sandbox**：微软主导的轻量运维工具
- **DaemonSet 部署**：每个节点自动运行清理任务

## 关键机制或特性

- ImageJob CRD 定义清理任务
- 支持 Trivy 漏洞扫描集成
- 可配置的保留策略（按年龄/大小/名称）
- 定时调度（CronJob 式）
- 非使用镜像自动识别和删除
- Prometheus 指标导出

## 使用场景与最佳实践

- 节点磁盘空间管理
- 自动化镜像垃圾回收
- 安全合规的镜像生命周期管理
- 大规模集群的镜像清理自动化
- 开发环境的定期空间回收

## 参考链接

- https://eraser-dev.github.io/eraser/
- https://github.com/eraser-dev/eraser

## Related

- [[domain-17-system-foundation/topic-dictionary/fundamentals/docker.md|Docker]]
- [[domain-17-system-foundation/topic-dictionary/security/trivy.md|Trivy]]
- [[domain-17-system-foundation/topic-dictionary/operations/k8sgpt.md|K8sGPT]]
