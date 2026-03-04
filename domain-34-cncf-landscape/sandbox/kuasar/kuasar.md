# Kuasar

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **GitHub** | https://github.com/kuasar-io/kuasar |
| **许可证** | Apache-2.0 |
| **开发语言** | Rust |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Kuasar 是一个统一的容器沙箱管理框架，支持在同一个节点上同时运行多种类型的沙箱（MicroVM、App Kernel、Wasm）。它重新设计了 containerd 的 Sandbox API，将沙箱管理逻辑从 shim 中分离出来，使得一个 Sandboxer 进程可以管理同类型的所有沙箱实例，大幅减少常驻进程数量和内存开销。

### 核心特性

- **多沙箱类型**: 同一节点支持 MicroVM (Cloud Hypervisor/QEMU/Firecracker)、App Kernel (gVisor/Quark)、Wasm (WasmEdge/Wasmtime)
- **统一管理**: 通过 containerd Sandbox API 统一管理所有沙箱类型
- **1:N 架构**: 一个 Sandboxer 进程管理 N 个沙箱，替代传统 1:1 shim 模式
- **低开销**: 显著减少常驻进程数和内存占用
- **Rust 实现**: 使用 Rust 编写，保证内存安全和高性能
- **可插拔**: 通过 Sandboxer 插件机制支持扩展新的沙箱类型

---

## 架构设计

```
┌──────────────────────────────────────────────────┐
│                 containerd                         │
│  ┌────────────────────────────────────────────┐   │
│  │          Sandbox API (v2)                   │   │
│  └─────────────────┬──────────────────────────┘   │
└────────────────────┼──────────────────────────────┘
                     │
     ┌───────────────┼───────────────┐
     │               │               │
┌────▼────┐   ┌──────▼─────┐  ┌─────▼──────┐
│ vmm-    │   │ app-kernel- │  │ wasm-      │
│sandboxer│   │ sandboxer   │  │ sandboxer  │
│         │   │             │  │            │
│ ┌─────┐ │   │ ┌─────────┐│  │ ┌────────┐│
│ │VM 1 │ │   │ │gVisor 1 ││  │ │Wasm 1  ││
│ │VM 2 │ │   │ │gVisor 2 ││  │ │Wasm 2  ││
│ │VM N │ │   │ │gVisor N ││  │ │Wasm N  ││
│ └─────┘ │   │ └─────────┘│  │ └────────┘│
│         │   │             │  │            │
│Cloud HV │   │ Quark       │  │ WasmEdge  │
│QEMU     │   │             │  │ Wasmtime  │
│Firecrack│   │             │  │            │
└─────────┘   └─────────────┘  └────────────┘

传统 shim 模式:          Kuasar 模式:
shim 1 → sandbox 1       Sandboxer → sandbox 1
shim 2 → sandbox 2                 → sandbox 2
shim N → sandbox N                 → sandbox N
(N 个进程)               (1 个进程)
```

---

## 快速开始

### 安装

```bash
# 构建 Kuasar
git clone https://github.com/kuasar-io/kuasar.git
cd kuasar
make build

# 安装 Sandboxer
sudo make install
```

### 配置 containerd

```toml
# /etc/containerd/config.toml
[proxy_plugins.vmm]
  type = "sandbox"
  address = "/run/vmm-sandboxer.sock"

[proxy_plugins.quark]
  type = "sandbox"
  address = "/run/quark-sandboxer.sock"

[proxy_plugins.wasm]
  type = "sandbox"
  address = "/run/wasm-sandboxer.sock"
```

### 启动 Sandboxer

```bash
# 启动 MicroVM Sandboxer (Cloud Hypervisor)
vmm-sandboxer --listen /run/vmm-sandboxer.sock \
  --hypervisor cloud-hypervisor

# 启动 Wasm Sandboxer
wasm-sandboxer --listen /run/wasm-sandboxer.sock \
  --runtime wasmedge
```

### Kubernetes RuntimeClass

```yaml
# MicroVM 运行时
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: vmm
handler: vmm

---
# Wasm 运行时
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: wasm
handler: wasm

---
# 使用 MicroVM 运行安全敏感工作负载
apiVersion: v1
kind: Pod
metadata:
  name: secure-app
spec:
  runtimeClassName: vmm
  containers:
    - name: app
      image: nginx:latest

---
# 使用 Wasm 运行轻量级函数
apiVersion: v1
kind: Pod
metadata:
  name: wasm-func
spec:
  runtimeClassName: wasm
  containers:
    - name: func
      image: myorg/wasm-func:latest
```

---

## 性能对比

| 指标 | Kuasar (1:N) | 传统 Shim (1:1) | 改善 |
|:---|:---|:---|:---|
| 100 Pod 常驻进程数 | 1 | 100 | 99% |
| 100 Pod 额外内存 | ~10MB | ~350MB | 97% |
| Pod 启动延迟 | ~200ms | ~300ms | 33% |
| 沙箱创建时间 | ~150ms | ~250ms | 40% |

---

## 与其他方案对比

| 特性 | Kuasar | Kata Containers | gVisor | containerd-shim |
|:---|:---|:---|:---|:---|
| 架构 | 1:N Sandboxer | 1:1 Shim | 1:1 Shim | 1:1 Shim |
| 沙箱类型 | MicroVM/AppKernel/Wasm | MicroVM | App Kernel | 无 (runc) |
| 多类型混合 | 支持 | 单一 | 单一 | 单一 |
| 进程开销 | 低 (1个管理进程) | 高 (每Pod一个) | 高 | 高 |
| 语言 | Rust | Go | Go | Go |

---

## 最佳实践

1. **沙箱类型选择**: 安全敏感用 MicroVM，高密度用 App Kernel，轻量函数用 Wasm
2. **混合部署**: 在同一集群中通过 RuntimeClass 为不同工作负载选择合适的沙箱类型
3. **资源规划**: MicroVM 需要更多内存开销，合理规划节点容量
4. **监控**: 监控 Sandboxer 进程的资源使用和沙箱创建延迟
5. **升级策略**: Sandboxer 管理多个沙箱，升级时需要 drain 节点

---

## 参考资源

- [Kuasar GitHub](https://github.com/kuasar-io/kuasar)
- [Kuasar 设计文档](https://github.com/kuasar-io/kuasar/blob/main/docs/design.md)
- [containerd Sandbox API](https://github.com/containerd/containerd/blob/main/api/services/sandbox/v1/sandbox.proto)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
