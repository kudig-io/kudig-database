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

## 安装与配置

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
./c2w --image python:3.11 --assets-to-external-bundle -o python.wasm
# 然后在 HTML 中加载 python.wasm
```

### 高级转换选项

```bash
# RISC-V 目标（更小体积）
./c2w --image alpine:latest --target-arch=riscv64 -o alpine-rv.wasm

# 分离外部层（减小 Wasm 体积）
./c2w --image ubuntu:22.04 --assets-to-external-bundle -o ubuntu.wasm
# 生成 ubuntu.wasm + ubuntu_assets/ 目录

# 配置网络代理
./c2w --image curl-image --net-proxy http://proxy:8080 -o curl.wasm

# 指定自定义内核配置
./c2w --image myapp --kernel-config ./custom-kernel.config -o myapp.wasm
```

### Kubernetes 集成部署

```yaml
# 通过 containerd-shim-wasmtime 运行 Wasm 容器
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: wasmtime
handler: wasmtime
---
apiVersion: v1
kind: Pod
metadata:
  name: wasm-nginx
spec:
  runtimeClassName: wasmtime
  containers:
    - name: nginx
      image: registry.internal/nginx-wasm:latest
      command: ["nginx.wasm"]
```

## 运维操作

```bash
# 🟢 查看转换后的 Wasm 文件大小
ls -lh *.wasm

# 🟢 在 WasmEdge 中运行
wasmedge nginx.wasm

# 🟢 在 WAMR 中运行
iwasm nginx.wasm

# 🟢 检查 Wasm 模块信息
wasm-tools print nginx.wasm | head -50

# 🟡 批量转换镜像
for img in $(cat images.txt); do
  ./c2w --image $img --target-arch=riscv64 -o $(basename $img).wasm
done

# 🟢 验证 Wasm 模块可执行
wasmtime --invoke _start nginx.wasm
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| 转换失败 OOM | 镜像层过大 | `dmesg \| grep oom` | 使用 --assets-to-external-bundle |
| Wasm 运行崩溃 | 内核模拟器不兼容 | `wasmtime nginx.wasm 2>&1` | 尝试不同 target-arch |
| 网络不通 | 代理未配置 | 检查 WASI 网络接口 | 配置 --net-proxy |
| 启动极慢 | 镜像未精简 | 检查 Wasm 文件大小 | 使用 Alpine 基础镜像 |
| K8s Pod 失败 | RuntimeClass 未配置 | `kubectl get runtimeclass` | 部署 containerd-shim-wasmtime |

### 排查流程

```
container2wasm 异常
├─ 转换失败？
│  ├─ 内存不足 → 使用外部层分离
│  ├─ 不支持的架构 → 检查 --target-arch 参数
│  └─ 镜像拉取失败 → 检查网络和认证
├─ Wasm 运行异常？
│  ├─ 启动失败 → 检查 Wasm 运行时版本
│  ├─ 内核 panic → 尝试不同目标架构
│  └─ I/O 错误 → 检查 WASI 接口配置
└─ K8s 集成失败？
   ├─ RuntimeClass 不存在 → 部署 shim
   └─ containerd 配置错误 → 检查 /etc/containerd/config.toml
```

## 生产案例

### 案例 1: 浏览器内运行调试工具

**场景**: 开发者需要在浏览器中运行完整的 Linux 工具链（curl、python、tcpdump）进行远程调试。

**方案**:
1. 将调试工具容器转换为 Wasm
2. 通过 Web 页面加载运行
3. 使用 Service Worker 缓存 Wasm 模块

**效果**: 无需安装任何本地工具，打开浏览器即可使用完整 Linux 调试环境。

### 案例 2: IoT 边缘设备轻量化容器

**场景**: 资源受限的 ARM IoT 设备（512MB RAM）需运行容器化应用，无法运行 containerd。

**方案**:
1. 将应用容器转换为 RISC-V Wasm 模块
2. 使用 WAMR（WebAssembly Micro Runtime）运行
3. 无需完整容器运行时，仅需 4MB WAMR 运行时

**效果**: 内存占用从 200MB（containerd+容器）降至 30MB，启动时间从 5s 降至 500ms。

## 对比与替代方案

| 维度 | container2wasm | Spin | wasmCloud | 原生容器 |
|------|---------------|------|-----------|----------|
| 容器兼容 | ✅ 无需修改 | ❌ 需改写 | ❌ 需改写 | ✅ |
| 性能 | ⭐⭐ 模拟开销 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| 浏览器运行 | ✅ | ⚠️ | ❌ | ❌ |
| 体积 | 大（含内核） | 小 | 小 | 中 |
| 安全沙箱 | ✅ Wasm 隔离 | ✅ | ✅ | 部分 |
| 成熟度 | 实验性 | 生产就绪 | 生产就绪 | 成熟 |

## 检查清单

- [ ] Wasm 运行时版本支持 WASI Preview 2
- [ ] 转换后的 Wasm 体积可接受（< 50MB）
- [ ] 网络代理已配置（如需网络访问）
- [ ] K8s RuntimeClass 已配置（如需集群运行）
- [ ] 生产环境已评估性能开销（模拟层 5-10x 降速）
- [ ] 安全沙箱边界已验证

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
