---
title: Kata Containers 安全容器运行时
description: 'Kata Containers 轻量级 VM 运行时架构、与 containerd 集成、多 Hypervisor 后端与性能开销分析'
summary: 'Kata Containers 轻量级 VM 运行时架构、与 containerd 集成、多 Hypervisor 后端与性能开销分析'
category: container-runtime
tags:
- kata-containers
- security
- vm-runtime
- hypervisor
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Kata Containers 是什么
- 如何配置 Kata Containers 与 containerd 集成
- Kata Containers 性能开销有多大
trigger_keywords:
- kata-containers
- security-container
- vm-runtime
- hypervisor
- sandbox
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Kata Containers 安全容器运行时

## 1. 架构概述

Kata Containers 是一种轻量级虚拟机运行时，将容器工作负载运行在独立的 VM 中，提供硬件级别的安全隔离。它兼容 OCI 和 CRI 标准，可以无缝替代 runc，同时提供 VM 级别的安全边界。

### 1.1 核心架构

```
┌──────────────────────────────────────────────────┐
│                    Kubernetes                      │
│  ┌─────────────────────────────────────────────┐ │
│  │              containerd (CRI)                │ │
│  │  ┌──────────┐  ┌──────────────────────────┐ │ │
│  │  │  runc    │  │  kata-runtime (shimv2)   │ │ │
│  │  │(普通容器)│  │  ┌──────────────────────┐│ │ │
│  │  │          │  │  │   Guest Kernel       ││ │ │
│  │  │          │  │  │  ┌────────────────┐ ││ │ │
│  │  │          │  │  │  │  Container     │ ││ │ │
│  │  │          │  │  │  │  (gVisor/应用) │ ││ │ │
│  │  │          │  │  │  └────────────────┘ ││ │ │
│  │  │          │  │  │  ┌────────────────┐ ││ │ │
│  │  │          │  │  │  │  Agent         │ ││ │ │
│  │  │          │  │  │  └────────────────┘ ││ │ │
│  │  └──────────┘  │  └──────────────────────┘│ │ │
│  │                │  ┌──────────────────────┐│ │ │
│  │                │  │   Hypervisor (QEMU/  ││ │ │
│  │                │   │   Cloud HV/Firecracker)│ │
│  │                │  └──────────────────────┘│ │ │
│  └─────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────┘
```

### 1.2 关键组件

| 组件 | 作用 |
|------|------|
| **kata-runtime** | OCI 兼容的容器运行时 |
| **kata-shimv2** | containerd shim，管理 VM 生命周期 |
| **hypervisor** | 轻量级虚拟机管理程序 |
| **guest kernel** | VM 内核，独立于宿主机 |
| **kata-agent** | VM 内代理，管理容器生命周期 |
| **virtiofs** | 文件系统共享（替代 9pfs） |

## 2. 安装配置

### 2.1 安装 Kata Containers

```bash
# 方式 1：使用官方安装脚本
export KATA_VERSION="3.6.0"
bash -c "$(curl -fsSL https://raw.githubusercontent.com/kata-containers/kata-containers/main/utils/kata-manager.sh)"

# 方式 2：通过包管理器（Ubuntu）
sudo apt-get update
sudo apt-get install -y kata-runtime kata-proxy kata-shim

# 方式 3：使用预构建二进制
wget https://github.com/kata-containers/kata-containers/releases/download/${KATA_VERSION}/kata-static-${KATA_VERSION}-amd64.tar.xz
sudo tar -xvf kata-static-${KATA_VERSION}-amd64.tar.xz -C /

# 验证安装
kata-runtime --version
kata-runtime check --only-list-extensions
```

### 2.2 Hypervisor 后端

| 后端 | 特点 | 推荐场景 |
|------|------|---------|
| **QEMU** | 功能最全，兼容性最好 | 开发测试、需要设备直通 |
| **Cloud Hypervisor** | Rust 实现，轻量快速 | 生产环境推荐 |
| **Firecracker** | 极简设计，启动最快 | 无服务器、高密度场景 |
| **ACRN** | 嵌入式优化 | 边缘计算 |

```bash
# 配置 Cloud Hypervisor（推荐生产）
sudo mkdir -p /etc/kata-containers
sudo cp /opt/kata/share/defaults/kata-containers/configuration-clh.toml \
        /etc/kata-containers/configuration.toml

# 配置 Firecracker
sudo cp /opt/kata/share/defaults/kata-containers/configuration-fc.toml \
        /etc/kata-containers/configuration.toml
```

### 2.3 containerd 集成（Runtime Handler）

```toml
# /etc/containerd/config.toml
version = 2

[plugins."io.containerd.grpc.v1.cri".containerd]
  # 默认运行时
  default_runtime_name = "runc"

  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes]
    # 普通容器运行时
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
      runtime_type = "io.containerd.runc.v2"
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
        BinaryName = "/usr/bin/runc"

    # Kata Containers 运行时
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata]
      runtime_type = "io.containerd.kata.v2"
      privileged_without_host_devices = true
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata.options]
        ConfigPath = "/etc/kata-containers/configuration.toml"

    # Kata with Cloud Hypervisor
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata-clh]
      runtime_type = "io.containerd.kata.v2"
      privileged_without_host_devices = true
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata-clh.options]
        ConfigPath = "/etc/kata-containers/configuration-clh.toml"
```

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 重启 containerd
sudo systemctl restart containerd

