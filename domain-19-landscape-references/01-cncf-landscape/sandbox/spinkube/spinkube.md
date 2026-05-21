---
title: SpinKube
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- helm
- containerd
- redis
- hpa
- gateway
- crd
- operator
- wasm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- SpinKube 是什么
- 如何 SpinKube
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- SpinKube
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- redis-basics
- tls-basics
---

title: SpinKube
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- containerd
- redis
- hpa
- gateway
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
- SpinKube 是什么
- 如何 SpinKube
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- SpinKube
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

# SpinKube

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://spinkube.dev/ |
| **GitHub** | https://github.com/spinkube |
| **许可证** | Apache-2.0 |
| **开发语言** | Rust, Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

SpinKube 是一个在 Kubernetes 上运行 WebAssembly (Wasm) 微服务和应用的开源平台。它将 Fermyon Spin 框架与 Kubernetes 集成，使开发者能够像部署容器一样部署 Wasm 应用，同时获得更快的启动速度、更小的资源占用和更强的安全隔离。

### 核心特性

- **Wasm 原生**: 在 Kubernetes 中原生运行 Spin 应用（WebAssembly）
- **亚毫秒启动**: Wasm 模块冷启动远快于容器
- **SpinApp CRD**: 声明式管理 Wasm 应用的生命周期
- **containerd shim**: 通过 containerd-shim-spin 实现容器运行时集成
- **自动伸缩**: 集成 Kubernetes HPA/KEDA 实现弹性伸缩
- **OCI 分发**: Wasm 应用打包为 OCI artifact 分发
- **多语言**: 支持 Rust, Go, JavaScript, TypeScript, Python 等

---

## 架构设计

```
┌────────────────────────────────────────────────────┐
│                 Kubernetes Cluster                   │
│                                                      │
│  ┌──────────────────────────────────────────────┐   │
│  │           Spin Operator                       │   │
│  │  (Watches SpinApp CRD, manages workloads)    │   │
│  └────────────────────┬─────────────────────────┘   │
│                       │                              │
│  ┌────────────────────┴─────────────────────────┐   │
│  │          SpinApp Resources                    │   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐     │   │
│  │  │SpinApp A │ │SpinApp B │ │SpinApp C │     │   │
│  │  └─────┬────┘ └─────┬────┘ └─────┬────┘     │   │
│  └────────┼─────────────┼─────────────┼─────────┘   │
│           │             │             │              │
│  ┌────────┴─────────────┴─────────────┴─────────┐   │
│  │        RuntimeClass: wasmtime-spin            │   │
│  │                                               │   │
│  │  ┌─────────────────────────────────────────┐ │   │
│  │  │  containerd-shim-spin (per node)        │ │   │
│  │  │                                         │ │   │
│  │  │  ┌──────────┐  ┌──────────────────────┐│ │   │
│  │  │  │ Spin     │  │  Wasmtime Runtime    ││ │   │
│  │  │  │ Engine   │  │  (Wasm Execution)    ││ │   │
│  │  │  └──────────┘  └──────────────────────┘│ │   │
│  │  └─────────────────────────────────────────┘ │   │
│  └──────────────────────────────────────────────┘   │
└────────────────────────────────────────────────────┘
```

### 核心组件

| 组件 | 说明 |
|:---|:---|
| **Spin Operator** | Kubernetes Operator，管理 SpinApp CRD 和工作负载 |
| **SpinApp CRD** | 自定义资源，定义 Wasm 应用的部署规格 |
| **containerd-shim-spin** | containerd shim，使 containerd 能运行 Spin 应用 |
| **Spin** | Fermyon 的 Wasm 微服务框架 |
| **RuntimeClass** | Kubernetes RuntimeClass，指向 Spin shim |

---

## 快速开始

### 安装 SpinKube

```bash
# 安装 cert-manager（前置依赖）
kubectl apply -f https://github.com/cert-manager/cert-manager/releases/download/v1.14.0/cert-manager.yaml

# 安装 containerd-shim-spin（通过 RuntimeClass installer）
kubectl apply -f https://github.com/spinkube/containerd-shim-spin/releases/download/v0.15.0/runtime-class.yaml

# 安装 Spin Operator
helm repo add spinkube https://spinkube.github.io/spin-operator
helm install spin-operator spinkube/spin-operator \
  --namespace spin-operator \
  --create-namespace

# 创建 SpinAppExecutor
kubectl apply -f - <<EOF
apiVersion: core.spinkube.dev/v1alpha1
kind: SpinAppExecutor
metadata:
  name: containerd-shim-spin
spec:
  createDeployment: true
  deploymentConfig:
    runtimeClassName: wasmtime-spin-v2
EOF
```

### 创建 Spin 应用

```bash
# 安装 Spin CLI
curl -fsSL https://developer.fermyon.com/downloads/install.sh | bash

# 创建新项目
spin new -t http-rust my-api
cd my-api
```

