---
title: kube-rs
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- redis
- mysql
- statefulset
- rbac
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
- kube-rs 是什么
- 如何 kube-rs
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- kube-rs
- cncf
- landscape
---


# kube-rs

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kube.rs/ |
| **GitHub** | https://github.com/kube-rs/kube |
| **许可证** | Apache-2.0 |
| **开发语言** | Rust |
| **CNCF 状态** | Sandbox |

---

## 项目概述

kube-rs 是 Rust 语言的 Kubernetes 客户端库，提供类型安全的 Kubernetes API 交互能力。它包含低级 API 客户端（kube-client）、运行时抽象（kube-runtime）和 CRD 代码生成（kube-derive），使开发者能用 Rust 构建高性能、内存安全的 Kubernetes Controller 和 Operator。

### 核心特性

- **类型安全**: 编译时验证 API 调用，减少运行时错误
- **异步优先**: 基于 tokio/async-await 的完全异步 API
- **CRD 支持**: `#[derive(CustomResource)]` 宏自动生成 CRD 相关代码
- **Controller Runtime**: 内置 Controller、Watcher、Reflector 抽象
- **高性能**: Rust 原生性能，极低的内存和 CPU 占用
- **多集群**: 支持自定义 kubeconfig 和多集群访问
- **OpenAPI**: 支持从 OpenAPI schema 生成客户端代码

---

## 架构设计

```
┌─────────────────────────────────────────────┐
│              kube-rs Ecosystem               │
│                                              │
│  ┌──────────────┐  ┌──────────────────────┐ │
│  │  kube-client  │  │    kube-runtime       │ │
│  │               │  │                       │ │
│  │  Api<K>       │  │  Controller<K>        │ │
│  │  Client       │  │  watcher()            │ │
│  │  Discovery    │  │  reflector()          │ │
│  │  Config       │  │  Store / Writer       │ │
│  └──────┬───────┘  └───────────┬───────────┘ │
│         │                       │             │
│  ┌──────┴───────────────────────┴───────────┐│
│  │           kube-derive                     ││
│  │  #[derive(CustomResource)]               ││
│  │  CRD schema generation                    ││
│  └───────────────────────────────────────────┘│
└──────────────────┬────────────────────────────┘
                   │
                   ▼
            K8s API Server
```

---

## 快速开始

### 项目设置

```toml
# Cargo.toml
[dependencies]
kube = { version = "0.92", features = ["runtime", "derive", "client"] }
k8s-openapi = { version = "0.22", features = ["latest"] }
tokio = { version = "1", features = ["full"] }
serde = { version = "1", features = ["derive"] }
serde_json = "1"
thiserror = "1"
tracing = "0.1"
tracing-subscriber = "0.3"
schemars = "0.8"
```

### 基本 API 操作

```rust
use k8s_openapi::api::core::v1::Pod;
use kube::{Api, Client, ResourceExt};
use kube::api::{ListParams, PostParams, DeleteParams};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let client = Client::try_default().await?;
    let pods: Api<Pod> = Api::namespaced(client, "default");

    // 列出 Pod
    let lp = ListParams::default().labels("app=myapp");
    for pod in pods.list(&lp).await? {
        println!("Found pod: {}", pod.name_any());
    }

    // 获取单个 Pod
    let pod = pods.get("my-pod").await?;
    println!("Pod status: {:?}", pod.status);

    // 删除 Pod
    pods.delete("my-pod", &DeleteParams::default()).await?;

    Ok(())
}
```

### 自定义资源 (CRD)

```rust
use kube::CustomResource;
use schemars::JsonSchema;
use serde::{Deserialize, Serialize};

#[derive(CustomResource, Debug, Serialize, Deserialize, Clone, JsonSchema)]
#[kube(
    group = "myapp.example.com",
    version = "v1",
    kind = "Database",
    namespaced,
    status = "DatabaseStatus",
    printcolumn = r#"{"name":"Engine", "type":"string", "jsonPath":".spec.engine"}"#,
    printcolumn = r#"{"name":"Size", "type":"string", "jsonPath":".spec.size"}"#
)]
pub struct DatabaseSpec {
    pub engine: String,           // postgres, mysql, redis
    pub version: String,          // e.g., "16.0"
    pub size: String,             // small, medium, large
    pub replicas: i32,
    pub storage_class: Option<String>,
    pub backup_enabled: bool,
}

#[derive(Debug, Serialize, Deserialize, Clone, JsonSchema)]
pub struct DatabaseStatus {
    pub phase: String,            // Pending, Running, Failed
    pub ready_replicas: i32,
    pub endpoint: Option<String>,
    pub conditions: Vec<Condition>,
}

#[derive(Debug, Serialize, Deserialize, Clone, JsonSchema)]
pub struct Condition {
    pub type_: String,
    pub status: String,
    pub message: Option<String>,
    pub last_transition_time: Option<String>,
}
```

