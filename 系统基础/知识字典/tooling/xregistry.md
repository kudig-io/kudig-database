---
title: xRegistry 增强型注册表
description: xRegistry 是 CNCF 社区的增强型 OCI 注册表实现，在标准 Distribution 基础上扩展了搜索、标签管理、配额控制和多仓库同步等企业级功...
summary: xRegistry 是 CNCF 社区的增强型 OCI 注册表实现，在标准 Distribution 基础上扩展了搜索、标签管理、配额控制和多仓库同步等企业级功...
category: dictionary
tags:
- k8s
- glossary
- tooling
- registry
- oci
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- xRegistry 增强型注册表 是什么
- xRegistry 详解
trigger_keywords:
- xRegistry 增强型注册表
- xRegistry
- dictionary
prerequisites:
- kubernetes
---



# xRegistry 增强型注册表（xRegistry）

## 概述

xRegistry 是 CNCF 社区的增强型 OCI 注册表实现，在标准 Distribution 基础上扩展了搜索、标签管理、配额控制和多仓库同步等企业级功能。

## 核心概念/原理

- **增强 OCI**：在标准 OCI Registry 基础上扩展
- **企业功能**：搜索/配额/同步/审计
- **CNCF 社区**：活跃的 Registry 增强社区
- **多后端**：支持文件系统/S3/GCS 存储

## 关键机制或特性

- 全文搜索（镜像/标签/注释）
- 配额管理（存储/拉取频率限制）
- 多仓库同步（跨区域复制）
- 访问控制（RBAC）
- 审计日志
- 标签管理（不可变标签/保留策略）
- Garbage Collection 增强

## 使用场景与最佳实践

- 企业内部的高级 OCI Registry
- 多区域镜像同步
- 存储配额和成本控制
- 合规要求下的审计和保留策略
- Harbor 的轻量替代方案

## 参考链接

- https://github.com/xregistry/xregistry

## Related

- [[系统基础/知识字典/tooling/distribution.md|Distribution]]
- [[系统基础/知识字典/tooling/harbor.md|Harbor]]
- [[系统基础/知识字典/tooling/zot.md|zot]]
