---
title: WasmEdge
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- containerd
- cri-o
- docker
- wasm
- serverless
- llm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- WasmEdge 是什么
- 如何 WasmEdge
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- WasmEdge
- cncf
- landscape
---

# WasmEdge

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://wasmedge.org/ |
| **GitHub** | https://github.com/WasmEdge/WasmEdge |
| **许可证** | Apache-2.0 |
| **开发语言** | C++, Rust |
| **CNCF 状态** | Sandbox |

---

## 项目概述

WasmEdge 是一个轻量级、高性能、可扩展的 WebAssembly (Wasm) 运行时，适用于云原生、边缘计算和去中心化应用。它是目前最快的 Wasm 运行时之一，支持 AOT (Ahead-of-Time) 编译，并提供丰富的宿主函数扩展，包括网络套接字、TensorFlow 推理、Key-Value 存储等。

### 核心特性

- **高性能**: AOT 编译，接近原生代码执行速度
- **轻量级**: 启动时间低于 1ms，内存占用远小于容器
- **WASI 支持**: 完整的 WASI (WebAssembly System Interface) 支持
- **网络支持**: 原生 Socket API，支持 HTTP/HTTPS 客户端和服务端
- **AI 推理**: 集成 GGML/llama.cpp 后端，支持 LLM 推理
- **容器集成**: 通过 crun/youki 与 containerd/CRI-O 集成
- **多语言**: 支持 Rust, C/C++, Go, JavaScript, Python 编译为 Wasm
- **插件系统**: 可扩展的插件架构，支持自定义宿主函数

---

## 架构设计

```
┌─────────────────────────────────────────────────┐
│                WasmEdge Runtime                   │
│                                                   │
│  ┌───────────────────────────────────────────┐   │
│  │              Wasm Application              │   │
│  │  (Rust/C/Go/JS/Python → .wasm)            │   │
│  └──────────────────┬────────────────────────┘   │
│                     │                             │
│  ┌──────────────────┴────────────────────────┐   │
│  │           WasmEdge VM                      │   │
│  │                                            │   │
│  │  ┌──────────┐  ┌──────────┐  ┌─────────┐ │   │
│  │  │Interpre- │  │   AOT    │  │  JIT    │ │   │
│  │  │  ter     │  │ Compiler │  │(future) │ │   │
│  │  └──────────┘  └──────────┘  └─────────┘ │   │
│  └──────────────────┬────────────────────────┘   │
│                     │                             │
│  ┌──────────────────┴────────────────────────┐   │
│  │            Host Functions / Plugins        │   │
│  │                                            │   │
│  │  ┌──────┐ ┌───────┐ ┌──────┐ ┌────────┐ │   │
│  │  │ WASI │ │Network│ │ WASI │ │ GGML/  │ │   │
│  │  │      │ │Socket │ │  NN  │ │ LLM    │ │   │
│  │  └──────┘ └───────┘ └──────┘ └────────┘ │   │
│  │  ┌──────┐ ┌───────┐ ┌──────┐            │   │
│  │  │Crypto│ │ HTTP  │ │Tensor│            │   │
│  │  │      │ │       │ │Flow  │            │   │
│  │  └──────┘ └───────┘ └──────┘            │   │
│  └───────────────────────────────────────────┘   │
└─────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装 WasmEdge

```bash
# 一键安装
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash

# 验证安装
wasmedge --version

# 安装带 GGML 插件版本（用于 LLM 推理）
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- --plugins wasmedge_rustls wasi_nn-ggml
```

### 运行 Wasm 应用

```bash
# 直接运行 Wasm 模块
wasmedge hello.wasm

# AOT 编译后运行（更高性能）
wasmedgec hello.wasm hello_aot.wasm
wasmedge hello_aot.wasm

# 运行带参数的应用
wasmedge --dir /data:/data app.wasm --input /data/file.txt
```

### Rust 开发示例

```rust
// src/main.rs - HTTP 服务器
use hyper::service::{make_service_fn, service_fn};
use hyper::{Body, Request, Response, Server};
use std::convert::Infallible;
use std::net::SocketAddr;

async fn handle(_req: Request<Body>) -> Result<Response<Body>, Infallible> {
    Ok(Response::new(Body::from("Hello from WasmEdge!")))
}

