---
title: Container Runtime (entities)
description: Container Runtime — Kubernetes 生产运维知识库
summary: Container Runtime — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- container
- runtime
- containerd
- cri-o
- cri
- kubelet
- docker
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Container Runtime 是什么
- 如何 Container Runtime
trigger_keywords:
- Container
- Runtime
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Container Runtime

## 概述

Container Runtime 是运行容器的底层软件组件，负责镜像管理、容器生命周期管理和资源隔离。在 Kubernetes 生态中，容器运行时通过 CRI（Container Runtime Interface）与 kubelet 交互。主流容器运行时包括 containerd（CNCF 毕业）、CRI-O 和 cri-dockerd。容器运行时是 Kubernetes 节点组件的核心，决定了 Pod 的启动速度、隔离性和资源效率。

## 核心特性

- **CRI 标准接口**: gRPC API（RuntimeService + ImageService）与 kubelet 解耦
- **OCI 运行时**: 底层调用 runc/crun/kata 等 OCI Runtime 创建容器
- **镜像管理**: Pull/Push/List/Delete OCI 兼容镜像
- **Snapshot 管理**: OverlayFS、btrfs 等联合文件系统管理镜像层
- **RuntimeClass**: 允许 Pod 选择不同的容器运行时
- **多沙箱**: 支持 gVisor、Kata Containers、Wasm 等隔离级别

## 架构

容器运行时架构分为高层和低层。高层运行时（containerd、CRI-O）实现 CRI 接口，管理 Pod Sandbox 和容器生命周期。kubelet 通过 CRI gRPC 调用高层运行时的 `RunPodSandbox`、`CreateContainer`、`StartContainer` 方法。高层运行时负责镜像拉取、存储管理和网络配置。低层运行时（runc、crun、kata-runtime）通过 OCI Runtime Specification 实际创建和运行容器进程。containerd 通过 containerd-shim 管理每个容器进程。

## CRI 接口

CRI（Container Runtime Interface）是 kubelet 与容器运行时之间的 gRPC API。RuntimeService 管理容器生命周期（PodSandbox 和 Container 的创建/启动/停止/删除）。ImageService 管理镜像操作（PullImage/ListImages/RemoveImage）。CRI 的标准化使 Kubernetes 可以在不修改核心代码的情况下支持不同的容器运行时。

## 运行时选择对比

| 运行时 | 优势 | 劣势 | 适用场景 |
|--------|------|------|----------|
| **containerd** | 轻量、高性能、CNCF 毕业 | 调试需 nerdctl | 通用生产 |
| **CRI-O** | K8s 专用、最小依赖 | 功能较少 | 纯 K8s 环境 |
| **cri-dockerd** | Docker 兼容 | 重、已弃用 | 兼容旧系统 |

## Kubernetes 集成

Kubernetes v1.24 移除了内置的 dockershim，CRI 成为唯一的容器运行时接口。节点上 kubelet 通过 Unix Socket 连接容器运行时（containerd: /run/containerd/containerd.sock）。RuntimeClass CRD 允许为不同 Pod 选择不同的运行时——标准容器使用 `runc`/`crun`，安全敏感工作负载使用 `gVisor`/`kata`，Wasm 工作负载使用 `wasmtime`。containerd 通过配置文件（config.toml）管理运行时、镜像仓库和插件。

## 生产使用场景

1. **通用容器**: 使用 containerd + runc 运行标准工作负载
2. **安全隔离**: 使用 Kata Containers 运行不可信工作负载
3. **多租户**: 使用 gVisor 提供更强的进程隔离
4. **WASM 应用**: 使用 wasmtime-spin 运行 WebAssembly 模块

## 安装与配置

```bash
# containerd 安装（通常由 kubeadm/k0s 自动安装）
apt install containerd
# 配置 SystemdCgroup
containerd config default | tee /etc/containerd/config.toml
sed -i 's/SystemdCgroup = false/SystemdCgroup = true/' /etc/containerd/config.toml
systemctl restart containerd

# CRI-O 安装
apt install cri-o
systemctl enable --now crio
```

### containerd 配置详解 (config.toml)

```toml
version = 2
[plugins."io.containerd.grpc.v1.cri"]
  sandbox_image = "registry.k8s.io/pause:3.9"
  [plugins."io.containerd.grpc.v1.cri".containerd]
    default_runtime_name = "runc"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
      runtime_type = "io.containerd.runc.v2"
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
        SystemdCgroup = true
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata]
      runtime_type = "io.containerd.kata.v2"
  [plugins."io.containerd.grpc.v1.cri".registry]
    [plugins."io.containerd.grpc.v1.cri".registry.mirrors]
      [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
        endpoint = ["https://mirror.example.com"]
```

### RuntimeClass 配置

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata
handler: kata
scheduling:
  nodeSelector:
    runtime: kata
---
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: runsc
---
# Pod 中使用 RuntimeClass
apiVersion: v1
kind: Pod
metadata:
  name: sandboxed-app
spec:
  runtimeClassName: kata
  containers:
  - name: app
    image: nginx:latest
```

## 运维操作

```bash
# 🟢 查看容器运行时状态
systemctl status containerd
ctr version
crictl version

