---
title: KusionStack
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- redis
- mysql
- crd
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- KusionStack 是什么
- 如何 KusionStack
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- KusionStack
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- iac-basics
- redis-basics
- mysql-basics
---

title: KusionStack
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- redis
- mysql
- crd
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- KusionStack 是什么
- 如何 KusionStack
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- KusionStack
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# KusionStack

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kusionstack.io/ |
| **GitHub** | https://github.com/KusionStack/kusion |
| **许可证** | Apache-2.0 |
| **开发语言** | Go, KCL |
| **CNCF 状态** | Sandbox |

---

## 项目概述

KusionStack 是一个云原生可编程技术栈，提供以应用为中心的配置管理和交付能力。它使用 KCL (Kusion Configuration Language) 作为配置语言，结合 Kusion 引擎实现从应用配置到多云/多环境的一致性交付。KusionStack 支持 Kubernetes、Terraform 等多种 IaC 后端，让平台团队可以为开发者提供简化的自助式应用交付体验。

### 核心特性

- **KCL 语言**: 专为配置设计的编程语言，支持类型系统、约束验证和模块化
- **应用模型**: 以应用为中心的配置模型，屏蔽底层基础设施复杂性
- **多后端**: 支持 Kubernetes、Terraform、Pulumi 等多种后端
- **Konfig 库**: 预置的可复用配置模块库，快速组装应用配置
- **Preview/Apply**: 预览变更影响后再应用，避免意外变更
- **工作流集成**: 与 CI/CD 工具链无缝集成

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                   Developer Interface                  │
│                                                       │
│  ┌──────────────────────────────────────────────┐    │
│  │           App Configuration (KCL)              │    │
│  │                                                │    │
│  │  import models.app                             │    │
│  │                                                │    │
│  │  app = App {                                   │    │
│  │    name = "web-frontend"                       │    │
│  │    replicas = 3                                │    │
│  │    image = "nginx:1.25"                        │    │
│  │    resources = {...}                           │    │
│  │  }                                             │    │
│  └────────────────────┬─────────────────────────┘    │
│                       │                               │
│  ┌────────────────────▼─────────────────────────┐    │
│  │              Kusion Engine                     │    │
│  │  ┌──────────┐ ┌───────────┐ ┌─────────────┐  │    │
│  │  │ KCL      │ │ Konfig    │ │ Diff/Preview │  │    │
│  │  │ Compiler │ │ Module    │ │ Engine      │  │    │
│  │  └──────────┘ └───────────┘ └─────────────┘  │    │
│  └────────────────────┬─────────────────────────┘    │
└───────────────────────┼──────────────────────────────┘
                        │
         ┌──────────────┼──────────────┐
         │              │              │
   ┌─────▼─────┐  ┌─────▼─────┐  ┌────▼──────┐
   │Kubernetes │  │ Terraform │  │  Cloud    │
   │ Backend   │  │ Backend   │  │  APIs     │
   └─────┬─────┘  └─────┬─────┘  └─────┬─────┘
         │              │               │
   ┌─────▼─────┐  ┌─────▼─────┐  ┌─────▼─────┐
   │ K8s       │  │ Cloud     │  │ SaaS      │
   │ Resources │  │ Infra     │  │ Services  │
   └───────────┘  └───────────┘  └───────────┘
```

---

## 快速开始

### 安装 Kusion CLI

```bash
# macOS
brew install KusionStack/tap/kusion

# Linux
curl -fsSL https://kusionstack.io/scripts/install.sh | bash

# 验证安装
kusion version
```

### 创建项目

```bash
# 初始化项目
kusion init --name my-app

# 项目结构
my-app/
├── project.yaml      # 项目配置
├── stack/           
│   ├── dev/          # 开发环境
│   │   └── main.k
│   └── prod/         # 生产环境
│       └── main.k
└── kcl.mod           # KCL 模块配置
```

### 编写应用配置 (KCL)

```python
# stack/dev/main.k
import kam.v1.app_configuration as ac
import kam.v1.workload as wl
import kam.v1.workload.container as c
import kam.v1.workload.network as n

