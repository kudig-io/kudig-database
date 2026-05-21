---
title: container2wasm
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- docker
- wasm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- container2wasm 是什么
- 如何 container2wasm
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- container2wasm
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

title: container2wasm
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- docker
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
- container2wasm 是什么
- 如何 container2wasm
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- container2wasm
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
# container2wasm

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://github.com/aspect-build/container2wasm |
| **GitHub** | https://github.com/aspect-build/container2wasm |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

container2wasm 是一个将 Linux 容器镜像转换为 WebAssembly (WASM) 模块的工具。它通过嵌入 Linux 内核模拟器（基于 Bochs x86 模拟器或 TinyEMU RISC-V 模拟器），使原本为 x86_64/aarch64 编译的容器镜像能够在任何支持 WASM 的环境中运行，包括浏览器、边缘设备和 WASM 运行时（如 Wasmtime、WasmEdge）。

### 核心特性

- **容器转 WASM**: 将标准 OCI 容器镜像转换为可在 WASM 运行时执行的模块
- **跨架构运行**: x86_64/aarch64 容器可在任何 WASM 环境运行
- **浏览器支持**: 在浏览器中直接运行 Linux 容器，无需服务端
- **多模拟器后端**: 支持 Bochs (x86) 和 TinyEMU (RISC-V) 模拟器
- **交互式终端**: 提供 TTY 支持，可在浏览器中进行交互式操作
- **网络支持**: 通过 WebSocket 隧道提供网络连接能力

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                容器镜像转换流程                         │
│                                                       │
│  ┌──────────────┐                                     │
│  │ OCI Container│                                     │
│  │ Image        │                                     │
│  │ (x86_64/arm) │                                     │
│  └──────┬───────┘                                     │
│         │                                             │
│         ▼                                             │
│  ┌──────────────────────────────────────────────┐    │
│  │           container2wasm                       │    │
│  │                                                │    │
│  │  ┌────────────┐  ┌────────────────────────┐  │    │
│  │  │ Image      │  │ Linux Kernel +         │  │    │
│  │  │ Extractor  │  │ Root Filesystem        │  │    │
│  │  └─────┬──────┘  └────────────┬───────────┘  │    │
│  │        │                      │              │    │
│  │        └──────────┬───────────┘              │    │
│  │                   │                           │    │
│  │        ┌──────────▼───────────┐              │    │
│  │        │  Emulator Backend     │              │    │
│  │        │  ┌────────┐ ┌──────┐ │              │    │
│  │        │  │ Bochs  │ │TinyEMU│ │              │    │
│  │        │  │ (x86)  │ │(RISCV)│ │              │    │
│  │        │  └────────┘ └──────┘ │              │    │
│  │        └──────────┬───────────┘              │    │
│  └───────────────────┼──────────────────────────┘    │
│                      │                                │
│                      ▼                                │
│  ┌──────────────────────────────────────────────┐    │
│  │              WASM Module (.wasm)               │    │
│  │  ┌─────────────────────────────────────────┐  │    │
│  │  │ Emulator (WASM) + Linux Kernel + Image  │  │    │
│  │  └─────────────────────────────────────────┘  │    │
│  └──────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────┘

           运行环境
┌─────────────────────────────────────────┐
│  ┌──────────┐ ┌─────────┐ ┌──────────┐ │
│  │ Browser  │ │Wasmtime │ │WasmEdge  │ │
│  │ (WASM)   │ │ Runtime │ │ Runtime  │ │
│  └──────────┘ └─────────┘ └──────────┘ │
└─────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# 使用 Go 安装
go install github.com/aspect-build/container2wasm/cmd/c2w@latest

# 或下载预编译二进制
curl -LO https://github.com/aspect-build/container2wasm/releases/latest/download/c2w-linux-amd64
chmod +x c2w-linux-amd64
sudo mv c2w-linux-amd64 /usr/local/bin/c2w
```

### 转换容器镜像

```bash
# 将 Alpine 容器转换为 WASM
c2w --target-arch=amd64 alpine:latest alpine.wasm

