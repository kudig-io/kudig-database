---
title: Spin
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- containerd
- redis
- mysql
- postgresql
- operator
- wasm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Spin 是什么
- 如何 Spin
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Spin
- cncf
- landscape
---

# Spin

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://www.fermyon.com/spin |
| **GitHub** | https://github.com/fermyon/spin |
| **许可证** | Apache-2.0 |
| **开发语言** | Rust |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Spin 是由 Fermyon 开发的 WebAssembly (Wasm) 微服务框架，用于构建和运行基于事件驱动的 Wasm 应用。它提供极快的冷启动时间（亚毫秒级），支持多种编程语言（Rust、Go、Python、JavaScript、C#等），并内置 HTTP 触发器、Redis 触发器、键值存储、SQL 数据库等能力。Spin 应用可以部署到本地、Kubernetes（通过 SpinKube）或 Fermyon Cloud。

### 核心特性

- **亚毫秒冷启动**: Wasm 组件极快启动，适合 Serverless 场景
- **多语言支持**: Rust、Go、Python、JavaScript/TypeScript、C#、Zig 等
- **内置触发器**: HTTP、Redis、Cron、MQTT 等事件触发器
- **组件模型**: 基于 WASI 组件模型，组件间安全隔离
- **内置存储**: Key-Value Store、SQLite、外部 MySQL/PostgreSQL
- **OCI 分发**: 将 Spin 应用打包为 OCI Artifact 分发

---

## 架构设计

```
┌─────────────────────────────────────────────┐
│              Spin Runtime                    │
│                                              │
│  ┌──────────────────────────────────┐       │
│  │        Trigger Framework         │       │
│  │  ┌──────┐ ┌──────┐ ┌──────┐    │       │
│  │  │ HTTP │ │Redis │ │ Cron │    │       │
│  │  └──┬───┘ └──┬───┘ └──┬───┘    │       │
│  └─────┼────────┼────────┼─────────┘       │
│        │        │        │                   │
│  ┌─────▼────────▼────────▼─────────┐       │
│  │     Component Manager            │       │
│  │  ┌──────┐ ┌──────┐ ┌──────┐    │       │
│  │  │Comp A│ │Comp B│ │Comp C│    │       │
│  │  │(Rust)│ │ (Go) │ │ (JS) │    │       │
│  │  └──┬───┘ └──┬───┘ └──┬───┘    │       │
│  └─────┼────────┼────────┼─────────┘       │
│        │        │        │                   │
│  ┌─────▼────────▼────────▼─────────┐       │
│  │    Wasmtime / WASI Runtime       │       │
│  │  (沙箱隔离 / 能力模型)           │       │
│  └─────────────────────────────────┘       │
│                                              │
│  ┌──────────────────────────────────┐       │
│  │       Host Capabilities          │       │
│  │  KV Store │ SQLite │ HTTP Client │       │
│  │  Config   │ Variables │ LLM      │       │
│  └──────────────────────────────────┘       │
└─────────────────────────────────────────────┘
```

---

## 快速开始

### 安装 Spin CLI

```bash
# macOS/Linux
curl -fsSL https://developer.fermyon.com/downloads/install.sh | bash
sudo mv spin /usr/local/bin/

# 或使用 Homebrew
brew install fermyon/tap/spin

# 验证安装
spin --version
```

### 创建 Spin 应用

```bash
# 从模板创建项目 (Rust)
spin new -t http-rust my-api
cd my-api

# 或创建 JavaScript 项目
spin new -t http-js my-js-api

# 或创建 Python 项目
spin new -t http-py my-py-api
```

### Rust 组件示例

```rust
// src/lib.rs
use spin_sdk::http::{IntoResponse, Request, Response};
use spin_sdk::http_component;

#[http_component]
fn handle_request(req: Request) -> anyhow::Result<impl IntoResponse> {
    let body = format!("Hello from Spin! Path: {}", req.uri().path());
    Ok(Response::builder()
        .status(200)
        .header("content-type", "text/plain")
        .body(body)
        .build())
}
```

### 应用清单

