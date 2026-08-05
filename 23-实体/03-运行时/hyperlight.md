---
title: Hyperlight (entities)
description: '## 概述'
summary: 'Hyperlight 是一个轻量级虚拟机管理器 (VMM)，专为在毫秒级启动时间内运行函数式工作负载而设计。它创建超轻量的 micro-VM，每个 VM 可在 1-2 毫秒内启动，内存开销仅为几 MB。Hyperlight 特别适合 Serverless 和 FaaS 场景，提供比容器更强的隔离性，同时保持接近容器的启动速度和资源效率。'
category: entities
tags:
- k8s
- cncf
- runtime
- hyperlight
- argocd
- containerd
- falco
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
- Hyperlight 是什么
- 如何 Hyperlight
trigger_keywords:
- Hyperlight
prerequisites:
- kubectl-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Hyperlight

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Rust

## 概述

Hyperlight 是由 Microsoft 开发的轻量级虚拟机管理器（VMM），2024 年加入 CNCF Sandbox。它专为在毫秒级启动时间内运行函数式工作负载（Function Workloads）而设计。Hyperlight 创建超轻量的 micro-VM，每个 VM 可在 1-2 毫秒内启动，内存开销仅为 2-5 MB。Hyperlight 特别适合 Serverless、FaaS 和 AI Agent 安全沙箱场景，提供比容器更强的硬件级隔离，同时保持接近容器的启动速度和资源效率。

## 核心特性

- **极速启动**: 1-2ms VM 启动时间，接近进程创建速度
- **超低内存**: 每个 micro-VM 仅需 2-5MB 内存开销
- **硬件隔离**: 基于 Hypervisor（Microsoft Hypervisor / KVM）的硬件级沙箱
- **Host-Guest 通信**: 高效的 Host 函数调用和 Guest 回调机制
- **SandboxPool**: VM 实例池复用，减少创建开销
- **多语言 Guest**: 支持 Rust、Go、C、Python 编写的 Guest 代码

## 架构

Hyperlight 架构由 Host 和 Guest 两部分组成。Host 进程通过 Hyperlight SDK 创建 micro-VM——分配内存、加载 Guest 二进制文件到 Guest 内存空间、初始化 CPU 上下文。Guest 运行在硬件隔离的 VM 中，通过 Hypercall 与 Host 通信。Host 可以向 Guest 传递参数并调用 Guest 函数，Guest 也可以通过 Host Function 回调请求 Host 执行操作（如网络请求）。Guest 使用专用的内存布局和引导加载器，无需完整操作系统。支持 Microsoft Hypervisor（Windows/Azure）和 KVM（Linux）后端。

## Kubernetes 集成

Hyperlight 可作为 Kubernetes 中 AI Agent 和 Serverless 函数的安全沙箱运行时。通过自定义 RuntimeClass 或 Sidecar 模式集成。在 AI Agent 场景中，不可信的 Agent 代码运行在 Hyperlight micro-VM 中，通过 Host Function 受控地访问集群资源。与 containerd 的 shim 集成可实现将 Hyperlight VM 作为 Pod 的容器运行时替代。

## 生产使用场景

1. **AI Agent 沙箱**: 在隔离的 micro-VM 中运行不可信的 AI Agent 代码
2. **Serverless 函数**: 毫秒级启动的函数运行环境
3. **多租户隔离**: 在共享集群中为每个租户提供硬件级隔离
4. **安全计算**: 运行不可信代码（如用户提交的脚本）的沙箱

## 安装与配置

```bash
# Rust SDK
cargo add hyperlight-host hyperlight-guest
# 示例: 创建 Sandbox 运行 Guest 函数
use hyperlight_host::sandbox::Sandbox;
let mut sandbox = Sandbox::new()?;
let result = sandbox.call_guest_function("add", &[1, 2])?;
# Kubernetes 集成
kubectl apply -f https://github.com/hyperlight-dev/hyperlight/deploy/kubernetes.yaml
```

### Guest 函数示例

```rust
// guest/src/main.rs
use hyperlight_guest::guest_function;

#[guest_function]
fn process_data(input: &[u8]) -> Vec<u8> {
    // 在隔离的 micro-VM 中安全处理数据
    input.iter().map(|b| b.wrapping_add(1)).collect()
}

fn main() {
    hyperlight_guest::entrypoint!(process_data);
}
```

### Host 调用示例

```rust
// host/src/main.rs
use hyperlight_host::sandbox::{Sandbox, SandboxPool};

fn main() -> anyhow::Result<()> {
    // 创建 Sandbox Pool 复用 VM
    let pool = SandboxPool::new(10, "guest.wasm")?;
    
    // 从池中获取 Sandbox
    let mut sandbox = pool.acquire()?;
    
    // 调用 Guest 函数
    let result = sandbox.call_guest_function("process_data", &[1, 2, 3])?;
    println!("Result: {:?}", result);
    
    // 归还到池中
    pool.release(sandbox)?;
    Ok(())
}
```

## 运维操作

