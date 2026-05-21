---
title: Hyperlight
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- serverless
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Hyperlight 是什么
- 如何 Hyperlight
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Hyperlight
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

title: Hyperlight
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- serverless
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Hyperlight 是什么
- 如何 Hyperlight
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Hyperlight
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
# Hyperlight

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://hyperlight.dev/ |
| **GitHub** | https://github.com/hyperlight-dev/hyperlight |
| **许可证** | Apache-2.0 |
| **开发语言** | Rust |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Hyperlight 是一个轻量级虚拟机管理器 (VMM)，专为在毫秒级启动时间内运行函数式工作负载而设计。它创建超轻量的 micro-VM，每个 VM 可在 1-2 毫秒内启动，内存开销仅为几 MB。Hyperlight 特别适合 Serverless 和 FaaS 场景，提供比容器更强的隔离性，同时保持接近容器的启动速度和资源效率。

### 核心特性

- **毫秒级启动**: micro-VM 启动时间在 1-2 毫秒，接近进程启动速度
- **极低开销**: 每个 VM 仅需几 MB 内存，支持高密度部署
- **强隔离**: 基于硬件虚拟化（KVM/Hyper-V）提供完整的 VM 级隔离
- **嵌入式库**: 可作为 Rust 库嵌入到应用程序中，提供即时的 VM 创建能力
- **多 Guest 支持**: 支持 Rust 和 C 编写的 Guest 程序
- **跨平台**: 支持 Linux (KVM) 和 Windows (Hyper-V)

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                   Host Application                    │
│                                                       │
│  ┌─────────────────────────────────────────────────┐ │
│  │              Hyperlight Library (Rust)           │ │
│  │                                                   │ │
│  │  ┌───────────────┐  ┌───────────────────────┐   │ │
│  │  │ Sandbox       │  │ Guest Binary Loader    │   │ │
│  │  │ Manager       │  │ (ELF/PE 加载)          │   │ │
│  │  └───────┬───────┘  └───────────┬───────────┘   │ │
│  │          │                      │                │ │
│  │  ┌───────▼──────────────────────▼───────────┐   │ │
│  │  │           Hypervisor Abstraction          │   │ │
│  │  │  ┌─────────────┐  ┌─────────────────┐    │   │ │
│  │  │  │ KVM Backend │  │ Hyper-V Backend │    │   │ │
│  │  │  │ (Linux)     │  │ (Windows)       │    │   │ │
│  │  │  └─────────────┘  └─────────────────┘    │   │ │
│  │  └──────────────────────────────────────────┘   │ │
│  └─────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────┘
                          │
           ┌──────────────┼──────────────┐
           │              │              │
    ┌──────▼─────┐ ┌──────▼─────┐ ┌─────▼──────┐
    │ Micro-VM 1  │ │ Micro-VM 2  │ │ Micro-VM 3  │
    │ ┌─────────┐ │ │ ┌─────────┐ │ │ ┌─────────┐ │
    │ │Rust     │ │ │ │C Guest  │ │ │ │Rust     │ │
    │ │Guest    │ │ │ │Code     │ │ │ │Guest    │ │
    │ │Function │ │ │ │         │ │ │ │Function │ │
    │ └─────────┘ │ │ └─────────┘ │ │ └─────────┘ │
    │   ~2MB RAM  │ │   ~2MB RAM  │ │   ~2MB RAM  │
    │   ~1ms boot │ │   ~1ms boot │ │   ~1ms boot │
    └─────────────┘ └─────────────┘ └─────────────┘
```

---

## 快速开始

### 安装依赖

```bash
# Linux: 启用 KVM
sudo modprobe kvm
sudo modprobe kvm_intel  # 或 kvm_amd

# 安装 Rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

### 创建 Guest 程序

```rust
// guest/src/main.rs
#![no_std]
#![no_main]

use hyperlight_guest::*;

#[no_mangle]
pub extern "C" fn guest_main() {
    // Guest 函数逻辑
    let input = get_input();
    let result = process(input);
    set_output(result);
}

fn process(input: &[u8]) -> Vec<u8> {
    // 你的业务逻辑
    input.to_vec()
}
```

