---
title: Kubernetes 与 containerd 集成深度分析
summary: Kubernetes 与 containerd 集成深度分析：Kubernetes 通过 CRI（Container Runtime Interface）与
  containerd 通信。kubelet 作为节点代理，调用 containerd 的 CRI 插件来管理容器生命周期。
category: synthesis
tags:
- synthesis
- k8s
- containerd
- cluster
tier: supporting
sources: []
created: 2026-05-24
updated: 2026-07
last_updated: 2026-07
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes 与 containerd 集成深度分析

> 本文综合分析 Kubernetes 与 containerd 的集成架构、CRI 通信机制、运维要点和最佳实践。

## 核心关系

Kubernetes 通过 CRI（Container Runtime Interface）与 containerd 通信。kubelet 作为节点代理，通过 gRPC 调用 containerd 的 CRI 插件来管理容器生命周期。这种接口解耦设计使 Kubernetes 可以支持任何兼容 CRI 的容器运行时。

## 架构层次

### 完整调用链

```
kubelet → CRI gRPC → containerd → containerd-shim-runc-v2 → runc → Linux kernel
                           ↓
                    containerd 内部组件:
                    ├── cri-plugin (CRI 接口实现)
                    ├── containerd-shim (容器进程管理)
                    ├── metadata (bbolt 元数据存储)
                    ├── content store (镜像层存储)
                    └── snapshotter (overlayfs 快照管理)
```

### CRI 接口详解

CRI 定义了两个 gRPC 服务接口：

| 接口 | 主要方法 | 说明 |
|------|---------|------|
| ImageService | `PullImage`, `ListImages`, `RemoveImage`, `ImageStatus` | 镜像生命周期管理 |
| RuntimeService | `RunPodSandbox`, `CreateContainer`, `StartContainer`, `StopContainer` | 容器/Pod 生命周期管理 |
| RuntimeService | `ContainerStatus`, `PodSandboxStatus` | 状态查询 |
| RuntimeService | `ExecSync`, `Exec`, `Attach` | 容器内执行 |
| RuntimeService | `ListPodSandbox`, `ListContainer` | 列表查询 |

### Pod 创建的完整流程

```
1. kubelet PLEG（Pod Lifecycle Event Generator）检测到新 Pod
2. kubelet 调用 RuntimeService.RunPodSandbox
   → containerd 创建 pause 容器（Pod Sandbox）
   → pause 容器持有 Pod 的 network namespace 和 IPC namespace
3. kubelet 调用 ImageService.PullImage（如果镜像不存在）
   → containerd 从 registry 拉取镜像层
   → 存储到 content store
4. kubelet 对每个 init container 调用 CreateContainer + StartContainer
5. kubelet 对每个 main container 调用 CreateContainer + StartContainer
   → containerd 创建容器（基于镜像创建快照层）
   → containerd-shim 启动容器进程
6. kubelet 调用 ContainerStatus 轮询状态
```

## 运维要点

### 1. 版本兼容性

Kubernetes 版本与 containerd 版本有严格的兼容矩阵：

| Kubernetes | containerd | CRI 版本 | 说明 |
|------------|-----------|----------|------|
| 1.28-1.29 | 1.7.x | CRI v1 + v1alpha2 | 过渡期 |
| 1.30-1.31 | 1.7.x | CRI v1 | alpha2 废弃 |
| 1.32+ | 2.0+ | CRI v1 only | 仅支持 CRI v1 |

### 2. 镜像管理工具

containerd 使用两种命令行工具，用途不同：

```bash
# 🟢 低风险：只读/信息收集
# crictl: CRI 兼容工具（kubelet 视角）
crictl ps -a                          # 查看容器
crictl images                         # 查看镜像
crictl pods                           # 查看 Pod sandbox
crictl logs <container-id>            # 查看日志
crictl inspect <container-id>         # 检查容器详情

# ctr: containerd 原生工具（运行时视角）
ctr -n k8s.io images list             # 列出镜像（注意需要指定 namespace）
ctr -n k8s.io content list            # 查看镜像层
ctr -n k8s.io snapshots list          # 查看快照
```

**关键区别**：`crictl` 通过 CRI 接口操作（kubelet 兼容），`ctr` 直接操作 containerd API。排查问题时 `crictl` 更适合。

### 3. containerd 配置

```toml
# /etc/containerd/config.toml 关键配置
version = 2

[plugins."io.containerd.grpc.v1.cri"]
  # 镜像拉取配置
  [plugins."io.containerd.grpc.v1.cri".containerd]
    snapshotter = "overlayfs"
    default_runtime_name = "runc"
    
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
      runtime_type = "io.containerd.runc.v2"
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
        SystemdCgroup = true            # 使用 systemd cgroup driver

  # 镜像仓库配置
  [plugins."io.containerd.grpc.v1.cri".registry]
    config_path = "/etc/containerd/certs.d"  # mirror 配置目录

  # 日志配置
  [plugins."io.containerd.grpc.v1.cri".containerd]
    max_container_log_line_size = 16384
```

### 4. 日志路径

容器日志由 kubelet 管理，路径结构为：

```
/var/log/pods/<namespace>_<pod-name>_<pod-uid>/<container-name>/<restart-count>.log
# 示例: /var/log/pods/production_web-0_abc123/web/0.log
```

## 常见问题诊断

```bash
# 🟢 低风险：诊断操作
# 容器启动失败
journalctl -u containerd --since "10 min ago" | grep -i error

# Pod 卡在 ContainerCreating
crictl pods --state NotReady             # 检查 sandbox 状态
crictl ps -a | grep -i <pod-name>        # 检查容器状态

# 镜像拉取超时
# 检查 containerd registry mirror 配置
cat /etc/containerd/certs.d/*/hosts.toml

# 节点 NotReady（containerd 异常）
systemctl status containerd
crictl info                              # 检查 CRI 连接
```

## 最佳实践

- **确保 cgroup driver 一致性**：kubelet 和 containerd 必须使用相同的 cgroup driver（推荐 systemd），否则资源计量不准确
- **配置镜像加速器**：国内环境必须配置 registry mirror（`/etc/containerd/certs.d/`），避免镜像拉取超时导致 Pod 创建失败
- **使用 crictl 而非 ctr 做运维**：crictl 通过 CRI 接口操作，与 kubelet 视角一致，更安全可靠
- **定期清理未使用镜像**：配置 containerd GC 或定期执行 `crictl rmi --prune`，防止磁盘耗尽
- **监控 containerd shim 进程**：每个容器对应一个 shim 进程，shim 泄漏会消耗内存——监控 shim 进程数

## 常见陷阱

- **containerd namespace 隔离**：K8s 容器在 `k8s.io` namespace 下，使用 `ctr` 操作时必须加 `-n k8s.io`，否则看不到容器
- **sandbox image 拉取失败**：pause 镜像（registry.k8s.io/pause）拉取失败导致 Pod 无法创建——需要配置 mirror 或预拉取
- **容器日志被截断**：默认 max_container_log_line_size 为 16KB，超长日志行会被截断——需根据应用调整

## 相关页面

- [[kubernetes]] — 集群整体架构
- [[概念/containerd-pod-lifecycle.md|containerd Pod 生命周期]] — 容器生命周期详细映射
- [[概念/etcd-containerd-storage.md|etcd 与 containerd 存储]] — 存储层架构
- [[概念/container-runtime-evolution.md|容器运行时演进]] — 运行时发展路线
- [[pod-lifecycle]] — Pod 生命周期管理
- [[kubelet]] — 节点代理


<!-- risk-assessed -->
