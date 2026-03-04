# Drasi

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://drasi.io/ |
| **GitHub** | https://github.com/drasi-project/drasi-platform |
| **许可证** | Apache-2.0 |
| **开发语言** | Rust, C# |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Drasi 是由 Microsoft 开发的数据变更处理平台，允许你持续检测数据源中的变更并自动做出反应。它使用 Continuous Query（持续查询）对来自数据库、消息队列、事件流等多种数据源的变更进行实时过滤、聚合和关联，当查询结果发生变化时触发下游动作（如发送通知、调用 API、更新其他系统）。

### 核心特性

- **持续查询**: 基于 Cypher 查询语言定义变更检测逻辑
- **多数据源**: 支持 PostgreSQL、CosmosDB、Gremlin 图数据库、Kubernetes 等
- **实时响应**: 数据变更后毫秒级触发反应
- **多种反应器**: 支持 SignalR、Webhook、Azure Event Grid、Dapr 等输出
- **声明式配置**: 通过 YAML 声明数据源、查询和反应器
- **变更传播**: 自动维护查询结果状态，只传播差异变更

---

## 架构设计

```
┌─────────────────────────────────────────────────┐
│                 Drasi Platform                    │
│                                                   │
│  ┌───────────────────────────────────────┐       │
│  │           Sources (数据源)             │       │
│  │  ┌────────┐ ┌────────┐ ┌──────────┐  │       │
│  │  │PostgreSQL│ │CosmosDB│ │Kubernetes│  │       │
│  │  └────┬───┘ └────┬───┘ └────┬─────┘  │       │
│  └───────┼──────────┼──────────┼─────────┘       │
│          │          │          │                   │
│  ┌───────▼──────────▼──────────▼─────────┐       │
│  │     Continuous Queries (持续查询)      │       │
│  │  ┌─────────────────────────────┐      │       │
│  │  │ Cypher Query Engine         │      │       │
│  │  │ (变更检测 / 结果维护 / 差异) │      │       │
│  │  └──────────────┬──────────────┘      │       │
│  └─────────────────┼─────────────────────┘       │
│                    │                               │
│  ┌─────────────────▼─────────────────────┐       │
│  │        Reactions (反应器)              │       │
│  │  ┌───────┐ ┌───────┐ ┌──────────┐   │       │
│  │  │Webhook│ │SignalR │ │EventGrid │   │       │
│  │  └───────┘ └───────┘ └──────────┘   │       │
│  │  ┌───────┐ ┌───────┐               │       │
│  │  │ Dapr  │ │ Debug │               │       │
│  │  └───────┘ └───────┘               │       │
│  └───────────────────────────────────────┘       │
└─────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装 Drasi CLI

```bash
# 下载 CLI
curl -fsSL https://raw.githubusercontent.com/drasi-project/drasi-platform/main/cli/installers/install-drasi-cli.sh | bash

# 在 Kubernetes 集群中安装 Drasi
drasi init
```

### 定义数据源

```yaml
# source-postgres.yaml
apiVersion: v1
kind: Source
name: my-database
spec:
  kind: PostgreSQL
  properties:
    host: postgres.default.svc.cluster.local
    port: 5432
    user: drasi
    password: secret
    database: myapp
    ssl: false
    tables:
      - public.orders
      - public.customers
      - public.products
```

```bash
drasi apply -f source-postgres.yaml
```

### 定义持续查询

```yaml
# query-high-value-orders.yaml
apiVersion: v1
kind: ContinuousQuery
name: high-value-orders
spec:
  mode: query
  sources:
    subscriptions:
      - id: my-database
        nodes:
          - sourceLabel: orders
          - sourceLabel: customers
  query: >
    MATCH
      (o:orders)-[:PLACED_BY]->(c:customers)
    WHERE
      o.total > 1000
      AND o.status = 'pending'
    RETURN
      o.id AS order_id,
      o.total AS amount,
      c.name AS customer_name,
      c.email AS customer_email
```

```bash
drasi apply -f query-high-value-orders.yaml
```

### 定义反应器

```yaml
# reaction-webhook.yaml
apiVersion: v1
kind: Reaction
name: notify-high-value
spec:
  kind: Webhook
  properties:
    endpoint: https://api.example.com/notifications
    headers:
      Authorization: Bearer ${env:API_TOKEN}
  queries:
    high-value-orders:
      includeAdded: true
      includeUpdated: true
      includeDeleted: false
```

```bash
drasi apply -f reaction-webhook.yaml
```

---

## 高级查询示例

### 聚合查询

```yaml
# 监控每个区域的订单总额是否超过阈值
apiVersion: v1
kind: ContinuousQuery
name: region-threshold
spec:
  mode: query
  sources:
    subscriptions:
      - id: my-database
        nodes:
          - sourceLabel: orders
  query: >
    MATCH (o:orders)
    WHERE o.status = 'completed'
    WITH o.region AS region, sum(o.total) AS total_amount
    WHERE total_amount > 100000
    RETURN region, total_amount
```

### Kubernetes 资源监控

```yaml
# 监控 Pod 状态变更
apiVersion: v1
kind: Source
name: k8s-cluster
spec:
  kind: Kubernetes
  properties:
    kubeconfig: /path/to/kubeconfig
    resources:
      - pods
      - deployments

---
apiVersion: v1
kind: ContinuousQuery
name: failing-pods
spec:
  mode: query
  sources:
    subscriptions:
      - id: k8s-cluster
        nodes:
          - sourceLabel: pods
  query: >
    MATCH (p:pods)
    WHERE p.status_phase = 'Failed'
      OR p.status_containerStatuses_restartCount > 5
    RETURN
      p.metadata_name AS pod_name,
      p.metadata_namespace AS namespace,
      p.status_phase AS phase
```

---

## 支持的连接器

| 类型 | 数据源 | 反应器 |
|:---|:---|:---|
| 数据库 | PostgreSQL, CosmosDB Gremlin | - |
| Kubernetes | Kubernetes API | - |
| 消息/事件 | - | Azure Event Grid, Dapr Pub/Sub |
| 实时推送 | - | SignalR, Webhook |
| 调试 | - | Debug (日志输出) |
| 存储 | - | StorageQueue |

---

## 最佳实践

1. **查询优化**: 在 WHERE 子句中尽早过滤，减少需要维护的结果集大小
2. **数据源范围**: 只订阅查询实际需要的表/资源，减少变更事件处理量
3. **幂等反应**: 设计反应器处理逻辑时保证幂等性，应对重复触发
4. **监控查询**: 监控持续查询的延迟和吞吐，及时发现性能瓶颈
5. **渐进部署**: 先用 Debug 反应器验证查询逻辑正确，再切换到生产反应器

---

## 参考资源

- [Drasi 官方文档](https://drasi.io/docs/)
- [Drasi GitHub](https://github.com/drasi-project/drasi-platform)
- [Drasi 示例](https://github.com/drasi-project/drasi-platform/tree/main/examples)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
