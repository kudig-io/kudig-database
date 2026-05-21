---
title: KCL (KusionStack Configuration Language)
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- argocd
- redis
- ingress
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- KCL (KusionStack Configuration Language) 是什么
- 如何 KCL (KusionStack Configuration Language)
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- KCL
- KusionStack
- Configuration
- Language
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- gitops-basics
- redis-basics
---

title: KCL (KusionStack Configuration Language)
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- argocd
- redis
- ingress
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- KCL (KusionStack Configuration Language) 是什么
- 如何 KCL (KusionStack Configuration Language)
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- KCL
- KusionStack
- Configuration
- Language
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
# KCL (KusionStack Configuration Language)

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kcl-lang.io/ |
| **GitHub** | https://github.com/kcl-lang/kcl |
| **许可证** | Apache-2.0 |
| **开发语言** | Rust, Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

KCL (Kusion Configuration Language) 是一个开源的基于约束的记录与函数式配置语言，专为云原生配置和策略管理设计。它提供类型系统、schema 约束、配置合并和覆盖等能力，帮助团队以编程方式管理复杂的 Kubernetes 和云基础设施配置，减少配置错误。

### 核心特性

- **类型系统**: 静态类型检查，在编译阶段捕获配置错误
- **Schema 约束**: 使用 schema 定义配置结构和验证规则
- **配置合并**: 支持配置的继承、覆盖和组合
- **策略即代码**: 编写可复用的配置策略和约束规则
- **多输出格式**: 输出 YAML, JSON, TOML 等格式
- **IDE 支持**: VS Code, IntelliJ 插件提供语法高亮和自动补全
- **Kubernetes 集成**: 原生支持 Kubernetes 资源模型和 CRD
- **包管理**: OCI Registry 和 Git 仓库作为包的分发来源

---

## 快速开始

### 安装

```bash
# macOS
brew install kcl-lang/tap/kcl

# Linux/macOS 一键安装
curl -fsSL https://kcl-lang.io/script/install-cli.sh | /bin/bash

# 验证安装
kcl version
```

### 基础语法

```python
# main.k - KCL 基础配置
import manifests

# 定义 Schema
schema Deployment:
    name: str
    image: str
    replicas: int = 1
    port: int = 80
    
    check:
        replicas >= 1, "replicas must be >= 1"
        replicas <= 100, "replicas must be <= 100"
        port > 0 and port < 65536, "invalid port range"

# 创建实例
web = Deployment {
    name = "web-server"
    image = "nginx:1.25"
    replicas = 3
    port = 8080
}

# 输出 Kubernetes 资源
manifests.yaml_stream([{
    apiVersion = "apps/v1"
    kind = "Deployment"
    metadata.name = web.name
    spec = {
        replicas = web.replicas
        selector.matchLabels.app = web.name
        template = {
            metadata.labels.app = web.name
            spec.containers = [{
                name = web.name
                image = web.image
                ports = [{ containerPort = web.port }]
            }]
        }
    }
}])
```

```bash
# 运行并输出 YAML
kcl main.k
```

### 配置验证

```python
# validate.k - 策略验证
schema K8sDeployment:
    apiVersion: "apps/v1"
    kind: "Deployment"
    metadata: {
        name: str
        labels?: {str: str}
    }
    spec: {
        replicas: int
        selector: any
        template: any
    }
    
    check:
        spec.replicas >= 1, "must have at least 1 replica"
        metadata.name, "name is required"
```

---

## 配置详解

### Schema 继承和复用

```python
# base.k
schema AppBase:
    name: str
    namespace: str = "default"
    labels: {str: str} = {
        "managed-by": "kcl"
    }
    
schema WebApp(AppBase):
    image: str
    replicas: int = 2
    port: int = 80
    ingress_enabled: bool = False
    ingress_host?: str
    
    check:
        ingress_host if ingress_enabled, "ingress_host required when ingress enabled"

schema WorkerApp(AppBase):
    image: str
    replicas: int = 1
    queue: str
    concurrency: int = 5
```

### 配置覆盖和合并

```python
# base.k
config = {
    database = {
        host = "localhost"
        port = 5432
        name = "mydb"
        pool_size = 10
    }
    redis = {
        host = "localhost"
        port = 6379
    }
}

# production.k - 覆盖生产配置
config = {
    database = {
        host = "db.production.internal"
        pool_size = 50
        ssl = True
    }
    redis = {
        host = "redis.production.internal"
        cluster_mode = True
    }
}
```

