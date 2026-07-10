---
title: SchemaHero 数据库 Schema 管理
description: SchemaHero 是 Replicated 开源的 CNCF Sandbox 项目，以 GitOps 方式管理数据库 Schema 变更，通过声明式
  YAM...
summary: SchemaHero 是 Replicated 开源的 CNCF Sandbox 项目，以 GitOps 方式管理数据库 Schema 变更，通过声明式
  YAM...
category: dictionary
tags:
- k8s
- glossary
- configuration
- database
- gitops
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SchemaHero 数据库 Schema 管理 是什么
- SchemaHero 详解
trigger_keywords:
- SchemaHero 数据库 Schema 管理
- SchemaHero
- dictionary
prerequisites:
- kubernetes
---



# SchemaHero 数据库 Schema 管理（SchemaHero）

## 概述

SchemaHero 是 Replicated 开源的 CNCF Sandbox 项目，以 GitOps 方式管理数据库 Schema 变更，通过声明式 YAML 定义表结构，自动生成和执行 Migration SQL。

## 核心概念/原理

- **GitOps Schema**：YAML 声明式管理数据库 Schema
- **自动 Migration**：自动生成 ALTER/CREATE SQL
- **CNCF Sandbox**：Replicated 主导
- **多数据库**：支持 PostgreSQL/MySQL/CockroachDB/SQLite

## 关键机制或特性

- Table CRD 声明式定义表结构
- 自动检测 Schema 差异
- 生成并执行 Migration SQL
- 支持索引、约束、外键
- Plan/Apply 两阶段审核
- K8s Operator 模式部署

## 使用场景与最佳实践

- 数据库 Schema 的版本控制
- GitOps 方式的数据库变更管理
- 微服务数据库的独立 Schema 管理
- CI/CD Pipeline 中的 Schema 迁移
- 合规要求下的 Schema 变更审计

## 参考链接

- https://schemahero.io/
- https://github.com/schemahero/schemahero

## Related

- [[domain-17-system-foundation/知识字典/storage/cloudnativepg.md|CloudNativePG]]
- [[domain-17-system-foundation/知识字典/operations/flux.md|Flux]]
- [[domain-17-system-foundation/知识字典/operations/argo.md|Argo]]
