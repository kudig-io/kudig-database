# youki

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://containers.github.io/youki/ |
| **GitHub** | https://github.com/containers/youki |
| **许可证** | Apache-2.0 |
| **开发语言** | Rust |
| **CNCF 状态** | Sandbox |

---

## 项目概述

youki 是一个用 Rust 实现的 OCI 容器运行时，作为 runc 的替代品。它完全兼容 OCI Runtime Specification，同时利用 Rust 的内存安全特性减少潜在的安全漏洞。youki 可与 containerd、CRI-O、Podman 等高级容器运行时集成。

### 核心特性

- **OCI 兼容**: 完全实现 OCI Runtime Specification
- **Rust 实现**: 内存安全，无 C/C++ 常见的缓冲区溢出等漏洞
- **runc 替代**: 可直接替换 runc 用于 containerd/CRI-O/Podman
- **cgroup v1/v2**: 支持 cgroup v1 和 v2
- **Rootless 容器**: 支持无 root 权限运行容器
- **seccomp/AppArmor**: 支持 Linux 安全模块
- **Wasm 支持**: 实验性支持 WebAssembly 运行时 (WasmEdge/Wasmtime)

---

## 架构设计

```
┌──────────────────────────────────┐
│  containerd / CRI-O / Podman     │
│  (High-level Runtime)            │
└──────────────┬───────────────────┘
               │ OCI Runtime Spec
               ▼
┌──────────────────────────────────┐
│            youki                  │
│                                   │
│  ┌──────────┐  ┌───────────────┐ │
│  │ Container │  │ Namespace     │ │
│  │ Lifecycle │  │ Management    │ │
│  │ (create,  │  │ (pid, net,    │ │
│  │  start,   │  │  mnt, user,   │ │
│  │  kill,    │  │  uts, ipc)    │ │
│  │  delete)  │  │               │ │
│  └──────────┘  └───────────────┘ │
│  ┌──────────┐  ┌───────────────┐ │
│  │ cgroup    │  │ seccomp /     │ │
│  │ v1 / v2   │  │ AppArmor /    │ │
│  │           │  │ SELinux       │ │
│  └──────────┘  └───────────────┘ │
└──────────────────────────────────┘
               │
               ▼
        Linux Kernel
```

---

## 快速开始

### 安装

```bash
# 从源码编译
git clone https://github.com/containers/youki.git
cd youki
make youki-dev  # 开发版本
# 或
make release    # 生产版本

# 安装
sudo install -m 755 target/release/youki /usr/local/bin/youki

# 验证
youki --version
```

### 配置 containerd 使用 youki

```toml
# /etc/containerd/config.toml
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.youki]
  runtime_type = "io.containerd.runc.v2"
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.youki.options]
    BinaryName = "/usr/local/bin/youki"
```

### 配置 Podman 使用 youki

```toml
# /etc/containers/containers.conf
[engine]
runtime = "youki"

[engine.runtimes]
youki = ["/usr/local/bin/youki"]
```

### OCI 标准操作

```bash
# 创建容器 bundle
mkdir -p bundle/rootfs
cd bundle
youki spec  # 生成 config.json

# 容器生命周期
youki create my-container -b .
youki start my-container
youki state my-container
youki kill my-container SIGTERM
youki delete my-container

# 列出容器
youki list
```

---

## 与 runc 对比

| 特性 | youki (Rust) | runc (Go) |
|:---|:---|:---|
| **内存安全** | 编译时保证 | GC 管理 |
| **启动时间** | 略快 | 标准 |
| **二进制大小** | ~5MB | ~10MB |
| **OCI 兼容** | 完整 | 参考实现 |
| **cgroup v2** | 支持 | 支持 |
| **Wasm** | 实验性 | 不支持 |

---

## 最佳实践

1. **生产评估**: 在非生产环境充分测试后再替换 runc
2. **Rootless 模式**: 优先使用 rootless 模式运行容器
3. **安全增强**: 利用 Rust 的内存安全减少运行时安全风险
4. **Wasm 实验**: 尝试 youki 的 Wasm 运行时特性用于轻量级工作负载
5. **版本锁定**: 在生产环境锁定 youki 版本，避免未测试的更新

---

## 参考资源

- [youki 官方文档](https://containers.github.io/youki/)
- [youki GitHub](https://github.com/containers/youki)
- [OCI Runtime Specification](https://github.com/opencontainers/runtime-spec)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