### Controller 实现

```rust
use kube::runtime::controller::{Action, Controller};
use kube::runtime::watcher::Config;
use kube::{Api, Client, ResourceExt};
use std::sync::Arc;
use tokio::time::Duration;

struct Context {
    client: Client,
}

async fn reconcile(db: Arc<Database>, ctx: Arc<Context>) -> Result<Action, Error> {
    let name = db.name_any();
    let namespace = db.namespace().unwrap_or_default();
    let client = &ctx.client;

    tracing::info!("Reconciling Database {}/{}", namespace, name);

    let api: Api<Database> = Api::namespaced(client.clone(), &namespace);

    // 确保 StatefulSet 存在
    ensure_statefulset(client, &db).await?;

    // 确保 Service 存在
    ensure_service(client, &db).await?;

    // 更新状态
    let status = DatabaseStatus {
        phase: "Running".to_string(),
        ready_replicas: db.spec.replicas,
        endpoint: Some(format!("{}.{}.svc.cluster.local", name, namespace)),
        conditions: vec![],
    };
    update_status(&api, &name, status).await?;

    Ok(Action::requeue(Duration::from_secs(300)))
}

fn error_policy(db: Arc<Database>, err: &Error, _ctx: Arc<Context>) -> Action {
    tracing::error!("Error reconciling {}: {:?}", db.name_any(), err);
    Action::requeue(Duration::from_secs(60))
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    tracing_subscriber::fmt::init();
    let client = Client::try_default().await?;
    let databases = Api::<Database>::all(client.clone());
    let ctx = Arc::new(Context { client: client.clone() });

    Controller::new(databases, Config::default())
        .shutdown_on_signal()
        .run(reconcile, error_policy, ctx)
        .for_each(|res| async move {
            match res {
                Ok(o) => tracing::info!("Reconciled {:?}", o),
                Err(e) => tracing::error!("Reconcile error: {:?}", e),
            }
        })
        .await;

    Ok(())
}
```

---

## 高级功能

### Watcher 和 Reflector

```rust
use kube::runtime::{watcher, reflector, WatchStreamExt};
use kube::runtime::reflector::Store;
use futures::TryStreamExt;

// 使用 reflector 维护本地缓存
let (store, writer) = reflector::store();
let pods: Api<Pod> = Api::namespaced(client.clone(), "default");

let rf = reflector(writer, watcher(pods, Config::default()))
    .applied_objects()
    .default_backoff()
    .try_for_each(|pod| async move {
        tracing::info!("Updated: {}", pod.name_any());
        Ok(())
    });

// 从缓存读取（不发起 API 调用）
let cached_pods: Vec<Arc<Pod>> = store.state();
```

### 生成 CRD YAML

```rust
// 导出 CRD schema
fn main() {
    let crd = Database::crd();
    println!("{}", serde_yaml::to_string(&crd).unwrap());
}
```

```bash
cargo run --bin crdgen > database-crd.yaml
kubectl apply -f database-crd.yaml
```

---

## 与其他 K8s 客户端对比

| 特性 | kube-rs (Rust) | client-go (Go) | kubernetes (Python) |
|:---|:---|:---|:---|
| **类型安全** | 编译时 | 编译时 | 运行时 |
| **内存占用** | ~5MB | ~30MB | ~50MB+ |
| **二进制大小** | ~10MB | ~30MB | N/A |
| **异步支持** | 原生 async | goroutine | asyncio |
| **CRD 代码生成** | derive 宏 | code-generator | 手动 |

---

## 最佳实践

1. **错误处理**: 使用 `thiserror` 定义错误类型，优雅处理 API 调用失败
2. **重试策略**: 合理配置 `Action::requeue` 间隔，避免 API Server 过载
3. **缓存优先**: 使用 reflector Store 缓存，减少 API 调用
4. **权限最小化**: 为 Controller 配置最小 RBAC 权限
5. **可观测性**: 集成 tracing 记录 reconcile 过程，暴露 Prometheus 指标
6. **测试**: 使用 `kube::Client::try_from` 模拟测试 API 交互

---

## 参考资源

- [kube-rs 官方文档](https://kube.rs/)
- [kube-rs GitHub](https://github.com/kube-rs/kube)
- [kube-rs 示例](https://github.com/kube-rs/kube/tree/main/examples)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
