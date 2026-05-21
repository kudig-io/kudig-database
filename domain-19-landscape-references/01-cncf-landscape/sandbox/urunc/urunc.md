---
title: urunc (Unikernel Container Runtime)
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- containerd
- cri-o
- docker
- harbor
- serverless
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- urunc (Unikernel Container Runtime) 是什么
- 如何 urunc (Unikernel Container Runtime)
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- urunc
- Unikernel
- Container
- Runtime
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

title: urunc (Unikernel Container Runtime)
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- containerd
- cri-o
- docker
- harbor
- serverless
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- urunc (Unikernel Container Runtime) 是什么
- 如何 urunc (Unikernel Container Runtime)
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- urunc
- Unikernel
- Container
- Runtime
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
# urunc (Unikernel Container Runtime)

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://nubificus.co.uk/urunc |
| **GitHub** | https://github.com/nubificus/urunc |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

urunc 是一个符合 OCI 标准的容器运行时，专门用于在 Kubernetes 中运行 Unikernel 应用。Unikernel 是将应用与最小化操作系统库编译为单一镜像的技术，具有极小的攻击面、亚毫秒级启动时间和极低的内存占用。urunc 将 Unikernel 打包为 OCI 镜像，使其能够通过标准的容器工作流（containerd、CRI-O）在 Kubernetes 上部署和管理。

### 核心特性

- **OCI 兼容**: 将 Unikernel 打包为标准 OCI 容器镜像
- **多 Unikernel 支持**: 支持 Unikraft、Rumprun、MirageOS 等框架
- **多 VMM 支持**: 支持 QEMU、Firecracker、Cloud Hypervisor 等虚拟机监控器
- **Kubernetes 原生**: 通过 containerd/CRI-O 集成到 Kubernetes
- **极致安全**: Unikernel 单地址空间，无 shell、无包管理器，极小攻击面
- **极快启动**: 亚毫秒级冷启动，适合 Serverless/FaaS 场景

---

## 架构设计

```
┌─────────────────────────────────────────────┐
│            Kubernetes                        │
│                                              │
│  ┌──────────────────────────────────┐       │
│  │    containerd / CRI-O            │       │
│  │    (容器运行时接口)               │       │
│  └──────────────┬───────────────────┘       │
│                 │                             │
│  ┌──────────────▼───────────────────┐       │
│  │          urunc                    │       │
│  │  (OCI Runtime / Unikernel 管理)  │       │
│  │                                   │       │
│  │  ┌──────────────────────┐        │       │
│  │  │ OCI Image → Unikernel│        │       │
│  │  │ 镜像解包 / VMM 配置   │        │       │
│  │  └──────────┬───────────┘        │       │
│  └─────────────┼────────────────────┘       │
│                │                             │
│  ┌─────────────▼────────────────────┐       │
│  │    VMM (Virtual Machine Monitor)  │       │
│  │  ┌────────┐ ┌───────────┐       │       │
│  │  │ QEMU   │ │Firecracker│       │       │
│  │  └────────┘ └───────────┘       │       │
│  │  ┌────────────────┐             │       │
│  │  │Cloud Hypervisor│             │       │
│  │  └────────────────┘             │       │
│  └─────────────┬────────────────────┘       │
│                │                             │
│  ┌─────────────▼────────────────────┐       │
│  │       Unikernel Instance          │       │
│  │  (单一应用 + 最小 OS 库)          │       │
│  │  Unikraft / Rumprun / MirageOS   │       │
│  └──────────────────────────────────┘       │
└─────────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# 构建 urunc
git clone https://github.com/nubificus/urunc.git
cd urunc
make build
sudo make install

# 配置 containerd 使用 urunc
# /etc/containerd/config.toml
```

### containerd 配置

```toml
# /etc/containerd/config.toml (添加 urunc runtime)
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.urunc]
  runtime_type = "io.containerd.urunc.v2"
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.urunc.options]
    BinaryName = "/usr/local/bin/urunc"
```

### 在 Kubernetes 中使用

```yaml
# RuntimeClass 定义
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: urunc
handler: urunc

---
# 使用 Unikernel 运行 Pod
apiVersion: v1
kind: Pod
metadata:
  name: unikernel-nginx
  labels:
    app: unikernel-nginx
spec:
  runtimeClassName: urunc
  containers:
    - name: nginx-unikernel
      image: harbor.nbfc.io/nubificus/urunc/nginx-qemu-unikraft:latest
      ports:
        - containerPort: 80
      resources:
        limits:
          cpu: "1"
          memory: "128Mi"
```

### 构建 Unikernel OCI 镜像

```dockerfile
# 使用 Unikraft 构建 Unikernel 然后打包为 OCI 镜像
FROM scratch
COPY unikernel.bin /unikernel/kernel
COPY rootfs/ /unikernel/rootfs/
LABEL com.urunc.unikernel.unikernelType="unikraft"
LABEL com.urunc.unikernel.hypervisor="qemu"
```

---

## 与其他方案对比

| 特性 | urunc (Unikernel) | runc (容器) | Kata Containers (microVM) | gVisor |
|:---|:---|:---|:---|:---|
| 隔离级别 | VM (Unikernel) | Namespace/Cgroup | VM (Linux) | 用户态内核 |
| 启动时间 | <1ms | ~100ms | ~100-500ms | ~150ms |
| 内存占用 | ~1-10MB | ~10-50MB | ~30-100MB | ~20-50MB |
| 攻击面 | 极小 | 中等 | 小 | 小 |
| 应用兼容 | 需专门编译 | 任意 Linux | 任意 Linux | 大部分 Linux |
| 适用场景 | 安全敏感/Serverless | 通用 | 多租户隔离 | 不信任工作负载 |

---

## 最佳实践

1. **应用选择**: Unikernel 最适合单一功能的微服务，不适合复杂的多进程应用
2. **VMM 选择**: Firecracker 适合高密度部署，QEMU 适合功能丰富的场景
3. **镜像构建**: 使用 Unikraft 的 kraft 工具简化 Unikernel 的构建流程
4. **资源配置**: Unikernel 内存需求远低于容器，合理设置 limits 节省资源
5. **混合部署**: 在同一集群中通过 RuntimeClass 混合部署容器和 Unikernel

---

## 参考资源

- [urunc GitHub](https://github.com/nubificus/urunc)
- [Unikraft](https://unikraft.org/)
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