```bash
# 🟢 查看 Hyperlight 运行时状态
kubectl get pods -l app=hyperlight-runtime

# 🟢 查看 micro-VM 池状态
kubectl exec deploy/hyperlight-runtime -- curl -s localhost:8080/pool/status

# 🟢 检查 Hypervisor 支持
kubectl exec deploy/hyperlight-runtime -- dmesg | grep -i "hypervisor\|kvm"

# 🟡 调整 Sandbox Pool 大小
kubectl patch deploy/hyperlight-runtime -p '{"spec":{"replicas":5}}'

# 🟢 查看 Guest 执行日志
kubectl logs deploy/hyperlight-runtime | grep -i "guest\|sandbox"
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| VM 创建失败 | Hypervisor 不可用 | `dmesg \| grep kvm` | 启用 KVM/Hyper-V |
| Guest 崩溃 | 内存不足 | 检查 Guest 日志 | 增加 VM 内存分配 |
| 启动慢 | 池未预热 | 检查 Pool 状态 | 增大 Pool 大小 |
| Host 调用失败 | 函数签名不匹配 | 检查 Guest 导出函数 | 确认函数名和参数类型 |
| 性能下降 | VM 复用率低 | 监控 Pool 命中率 | 调整 Pool 配置 |

### 排查流程

```
Hyperlight 异常
├─ VM 无法创建？
│  ├─ Hypervisor 不可用 → 检查 KVM/Hyper-V
│  ├─ 权限不足 → 检查 /dev/kvm 权限
│  └─ 资源不足 → 检查节点内存
├─ Guest 执行失败？
│  ├─ 函数未找到 → 检查 Guest 导出
│  ├─ 参数错误 → 检查序列化格式
│  └─ 内存溢出 → 增加 VM 内存
└─ 性能问题？
   ├─ 启动慢 → 使用 SandboxPool
   └─ 调用延迟 → 检查 Host-Guest 通信
```

## 生产案例

### 案例 1: AI Agent 安全沙箱

**场景**: AI Agent 需执行用户提供的代码，但不能影响主机安全。

**方案**:
1. 用户代码编译为 Guest 二进制
2. 在 Hyperlight micro-VM 中执行
3. 通过 Host Function 受控访问资源
4. 执行完成后销毁 VM

**效果**: 硬件级隔离，启动 < 2ms，无容器逃逸风险。

### 案例 2: Serverless 函数平台

**场景**: FaaS 平台需毫秒级冷启动和强隔离。

**方案**:
1. SandboxPool 预热 VM 实例
2. 函数调用时从池中获取 VM
3. 执行完成后归还复用

**效果**: 冷启动 < 5ms，内存开销 < 5MB/VM，支持高并发。

## 对比与替代方案

| 维度 | Hyperlight | Firecracker | gVisor | Kata Containers |
|------|------------|-------------|--------|------------------|
| 启动时间 | 1-2ms | ~125ms | ~100ms | ~500ms |
| 内存开销 | 2-5MB | ~5MB | ~50MB | ~100MB |
| 隔离级别 | 硬件 VM | 硬件 VM | 用户态内核 | 硬件 VM |
| 兼容性 | 专用 Guest | 完整 Linux | 部分系统调用 | 完整 Linux |
| 成熟度 | 新兴 | 生产验证 | 生产验证 | 生产验证 |

## 检查清单

- [ ] 节点支持 Hypervisor（KVM/Hyper-V）
- [ ] /dev/kvm 权限已配置
- [ ] SandboxPool 大小已优化
- [ ] Guest 二进制已编译并测试
- [ ] Host Function 回调已实现
- [ ] 监控告警：VM 创建失败/执行超时
- [ ] 资源限制已配置（CPU/内存）
- [ ] 安全策略已定义（允许的系统调用）

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Hyperlight** | 极速启动、极低内存 | 较新、社区小 |
| Firecracker | AWS 生产验证、成熟 | 启动约 125ms、内存约 5MB |
| gVisor | 用户态内核、兼容性好 | 性能开销较大 |
| Kata Containers | 标准化、安全 | 启动较慢、资源开销大 |

## 架构定位

在 CNCF 生态中，Hyperlight 属于 **Runtime / Sandbox** 类别，代表了 micro-VM 在 AI Agent 和 Serverless 场景中的应用方向。它在隔离性与性能之间找到了新的平衡点。

## 参考链接

- [[23-实体/argocd.md|[[argocd|argocd]]]]

## Related

- [[falco]] — Falco
- [[operator-framework]] — Operator Framework
- [[clusternet]] — Clusternet
- [[kubeslice]] — KubeSlice
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- hyperlight
- [[23-实体/urunc.md|[[23-实体/03-运行时/urunc|urunc]]]]
- [[23-实体/03-运行时/flatcar.md|Flatcar Container Linux]]
- [[23-实体/03-运行时/composefs.md|composefs]]
- [[23-实体/03-运行时/03-containerd-upgrade-migration.md|containerd 升级迁移]]
- [[23-实体/03-运行时/wasmedge.md|WasmEdge]]
- [[23-实体/03-运行时/spinkube.md|SpinKube]]
- [[23-实体/03-运行时/04-containerd-windows-support.md|containerd Windows 支持]]
- [[23-实体/03-运行时/01-containerd-v2-features.md|containerd 2.0 新特性]]
- [[23-实体/03-运行时/07-containerd-multi-tenant.md|containerd 多租户]]
- [[23-实体/09-编排调度/k0s.md|K0s]]
- [[23-实体/03-运行时/02-containerd-security-hardening.md|containerd 安全加固]]
- [[23-实体/03-运行时/bootc.md|bootc]]
- [[23-实体/03-运行时/container2wasm.md|container2wasm]]
- [[23-实体/09-编排调度/kubean.md|Kubean]]
- [[23-实体/15-参考与索引/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