```bash
# 构建 Guest
cargo build --target x86_64-unknown-none --release
```

### Host 调用 Guest

```rust
// host/src/main.rs
use hyperlight_host::{Sandbox, SandboxConfiguration};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // 配置 Sandbox
    let config = SandboxConfiguration::default()
        .with_memory_size(4 * 1024 * 1024)  // 4 MB
        .with_guest_binary("path/to/guest.bin");
    
    // 创建 Sandbox (micro-VM)
    let mut sandbox = Sandbox::new(config)?;
    
    // 调用 Guest 函数
    let input = b"Hello, Hyperlight!";
    let output = sandbox.call_guest_function("process", input)?;
    
    println!("Result: {:?}", output);
    Ok(())
}
```

### 运行

```bash
cargo run --release
```

---

## 高级功能

### 函数注册和调用

```rust
// Host 注册可供 Guest 调用的 Host 函数
use hyperlight_host::{Sandbox, HostFunction};

fn host_log(message: &str) {
    println!("[HOST LOG] {}", message);
}

let mut sandbox = Sandbox::new(config)?;
sandbox.register_host_function("log", host_log);

// Guest 调用 Host 函数
#[no_mangle]
pub extern "C" fn guest_main() {
    call_host_function("log", "Hello from guest!");
}
```

### 多 Sandbox 池

```rust
use hyperlight_host::SandboxPool;

// 创建 Sandbox 池以复用 VM
let pool = SandboxPool::new(10);  // 预创建 10 个 VM

// 从池中获取 Sandbox
let sandbox = pool.acquire()?;
let result = sandbox.call_guest_function("handler", input)?;
pool.release(sandbox);
```

### 内存配置

```rust
let config = SandboxConfiguration::default()
    .with_memory_size(8 * 1024 * 1024)  // 8 MB
    .with_stack_size(64 * 1024)          // 64 KB 栈
    .with_heap_size(1024 * 1024);        // 1 MB 堆
```

### 超时控制

```rust
use std::time::Duration;

let result = sandbox.call_guest_function_with_timeout(
    "handler",
    input,
    Duration::from_millis(100),  // 100ms 超时
)?;
```

---

## 与其他方案对比

| 特性 | Hyperlight | Firecracker | gVisor | Kata Containers |
|:---|:---|:---|:---|:---|
| 启动时间 | ~1-2ms | ~125ms | ~150ms | ~500ms |
| 内存开销 | ~2-4MB | ~5MB | ~100MB | ~100MB |
| 隔离级别 | 硬件 VM | 硬件 VM | 用户态内核 | 硬件 VM |
| 使用方式 | 嵌入式库 | 独立 VMM | OCI Runtime | OCI Runtime |
| Guest OS | 无 (裸金属) | Linux | Linux | Linux |
| 适用场景 | FaaS/函数 | 容器/FaaS | 容器 | 容器 |

---

## 最佳实践

1. **Sandbox 池**: 对于高并发场景，使用 SandboxPool 复用 VM 实例减少创建开销
2. **最小内存**: 根据 Guest 实际需求配置最小内存，提高部署密度
3. **超时保护**: 为所有 Guest 调用设置超时，防止恶意或异常 Guest 阻塞
4. **无状态 Guest**: 设计无状态的 Guest 函数，便于 Sandbox 复用
5. **Host 函数最小化**: 减少 Host 函数暴露面，降低安全风险

---

## 参考资源

- [Hyperlight 官方文档](https://hyperlight.dev/docs/)
- [Hyperlight GitHub](https://github.com/hyperlight-dev/hyperlight)
- [Hyperlight 设计文档](https://hyperlight.dev/design/)
- [Azure Hyperlight 博客](https://azure.microsoft.com/en-us/blog/hyperlight/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[log.md|log]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/linux.md|linux]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/cncf-runtime|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
