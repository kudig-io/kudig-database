# OpenFGA

> **成熟度**: Incubating | **加入时间**: 2022-12 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://openfga.dev |
| **GitHub** | https://github.com/openfga/openfga |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Security & Authorization |

---

## 项目概述

OpenFGA 是细粒度授权（Fine-Grained Authorization）系统，基于 Google Zanzibar 论文设计。它提供灵活的关系型访问控制，支持复杂的权限模型如 RBAC、ABAC 和 ReBAC。

## 核心特性

- **关系型授权**: 基于用户、对象、关系的灵活模型
- **高性能**: 毫秒级权限检查响应
- **DSL 建模**: 简洁的授权模型定义语言
- **多租户**: 原生支持多个授权模型隔离
- **SDK 支持**: Go、Node.js、Python、Java、.NET SDK
- **可扩展**: 水平扩展支持海量权限数据

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                      OpenFGA Architecture                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    Application Layer                       │ │
│  │                                                            │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐   │ │
│  │  │  Web App    │  │  API Server │  │  Microservices  │   │ │
│  │  └──────┬──────┘  └──────┬──────┘  └────────┬────────┘   │ │
│  │         │                │                  │             │ │
│  │         └────────────────┼──────────────────┘             │ │
│  │                          │                                │ │
│  │                    SDK (Check/Write)                      │ │
│  └──────────────────────────┼─────────────────────────────────┘ │
│                             ▼                                    │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    OpenFGA Server                          │ │
│  │                                                            │ │
│  │  ┌───────────────────────────────────────────────────┐    │ │
│  │  │                 gRPC / HTTP API                   │    │ │
│  │  └───────────────────────────────────────────────────┘    │ │
│  │                          │                                 │ │
│  │  ┌─────────────┐  ┌─────┴─────┐  ┌─────────────────────┐ │ │
│  │  │Authorization│  │  Tuple    │  │  Expand/ListObjects │ │ │
│  │  │   Model     │  │  Store    │  │    Engine           │ │ │
│  │  └─────────────┘  └───────────┘  └─────────────────────┘ │ │
│  │                          │                                 │ │
│  └──────────────────────────┼─────────────────────────────────┘ │
│                             ▼                                    │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    Data Store                              │ │
│  │  ┌───────────┐  ┌───────────┐  ┌───────────────────────┐ │ │
│  │  │ PostgreSQL│  │   MySQL   │  │       Memory          │ │ │
│  │  └───────────┘  └───────────┘  └───────────────────────┘ │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 核心概念

| 概念 | 说明 | 示例 |
|------|------|------|
| Type | 对象类型 | document, folder, user |
| Relation | 关系定义 | owner, editor, viewer |
| Tuple | 权限关系实例 | user:alice is owner of document:readme |
| Check | 权限检查 | Can user:alice view document:readme? |

---

## 快速开始

### 安装 OpenFGA

```bash
# Docker 运行
docker run -p 8080:8080 -p 3000:3000 openfga/openfga run

# Kubernetes (Helm)
helm repo add openfga https://openfga.github.io/helm-charts
helm install openfga openfga/openfga \
  --namespace openfga \
  --create-namespace
```

### 定义授权模型

```yaml
# model.fga
model
  schema 1.1

type user

type document
  relations
    define owner: [user]
    define editor: [user] or owner
    define viewer: [user] or editor
    
type folder
  relations
    define owner: [user]
    define viewer: [user] or owner
    define parent: [folder]
    define can_create_document: owner or can_create_document from parent
```

### 使用 CLI 创建模型

```bash
# 安装 CLI
brew install openfga/tap/fga

# 创建 Store
fga store create --name "my-store"

# 写入模型
fga model write --store-id $STORE_ID --file model.fga

# 写入权限关系
fga tuple write --store-id $STORE_ID user:alice owner document:readme
fga tuple write --store-id $STORE_ID user:bob viewer document:readme

# 检查权限
fga query check --store-id $STORE_ID user:alice viewer document:readme
# Result: allowed: true
```

---

## SDK 集成

### Node.js