```rust
// src/lib.rs
use spin_sdk::http::{IntoResponse, Request, Response};
use spin_sdk::http_component;

#[http_component]
fn handle_request(req: Request) -> anyhow::Result<impl IntoResponse> {
    let name = req.query().get("name")
        .map(|n| n.to_string())
        .unwrap_or_else(|| "World".to_string());
    
    Ok(Response::builder()
        .status(200)
        .header("content-type", "application/json")
        .body(format!(r#"{{"message": "Hello, {}!"}}"#, name))
        .build())
}
```

```bash
# 构建并打包为 OCI artifact
spin build
spin registry push registry.example.com/my-api:v1.0
```

### 部署到 Kubernetes

```yaml
apiVersion: core.spinkube.dev/v1alpha1
kind: SpinApp
metadata:
  name: my-api
spec:
  image: "registry.example.com/my-api:v1.0"
  executor: containerd-shim-spin
  replicas: 2
  enableAutoscaling: true
  resources:
    limits:
      cpu: 100m
      memory: 128Mi
```

```bash
kubectl apply -f spinapp.yaml
kubectl get spinapp
kubectl get pods -l core.spinkube.dev/app-name=my-api
```

---

## 配置详解

### SpinApp 完整配置

```yaml
apiVersion: core.spinkube.dev/v1alpha1
kind: SpinApp
metadata:
  name: production-api
  namespace: apps
spec:
  image: "registry.example.com/production-api:v2.0"
  imagePullSecrets:
    - name: registry-secret
  executor: containerd-shim-spin
  replicas: 3
  enableAutoscaling: true
  resources:
    limits:
      cpu: 500m
      memory: 256Mi
    requests:
      cpu: 100m
      memory: 64Mi
  runtimeConfig:
    keyValueStores:
      - name: "default"
        type: "redis"
        options:
          - name: "url"
            value: "redis://redis.default.svc:6379"
    sqliteDatabases:
      - name: "default"
        type: "libsql"
        options:
          - name: "url"
            value: "https://my-db.turso.io"
          - name: "token"
            valueFrom:
              secretKeyRef:
                name: turso-token
                key: token
  variables:
    - name: api_key
      valueFrom:
        secretKeyRef:
          name: api-secrets
          key: api-key
```

### 多组件应用

```toml
# spin.toml
spin_manifest_version = 2

[application]
name = "fullstack-app"
version = "1.0.0"

[[trigger.http]]
route = "/api/..."
component = "api"

[[trigger.http]]
route = "/..."
component = "frontend"

[component.api]
source = "target/wasm32-wasip1/release/api.wasm"
allowed_outbound_hosts = ["redis://redis:6379", "https://external-api.com"]
key_value_stores = ["default"]

[component.frontend]
source = "frontend/dist"
files = [{ source = "frontend/dist", destination = "/" }]
```

### 自动伸缩配置

```yaml
apiVersion: core.spinkube.dev/v1alpha1
kind: SpinApp
metadata:
  name: scalable-api
spec:
  image: "registry.example.com/scalable-api:latest"
  executor: containerd-shim-spin
  enableAutoscaling: true
  autoscaler:
    minReplicas: 1
    maxReplicas: 50
    targetCPUUtilizationPercentage: 60
```

---

## 与容器对比

| 维度 | SpinKube (Wasm) | 传统容器 |
|:---|:---|:---|
| **启动时间** | <1ms | 秒级 |
| **镜像大小** | KB-MB | MB-GB |
| **内存占用** | ~1MB | ~50MB+ |
| **安全隔离** | Wasm 沙箱 | Linux namespace/cgroup |
| **冷启动密度** | 数千/节点 | 数十-数百/节点 |
| **编程模型** | 组件模型 | 进程模型 |

---

## 最佳实践

1. **适用场景**: 高密度短任务、API Gateway、边缘计算、Serverless 函数
2. **OCI 分发**: 使用 `spin registry push` 将 Wasm 应用作为 OCI artifact 分发
3. **状态管理**: 使用 Spin 的 Key-Value Store 或 SQLite 数据库组件管理状态
4. **渐进采用**: 从无状态 API 和辅助微服务开始，逐步扩大 Wasm 工作负载比例
5. **监控**: 利用 Kubernetes 标准监控工具监控 SpinApp 的 Pod 状态和资源使用
6. **多组件**: 利用 Spin 的多组件模型在一个应用中组合 API 和静态文件服务

---

## 参考资源

- [SpinKube 官方文档](https://spinkube.dev/docs/)
- [SpinKube GitHub](https://github.com/spinkube)
- [Spin Framework](https://developer.fermyon.com/spin)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/sql.md|sql]]
- [[domain-17-system-foundation/topic-cheat-sheet/linux.md|linux]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/cncf-runtime|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
