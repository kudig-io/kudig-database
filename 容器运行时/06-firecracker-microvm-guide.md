---
title: Firecracker microVM 指南
description: Firecracker microVM 用于容器强隔离，含 firecracker-containerd 部署、VM 模板与 Serverless 场景
summary: Firecracker microVM 用于容器强隔离，含 firecracker-containerd 部署、VM 模板与 Serverless 场景
category: container-runtime
tags:
- containerd
- cri
- runtime
- firecracker
- microvm
- isolation
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
audience:
- SRE
- 平台工程师
---

> **生产环境安全提示**
>
> 风险标注：🔴 高风险 / 🟡 中风险 / 🟢 只读。

# Firecracker microVM 指南

## 概述

Firecracker 是 AWS 开源的极简虚拟机监视器（VMM），基于 KVM，专为安全多租与 Serverless 设计（驱动 Lambda/Fargate）。每个容器跑在独立 microVM 里，拥有独立内核，启动时间 < 125ms，内存开销 ~5MB。`firecracker-containerd` 把它接入 containerd/CRI，让 K8s 工作负载也能获得 VM 级隔离，却保持容器级轻量。

## 隔离层级对比

| 方案 | 内核 | 启动时间 | 内存开销 | 隔离强度 |
|---|---|---|---|---|
| runc | 共享宿主 | ~10ms | ~0 | 弱 |
| gVisor | 用户态内核 | ~50ms | 低 | 中 |
| **Firecracker** | 独立 VM 内核 | **<125ms** | ~5MB | 强 |
| Kata (qemu) | 独立 VM | ~1s | ~50MB | 强 |

Firecracker 在"VM 级隔离 + 近容器启动速度"上独树一帜，适合高密度 Serverless。

## 前置要求

- 裸金属或支持嵌套虚拟化的实例（ACK/EC2 bare-metal，普通 VM 通常禁用 KVM）
- Linux kernel ≥ 4.14，KVM 模块可用
- `/dev/kvm` 可访问

``` bash
# 🟢 只读：验证 KVM 可用
ls -l /dev/kvm
test -w /dev/kvm && echo OK || echo "需要支持嵌套虚拟化的实例"
```

## firecracker-containerd 部署

``` bash
# 🟡 中风险：安装 VMM 与 runtime 二进制
# 1. 安装 firecracker
curl -sL https://github.com/firecracker-microvm/firecracker/releases/download/v1.7.0/firecracker-v1.7.0-x86_64.tgz \
  | sudo tar xz -C /usr/local/bin --strip-components=2 release-v1.7.0-x86_64/firecracker
# 2. 安装 firecracker-containerd runtime
sudo tar xz firecracker-containerd.tgz -C /usr/local/bin \
  firecracker-containerd runtime vmmond
```

## containerd 接入

```toml
# /etc/containerd/config.toml
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.firecracker]
  runtime_type = "io.containerd.firecracker.v2"
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.firecracker.options]
    # 内核与 rootfs 镜像
    KernelImagePath = "/var/lib/firecracker-containerd/runtime/vmlinux.bin"
    RootDrive = "/var/lib/firecracker-containerd/runtime/rootfs.ext4"
    KernelArgs = "console=ttyS0 reboot=k panic=1 pci=off"
    VMInfoDir = "/var/lib/firecracker-containerd/runtime"
    # CPU/内存默认配额
    CPUCount = 2
```

> ⚠️ **🟠 高危操作**

``` bash
# 🔴 高风险：重启 containerd
sudo systemctl restart containerd
crictl info | jq '.config.containerd.runtimes | keys'
```

## RuntimeClass 与 Pod

``` yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: firecracker
handler: firecracker
scheduling:
  nodeSelector:
    sandbox-runtime: firecracker
---
apiVersion: v1
kind: Pod
metadata: { name: isolated-fn }
spec:
  runtimeClassName: firecracker
  containers:
  - name: fn
    image: registry.cn-hangzhou.aliyuncs.com/demo/function:v1
```

## VM 模板与快照（启动加速）

Firecracker 支持 **VM 模板**（预创建进程骨架）与 **快照恢复**（从内存快照启动），把冷启动从 ~125ms 压到 ~10ms，是高并发 Serverless 的关键。

```toml
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.firecracker.options]
  # 启用模板
  VMTemplatePath = "/var/lib/firecracker-containerd/templates/default"
```

## 适用场景

| 场景 | 适配度 | 说明 |
|---|---|---|
| 函数计算 / FaaS | ⭐⭐⭐⭐⭐ | 原生设计目标 |
| SaaS 强多租隔离 | ⭐⭐⭐⭐ | 租户独立内核 |
| 不受信任镜像沙箱 | ⭐⭐⭐⭐ | VM 边界阻断逃逸 |
| 普通微服务 | ⭐⭐ | 启动/内存开销高于 runc，不划算 |
| 需要 eBPF/内核模块 | ⭐⭐ | VM 内内核受限 |

## 与 gVisor / Kata 取舍

- **要最强隔离 + 快启动 + 高密度** → Firecracker（需 KVM）
- **无 KVM（普通 ECS）+ 中等隔离** → gVisor
- **已有 QEMU 工具链 + 需完整 VM 能力** → Kata

## 典型故障

| 现象 | 根因 | 处理 |
|---|---|---|
| `/dev/kvm not found` | 实例无嵌套虚拟化 | 换 bare-metal / 支持嵌套的实例 |
| `start VM timeout` | 内核/rootfs 路径错 | 校验 `KernelImagePath` / `RootDrive` |
| Pod 启动慢 | 未用模板/快照 | 启用 VMTemplate |
| 密度上不去 | 每 VM 固定内存 | 调小 `CPUCount`/内存，用 oversubscribe |

## 生产检查清单

- [ ] 节点 `/dev/kvm` 可写，实例支持嵌套虚拟化
- [ ] firecracker-containerd runtime 已注册并通过 `crictl info` 验证
- [ ] 内核/rootfs 镜像置于内网，版本固定
- [ ] Serverless 高并发启用 VM 模板/快照恢复
- [ ] RuntimeClass 用 `nodeSelector` 隔离专用节点池

## 相关文档

- [[容器运行时/05-gvisor-sandbox-production.md|gVisor 生产指南]]
- [[容器运行时/运行时迁移/02-runtime-class-configuration.md|RuntimeClass 配置]]
- [[容器运行时/containerd-CRI-O/04-kata-containers-secure-container.md|Kata Containers]]
- [[容器运行时/containerd-CRI-O/06-rootless-containers-guide.md|Rootless 容器]]

<!-- risk-assessed -->
