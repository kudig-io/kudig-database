# Radius

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://radapp.io/ |
| **GitHub** | https://github.com/radius-project/radius |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Radius 是由 Microsoft 开发的云原生应用平台，提供了一种以应用为中心的方式来定义和部署云原生应用及其依赖的基础设施。它引入了 "Application Graph" 的概念，让开发者定义应用需要什么（如数据库、消息队列），而由平台工程师定义如何提供这些资源（Azure CosmosDB 还是本地 MongoDB），实现关注点分离。

### 核心特性

- **应用图 (Application Graph)**: 可视化应用组件及其依赖关系
- **关注点分离**: 开发者定义需求，运维定义实现（Recipe 模式）
- **多云部署**: 支持 Azure、AWS 和 Kubernetes 环境
- **Recipe 系统**: 使用 Terraform/Bicep 模板定义基础设施的标准化配置
- **Dapr 集成**: 原生集成 Dapr 构建微服务应用
- **声明式**: 使用 Bicep 或 Terraform 声明式定义应用

---

## 架构设计

```
┌──────────────────────────────────────────────────┐
│                  Radius Platform                   │
│                                                    │
│  ┌──────────────────────────────────────────┐     │
│  │           Radius Control Plane            │     │
│  │  ┌──────────┐  ┌──────────┐             │     │
│  │  │ App Model│  │ Recipe   │             │     │
│  │  │ Engine   │  │ Engine   │             │     │
│  │  └────┬─────┘  └────┬─────┘             │     │
│  └───────┼──────────────┼────────────────────┘     │
│          │              │                           │
│  ┌───────▼──────────────▼────────────────────┐     │
│  │         Environment & Recipes              │     │
│  │  ┌───────┐ ┌───────┐ ┌───────┐          │     │
│  │  │Azure  │ │ AWS   │ │  K8s  │          │     │
│  │  │Recipe │ │Recipe │ │Recipe │          │     │
│  │  │(Bicep)│ │(TF)   │ │(Helm) │          │     │
│  │  └───┬───┘ └───┬───┘ └───┬───┘          │     │
│  └──────┼─────────┼─────────┼───────────────┘     │
└─────────┼─────────┼─────────┼──────────────────────┘
     ┌────▼───┐ ┌───▼────┐ ┌──▼──────┐
     │ Azure  │ │  AWS   │ │  K8s   │
     │CosmosDB│ │  RDS   │ │MongoDB │
     │Cache   │ │  SQS   │ │Redis   │
     └────────┘ └────────┘ └────────┘
```

---

## 快速开始

### 安装

```bash
# 安装 Radius CLI
curl -fsSL "https://get.radapp.io/tools/rad/install.sh" | /bin/bash

# 初始化 Radius（在 K8s 集群中）
rad init

# 验证安装
rad version
```

### 定义应用 (Bicep)

```bicep
// app.bicep
import radius as radius

@description('Radius application')
resource app 'Applications.Core/applications@2023-10-01-preview' = {
  name: 'myapp'
  properties: {
    environment: environment
  }
}

@description('Web frontend container')
resource frontend 'Applications.Core/containers@2023-10-01-preview' = {
  name: 'frontend'
  properties: {
    application: app.id
    container: {
      image: 'myorg/frontend:latest'
      ports: {
        web: {
          containerPort: 3000
        }
      }
    }
    connections: {
      redis: {
        source: redis.id
      }
    }
  }
}

@description('Redis cache - 由 Recipe 提供')
resource redis 'Applications.Datastores/redisCaches@2023-10-01-preview' = {
  name: 'shared-cache'
  properties: {
    application: app.id
    environment: environment
    // 不指定具体实现 - 由 Environment Recipe 决定
  }
}
```

### 定义 Environment 和 Recipe

```bicep
// environment.bicep
resource env 'Applications.Core/environments@2023-10-01-preview' = {
  name: 'production'
  properties: {
    compute: {
      kind: 'kubernetes'
      namespace: 'production'
    }
    recipes: {
      'Applications.Datastores/redisCaches': {
        default: {
          templateKind: 'terraform'
          templatePath: 'ghcr.io/myorg/recipes/azure-redis:latest'
        }
      }
    }
  }
}
```

### 部署应用

```bash
# 部署应用
rad deploy app.bicep --environment production

# 查看应用图
rad app graph myapp

# 查看应用状态
rad app status myapp

# 查看资源
rad resource list --application myapp
```

---

## Recipe 示例

### Azure Redis Recipe (Terraform)

```hcl
# recipes/azure-redis/main.tf
terraform {
  required_providers {
    azurerm = { source = "hashicorp/azurerm" }
  }
}

variable "context" {
  type = any
  description = "Radius provided context"
}

resource "azurerm_redis_cache" "cache" {
  name                = var.context.resource.name
  location            = var.context.azure.location
  resource_group_name = var.context.azure.resourceGroup
  capacity            = 1
  family              = "C"
  sku_name            = "Standard"
}

output "result" {
  value = {
    values = {
      host = azurerm_redis_cache.cache.hostname
      port = azurerm_redis_cache.cache.ssl_port
    }
    secrets = {
      connectionString = azurerm_redis_cache.cache.primary_connection_string
    }
  }
}
```

---

## 与其他方案对比

| 特性 | Radius | Crossplane | Terraform | Helm |
|:---|:---|:---|:---|:---|
| 关注点 | 应用 + 基础设施 | 基础设施 | 基础设施 | K8s 应用 |
| 开发者体验 | 声明需求即可 | 需了解 XRD | 需了解 HCL | 需了解 Chart |
| Recipe/模板 | Recipe 系统 | Composition | Module | - |
| 应用图 | 内置 | 无 | 无 | 无 |
| 多云支持 | Azure/AWS/K8s | 多云 | 多云 | K8s 仅 |
| 适用场景 | 平台工程 | 基础设施即代码 | 基础设施即代码 | K8s 部署 |

---

## 最佳实践

1. **关注点分离**: 开发者通过 Portable Resource 声明需求，平台团队维护 Recipe
2. **环境分级**: 为 dev/staging/production 创建不同的 Environment 和 Recipe
3. **Recipe 标准化**: 将 Recipe 作为 OCI Artifact 管理，确保基础设施配置一致
4. **应用图**: 利用 Application Graph 可视化和理解应用的依赖关系
5. **渐进采纳**: 从新应用开始使用 Radius，逐步将已有应用迁移

---

## 参考资源

- [Radius 官方文档](https://docs.radapp.io/)
- [Radius GitHub](https://github.com/radius-project/radius)
- [Radius Recipes](https://github.com/radius-project/recipes)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
