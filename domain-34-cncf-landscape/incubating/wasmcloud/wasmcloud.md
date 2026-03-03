# wasmCloud

> **成熟度**: Incubating | **加入时间**: 2021-07 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://wasmcloud.com |
| **GitHub** | https://github.com/wasmCloud/wasmCloud |
| **许可证** | Apache-2.0 |
| **主要语言** | Rust |
| **CNCF 分类** | Serverless & WebAssembly |

---

## 项目概述

wasmCloud 是用于构建分布式 WebAssembly 应用的平台。它提供安全、可移植的应用运行环境，通过能力模型（Capability Model）实现组件与外部资源的解耦，支持跨云、边缘和本地的统一部署。

## 核心特性

- **WebAssembly 运行时**: 基于 wasmtime 的安全沙箱执行
- **能力模型**: 组件通过接口契约访问外部资源
- **位置透明**: 组件可在任意节点运行
- **热更新**: 无停机更新组件和配置
- **多语言支持**: Rust、Go、JavaScript、Python 等
- **分布式网络**: NATS 消息总线连接所有节点
- **声明式部署**: wadm 应用管理器

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                   wasmCloud Architecture                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    Application Layer                       │ │
│  │                                                            │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐   │ │
│  │  │  Component  │  │  Component  │  │   Component     │   │ │
│  │  │  (Actor)    │  │  (Actor)    │  │   (Actor)       │   │ │
│  │  │   Wasm      │  │   Wasm      │  │    Wasm         │   │ │
│  │  └──────┬──────┘  └──────┬──────┘  └────────┬────────┘   │ │
│  │         │                │                   │            │ │
│  │         └────────────────┼───────────────────┘            │ │
│  │                          │                                │ │
│  │                    WIT Interfaces                         │ │
│  │                          │                                │ │
│  │         ┌────────────────┼───────────────────┐            │ │
│  │         ▼                ▼                   ▼            │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐   │ │
│  │  │  HTTP       │  │ Key-Value   │  │   Messaging     │   │ │
│  │  │  Provider   │  │  Provider   │  │   Provider      │   │ │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘   │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│                              ▼                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                   wasmCloud Host                           │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐   │ │
│  │  │  wasmtime   │  │   NATS      │  │   Host Runtime  │   │ │
│  │  │  Runtime    │  │   Client    │  │                 │   │ │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘   │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│       ┌──────────────────────┼──────────────────────┐           │
│       ▼                      ▼                      ▼           │
│  ┌─────────┐          ┌─────────────┐        ┌─────────┐       │
│  │  Host   │◀────────▶│    NATS     │◀──────▶│  Host   │       │
│  │ (Node1) │          │  (Lattice)  │        │ (NodeN) │       │
│  └─────────┘          └─────────────┘        └─────────┘       │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 核心概念

| 概念 | 说明 |
|------|------|
| Component | WebAssembly 组件（业务逻辑） |
| Provider | 能力提供者（HTTP、数据库、消息队列等） |
| Lattice | 由 NATS 连接的 wasmCloud 主机网络 |
| Link | 组件与 Provider 的运行时绑定 |
| WIT | WebAssembly Interface Type（接口定义） |

---

## 快速开始

### 安装 wash CLI

```bash
# macOS
brew install wasmcloud/wasmcloud/wash

# Linux
curl -s https://packagecloud.io/install/repositories/wasmcloud/core/script.deb.sh | sudo bash
sudo apt install wash

# 验证安装
wash --version
```

### 启动 wasmCloud

```bash
# 启动本地主机（包含 NATS）
wash up

# 查看主机状态
wash get hosts
```

### 创建 Hello World 组件

```bash
# 创建项目
wash new component hello --template-name hello-world-rust

cd hello

# 构建组件
wash build

# 部署到本地
wash app deploy wadm.yaml
```

### 组件代码示例 (Rust)

```rust
// src/lib.rs
wit_bindgen::generate!({
    world: "hello",
    exports: {
        "wasi:http/incoming-handler": HttpServer,
    },
});

use exports::wasi::http::incoming_handler::Guest;
use wasi::http::types::*;

struct HttpServer;

impl Guest for HttpServer {
    fn handle(request: IncomingRequest, response_out: ResponseOutparam) {
        let response = OutgoingResponse::new(Fields::new());
        response.set_status_code(200).unwrap();
        
        let body = response.body().unwrap();
        body.write().unwrap().blocking_write_and_flush(b"Hello from wasmCloud!").unwrap();
        body.finish().unwrap();
        
        ResponseOutparam::set(response_out, Ok(response));
    }
}
```