# 转换带自定义命令的容器
c2w --target-arch=amd64 \
  --build-arg=CMD="/bin/sh -c 'echo Hello from WASM'" \
  ubuntu:22.04 ubuntu.wasm

# 使用 RISC-V 模拟器 (更小体积)
c2w --target-arch=riscv64 alpine:latest alpine-riscv.wasm
```

### 在命令行运行

```bash
# 使用 Wasmtime 运行
wasmtime run --dir /::/ alpine.wasm

# 使用 WasmEdge 运行
wasmedge alpine.wasm

# 带网络支持运行
c2w-net --listen=:8080 &
wasmtime run --tcplisten=127.0.0.1:8080 alpine.wasm
```

### 在浏览器中运行

```html
<!DOCTYPE html>
<html>
<head>
  <script src="https://aspect-build.github.io/container2wasm/xterm.js"></script>
  <script src="https://aspect-build.github.io/container2wasm/c2w.js"></script>
</head>
<body>
  <div id="terminal"></div>
  <script>
    const term = new Terminal();
    term.open(document.getElementById('terminal'));
    
    container2wasm.run({
      wasmURL: './alpine.wasm',
      terminal: term,
    });
  </script>
</body>
</html>
```

---

## 高级功能

### 构建自定义镜像

```dockerfile
# Dockerfile
FROM alpine:latest
RUN apk add --no-cache python3 py3-pip
COPY app.py /app/
CMD ["python3", "/app/app.py"]
```

```bash
# 构建并转换
docker build -t my-python-app .
c2w --target-arch=amd64 my-python-app python-app.wasm
```

### 多层镜像优化

```bash
# 压缩层减少 WASM 体积
c2w --target-arch=amd64 \
  --assets-to-external-bundle \
  --external-bundle=alpine-layers.tar \
  alpine:latest alpine-slim.wasm

# 运行时加载外部层
wasmtime run --mapdir /layers::./alpine-layers alpine-slim.wasm
```

### 网络配置

```bash
# 启动网络代理
c2w-net --listen=:8080 --network=bridge

# 容器访问网络
# 在 WASM 容器内: wget http://example.com
```

### 调试模式

```bash
# 启用调试输出
c2w --debug \
  --target-arch=amd64 \
  --kernel-log=kernel.log \
  alpine:latest alpine-debug.wasm
```

---

## 与其他方案对比

| 特性 | container2wasm | Wasm Containers | Krustlet | runwasi |
|:---|:---|:---|:---|:---|
| 输入格式 | OCI 镜像 | WASM 原生 | WASM 原生 | WASM 原生 |
| Linux 兼容 | 完全兼容 | 受限 | 受限 | 受限 |
| 浏览器运行 | 支持 | 部分支持 | 不支持 | 不支持 |
| 性能 | 模拟器开销 | 原生 | 原生 | 原生 |
| 适用场景 | 遗留应用 | WASM 原生应用 | K8s 工作负载 | K8s 工作负载 |
| 镜像体积 | 较大 | 小 | 小 | 小 |

---

## 最佳实践

1. **选择 RISC-V**: 对于体积敏感的场景，使用 `--target-arch=riscv64` 生成更小的 WASM
2. **精简镜像**: 使用 Alpine 等轻量镜像减少转换后的 WASM 体积
3. **外部层**: 对大型镜像使用 `--assets-to-external-bundle` 分离层数据
4. **浏览器优化**: 预加载 WASM 模块并使用 Service Worker 缓存
5. **网络隔离**: 生产环境谨慎配置网络代理，避免安全风险

---

## 参考资源

- [container2wasm GitHub](https://github.com/aspect-build/container2wasm)
- [在线演示](https://aspect-build.github.io/container2wasm/)
- [Bochs x86 模拟器](https://bochs.sourceforge.io/)
- [TinyEMU RISC-V 模拟器](https://bellard.org/tinyemu/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/linux.md|linux]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[domain-17-system-foundation/topic-cheat-sheet/docker.md|docker]]
- [[entities/cncf-runtime|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