```bash
# 合并输出
kcl base.k production.k
```

### Kubernetes 资源生成

```python
# k8s.k
import k8s.api.apps.v1 as apps
import k8s.api.core.v1 as core

# 使用 KCL Kubernetes 模型
deployment = apps.Deployment {
    metadata = {
        name = "api-server"
        namespace = "production"
        labels = {
            app = "api-server"
            version = "v2.0"
        }
    }
    spec = {
        replicas = 3
        selector.matchLabels = {
            app = "api-server"
        }
        template = {
            metadata.labels = {
                app = "api-server"
                version = "v2.0"
            }
            spec = {
                containers = [{
                    name = "api"
                    image = "api-server:v2.0"
                    ports = [{ containerPort = 8080 }]
                    resources = {
                        requests = {
                            cpu = "200m"
                            memory = "256Mi"
                        }
                        limits = {
                            cpu = "1000m"
                            memory = "1Gi"
                        }
                    }
                    livenessProbe = {
                        httpGet = {
                            path = "/healthz"
                            port = 8080
                        }
                        initialDelaySeconds = 10
                        periodSeconds = 15
                    }
                }]
            }
        }
    }
}
```

### 包管理

```bash
# 初始化 KCL 项目
kcl mod init my-project

# 添加 Kubernetes 模型依赖
kcl mod add k8s:1.31

# 从 OCI Registry 添加包
kcl mod add oci://ghcr.io/kcl-lang/hello-world

# 运行项目
kcl run
```

```toml
# kcl.mod
[package]
name = "my-project"
version = "0.1.0"
edition = "v0.10.0"

[dependencies]
k8s = "1.31"
```

---

## 高级功能

### 策略即代码

```python
# policy.k - 安全策略
schema SecurityPolicy:
    """Kubernetes 安全策略检查"""
    
    # 检查容器是否使用 root 用户
    check_no_root = lambda containers: [any] -> bool {
        all c in containers {
            c?.securityContext?.runAsNonRoot == True
        }
    }
    
    # 检查是否设置资源限制
    check_resource_limits = lambda containers: [any] -> bool {
        all c in containers {
            c?.resources?.limits?.cpu and c?.resources?.limits?.memory
        }
    }
    
    # 检查镜像标签
    check_image_tag = lambda containers: [any] -> bool {
        all c in containers {
            not c.image.endswith(":latest")
        }
    }
```

### 与 GitOps 集成

```bash
# 使用 KCL 配合 ArgoCD
# argocd-cm ConfigMap 中添加 KCL 插件
apiVersion: v1
kind: ConfigMap
metadata:
  name: argocd-cm
data:
  configManagementPlugins: |
    - name: kcl
      generate:
        command: ["kcl"]
        args: ["run", "."]
```

---

## 与其他配置语言对比

| 特性 | KCL | CUE | Jsonnet | Kustomize |
|:---|:---|:---|:---|:---|
| **类型系统** | 静态 | 静态 | 动态 | 无 |
| **Schema** | 内置 | 内置 | 无 | 无 |
| **约束验证** | 内置 | 内置 | 手动 | 无 |
| **继承** | 支持 | 支持 | Mixin | Overlay |
| **IDE 支持** | 丰富 | 基础 | 中等 | 无 |
| **学习曲线** | 中等 | 较高 | 中等 | 低 |

---

## 最佳实践

1. **Schema 优先**: 先定义 Schema 和约束规则，再编写具体配置
2. **模块化**: 按功能拆分 KCL 文件，使用包管理组织代码
3. **环境覆盖**: 使用配置合并实现环境差异化（dev/staging/prod）
4. **策略验证**: 编写安全和合规策略，在 CI 阶段拦截违规配置
5. **版本管理**: 使用 OCI Registry 发布和管理 KCL 包版本
6. **IDE 工具**: 使用 VS Code KCL 插件获得类型提示和错误检查

---

## 参考资源

- [KCL 官方文档](https://kcl-lang.io/docs/)
- [KCL GitHub](https://github.com/kcl-lang/kcl)
- [KCL Playground](https://play.kcl-lang.io/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/gitops.md|gitops]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/argo.md|argo]]
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