```typescript
import { OpenFgaClient } from '@openfga/sdk';

const fga = new OpenFgaClient({
  apiUrl: 'http://localhost:8080',
  storeId: 'your-store-id',
});

// 写入权限关系
await fga.write({
  writes: [
    {
      user: 'user:alice',
      relation: 'owner',
      object: 'document:readme',
    },
  ],
});

// 检查权限
const { allowed } = await fga.check({
  user: 'user:alice',
  relation: 'viewer',
  object: 'document:readme',
});
console.log(`Allowed: ${allowed}`);

// 列出用户可访问的对象
const { objects } = await fga.listObjects({
  user: 'user:alice',
  relation: 'viewer',
  type: 'document',
});
console.log('Accessible documents:', objects);
```

### Go

```go
package main

import (
    "context"
    openfga "github.com/openfga/go-sdk"
    "github.com/openfga/go-sdk/client"
)

func main() {
    fgaClient, _ := client.NewSdkClient(&client.ClientConfiguration{
        ApiUrl:  "http://localhost:8080",
        StoreId: "your-store-id",
    })
    
    // 写入权限关系
    body := client.ClientWriteRequest{
        Writes: []client.ClientTupleKey{
            {
                User:     "user:alice",
                Relation: "owner",
                Object:   "document:readme",
            },
        },
    }
    fgaClient.Write(context.Background()).Body(body).Execute()
    
    // 检查权限
    resp, _ := fgaClient.Check(context.Background()).Body(client.ClientCheckRequest{
        User:     "user:alice",
        Relation: "viewer",
        Object:   "document:readme",
    }).Execute()
    
    fmt.Printf("Allowed: %v\n", resp.GetAllowed())
}
```

---

## 高级模型示例

### Google Drive 风格权限

```yaml
model
  schema 1.1

type user

type group
  relations
    define member: [user, group#member]

type folder
  relations
    define owner: [user, group#member]
    define editor: [user, group#member] or owner
    define viewer: [user, group#member] or editor
    define parent: [folder]
    define can_share: owner or can_share from parent

type document
  relations
    define owner: [user, group#member]
    define editor: [user, group#member] or owner or editor from parent
    define viewer: [user, group#member] or editor or viewer from parent
    define parent: [folder]
```

### GitHub 风格权限

```yaml
model
  schema 1.1

type user

type team
  relations
    define member: [user]
    define maintainer: [user]

type organization
  relations
    define owner: [user]
    define member: [user, team#member] or owner

type repository
  relations
    define owner: [organization]
    define admin: [user, team#member] or owner from owner
    define writer: [user, team#member] or admin
    define reader: [user, team#member] or writer
    define triager: [user, team#member]
```

---

## 性能优化

### 批量检查

```typescript
// 批量检查多个权限
const results = await fga.batchCheck({
  checks: [
    { user: 'user:alice', relation: 'viewer', object: 'document:1' },
    { user: 'user:alice', relation: 'viewer', object: 'document:2' },
    { user: 'user:alice', relation: 'viewer', object: 'document:3' },
  ],
});
```

### 上下文条件

```yaml
model
  schema 1.1

type user

type document
  relations
    define viewer: [user with ip_range]

condition ip_range(ip: ipaddress, allowed_range: string) {
  ip.in_cidr(allowed_range)
}
```

---

## 最佳实践

1. **模型设计**: 先设计清晰的权限模型，避免过度复杂
2. **批量操作**: 使用批量写入和检查减少网络往返
3. **缓存策略**: 在应用层缓存高频权限检查结果
4. **审计日志**: 记录所有权限变更用于合规审计
5. **测试覆盖**: 使用 OpenFGA 测试工具验证授权逻辑

---

## 参考资源

- [官方文档](https://openfga.dev/docs)
- [GitHub Repo](https://github.com/openfga/openfga)
- [Playground](https://play.fga.dev)
- [Zanzibar 论文](https://research.google/pubs/pub48190/)
- [建模指南](https://openfga.dev/docs/modeling)

---

**维护者**: Kudig Team | **许可证**: MIT
