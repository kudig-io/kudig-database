---
title: Drasi (entities)
description: '## 概述'
summary: 'Drasi 是由 Microsoft 开发的数据变更处理平台，允许你持续检测数据源中的变更并自动做出反应。它使用 Continuous Query（持续查询）对来自数据库、消息队列、事件流等多种数据源的变更进行实时过滤、聚合和关联，当查询结果发生变化时触发下游动作（如发送通知、调用 API、更新其他系统）。'
category: entities
tags:
- k8s
- cncf
- streaming
- drasi
- crd
- operator
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Drasi 是什么
- 如何 Drasi
trigger_keywords:
- Drasi
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Drasi

> **CNCF 状态**: Sandbox | **类别**: Streaming | **主要语言**: Rust, C#

## 概述

Drasi 是一个 CNCF 沙箱项目，由 Microsoft 主导开发，是一个分布式数据变更事件处理平台。它监控各种数据源（数据库、API、文件系统）的数据变更，并通过 Reaction 框架触发相应的处理逻辑。Drasi 特别适合 Kubernetes 环境中的 GitOps 场景——监控 Git 仓库、K8s API Server、配置存储的数据变更，并实时驱动配置同步、自动化部署等响应式工作流。

## Key Features（核心能力）

- **数据源监控**：支持 K8s API、Git、Azure CosmosDB、SQL Server 等数据源变更监控
- **Reaction 框架**：通过可插拔的 Reaction 处理数据变更事件
- **查询语言**：使用 Cypher 查询语言定义数据变更关注点
- **边缘部署**：支持在边缘节点部署轻量级 Source
- **低延迟**：基于 Rust 实现的核心引擎，提供毫秒级变更检测
- **K8s 原生**：以 CRD 方式定义 Source、Reaction 和 Query

## 架构与工作原理

Drasi 架构包含三个核心概念：Source 监控数据源变更（如 K8s API Watch、Git Poll）；Query 使用 Cypher 表达式定义关注的数据变更模式；Reaction 是变更事件的处理器，如触发 K8s 部署、发送通知、更新数据库。Drasi Controller 管理这些 CRD 资源的生命周期，Query Engine 基于 Rust 实现高性能的事件匹配和分发。

## K8s 集成

Drasi 以 Kubernetes CRD 形式部署：Source CRD 定义数据源连接和监控规则；Query CRD 定义 Cypher 查询表达式；Reaction CRD 定义事件处理动作。Drasi Controller 监听这些 CRD 的变化并协调相应的处理组件。典型用法是监控 K8s ConfigMap/Secret 变更，自动触发边缘节点的配置同步。

## 生产用例

- **GitOps 自动化**：监控 Git 仓库变更自动触发部署
- **边缘配置同步**：实时同步中心集群配置到边缘节点
- **数据变更通知**：数据库变更事件驱动下游处理
- **K8s 事件响应**：监控 K8s 资源变更并触发自动化工作流

## 安装与配置

```bash
# 🟢 安装 Drasi 平台
kubectl apply -f https://github.com/drasi-project/drasi-platform/releases/latest/download/drasi.yaml

# 🟢 验证安装
kubectl get pods -n drasi-system
kubectl get crd | grep drasi.io

# 🟢 安装 Drasi CLI
curl -fsSL https://raw.githubusercontent.com/drasi-project/drasi-platform/main/cli/install.sh | bash

# 🟢 查看 Source 状态
kubectl get sources -A

# 🟢 查看 Query 状态
kubectl get queries -A

# 🟢 查看 Reaction 状态
kubectl get reactions -A

# 🟡 删除 Source
kubectl delete source <source-name> -n <namespace>
```

### Source CRD 示例

```yaml
apiVersion: drasi.io/v1alpha1
kind: Source
metadata:
  name: k8s-source
  namespace: drasi-system
spec:
  kind: Kubernetes
  properties:
    kubeConfig: ""  # 空表示使用 in-cluster 配置
    namespaces:
    - default
    - production
    resources:
    - kind: Deployment
      apiVersion: apps/v1
    - kind: ConfigMap
      apiVersion: v1
```

### Query CRD 示例

```yaml
apiVersion: drasi.io/v1alpha1
kind: Query
metadata:
  name: config-change-detector
  namespace: drasi-system
spec:
  sources:
    k8s-source:
      kind: Kubernetes
  query: |
    MATCH (c:ConfigMap)
    WHERE c.metadata.namespace = 'production'
    AND c.metadata.labels['app'] = 'my-service'
    RETURN c.metadata.name AS name, c.data AS data
  reactions:
  - reaction: notify-reaction
```

