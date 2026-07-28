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

OpenFGA 通过 Helm Chart 或 Operator 部署在 Kubernetes 上，支持 PostgreSQL 作为持久化后端。通过 Kubernetes Service 暴露 gRPC 和 HTTP API，应用通过 SDK 或 Envoy 外部认证过滤器调用。生产环境推荐部署多副本 OpenFGA 实现高可用，配合 PostgreSQL 主从或云托管数据库。可与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 Envoy Gateway 集成，在 API 网关层执行授权检查。

## 生产场景

1. **SaaS 多租户权限**: 为每个租户创建独立 Store，管理文档、项目等资源的细粒度权限
2. **协作平台授权**: 类似 Google Docs / Notion 的共享、编辑、评论权限模型
3. **API 网关授权**: 在 Envoy/Kong 网关层集成 OpenFGA，对每个请求执行权限检查
4. **微服务间授权**: 服务网格场景下，基于服务身份和资源关系控制跨服务访问

## 安装与配置

```bash
# Helm 安装 OpenFGA
helm repo add openfga https://openfga.github.io/helm-charts
helm install openfga openfga/openfga \
  --namespace openfga --create-namespace \
  --set datastore.engine=postgres \
  --set datastore.uri="postgresql://user:pass@postgres:5432/openfga" \
  --set replicaCount=3

# 安装 CLI
brew install openfga/tap/fga

# 等待就绪
kubectl wait --for=condition=available deployment/openfga -n openfga --timeout=120s
```

```yaml
# FGA 授权模型示例 (model.fga)
model
  schema 1.1

type user

type organization
  relations
    define admin: [user]
    define member: [user] or admin

type document
  relations
    define owner: [user]
    define editor: [user] or owner
    define viewer: [user, organization#member] or editor
    define can_edit: editor
    define can_view: viewer

type folder
  relations
    define parent: [folder]
    define owner: [user] or owner from parent
    define editor: [user] or owner
    define viewer: [user, organization#member] or editor
```

```bash
# 创建 Store 和授权模型
export FGA_URL=http://localhost:8080
STORE_ID=$(fga store create --name "my-app" --json | jq -r '.store.id')
fga model create --store-id $STORE_ID --file model.fga

# 写入关系元组
fga tuple write --store-id $STORE_ID "user:alice" "editor" "document:report-2024"
fga tuple write --store-id $STORE_ID "user:bob" "viewer" "document:report-2024"
fga tuple write --store-id $STORE_ID "user:charlie" "admin" "organization:acme"

# 权限检查
fga check --store-id $STORE_ID "user:alice" "can_edit" "document:report-2024"  # true
fga check --store-id $STORE_ID "user:bob" "can_edit" "document:report-2024"   # false

# 列出用户有权限的对象
fga list-objects --store-id $STORE_ID "user:alice" "can_view" "document"
```

## 运维操作

```bash
# 🟢 低风险：查看 Store 列表
fga store list

# 🟢 低风险：权限检查
curl -X POST $FGA_URL/stores/$STORE_ID/check -d '{
  "tuple_key": {"user": "user:alice", "relation": "can_edit", "object": "document:report"}
}'

# 🟡 中风险：写入/删除关系元组
fga tuple write --store-id $STORE_ID "user:dave" "editor" "document:new-doc"
fga tuple delete --store-id $STORE_ID "user:dave" "editor" "document:new-doc"

# 🟡 中风险：更新授权模型
fga model create --store-id $STORE_ID --file updated-model.fga

# 🔴 高风险：删除 Store（所有权限数据丢失）
fga store delete --store-id $STORE_ID
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Check 返回错误 | 模型未创建/不匹配 | `fga model list --store-id $ID` | 确认授权模型已创建 |
| 延迟过高 | 关系图遍历深/缓存未命中 | 检查 OpenFGA metrics | 增加 Redis 缓存，优化模型层级 |
| 连接失败 | PostgreSQL 不可达 | `kubectl logs deploy/openfga -n openfga` | 检查 datastore.uri 和网络 |
| 权限结果错误 | 模型定义逻辑错误 | `fga check --verbose` | 使用 Playground 调试模型 |
| 写入失败 | Tuple 冲突/格式错误 | 检查 API 响应错误信息 | 确认 tuple 格式符合模型定义 |

```
排查流程：
├── 服务不可用？
│   ├── kubectl get pods -n openfga → 检查 Pod 状态
│   ├── kubectl logs → 查看启动错误
│   └── 检查 PostgreSQL 连接
├── 权限结果异常？
│   ├── fga check --verbose → 查看详细推理过程
│   ├── 使用 OpenFGA Playground 调试模型
│   └── 检查关系元组是否正确写入
└── 性能问题？
    ├── 检查 Prometheus 指标（check_latency）
    ├── 确认 Redis 缓存已启用
    └── 优化模型层级深度
```

## 生产案例

### 案例 1：SaaS 多租户细粒度权限

- **场景**：协作平台需要实现类 Google Docs 的权限模型（所有者/编辑者/查看者 + 组织继承）
- **排查**：传统 RBAC 无法表达"用户 A 是文档 X 的编辑者，且组织成员自动获得查看权限"
- **方案**：使用 OpenFGA ReBAC 模型，定义 document 类型的 owner/editor/viewer 关系，支持组织继承
- **效果**：权限检查延迟 < 5ms，支持 100万用户 + 10亿关系元组

### 案例 2：API 网关层统一授权

- **场景**：微服务架构中每个服务自行实现权限检查，逻辑分散且不一致
- **排查**：15 个服务各自实现权限逻辑，安全审计发现多处不一致
- **方案**：在 Envoy Gateway 集成 OpenFGA 外部授权，所有请求在网关层统一执行权限检查
- **效果**：消除服务内权限代码，安全审计通过率 100%，权限变更集中管理

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

- [[22-概念/08-可靠性与运维/microservice-resilience-patterns.md|microservice-resilience-patterns]]
- [[22-概念/05-安全/security-defense-depth.md|security-defense-depth]]

## Related

- [[dapr]] — Dapr
- [[envoy]] — Envoy
- [[cert-manager]] — cert-manager
- [[zot]] — zot
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- openfga
- [[23-实体/kubearmor.md|[[kubearmor|KubeArmor]]]]
- [[23-实体/06-安全/tokenetes.md|Tokenetes]]
- [[23-实体/06-安全/containerssh.md|ContainerSSH]]
- [[23-实体/06-安全/parsec.md|Parsec]]
- [[23-实体/06-安全/athenz.md|Athenz]]
- [[23-实体/06-安全/keylime.md|Keylime]]
- [[23-实体/06-安全/cartography.md|Cartography]]
- [[23-实体/06-安全/bank-vaults.md|Bank-Vaults]]
- [[23-实体/06-安全/hexa.md|Hexa]]
- [[23-实体/06-安全/paralus.md|Paralus]]
- [[23-实体/15-参考与索引/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
