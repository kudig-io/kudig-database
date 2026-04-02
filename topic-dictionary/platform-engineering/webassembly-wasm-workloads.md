# WebAssembly（Wasm）工作负载

## 概述

**WebAssembly（Wasm）** 最初为浏览器设计，现已成为云原生领域的新兴运行时标准。在 Kubernetes 上运行 Wasm 工作负载具有**毫秒级冷启动、极小的镜像体积、沙箱级安全隔离**等优势，特别适用于边缘计算、Serverless、微服务和高并发事件驱动场景。2026 年，Wasm 正在成为 Kubernetes 的"第三运行时"（与容器、VM 并列）。

## 核心概念/原理

### 1. WebAssembly 技术特性

| 特性 | 容器 | WebAssembly | 说明 |
|------|------|-------------|------|
| 启动时间 | 秒级 | 毫秒级 | 适合 Serverless 和自动扩缩容 |
| 镜像大小 | MB–GB 级 | KB–MB 级 | 更快的分发和更低的存储成本 |
| 安全边界 | OS 级命名空间 | 沙箱级 Capability 模型 | 默认无权限，显式授权 |
| 资源开销 | 较高（需完整 OS 层） | 极低（直接运行在宿主运行时上） | 单节点可运行数万个 Wasm 实例 |

### 2. Wasm 运行时

Kubernetes 上主流的 Wasm 运行时包括：
- **WasmEdge**：CNCF 沙箱项目，高性能、支持 AI 推理扩展
- **Wasmtime**：Bytecode Alliance 出品，专注于安全性和标准兼容性
- **Spin / Fermyon**：面向微服务和事件驱动的 Wasm 应用框架
- **wasmedge-containers**：允许 Kubernetes 通过 containerd shim 直接调度 Wasm 模块

### 3. Kubernetes 集成方式

Wasm 工作负载通过 **Containerd Shim** 集成到 Kubernetes 中：
1. 在节点上安装支持 Wasm 的 containerd runtime（如 `containerd-wasm-shims`）
2. 在 RuntimeClass 中注册 Wasm 运行时
3. Pod 的 `runtimeClassName` 指定为 wasm，即可运行 Wasm 模块

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: wasmtime-spin
handler: spin
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: wasm-app
spec:
  template:
    spec:
      runtimeClassName: wasmtime-spin
      containers:
      - name: app
        image: ghcr.io/fermyon/spin-app:latest
```

### 4. WASI（WebAssembly System Interface）

WASI 为 Wasm 模块提供标准化的系统调用接口（文件、网络、时钟等），使其能够脱离浏览器独立运行。2026 年的 WASI Preview 2 已支持：
- 组件模型（Component Model）
- 网络套接字
- HTTP 客户端/服务器
- 多语言互操作（Rust、Go、C++、Python 等）

## 关键机制或特性

### 轻量 Serverless

Wasm 的毫秒级启动使其成为 Kubernetes 上 **Scale-to-Zero Serverless** 的理想载体：
- 从 0 到 1 的实例启动时间 < 100ms
- 支持每秒数千次的并发伸缩
- 比传统容器 Serverless 平台（Knative）资源效率更高

### 边缘计算

在资源受限的边缘设备上，Wasm 优势更加明显：
- 镜像体积小，便于通过蜂窝网络或卫星链路分发
- 低内存占用，单边缘节点可运行更多工作负载
- 与 K3s 等轻量级 Kubernetes 发行版结合使用

### 安全模型

Wasm 采用 **Capability-based Security**：
- 模块默认无任何系统访问权限
- 所有能力（文件读写、网络访问）必须显式声明和授予
- 即使运行时漏洞被利用，攻击者也被限制在沙箱内

## 使用场景

1. **高并发 API Gateway/边缘代理**：使用 Wasm 编写 Envoy 过滤器，动态加载安全策略
2. **Serverless 函数平台**：替代传统容器，实现极速冷启动和按需计费
3. **边缘 AI 推理**：在工厂传感器、摄像头等边缘节点部署轻量级 AI 推理模块
4. **CI/CD 插件与 Webhook**：快速、安全地运行不可信的用户自定义脚本
5. **微服务拆分**：将细粒度服务以 Wasm 模块形式部署，降低资源开销

## 最佳实践/注意事项

- **明确选择 RuntimeClass**：确保节点已安装对应的 Wasm shim，并在 Pod 中正确声明 `runtimeClassName`
- **镜像仓库兼容性**：并非所有镜像仓库都支持 Wasm artifact 格式，推荐使用 OCI-compliant 仓库（如 Harbor 2.5+）
- **调试工具尚在发展**：Wasm 的调试和可观测性生态不如容器成熟，需配合专门的 trace 和 log 工具
- **避免 I/O 密集型工作负载**：当前 WASI 的异步 I/O 能力仍在演进，高 I/O 场景建议继续使用容器
- **语言工具链选择**：Rust 和 Go 的 Wasm 支持最成熟；Python、JavaScript 可通过 WASI SDK 运行，但体积和性能可能受限
- **监控 Wasm 运行时健康**：除了应用指标，还需监控 Wasm runtime（如 WasmEdge）的内存和 CPU 占用
- **渐进式采用**：从边缘网关、Serverless 函数等适合场景开始，逐步探索核心业务负载的 Wasm 化

## 参考链接

- [WasmEdge Documentation](https://wasmedge.org/book/en/)
- [Spin - Serverless WebAssembly](https://developer.fermyon.com/spin/)
- [Containerd Wasm Shims](https://github.com/containerd/runwasi)
- [Ajeet Singh Raina - Top 5 Trends Shaping Kubernetes in 2026](https://www.ajeetraina.com/top-5-trends-shaping-kubernetes-in-2026/)
- [CNCF WebAssembly Landscape](https://landscape.cncf.io/card-mode?category=wasm&grouping=category)