#[tokio::main(flavor = "current_thread")]
async fn main() {
    let addr = SocketAddr::from(([0, 0, 0, 0], 8080));
    let make_svc = make_service_fn(|_conn| async {
        Ok::<_, Infallible>(service_fn(handle))
    });
    let server = Server::bind(&addr).serve(make_svc);
    server.await.unwrap();
}
```

```bash
# 编译为 Wasm
cargo build --target wasm32-wasip1 --release

# 运行
wasmedge target/wasm32-wasip1/release/http-server.wasm
```

---

## Kubernetes 集成

### 使用 containerd + crun

```bash
# 配置 containerd 使用 crun + WasmEdge
# /etc/containerd/config.toml
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.wasmedge]
  runtime_type = "io.containerd.runc.v2"
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.wasmedge.options]
    BinaryName = "/usr/bin/crun"

# 创建 RuntimeClass
cat <<EOF | kubectl apply -f -
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: wasmedge
handler: wasmedge
EOF
```

### 部署 Wasm Pod

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: wasm-http-server
  annotations:
    module.wasm.image/variant: compat-smart
spec:
  runtimeClassName: wasmedge
  containers:
    - name: server
      image: registry.example.com/wasm-http-server:latest
      ports:
        - containerPort: 8080
      resources:
        limits:
          cpu: "100m"
          memory: "32Mi"
```

### 与 Docker 集成

```bash
# Docker Desktop 支持 Wasm 运行时
docker run --runtime=io.containerd.wasmedge.v1 \
  --platform wasi/wasm \
  registry.example.com/wasm-app:latest
```

---

## LLM 推理

### 运行本地 LLM

```bash
# 下载 LLM GGUF 模型
curl -LO https://huggingface.co/TheBloke/Llama-2-7B-Chat-GGUF/resolve/main/llama-2-7b-chat.Q4_K_M.gguf

# 使用 WasmEdge + llama 运行
wasmedge --dir .:. \
  --nn-preload default:GGML:AUTO:llama-2-7b-chat.Q4_K_M.gguf \
  llama-chat.wasm \
  --prompt-template llama-2-chat \
  --ctx-size 4096
```

### API 服务模式

```bash
# 启动 OpenAI 兼容的 API 服务
wasmedge --dir .:. \
  --nn-preload default:GGML:AUTO:model.gguf \
  llama-api-server.wasm \
  --prompt-template llama-2-chat \
  --ctx-size 4096 \
  --socket-addr 0.0.0.0:8080

# 调用 API
curl -X POST http://localhost:8080/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "default",
    "messages": [{"role": "user", "content": "Hello!"}],
    "temperature": 0.7
  }'
```

---

## 与其他 Wasm 运行时对比

| 特性 | WasmEdge | Wasmtime | WAMR | Wasmer |
|:---|:---|:---|:---|:---|
| **AOT 编译** | 支持 | 支持 | 支持 | 支持 |
| **WASI** | 完整 | 完整 | 部分 | 完整 |
| **网络套接字** | 原生 | WASI 实验 | 有限 | 部分 |
| **AI/ML 推理** | GGML/TF | 无 | 无 | 无 |
| **容器集成** | crun/youki | wasmtime | 无 | 无 |
| **启动时间** | <1ms | ~5ms | <1ms | ~3ms |
| **嵌入式支持** | 良好 | 良好 | 最佳 | 良好 |

---

## 最佳实践

1. **AOT 编译**: 生产环境使用 `wasmedgec` 预编译 Wasm 模块提升性能
2. **资源限制**: Wasm 天然沙箱隔离，配合 Kubernetes 资源限制双重保护
3. **镜像大小**: Wasm 镜像通常只有几 MB，相比容器镜像显著减少存储和传输
4. **冷启动**: 利用 <1ms 启动特性优化 Serverless 冷启动场景
5. **LLM 部署**: 使用 WasmEdge GGML 插件在边缘设备运行量化 LLM
6. **渐进迁移**: 从高频短任务开始迁移到 Wasm，逐步扩展到更多工作负载

---

## 参考资源

- [WasmEdge 官方文档](https://wasmedge.org/docs/)
- [WasmEdge GitHub](https://github.com/WasmEdge/WasmEdge)
- [WasmEdge Book](https://wasmedge.org/book/en/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