# 🟢 列出运行中的容器
crictl ps
crictl pods

# 🟢 查看容器详细信息
crictl inspect <container-id>
crictl inspectp <pod-sandbox-id>

# 🟢 查看镜像列表
crictl images
ctr -n k8s.io images ls

# 🟡 拉取镜像
crictl pull nginx:latest
ctr -n k8s.io images pull docker.io/library/nginx:latest

# 🟡 查看容器日志
crictl logs <container-id>
crictl logs --tail 100 <container-id>

# 🟢 查看运行时指标
curl --unix-socket /run/containerd/containerd.sock http://localhost/v1/metrics

# 🔴 重启容器运行时（影响所有 Pod）
systemctl restart containerd

# 🔴 清理未使用镜像
crictl rmi --prune
ctr -n k8s.io images prune --all
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Pod Pending + ContainerCreating | 镜像拉取失败 | `crictl pull <image>` | 检查网络/镜像仓库/凭证 |
| RunContainerError | OCI Runtime 失败 | `journalctl -u containerd` | 检查 runc/crun 版本兼容性 |
| 容器 OOMKilled | 内存限制过低 | `crictl inspect <id> \| jq .info.runtimeSpec` | 调整 resources.limits.memory |
| CRI 连接失败 | Socket 不存在 | `ls /run/containerd/containerd.sock` | 重启 containerd 服务 |
| 磁盘压力 | 镜像层堆积 | `du -sh /var/lib/containerd/` | 清理未用镜像，配置 GC |
| SystemdCgroup 不匹配 | kubelet 与 containerd 配置不一致 | 检查 kubelet 和 config.toml | 统一设置为 SystemdCgroup=true |

### 排查流程

```
Pod 异常 → crictl pods 查看 Sandbox 状态
  ├─ Sandbox NotReady → 检查 containerd 服务状态
  │   ├─ 服务停止 → systemctl restart containerd
  │   └─ 服务运行 → 检查磁盘/内存/日志
  └─ Sandbox Ready → crictl ps 查看容器状态
      ├─ 容器未创建 → 检查镜像拉取 (crictl pull)
      ├─ 容器退出 → crictl logs 查看退出原因
      └─ 容器运行中 → 检查应用层问题
```

## 生产案例

### 案例1: SystemdCgroup 不匹配导致节点 NotReady

**场景**: 升级 containerd 后节点变为 NotReady，Pod 无法创建  
**排查**: kubelet 日志显示 cgroup driver 不匹配，kubelet 用 systemd，containerd 用 cgroupfs  
**方案**: 修改 config.toml 设置 `SystemdCgroup = true`，重启 containerd  
**效果**: 节点恢复 Ready，后续在节点初始化脚本中统一配置  

### 案例2: 镜像层磁盘压力导致节点驱逐

**场景**: 生产节点频繁触发 DiskPressure 驱逐  
**排查**: `/var/lib/containerd` 占用 80GB，大量未使用镜像层  
**方案**: 配置 image GC 策略（highThreshold=85, lowThreshold=80）+ 定期 crictl rmi --prune  
**效果**: 磁盘使用稳定在 40%，消除驱逐事件  

## 运行时选择对比

| 运行时 | 优势 | 劣势 | 适用场景 |
|--------|------|------|----------|
| **containerd** | 轻量、高性能、CNCF 毕业 | 调试需 nerdctl | 通用生产 |
| **CRI-O** | K8s 专用、最小依赖 | 功能较少 | 纯 K8s 环境 |
| **cri-dockerd** | Docker 兼容 | 重、已弃用 | 兼容旧系统 |
| **Kata Containers** | VM 级隔离 | 启动慢、资源开销 | 多租户/不可信工作负载 |
| **gVisor** | 进程级隔离、轻量 | 系统调用兼容性 | 安全敏感应用 |
| **Wasm (wasmtime)** | 极快启动、极小体积 | 生态不成熟 | 边缘/FaaS |

## 检查清单

- [ ] 确认 SystemdCgroup = true（与 kubelet 一致）
- [ ] 配置镜像仓库镜像加速
- [ ] 设置 image GC 策略防止磁盘压力
- [ ] 生产环境使用 containerd 或 CRI-O（避免 dockershim）
- [ ] 多租户场景配置 RuntimeClass（kata/gvisor）
- [ ] 监控 containerd 内存/CPU/磁盘使用
- [ ] 定期清理未使用镜像和容器
- [ ] 配置日志轮转防止日志磁盘占用

## 相关链接

- [[containerd]] — containerd
- [[cri-o]] — CRI-O
- [[kubernetes]] — Kubernetes
- [[docker]] — Docker
- [[实体/kubelet.md|kubelet]]

## Related

- [[containerd]] — containerd
- [[cri-o]] — CRI-O
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[docker]] — Docker
- [[实体/kubelet.md|kubelet]]
- [[pod-lifecycle|Pod Lifecycle]]
- [[故障诊断/高级排障/02-node-components/03-container-runtime-troubleshooting.md|03-container-runtime-troubleshooting]]

<!-- risk-assessed -->