### Reaction CRD 示例

```yaml
apiVersion: drasi.io/v1alpha1
kind: Reaction
metadata:
  name: notify-reaction
  namespace: drasi-system
spec:
  kind: Debug
  properties:
    logLevel: info
  # 或使用 HTTP Reaction
  # kind: Http
  # properties:
  #   url: https://webhook.example.com/drasi-events
  #   method: POST
  #   headers:
  #     Content-Type: application/json
```

## 运维操作

### 常用命令

```bash
# 🟢 查看 Drasi 组件日志
kubectl logs -n drasi-system -l app=drasi-controller --tail=100
kubectl logs -n drasi-system -l app=drasi-query-engine --tail=100

# 🟢 查看 Source 连接状态
kubectl describe source k8s-source -n drasi-system

# 🟢 查看 Query 执行统计
kubectl describe query config-change-detector -n drasi-system

# 🟡 重启 Query Engine
kubectl rollout restart deployment/drasi-query-engine -n drasi-system

# 🟢 查看 Drasi 事件流
kubectl logs -n drasi-system -l app=drasi-reaction --tail=50 -f
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Source 无法连接 | RBAC 权限不足 | `kubectl describe source <name>` | 检查 ServiceAccount 权限 |
| Query 无结果 | Cypher 语法错误/Source 未就绪 | `kubectl describe query <name>` | 修正查询语法，确认 Source Active |
| Reaction 未触发 | Reaction 配置错误/网络不通 | `kubectl logs -l app=drasi-reaction` | 检查 Reaction 连接配置 |
| 高延迟 | 数据源变更量过大 | `kubectl top pod -n drasi-system` | 增加 Query Engine 副本/资源 |
| Pod CrashLoopBackOff | 配置格式错误 | `kubectl logs <pod> -n drasi-system --previous` | 修正 CRD 配置 |

### 排查流程

```
1. kubectl get sources,queries,reactions -A → 确认资源状态
2. kubectl describe source <name> → 检查连接和同步状态
3. kubectl logs -l app=drasi-query-engine → 查看查询引擎日志
4. kubectl logs -l app=drasi-reaction → 查看 Reaction 处理日志
5. 验证 Cypher 查询语法正确性
```

## 生产案例

### 案例1: K8s 配置变更自动同步
- **场景**: 多集群环境，中心集群 ConfigMap 变更需实时同步到边缘
- **方案**: Source 监控中心 ConfigMap → Query 过滤目标标签 → Reaction 触发边缘同步 API
- **效果**: 配置传播延迟从分钟级降至秒级

### 案例2: GitOps 增强
- **场景**: 需要在 ArgoCD 同步前进行自定义验证
- **方案**: Source 监控 Git 仓库 → Query 匹配特定路径变更 → Reaction 触发验证流水线
- **效果**: 在 GitOps 流程中增加了自动化质量门禁

## 对比替代方案

| 维度 | Drasi | ArgoCD/Flux | Knative Eventing |
|------|-------|-------------|------------------|
| 数据源类型 | 多种 (DB/Git/K8s/API) | 仅 Git | 多种事件源 |
| 查询能力 | Cypher 声明式 | 无 | 有限过滤 |
| 专注领域 | 数据变更检测 | GitOps 同步 | 事件驱动 |
| 成熟度 | Sandbox | Graduated/Incubating | Incubating |
| 边缘支持 | 原生支持 | 有限 | 有限 |

## 检查清单

- [ ] Source 连接配置已验证 (RBAC/网络)
- [ ] Query Cypher 语法已在测试环境验证
- [ ] Reaction 端点可达且认证配置正确
- [ ] Query Engine 资源 requests/limits 已设置
- [ ] 监控 Drasi 组件健康状态
- [ ] 配置变更事件有日志审计

## Related

- [[youki]] — youki
- [[easegress]] — Easegress
- [[perses]] — Perses
- [[tremor]] — Tremor
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- drasi
- [[23-实体/nats.md|[[NATS|NATS]]]]
- [[23-实体/cncf-infrastructure.md|[[CNCF 基础设施与混沌工程项目全景|CNCF 基础设施与混沌工程项目全景]]]] — Cross-reference


<!-- risk-assessed -->