```toml
# spin.toml
spin_manifest_version = 2

[application]
name = "my-api"
version = "0.1.0"
description = "My Spin Application"

[[trigger.http]]
route = "/api/..."
component = "api-handler"

[component.api-handler]
source = "target/wasm32-wasi/release/my_api.wasm"
allowed_outbound_hosts = ["https://api.example.com"]

[component.api-handler.build]
command = "cargo build --target wasm32-wasi --release"

[[trigger.http]]
route = "/static/..."
component = "static-files"

[component.static-files]
source = { url = "https://github.com/fermyon/spin-fileserver/releases/download/v0.1.0/spin_static_fs.wasm", digest = "sha256:..." }
files = [{ source = "static/", destination = "/" }]
```

### 构建与运行

```bash
# 构建所有组件
spin build

# 本地运行
spin up

# 监听指定端口
spin up --listen 0.0.0.0:8080
```

---

## 内置存储

### Key-Value Store

```rust
use spin_sdk::key_value::Store;

#[http_component]
fn handle(req: Request) -> anyhow::Result<impl IntoResponse> {
    let store = Store::open_default()?;
    store.set("counter", &serde_json::to_vec(&42)?)?;
    let val = store.get("counter")?;
    Ok(Response::new(200, val))
}
```

### SQLite 数据库

```rust
use spin_sdk::sqlite::{Connection, Value};

#[http_component]
fn handle(req: Request) -> anyhow::Result<impl IntoResponse> {
    let conn = Connection::open_default()?;
    conn.execute(
        "CREATE TABLE IF NOT EXISTS todos (id INTEGER PRIMARY KEY, title TEXT, done BOOLEAN)",
        &[],
    )?;
    conn.execute(
        "INSERT INTO todos (title, done) VALUES (?, ?)",
        &[Value::Text("Buy milk".into()), Value::Integer(0)],
    )?;
    let rows = conn.execute("SELECT * FROM todos", &[])?;
    Ok(Response::new(200, format!("{:?}", rows.rows)))
}
```

---

## 部署方式

### 推送到 OCI Registry

```bash
# 推送到容器镜像仓库
spin registry push ghcr.io/myorg/my-api:v1.0

# 从 OCI 运行
spin up --from ghcr.io/myorg/my-api:v1.0
```

### 部署到 Kubernetes (SpinKube)

```yaml
apiVersion: core.spinoperator.dev/v1alpha1
kind: SpinApp
metadata:
  name: my-api
spec:
  image: "ghcr.io/myorg/my-api:v1.0"
  executor: containerd-shim-spin
  replicas: 3
```

---

## 与其他方案对比

| 特性 | Spin | AWS Lambda | Cloudflare Workers | Knative |
|:---|:---|:---|:---|:---|
| 运行时 | Wasm | 容器/VM | V8 Isolate | 容器 |
| 冷启动 | <1ms | 100ms-秒级 | <5ms | 秒级 |
| 语言支持 | Rust/Go/JS/Python/C# | 广泛 | JS/Wasm | 广泛 |
| 部署目标 | 本地/K8s/Cloud | AWS 仅 | Cloudflare 仅 | Kubernetes |
| 二进制大小 | KB-MB | MB-GB | KB-MB | MB-GB |
| 安全隔离 | Wasm 沙箱 | VM/容器 | V8 沙箱 | 容器 |

---

## 最佳实践

1. **组件粒度**: 每个路由前缀使用独立组件，实现最小权限和独立部署
2. **最小权限**: 通过 `allowed_outbound_hosts` 限制组件的外部访问范围
3. **存储选择**: 简单 KV 用内置 KV Store，关系数据用 SQLite 或外部 DB
4. **OCI 分发**: 使用 OCI Registry 管理 Spin 应用版本
5. **Wasm 优化**: 使用 `wasm-opt` 优化 Wasm 二进制大小

---

## 参考资源

- [Spin 官方文档](https://developer.fermyon.com/spin)
- [Spin GitHub](https://github.com/fermyon/spin)
- [Spin 模板集](https://developer.fermyon.com/spin/v2/spin-templates)
- [SpinKube](https://www.spinkube.dev/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
