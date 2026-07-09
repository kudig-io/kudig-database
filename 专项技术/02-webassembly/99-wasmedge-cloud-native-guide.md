---
title: WebAssembly (Wasm) 云原生实践指南
description: '# WebAssembly (Wasm) 云原生实践指南'
summary: 'Docker / containerd ──► runc ──► Linux Namespace/CGroup'
category: webassembly-cloud-native
tags:
- k8s
- wasm
- webassembly
- cloud-native
- kubelet
- istio
- envoy
- containerd
- docker
- hpa
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- 开发工程师
- SRE
estimated_read_time: 5min
intent_queries:
- WebAssembly (Wasm) 云原生实践指南 是什么
- 如何 WebAssembly (Wasm) 云原生实践指南
- Kubernetes 38 webassembly cloud native 最佳实践
trigger_keywords:
- WebAssembly
- Wasm
- 云原生实践指南
- webassembly
- cloud
- native
prerequisites:
- kubectl-basics
- service-mesh-basics
- ebpf-basics
- gpu-scheduling-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# WebAssembly (Wasm) 云原生实践指南

> **适用版本**: [[WasmEdge|WasmEdge]] v0.14 / [[containerd|containerd]] wasm shims v0.8 / [[Spin|Spin]] v3.2  
> **最后更新**: 2026-04-24  
> **难度**: 高级

---

## 📋 目录

