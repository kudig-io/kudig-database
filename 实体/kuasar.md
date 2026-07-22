---
title: Kuasar (entities)
description: '## 概述'
summary: 'Kuasar 是一个统一的容器沙箱管理框架，支持在同一个节点上同时运行多种类型的沙箱（MicroVM、App Kernel、Wasm）。它重新设计了 containerd 的 Sandbox API，将沙箱管理逻辑从 shim 中分离出来，使得一个 Sandboxer 进程可以管理同类型的所有沙箱实例，大幅减少常驻进程数量和内存开销。'
category: entities
tags:
- k8s
- cncf
- runtime
- kuasar
- containerd
- crd
- operator
- wasm
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kuasar 是什么
- 如何 Kuasar
trigger_keywords:
- Kuasar
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Kuasar

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Rust

## 概述

Kuasar 是一个 CNCF 沙箱项目，由华为开源，是一个高性能的容器沙箱运行时。它利用 Linux 内核的多种沙箱技术（VM、MicroVM、WASM、Kontain）为容器提供高效的隔离方案。Kuasar 作为 containerd 的沙箱 API 实现，支持多种沙箱后端，同时保持极低的资源开销和启动延迟。与 Kata Containers 类似但架构更轻量，特别适合 Serverless 和多租户场景。

## Key Features（核心能力）

- **多沙箱后端**：支持 MicroVM（Cloud Hypervisor/StratoVirt）、WASM、Kontain 等隔离方案
- **低开销**：沙箱管理器共享，不每个 Pod 启动独立 shim 进程
- **Sandbox API 原生**：基于 containerd 2.0 Sandbox API 设计
- **快速启动**：MicroVM 启动时间在 100ms 以内
- **Rust 实现**：内存安全和高性能的 Rust 核心引擎
- **多种容器格式**：支持 OCI 容器和 WASM 模块

## 架构与工作原理

Kuasar 架构基于 containerd Sandboxed API。每个沙箱由一个 Kuasar-shim 进程管理（而非每个 Pod 一个 shim），大幅减少资源开销。shim 通过 VMM（Virtual Machine Monitor）接口管理底层沙箱实例——MicroVM 后端使用 Cloud Hypervisor 或 StratoVirt，WASM 后端使用 WasmEdge，Kontain 后端使用 Kontain Runtime。所有沙箱共享 containerd 的镜像管理。

## K8s 集成

Kuasar 通过 RuntimeClass 与 Kubernetes 集成。在 K8s 节点上安装 Kuasar 并配置 containerd 使用 Kuasar 作为沙箱运行时。创建 RuntimeClass（如 kuasar-vmm）指定 handler 为 kuasar。Pod 通过 runtimeClassName: kuasar-vmm 选择使用 Kuasar MicroVM 沙箱。与 K8s Device Plugin 集成支持设备直通。

## 生产用例

- **Serverless 平台**：为 FaaS 提供快速启动和强隔离的沙箱环境
- **多租户安全**：利用 MicroVM 提供接近硬件级的隔离
- **遗留应用容器化**：需要完整 OS 内核的遗留应用安全运行
- **合规环境**：满足金融/医疗等对隔离性的严格要求

## 安装与配置

```bash
# 🟢 安装 Kuasar（从源码编译）
git clone https://github.com/kuasar-io/kuasar.git
cd kuasar
cargo build --release

# 🟢 安装预编译二进制
curl -L https://github.com/kuasar-io/kuasar/releases/latest/download/kuasar-linux-amd64.tar.gz | tar xz
mv kuasar-* /usr/local/bin/

# 🟢 配置 containerd
# /etc/containerd/config.toml:
# [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kuasar-vmm]
#   runtime_type = "io.containerd.kuasar.v1"
#   [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kuasar-vmm.options]
#     Sandboxer = "vmm"
#
# [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kuasar-wasm]
#   runtime_type = "io.containerd.kuasar.v1"
#   [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kuasar-wasm.options]
#     Sandboxer = "wasm"

# 🟢 重启 containerd
systemctl restart containerd

# 🟢 创建 RuntimeClass
kubectl apply -f runtimeclass.yaml

# 🟢 验证安装
crictl info | grep kuasar
```

### RuntimeClass 和 Pod 配置

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kuasar-vmm
handler: kuasar-vmm
scheduling:
  nodeSelector:
    kuasar.io/vmm: enabled
---
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kuasar-wasm
handler: kuasar-wasm
scheduling:
  nodeSelector:
    kuasar.io/wasm: enabled
