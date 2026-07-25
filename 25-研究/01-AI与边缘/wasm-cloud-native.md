---
title: WebAssembly 云原生应用
summary: 研究 WebAssembly (Wasm) 在云原生领域的应用前景，涵盖 WASI、Spin、containerd-wasm-shim、边缘计算场景及与传统容器的对比。
category: research
tags:
- research
- wasm
- webassembly
- wasi
- edge-computing
- serverless
tier: supporting
created: '2026-07-21'
updated: '2026-07-21'
last_updated: '2026-07-21'
status: done
---

# WebAssembly 云原生应用

## 研究背景

WebAssembly (Wasm) 最初为浏览器设计，但 WASI (WebAssembly System Interface) 的出现使其成为服务端和边缘计算的轻量级运行时。Wasm 模块启动时间 < 1ms（vs 容器 100ms+），内存占用极小，天然适合 Serverless 和边缘场景。

## 核心问题

1. Wasm 在 K8s 生态中的集成现状和成熟度如何？
2. Wasm vs 容器：性能、安全、生态的对比？
3. 哪些场景最适合用 Wasm 替代容器？
4. Wasm 的局限性和当前不适合的场景？

## 调研发现

### 发现一：Wasm vs 容器对比

| 维度 | 容器 (OCI) | WebAssembly |
|------|-----------|-------------|
| 启动时间 | 100ms - 数秒 | < 1ms |
| 镜像大小 | 50MB - 1GB+ | 1MB - 10MB |
| 内存开销 | 10MB+ | < 1MB |
| 安全隔离 | namespace + cgroup | 沙箱 (能力模型) |
| 语言支持 | 任意 | Rust/Go/C/JS (编译到 Wasm) |
| 生态成熟度 | 极高 | 早期 |
| 系统调用 | 完整 Linux | WASI 子集 |
| 网络能力 | 完整 | 受限 (逐步完善) |
| GPU 支持 | 完整 | 无 |

### 发现二：K8s Wasm 集成方案

| 方案 | 状态 | 描述 |
|------|------|------|
| containerd-wasm-shim | Beta | 通过 RuntimeClass 运行 Wasm |
| SpinKube | 早期 | Fermyon Spin on K8s |
| wasmCloud | 早期 | 分布式 Wasm 运行时 |
| Krustlet | 已归档 | 纯 Wasm 节点 (已停止) |
| Docker+Wasm | Tech Preview | Docker Desktop 支持 |

### 发现三：适用场景判断

**适合 Wasm 的场景：**
- 边缘计算（资源受限、快速启动）
- Serverless 函数（冷启动敏感）
- 插件/扩展系统（安全沙箱执行）
- API 网关过滤器（Envoy Wasm Filter）
- 多租户隔离（不信任代码执行）

**不适合 Wasm 的场景：**
- 需要完整 Linux 系统调用
- GPU/硬件加速工作负载
- 需要丰富生态库（Python ML 等）
- 有状态长时间运行服务
- 需要完整网络栈

## 落地方案

### 渐进式引入路径

1. **Phase 1**: API 网关 Wasm 过滤器（Envoy/Istio）
2. **Phase 2**: 边缘节点 Wasm 函数（SpinKube）
3. **Phase 3**: 混合调度（容器 + Wasm RuntimeClass）
4. **Phase 4**: 评估全 Wasm 微服务（待生态成熟）

## 参考资源

- [WASI](https://wasi.dev/)
- [SpinKube](https://www.spinkube.dev/)
- [containerd Wasm Shim](https://github.com/containerd/runwasi)
- [Fermyon Spin](https://www.fermyon.com/spin)
- [wasmCloud](https://wasmcloud.com/)

## Related Tags

- [[27-标签/k8s|k8s]]
- [[27-标签/containerd|containerd]]
- [[27-标签/production|production]]