- [一、WebAssembly 云原生定位](#一webassembly-云原生定位)
- [二、容器 vs Wasm 运行时对比](#二容器-vs-wasm-运行时对比)
- [三、containerd Wasm Shims 部署](#三containerd-wasm-shims-部署)
- [四、WasmEdge 运行时](#四wasmedge-运行时)
- [五、Spin 微服务框架](#五spin-微服务框架)
- [六、K8s 中运行 Wasm 工作负载](#六k8s-中运行-wasm-工作负载)
- [七、Wasm 与 Service Mesh 集成](#七wasm-与-service-mesh-集成)
- [八、典型场景与案例](#八典型场景与案例)
- [九、Wasm 生态展望](#九wasm-生态展望)

---

## 一、WebAssembly 云原生定位

```
Wasm 在云原生中的位置
├── 边缘计算 (Edge)
│   ├── 启动时间 < 1ms (vs 容器 100ms+)
│   ├── 内存占用 < 1MB (vs 容器 10MB+)
│   └── 冷启动优化 (Serverless 场景)
│
├── 插件系统 (Plugins)
│   ├── Envoy Proxy Wasm 过滤器
│   ├── NGINX Wasm 模块
│   └── API Gateway 扩展
│
├── Serverless / FaaS
│   ├── 更细粒度计费
│   ├── 更高密度部署
│   └── 更快弹性响应
│
└── 沙箱安全 (Sandboxing)
    ├── Capability-based 安全模型
    ├── 比容器更小的攻击面
    └── 多语言统一运行时
```

### 为什么 Wasm + K8s?

| 场景 | 容器 | Wasm |
|:---|:---|:---|
| 启动时间 | 100ms - 1s | < 1ms |
| 内存开销 | 10-100MB | 1-10MB |
| 二进制大小 | 50-500MB | 1-50MB |
| 安全边界 | Namespace/CGroup | Capability-based |
| 冷启动频率 | 受限 | 极高 |
| 适用工作负载 | 长期运行 | 短期/事件驱动 |

---

## 二、容器 vs Wasm 运行时对比

```
传统容器运行时
Docker / containerd ──► runc ──► Linux Namespace/CGroup

Wasm 运行时
containerd ──► containerd-wasm-shims ──► Wasm Runtime (WasmEdge/wasmedge/wasmtime)

混合运行时 (K8s Node)
┌─────────────────────────────────────┐
│  Kubelet                              │
│    ├── containerd (标准容器)          │
│    └── containerd (Wasm shim)        │
│         ├── WasmEdge Runtime         │
│         └── wasi-libc                │
└─────────────────────────────────────┘
```

---

## 三、containerd Wasm Shims 部署

### 3.1 安装 Wasm Shims

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 下载最新 shims
wget https://github.com/containerd/runwasi/releases/download/containerd-shim-wasmedge%2Fv0.8.0/containerd-shim-wasmedge-v1-linux-x86_64.tar.gz
tar -xzf containerd-shim-wasmedge-v1-linux-x86_64.tar.gz
sudo mv containerd-shim-* /usr/local/bin/

# 配置 containerd
sudo mkdir -p /etc/containerd/
cat <<EOF | sudo tee /etc/containerd/config.toml
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.wasmedge]
  runtime_type = "io.containerd.wasmedge.v1"
EOF

sudo systemctl restart containerd
```
### 3.2 验证安装

```bash
# 查看可用 shims
ls /usr/local/bin/containerd-shim-*
# containerd-shim-wasmedge-v1
# containerd-shim-wasmtime-v1
# containerd-shim-spin-v1
```

---

## 四、WasmEdge 运行时

### 4.1 安装

```bash
# 安装 WasmEdge
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash
source $HOME/.wasmedge/env

# 验证
wasmedge --version
```

### 4.2 运行简单 Wasm 模块

```bash
# 拉取 Wasm 镜像 (OCI 格式)
ctr image pull ghcr.io/wasmedge/wasmedge-example:latest

# 运行 Wasm 模块
ctr run --rm --runtime=io.containerd.wasmedge.v1 \
  ghcr.io/wasmedge/wasmedge-example:latest \
  hello-wasm
```

### 4.3 构建 Wasm 应用 (Rust)

```rust
// src/main.rs
use std::net::TcpListener;
use std::io::{Read, Write};

fn main() {
    let listener = TcpListener::bind("0.0.0.0:8080").unwrap();
    println!("Server listening on port 8080");
    
    for stream in listener.incoming() {
        let mut stream = stream.unwrap();
        let mut buffer = [0; 1024];
        stream.read(&mut buffer).unwrap();
        
        let response = "HTTP/1.1 200 OK\r\n\r\nHello from WasmEdge!";
        stream.write(response.as_bytes()).unwrap();
        stream.flush().unwrap();
    }
}
```

```toml
# Cargo.toml
[package]
name = "wasm-app"
version = "0.1.0"
edition = "2021"

[dependencies]

[profile.release]
opt-level = 3
lto = true
```

```bash
# 编译为 Wasm
cargo build --target wasm32-wasi --release

# 运行
wasmedge target/wasm32-wasi/release/wasm-app.wasm
```

---

## 五、Spin 微服务框架

### 5.1 安装 Spin CLI

```bash
curl -fsSL https://developer.fermyon.com/downloads/install.sh | bash
spin --version
```

### 5.2 创建 HTTP 触发器应用

```bash
# 创建新应用
spin new http-rust hello-spin
cd hello-spin

# 构建
spin build

# 本地测试
spin up --listen 0.0.0.0:3000
```

### 5.3 Spin 应用结构

```
hello-spin/
├── spin.toml          # 应用配置
├── Cargo.toml
└── src/
    └── lib.rs
```

```toml
# spin.toml
spin_manifest_version = 2

[application]
name = "hello-spin"
version = "0.1.0"
authors = ["team@example.com"]
description = "Hello Spin app"

trigger.http
route = "/hello"
component = "hello"

[component.hello]
source = "target/wasm32-wasi/release/hello_spin.wasm"
allowed_outbound_hosts = []
[component.hello.build]
command = "cargo build --target wasm32-wasi --release"
watch = ["src/**/*.rs", "Cargo.toml"]
```

---

## 六、K8s 中运行 Wasm 工作负载

### 6.1 节点运行时类配置

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: wasmedge
handler: wasmedge
---
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: wasmtime
handler: wasmtime
---
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: spin
handler: spin
```

### 6.2 部署 Wasm Pod

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: wasm-hello
  namespace: default
spec:
  replicas: 3
  selector:
    matchLabels:
      app: wasm-hello
  template:
    metadata:
      labels:
        app: wasm-hello
    spec:
      runtimeClassName: wasmedge
      containers:
      - name: hello
        image: ghcr.io/wasmedge/wasmedge-example:latest
        resources:
          requests:
            memory: "64Mi"
            cpu: "100m"
          limits:
            memory: "128Mi"
            cpu: "500m"
---
apiVersion: v1
kind: Service
metadata:
  name: wasm-hello
spec:
  selector:
    app: wasm-hello
  ports:
  - port: 8080
    targetPort: 8080
```

### 6.3 HPA 自动扩展 Wasm 工作负载

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: wasm-hello-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: wasm-hello
  minReplicas: 3
  maxReplicas: 100
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
```

---

## 七、Wasm 与 Service Mesh 集成

### 7.1 Envoy Proxy-Wasm

```yaml
apiVersion: networking.istio.io/v1beta1
kind: EnvoyFilter
metadata:
  name: wasm-filter
  namespace: istio-system
spec:
  configPatches:
  - applyTo: HTTP_FILTER
    match:
      context: SIDECAR_INBOUND
      listener:
        filterChain:
          filter:
            name: envoy.filters.network.http_connection_manager
    patch:
      operation: INSERT_BEFORE
      value:
        name: envoy.filters.http.wasm
        typed_config:
          "@type": type.googleapis.com/envoy.extensions.filters.http.wasm.v3.Wasm
          config:
            name: my_filter
            root_id: my_root_id
            vm_config:
              vm_id: my_vm_id
              runtime: envoy.wasm.runtime.v8
              code:
                remote:
                  http_uri:
                    uri: https://example.com/filters/auth.wasm
                    cluster: google_storage
                    timeout: 10s
```

---

## 八、典型场景与案例

| 场景 | 方案 | 优势 |
|:---|:---|:---|
| **边缘 Serverless** | Spin + K8s + KEDA | 毫秒级冷启动 |
| **API Gateway 插件** | Envoy Proxy-Wasm | 动态加载，无需重启 |
| **微服务沙箱** | WasmEdge + containerd | 更小攻击面 |
| **AI 推理** | WasmEdge + WASI-NN | 跨平台模型部署 |
| **区块链智能合约** | Wasm 虚拟机 | 确定性执行 |

### 案例：Cloudflare Workers

```
Cloudflare Workers = V8 Isolate + Wasm
├── 全球 300+ 边缘节点
├── 冷启动 < 1ms
├── 每个 Worker 隔离
└── 支持 Rust/Go/AssemblyScript 编译为 Wasm
```

---

## 九、Wasm 生态展望

### 当前成熟度 (2026)

| 能力 | 成熟度 | 说明 |
|:---|:---|:---|
| K8s 集成 | Beta | containerd shim 可用 |
| Service Mesh 插件 | Beta | Envoy/Istio 支持 |
| 语言支持 | 良好 | Rust/Go/C++/AssemblyScript |
| 调试工具 | 初级 | 不如容器成熟 |
| 可观测性 | 初级 | 需适配 Wasm 运行时 |
| 生态系统 | 成长中 | Spin/Fermyon 领先 |

### 未来方向

- **WASI Preview 2**: 组件模型标准化
- **GPU 支持**: WASI-NN / WebGPU
- **分布式 Wasm**: 跨节点 Wasm 编排
- **Wasm 与 eBPF**: 内核态 + 用户态协同

---

## 参考链接

- [WasmEdge 文档](https://wasmedge.org/docs/)
- [Spin 框架](https://developer.fermyon.com/spin/)
- [containerd runwasi](https://github.com/containerd/runwasi)
- [WebAssembly CNCF](https://www.cncf.io/projects/wasmcloud/)
- [Bytecode Alliance](https://bytecodealliance.org/)
- [Proxy-Wasm](https://github.com/proxy-wasm/spec)

---

## Obsidian 相关文档

- domain-38-webassembly-cloud-native MOC
- [[专项技术/README.md|Domain 15: WebAssembly 云原生 (WebAssembly Cloud Native)]]
- Domain-38 WebAssembly 云原生 — 开源项目索引
- WebAssembly 云原生基础
- containerd Wasm 运行时
- SpinKube 框架实践
- wasmCloud 平台
- WasmEdge 运行时
- Wasm 组件模型 (Wasm Component Model)
- Wasm 插件系统 (Wasm Plugin System)
- Wasm AI 推理 (Wasm AI Inference)
- Wasm Serverless (Wasm Serverless)

## See Also

- 09-wasm-serverless
- 10-wasm-security-sandbox
- 01-wasm-fundamentals-cloud-native
- 02-containerd-wasm-shim

```

<!-- risk-assessed -->
