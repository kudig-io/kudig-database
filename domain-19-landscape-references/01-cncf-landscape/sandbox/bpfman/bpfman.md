---
title: bpfman
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- cilium
- docker
- ingress
- crd
- operator
- ebpf
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- bpfman 是什么
- 如何 bpfman
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- bpfman
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- ebpf-basics
- cilium-basics
---

title: bpfman
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- cilium
- docker
- ingress
- crd
- operator
- ebpf
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- bpfman 是什么
- 如何 bpfman
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- bpfman
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
# bpfman

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://bpfman.io/ |
| **GitHub** | https://github.com/bpfman/bpfman |
| **许可证** | Apache-2.0 |
| **开发语言** | Rust |
| **CNCF 状态** | Sandbox |

---

## 项目概述

bpfman 是一个 eBPF 程序管理器，提供系统守护进程和 Kubernetes Operator，用于集中加载、管理和监控 eBPF 程序。它解决了多个应用同时使用 eBPF 时的管理混乱问题，提供统一的 eBPF 程序生命周期管理、多程序共享挂载点、权限控制和可观测性，使 eBPF 程序的部署和运维更加安全和可控。

### 核心特性

- **集中管理**: 统一管理系统上所有 eBPF 程序的加载和卸载
- **多程序共享**: 多个 eBPF 程序可以安全地共享同一个挂载点（如 TC、XDP）
- **权限控制**: 应用无需 root 权限即可加载 eBPF 程序
- **Kubernetes Operator**: 通过 CRD 声明式管理 eBPF 程序部署
- **程序可见性**: 查看系统上所有已加载的 eBPF 程序及其状态
- **多种程序类型**: 支持 XDP、TC、Tracepoint、kprobe、uprobe 等

---

## 架构设计

```
┌──────────────────────────────────────────────────┐
│           Kubernetes (可选)                        │
│  ┌────────────────────────────┐                   │
│  │  bpfman Operator            │                   │
│  │  (BpfProgram CRD 控制器)    │                   │
│  └──────────────┬─────────────┘                   │
└─────────────────┼─────────────────────────────────┘
                  │ gRPC
┌─────────────────▼─────────────────────────────────┐
│              bpfman Daemon                          │
│                                                     │
│  ┌────────────────────────────────────────────┐    │
│  │         Program Manager                     │    │
│  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────────┐ │    │
│  │  │ XDP  │ │  TC  │ │Trace │ │ kprobe/  │ │    │
│  │  │Loader│ │Loader│ │point │ │ uprobe   │ │    │
│  │  └──────┘ └──────┘ └──────┘ └──────────┘ │    │
│  └───────────────────┬────────────────────────┘    │
│                      │                              │
│  ┌───────────────────▼────────────────────────┐    │
│  │         eBPF Bytecode Store                 │    │
│  │  (OCI Image / 本地文件 / URL)               │    │
│  └───────────────────┬────────────────────────┘    │
└──────────────────────┼──────────────────────────────┘
                       │
              ┌────────▼────────┐
              │   Linux Kernel  │
              │   (eBPF 子系统) │
              └─────────────────┘
```

---

## 快速开始

### 系统守护进程模式

```bash
# 安装 bpfman
cargo install bpfman

# 启动 bpfman daemon
sudo bpfman system service

# 加载 XDP 程序
bpfman load image \
  --image-url quay.io/bpfman-bytecode/xdp_pass:latest \
  xdp \
  --iface eth0 \
  --priority 50

# 列出已加载的程序
bpfman list

# 卸载程序
bpfman unload <program-id>
```

### Kubernetes Operator 模式

```bash
# 安装 bpfman Operator
kubectl apply -f https://github.com/bpfman/bpfman-operator/releases/latest/download/install.yaml
```

### 声明式 eBPF 程序部署

```yaml
# XDP 程序
apiVersion: bpfman.io/v1alpha1
kind: XdpProgram
metadata:
  name: xdp-pass
spec:
  bpfFunctionName: xdp_pass
  bytecode:
    image:
      url: quay.io/bpfman-bytecode/xdp_pass:latest
  interfaceSelector:
    primaryNodeInterface: true
  priority: 50
  nodeselector:
    kubernetes.io/os: linux

---
# TC 程序
apiVersion: bpfman.io/v1alpha1
kind: TcProgram
metadata:
  name: tc-stats
spec:
  bpfFunctionName: tc_stats
  bytecode:
    image:
      url: quay.io/bpfman-bytecode/tc_stats:latest
  interfaceSelector:
    interfaces:
      - eth0
  direction: ingress
  priority: 100
```

### Tracepoint 程序

```yaml
apiVersion: bpfman.io/v1alpha1
kind: TracepointProgram
metadata:
  name: sched-tracepoint
spec:
  bpfFunctionName: sched_process_exec
  bytecode:
    image:
      url: quay.io/bpfman-bytecode/tracepoint:latest
  names:
    - sched/sched_process_exec
```

---

## eBPF 程序打包

```dockerfile
# 将 eBPF bytecode 打包为 OCI 镜像
FROM scratch
COPY --chmod=0644 xdp_pass.o /
LABEL io.ebpf.programs="xdp_pass"
```

```bash
# 构建并推送
podman build -t quay.io/myorg/my-ebpf:latest .
podman push quay.io/myorg/my-ebpf:latest
```

---

## 与其他方案对比

| 特性 | bpfman | Cilium | bpftool | libbpf |
|:---|:---|:---|:---|:---|
| 定位 | eBPF 管理平台 | 网络/安全 | 调试工具 | 开发库 |
| 多程序管理 | 集中管理 | 内部管理 | 手动 | 手动 |
| K8s 集成 | CRD Operator | 内置 | 无 | 无 |
| 权限管理 | 细粒度控制 | 自管理 | 需 root | 需 CAP_BPF |
| OCI 分发 | 支持 | 不适用 | 不适用 | 不适用 |
| 适用场景 | eBPF 平台化管理 | 网络方案 | 调试分析 | 程序开发 |

---

## 最佳实践

1. **OCI 打包**: 将 eBPF bytecode 打包为 OCI 镜像，通过 Registry 管理版本
2. **优先级设置**: 合理设置程序优先级，确保关键程序优先执行
3. **节点选择器**: 使用 nodeSelector 控制 eBPF 程序的部署范围
4. **监控**: 监控 bpfman 暴露的指标，跟踪 eBPF 程序的加载状态和错误
5. **内核版本**: 确保节点内核版本支持所需的 eBPF 程序类型

---

## 参考资源

- [bpfman 官方文档](https://bpfman.io/docs/)
- [bpfman GitHub](https://github.com/bpfman/bpfman)
- [bpfman Operator](https://github.com/bpfman/bpfman-operator)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[domain-17-system-foundation/topic-cheat-sheet/docker.md|docker]]
- [[entities/kubernetes.md|kubernetes]]
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
