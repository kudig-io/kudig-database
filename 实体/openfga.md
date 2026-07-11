---
title: OpenFGA (entities)
description: '## 概述'
summary: 'OpenFGA 是细粒度授权（Fine-Grained Authorization）系统，基于 Google Zanzibar 论文设计。'
category: entities
tags:
- k8s
- cncf
- security
- openfga
- envoy
- rbac
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenFGA 是什么
- 如何 OpenFGA
trigger_keywords:
- OpenFGA
prerequisites:
- kubectl-basics
- tls-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# OpenFGA

> **CNCF 状态**: Incubating | **类别**: Security | **主要语言**: Go

## 概述

OpenFGA 是细粒度授权（Fine-Grained Authorization）系统，由 Auth0/Okta 开源并贡献给 CNCF，2023 年从沙箱晋升为孵化项目。它基于 Google Zanzibar 论文设计，提供灵活的关系型访问控制（ReBAC），支持复杂的权限模型如 RBAC、ABAC 和 ReBAC。OpenFGA 允许开发者定义任意的关系型权限模型（如"用户 X 是文档 Y 的编辑者"），并以毫秒级延迟执行权限检查。与传统的角色映射方案不同，OpenFGA 将权限表示为关系图（Relationship Tuple），支持继承、组合和条件推理，能够处理数百万用户和数十亿关系的规模。它提供 Go、Node.js、Python、Java、.NET 多语言 SDK。

## 核心能力

- **关系型授权**: 基于用户-对象-关系三元组（Tuple）的灵活权限模型
- **高性能**: 毫秒级权限检查响应，支持每秒百万级检查
- **DSL 建模**: 简洁的授权模型定义语言，支持继承和组合
- **多租户**: 原生支持多个 Store（授权模型隔离）
- **SDK 支持**: Go、Node.js、Python、Java、.NET 全语言覆盖
- **水平扩展**: 无状态设计，可水平扩展支持海量权限数据

## 架构

OpenFGA 的核心设计灵感来自 Google Zanzibar：

- **Authorization Model**: 使用 FGA DSL 定义实体类型和关系规则
- **Relationship Tuple**: 用户-关系-对象三元组（如 `user:alice editor document:report`）
- **Tuple Store**: 存储所有关系元组，支持 PostgreSQL 或内存存储
- **Check Engine**: 解析授权模型，遍历关系图，返回 allow/deny
- **API Layer**: gRPC 和 HTTP/JSON API，支持批量检查和流式写入
- **Cache Layer**: 基于 Redis 或内存的查询缓存，加速重复检查

权限检查流程：`应用 → OpenFGA Check API → 解析模型 → 遍历关系图 → allow/deny`

## K8s 集成

OpenFGA 通过 Helm Chart 或 Operator 部署在 Kubernetes 上，支持 PostgreSQL 作为持久化后端。通过 Kubernetes Service 暴露 gRPC 和 HTTP API，应用通过 SDK 或 Envoy 外部认证过滤器调用。生产环境推荐部署多副本 OpenFGA 实现高可用，配合 PostgreSQL 主从或云托管数据库。可与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 Envoy Gateway 集成，在 API 网关层执行授权检查。

## 生产场景

1. **SaaS 多租户权限**: 为每个租户创建独立 Store，管理文档、项目等资源的细粒度权限
2. **协作平台授权**: 类似 Google Docs / Notion 的共享、编辑、评论权限模型
3. **API 网关授权**: 在 Envoy/Kong 网关层集成 OpenFGA，对每个请求执行权限检查
4. **微服务间授权**: 服务网格场景下，基于服务身份和资源关系控制跨服务访问

## 安装

```bash
# Helm 安装 OpenFGA
helm repo add openfga https://openfga.github.io/helm-charts
helm install openfga openfga/openfga \
  --set datastore.engine=postgres \
  --set datastore.uri=postgresql://user:pass@postgres:5432/openfga \
  --namespace openfga --create-namespace

# 安装 CLI
brew install openfga/tap/fga

# 创建授权模型
fga model create --store-id $STORE_ID --file model.fga
```

## 对比

| 特性 | OpenFGA | OPA/Cedar | Casbin | Auth0 |
|------|---------|-----------|--------|-------|
| 模型类型 | ReBAC | RBAC/ABAC | RBAC/ABAC | RBAC |
| Zanzibar | ✅ 原生 | ❌ | ❌ | ✅ |
| 性能 | 极高 | 高 | 中 | 高 |
| 开源 | ✅ | ✅ | ✅ | ❌ |

## 架构定位

在 CNCF 生态中，OpenFGA 属于 **Security** 类别，为云原生应用提供细粒度关系型授权能力。

## 参考链接

- [[概念/microservice-resilience-patterns.md|microservice-resilience-patterns]]
- [[概念/security-defense-depth.md|security-defense-depth]]

## Related

- [[dapr]] — Dapr
- [[envoy]] — Envoy
- [[cert-manager]] — cert-manager
- [[zot]] — zot
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- openfga
- [[实体/kubearmor.md|[[KubeArmor|KubeArmor]]]]
- [[实体/tokenetes.md|Tokenetes]]
- [[实体/containerssh.md|ContainerSSH]]
- [[实体/parsec.md|Parsec]]
- [[实体/athenz.md|Athenz]]
- [[实体/keylime.md|Keylime]]
- [[实体/cartography.md|Cartography]]
- [[实体/bank-vaults.md|Bank-Vaults]]
- [[实体/hexa.md|Hexa]]
- [[实体/paralus.md|Paralus]]
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
