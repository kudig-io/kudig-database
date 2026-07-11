---
title: container2wasm (entities)
description: '## 概述'
summary: 'container2wasm 是一个将 Linux 容器镜像转换为 WebAssembly (WASM) 模块的工具。它通过嵌入 Linux 内核模拟器（基于 Bochs x86 模拟器或 TinyEMU RISC-V 模拟器），使原本为 x86_64/aarch64 编译的容器镜像能够在任何支持 WASM 的环境中运行，'
category: entities
tags:
- k8s
- cncf
- runtime
- container2wasm
- crd
- operator
- wasm
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- container2wasm 是什么
- 如何 container2wasm
trigger_keywords:
- container2wasm
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# container2wasm

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Go

## 概述

container2wasm（c2w）是由 NTT（日本电信电话公司）开发的开源工具，2023 年进入 CNCF Sandbox。它将标准的 **Linux 容器镜像转换为 WebAssembly (WASM) 模块**。核心机制是在 Wasm 中嵌入完整的 Linux 内核模拟器（Bochs 用于 x86，TinyEMU 用于 RISC-V），使得原本为 x86_64/aarch64 编译的容器镜像可以在任何支持 Wasm 的环境中运行——包括浏览器、边缘设备和 Wasm 运行时（Wasmtime、WasmEdge、WAMR）。

c2w 的独特价值在于**无需修改原始容器镜像即可跨平台运行**。例如，一个 x86 的 Python 应用容器可以被转换为 Wasm 模块，然后在 ARM 设备或浏览器中运行，无需重新编译。

## Key Features

- **容器→Wasm 转换**：将 OCI 容器镜像（x86_64/aarch64/RISC-V）编译为 Wasm 模块
- **嵌入内核模拟器**：内置 Bochs（x86）或 TinyEMU（RISC-V）模拟器运行完整 Linux
- **多目标平台**：生成的 Wasm 可在浏览器、Wasmtime、WasmEdge、WAMR 中运行
- **外部层分离**：`--assets-to-external-bundle` 将大型层数据分离，减小 Wasm 体积
- **网络代理**：支持配置 HTTP/SOCKS 代理实现 Wasm 容器的网络访问
- **浏览器运行**：生成的 Wasm 可直接在浏览器中通过 JS 加载运行

## Architecture

c2w 的工作原理：将容器镜像的根文件系统、Linux 内核和 Bochs/TinyEMU 模拟器打包为一个 Wasm 模块。运行时，Wasm 运行时加载该模块，模拟器在 Wasm 内部启动 Linux 内核，内核挂载容器文件系统并运行容器的 entrypoint。外部 I/O（网络、文件）通过 Wasm 的 WASI 接口桥接到宿主环境。

## K8s 集成

container2wasm 生成的 Wasm 模块可以在 Kubernetes 中通过 containerd-shim-spin 或 Wasm 运行时运行。也适合在浏览器中运行 Kubernetes 工具（如 kubectl 的 Wasm 版本），或在边缘设备上运行容器化应用而无需完整的容器运行时。

## 生产部署要点

- **选择 RISC-V**：对于体积敏感的场景，使用 `--target-arch=riscv64` 生成更小的 WASM
- **精简镜像**：使用 Alpine 等轻量镜像减少转换后的 WASM 体积
- **外部层**：对大型镜像使用 `--assets-to-external-bundle` 分离层数据
- **浏览器优化**：预加载 WASM 模块并使用 Service Worker 缓存
- **网络隔离**：生产环境谨慎配置网络代理，避免安全风险

## 生产场景

1. **浏览器中运行容器**：在浏览器中运行完整的 Linux 工具（如 curl、python）进行调试
2. **跨平台容器分发**：一份容器镜像在 x86/ARM/RISC-V 设备上一致运行
3. **沙箱化执行**：不信任的容器在 Wasm 沙箱中安全执行
4. **边缘轻量化**：在资源受限设备上运行容器应用，无需 containerd/docker

## 安装

```bash
# 安装 container2wasm
git clone https://github.com/ktock/container2wasm
cd container2wasm
make

# 将容器镜像转换为 Wasm
./c2w --image nginx:latest --target-arch=x86_64 -o nginx.wasm

# 在 wasmtime 中运行
wasmtime nginx.wasm

# 在浏览器中运行（需要配套的 JS loader）
# 使用 container2wasm 提供的浏览器加载页面
./c2w --image python:3.11 --assets-to-external-bundle -o python.wasm
# 然后在 HTML 中加载 python.wasm
```

## 对比

| 特性 | container2wasm | Spin | wasmCloud | Kraken |
|------|---------------|------|-----------|--------|
| 容器兼容 | ✅ 原生容器 | ❌ 需改写 | ❌ 需改写 | ❌ |
| 内核模拟 | ✅ Bochs/TinyEMU | ❌ | ❌ | ❌ |
| 浏览器运行 | ✅ | ⚠️ | ❌ | ❌ |
| 性能 | ⭐⭐ 模拟开销 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | - |

## 参考链接

- [[概念/kubernetes-architecture-overview.md|kubernetes-architecture-overview]]

## Related

- [[kubevirt]] — KubeVirt
- [[wasmcloud]] — wasmCloud
- [[spiderpool]] — Spiderpool
- [[ratify]] — Ratify
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- container2wasm
- [[实体/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