---

## 应用部署 (wadm)

### 应用清单

```yaml
# wadm.yaml
apiVersion: core.oam.dev/v1beta1
kind: Application
metadata:
  name: hello-world
  annotations:
    version: v1.0.0
spec:
  components:
    - name: http-hello
      type: component
      properties:
        image: file://./build/http_hello_world_s.wasm
      traits:
        - type: spreadscaler
          properties:
            instances: 3
        - type: link
          properties:
            target: httpserver
            namespace: wasi
            package: http
            interfaces: [incoming-handler]

    - name: httpserver
      type: capability
      properties:
        image: ghcr.io/wasmcloud/http-server:0.21.0
      traits:
        - type: spreadscaler
          properties:
            instances: 1
        - type: link
          properties:
            target: http-hello
            namespace: wasi
            package: http
            interfaces: [incoming-handler]
            source_config:
              - name: default-http
                properties:
                  address: 0.0.0.0:8080
```

### 部署命令

```bash
# 部署应用
wash app deploy wadm.yaml

# 查看应用状态
wash app list
wash app status hello-world

# 更新应用
wash app deploy wadm.yaml --version v1.1.0

# 删除应用
wash app delete hello-world
```

---

## 常用 Providers

| Provider | 功能 | 接口 |
|----------|------|------|
| http-server | HTTP 入站请求 | wasi:http/incoming-handler |
| http-client | HTTP 出站请求 | wasi:http/outgoing-handler |
| keyvalue-redis | Redis KV 存储 | wasi:keyvalue/* |
| keyvalue-vault | Vault KV 存储 | wasi:keyvalue/* |
| messaging-nats | NATS 消息 | wasmcloud:messaging/* |
| sqldb-postgres | PostgreSQL | wasmcloud:sqldb/* |
| blobstore-s3 | S3 对象存储 | wasi:blobstore/* |

---

## Kubernetes 部署

```yaml
# wasmcloud-host.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: wasmcloud-host
spec:
  replicas: 3
  selector:
    matchLabels:
      app: wasmcloud-host
  template:
    metadata:
      labels:
        app: wasmcloud-host
    spec:
      containers:
        - name: wasmcloud
          image: ghcr.io/wasmcloud/wasmcloud:latest
          env:
            - name: WASMCLOUD_NATS_HOST
              value: nats.default.svc.cluster.local
            - name: WASMCLOUD_NATS_PORT
              value: "4222"
            - name: WASMCLOUD_LATTICE
              value: default
          ports:
            - containerPort: 4222
```

---

## 多语言支持

### Go 组件

```go
//go:generate wit-bindgen-go generate --world hello --out gen ./wit

package main

import (
    "gen/wasi/http/types"
)

func init() {
    types.Exports.Handle = handleRequest
}

func handleRequest(req types.IncomingRequest, out types.ResponseOutparam) {
    resp := types.NewOutgoingResponse(types.NewFields())
    resp.SetStatusCode(200)
    body := resp.Body()
    body.Write().BlockingWriteAndFlush([]byte("Hello from Go!"))
    body.Finish()
    types.ResponseOutparamSet(out, types.Ok(resp))
}

func main() {}
```

---

## 最佳实践

1. **组件轻量化**: 保持组件专注于业务逻辑，外部资源访问通过 Provider
2. **版本管理**: 使用 OCI 镜像仓库管理组件版本
3. **测试策略**: 使用 wash 测试工具验证组件行为
4. **监控集成**: 配置 OTEL 导出追踪和指标
5. **安全边界**: 利用 Wasm 沙箱和能力模型实现最小权限

---

## 参考资源

- [官方文档](https://wasmcloud.com/docs)
- [GitHub Repo](https://github.com/wasmCloud/wasmCloud)
- [示例应用](https://github.com/wasmCloud/wasmCloud/tree/main/examples)
- [WIT 规范](https://component-model.bytecodealliance.org/design/wit.html)

---

**维护者**: Kudig Team | **许可证**: MIT