# 应用配置
web_frontend: ac.AppConfiguration {
    workload: wl.Service {
        containers: {
            "nginx": c.Container {
                image: "nginx:1.25"
                resources: {
                    cpu: "500m"
                    memory: "512Mi"
                }
            }
        }
        replicas: 3
    }
    
    accessories: {
        "network": n.Network {
            ports: [
                n.Port {
                    port: 80
                    public: True
                }
            ]
        }
    }
}
```

### 预览和应用

```bash
# 预览变更
cd my-app/stack/dev
kusion preview

# 输出:
# Previewing changes:
# + Deployment/web-frontend will be created
# + Service/web-frontend will be created

# 应用配置
kusion apply

# 查看状态
kusion status
```

---

## 高级功能

### 环境差异化

```python
# stack/prod/main.k
import kam.v1.app_configuration as ac
import kam.v1.workload as wl
import kam.v1.workload.container as c

# 生产环境配置 - 更多资源和副本
web_frontend: ac.AppConfiguration {
    workload: wl.Service {
        containers: {
            "nginx": c.Container {
                image: "nginx:1.25"
                resources: {
                    cpu: "2"
                    memory: "2Gi"
                }
            }
        }
        replicas: 10  # 生产环境更多副本
    }
}
```

### 使用 Konfig 模块

```python
# 使用预置模块
import konfig.apps.mysql as mysql
import konfig.apps.redis as redis

# 数据库
db: mysql.MySQL {
    version: "8.0"
    storage: "100Gi"
    replicas: 3
}

# 缓存
cache: redis.Redis {
    version: "7.0"
    mode: "cluster"
    replicas: 6
}
```

### 约束验证

```python
# 定义约束规则
import regex

schema AppConfig:
    name: str
    replicas: int
    image: str
    
    # 约束检查
    check:
        1 <= replicas <= 100, "replicas must be between 1 and 100"
        regex.match(image, r"^[\w\-\.]+:[\w\.\-]+$"), "invalid image format"

# 使用约束
config = AppConfig {
    name: "web"
    replicas: 3
    image: "nginx:1.25"
}
```

### Terraform 后端

```python
# 使用 Terraform 管理云资源
import kam.v1.app_configuration as ac
import kam.v1.accessories.postgres as pg

# 应用 + RDS 数据库
my_app: ac.AppConfiguration {
    workload: wl.Service {
        containers: {
            "app": c.Container {
                image: "my-app:v1.0"
                env: {
                    DATABASE_URL: "${postgres.url}"
                }
            }
        }
    }
    
    # Terraform 管理的 RDS
    accessories: {
        "postgres": pg.Postgres {
            type: "cloud"  # 使用 Terraform 创建 RDS
            instanceClass: "db.t3.medium"
            storageGB: 100
        }
    }
}
```

---

## 与其他方案对比

| 特性 | KusionStack | Pulumi | CDK for Terraform | Crossplane |
|:---|:---|:---|:---|:---|
| 语言 | KCL (DSL) | TS/Python/Go | TS/Python | YAML/CRD |
| 应用抽象 | 内置模型 | 需自定义 | 需自定义 | Composition |
| 多后端 | K8s/TF/Pulumi | 云 API | Terraform | 云 API |
| 类型系统 | 强类型+约束 | 语言原生 | 语言原生 | OpenAPI |
| 学习曲线 | 中等 (新语言) | 低 (熟悉语言) | 低 | 低 |
| 配置复用 | Konfig 库 | Package | Module | XRD |

---

## 最佳实践

1. **项目结构**: 按环境组织 stack，共享配置放在项目级别
2. **模块复用**: 将通用配置封装为 Konfig 模块，团队内共享
3. **约束前置**: 使用 KCL schema 约束在编写阶段捕获配置错误
4. **Preview 必做**: 在 apply 前始终执行 preview 确认变更影响
5. **CI/CD 集成**: 将 kusion preview/apply 集成到 GitOps 流程

---

## 参考资源

- [KusionStack 官方文档](https://kusionstack.io/docs/)
- [Kusion GitHub](https://github.com/KusionStack/kusion)
- [KCL 语言](https://kcl-lang.io/)
- [Konfig 模块库](https://github.com/KusionStack/konfig)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/sql.md|sql]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/gitops.md|gitops]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