# 验证运行时
crictl info | jq '.config.containerd.runtimes'
```
### 2.4 Kubernetes RuntimeClass

```yaml
# RuntimeClass 定义
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata
handler: kata
overhead:
  podFixed:
    memory: "160Mi"
    cpu: "250m"
scheduling:
  nodeSelector:
    kata-runtime: "true"
---
# Kata with Cloud Hypervisor
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata-clh
handler: kata-clh
overhead:
  podFixed:
    memory: "120Mi"
    cpu: "200m"
---
# 使用 RuntimeClass 的 Pod
apiVersion: v1
kind: Pod
metadata:
  name: secure-app
spec:
  runtimeClassName: kata
  containers:
  - name: app
    image: my-secure-app:latest
    securityContext:
      privileged: false
      readOnlyRootFilesystem: true
```

## 3. 性能开销分析

### 3.1 启动延迟

| 指标 | runc | Kata (QEMU) | Kata (Cloud HV) | Kata (Firecracker) |
|------|------|-------------|------------------|-------------------|
| 冷启动 | ~200ms | ~1.5s | ~800ms | ~500ms |
| 热启动 | ~100ms | ~500ms | ~300ms | ~200ms |
| 内存开销 | ~10MB | ~130MB | ~80MB | ~50MB |
| CPU 开销 | 基准 | +5-10% | +3-5% | +2-3% |

### 3.2 I/O 性能

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 性能基准测试
# 宿主机（runc）
docker run --rm -it alpine fio --name=test --rw=randread --bs=4k --size=1G --runtime=30

# Kata 容器
kubectl run fio-kata --image=fio/fio --restart=Never \
  --overrides='{"spec":{"runtimeClassName":"kata","containers":[{"name":"fio-kata","image":"fio/fio","args":["--name=test","--rw=randread","--bs=4k","--size=1G","--runtime=30"]}]}}'
```
| I/O 模式 | runc | Kata (virtiofs) | Kata (9pfs) |
|----------|------|-----------------|-------------|
| 顺序读 | 基准 | -5-10% | -30-50% |
| 随机读 | 基准 | -10-15% | -40-60% |
| 顺序写 | 基准 | -5-10% | -30-50% |
| 网络吞吐 | 基准 | -3-5% | -10-15% |

### 3.3 优化配置

```toml
# /etc/kata-containers/configuration.toml 优化

# 使用 virtiofs（替代 9pfs，大幅提升 I/O）
[hypervisor.qemu]
shared_fs = "virtiofsd"

# 内存大页（减少 TLB miss）
enable_hugepages = true

# vCPU 和内存配置
default_vcpus = 1
default_memory = 2048

# 热插拔支持
enable_iommu = false
enable_vhost_user_store = true

# 安全配置
disable_seccomp = false
sandbox_cgroup_only = true
```

## 4. 安全模型

### 4.1 安全边界对比

```
普通容器 (runc):
┌────────────────────────┐
│       宿主机内核        │ ← 共享攻击面
│  ┌────┐ ┌────┐ ┌────┐ │
│  │ C1 │ │ C2 │ │ C3 │ │
│  └────┘ └────┘ └────┘ │
└────────────────────────┘

Kata Containers:
┌────────────────────────┐
│       宿主机内核        │
│  ┌──────────────────┐  │
│  │    Hypervisor    │  │ ← 硬件隔离
│  │  ┌────────────┐  │  │
│  │  │ Guest Kernel│  │  │ ← 独立内核
│  │  │ ┌────────┐ │  │  │
│  │  │ │Container│ │  │  │
│  │  │ └────────┘ │  │  │
│  │  └────────────┘  │  │
│  └──────────────────┘  │
└────────────────────────┘
```

### 4.2 安全加固配置

```yaml
# 高安全 Pod 配置
apiVersion: v1
kind: Pod
metadata:
  name: high-security-app
  annotations:
    # 限制内核能力
    io.kata-containers.config.hypervisor.enable_iommu: "false"
spec:
  runtimeClassName: kata
  containers:
  - name: app
    image: my-app:latest
    securityContext:
      privileged: false
      runAsNonRoot: true
      runAsUser: 1000
      readOnlyRootFilesystem: true
      allowPrivilegeEscalation: false
      capabilities:
        drop:
        - ALL
    resources:
      limits:
        cpu: "2"
        memory: "2Gi"
      requests:
        cpu: "500m"
        memory: "512Mi"
```

## 5. 生产最佳实践

| 实践 | 建议 |
|------|------|
| Hypervisor 选择 | 生产用 Cloud Hypervisor，高密度用 Firecracker |
| 共享文件系统 | 优先 virtiofs，避免 9pfs |
| 资源限制 | 设置合理的 overhead（CPU +10%, 内存 +160MB） |
| 节点规划 | Kata 节点单独标签，避免与普通容器混部 |
| 安全基线 | 启用 seccomp、AppArmor、SELinux |
| 监控 | 监控 VM 数量、内存使用、启动延迟 |

## Related

- [[domain-13-container-runtime/containerd-CRI-O/05-gvisor-sandbox-runtime|gVisor 沙箱运行时]]
- [[domain-13-container-runtime/containerd-CRI-O/06-rootless-containers-guide|Rootless 容器指南]]

## See Also

- [Kata Containers 官方文档](https://katacontainers.io/)
- [Kata Containers GitHub](https://github.com/kata-containers/kata-containers)


<!-- risk-assessed -->