---
# 使用 MicroVM 沙箱的 Pod
apiVersion: v1
kind: Pod
metadata:
  name: secure-workload
spec:
  runtimeClassName: kuasar-vmm
  containers:
    - name: app
      image: myorg/secure-app:v1
      resources:
        requests:
          cpu: 100m
          memory: 128Mi
        limits:
          cpu: "1"
          memory: 512Mi
---
# 使用 WASM 沙箱的 Pod
apiVersion: v1
kind: Pod
metadata:
  name: wasm-workload
spec:
  runtimeClassName: kuasar-wasm
  containers:
    - name: wasm-app
      image: myorg/wasm-module:v1
```

## 运维操作

```bash
# 🟢 查看 Kuasar Sandboxer 状态
systemctl status kuasar-vmm
systemctl status kuasar-wasm

# 🟢 查看活跃沙箱
crictl pods --runtime kuasar-vmm
crictl ps --runtime kuasar-vmm

# 🟢 查看沙箱资源使用
ps aux | grep cloud-hypervisor
ps aux | grep kuasar

# 🟡 重启 Sandboxer
systemctl restart kuasar-vmm

# 🔴 停止所有沙箱（会影响运行中的 Pod）
systemctl stop kuasar-vmm
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| Pod 创建失败 | Sandboxer 未运行 | `systemctl status kuasar-*` | 重启 Sandboxer |
| MicroVM 启动慢 | 内核镜像缺失 | 查看 Sandboxer 日志 | 检查 kernel/rootfs 路径 |
| WASM 模块加载失败 | WasmEdge 未安装 | `which wasmedge` | 安装 WasmEdge |
| 容器内网络不通 | VM 网络配置错误 | 检查 veth/tap 设备 | 修复 CNI 配置 |

```bash
# 排查流程
# 1. 检查 Sandboxer 状态
systemctl status kuasar-vmm
journalctl -u kuasar-vmm --since "5 min ago"

# 2. 检查 containerd 日志
journalctl -u containerd --since "5 min ago" | grep kuasar

# 3. 检查 VM 进程
ps aux | grep -E "cloud-hypervisor|stratovirt"

# 4. 检查 RuntimeClass
kubectl get runtimeclass
kubectl describe runtimeclass kuasar-vmm
```

## 生产案例

### 案例1：Serverless 平台强隔离
- **场景**：FaaS 平台需要为每个函数提供强隔离，启动时间 < 200ms
- **方案**：Kuasar MicroVM 后端（Cloud Hypervisor）；共享 Sandboxer 减少开销；预热 VM 池实现快速启动
- **效果**：启动时间 80ms，内存开销比 Kata 减少 50%，安全隔离达到 VM 级别

### 案例2：多租户容器平台
- **场景**：SaaS 平台多租户共享集群，需要防止租户间逃逸
- **方案**：每个租户 Pod 使用 kuasar-vmm RuntimeClass；MicroVM 提供硬件级隔离；资源限制在 VM 级别执行
- **效果**：通过安全审计，租户隔离达到金融级要求，资源开销仅增加 5%

## 对比替代方案

| 维度 | Kuasar | Kata Containers | gVisor | 普通容器 |
|------|--------|----------------|--------|----------|
| 隔离级别 | VM/WASM | VM | 用户态内核 | Namespace |
| 启动时间 | ~80ms | ~200ms | ~150ms | ~50ms |
| 内存开销 | 极低(共享shim) | 高(每Pod shim) | 中 | 极低 |
| 兼容性 | 强(VM) | 强 | 中 | 完全 |
| 多后端 | 支持 | 仅 VM | 仅用户态 | - |

## 检查清单

- [ ] Kuasar 二进制已安装且版本正确
- [ ] containerd 配置已更新并重启
- [ ] RuntimeClass 已创建
- [ ] 节点已标记对应标签
- [ ] VMM 内核和 rootfs 已配置
- [ ] 已在测试 Pod 验证沙箱运行
- [ ] 监控已配置（沙箱数量/资源使用）

## Related

- [[cozystack]] — Cozystack
- [[fluid]] — Fluid
- storage.md|cncf-storage]] — CNCF 存储与数据库项目全景
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[containerd]] — containerd

- kuasar
- [[实体/urunc.md|[[urunc (Unikernel Container Runtime)|urunc]]]]
- [[实体/hyperlight.md|Hyperlight]]
- [[实体/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference


<!-- risk-assessed -->
