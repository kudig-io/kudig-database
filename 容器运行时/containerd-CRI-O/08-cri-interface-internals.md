---
title: CRI gRPC 接口内部
description: 深入 CRI（Container Runtime Interface）gRPC 协议，RuntimeService 与 ImageService 关键方法、调用时序与排障映射
summary: 深入 CRI（Container Runtime Interface）gRPC 协议，RuntimeService 与 ImageService 关键方法、调用时序与排障映射
category: container-runtime
tags:
- containerd
- cri
- runtime
- grpc
- kubernetes
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
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# CRI gRPC 接口内部

## 概述

CRI（Container Runtime Interface）是 kubelet 与容器运行时之间的 gRPC 协议，定义在 `k8s.io/cri-api`。它由两个服务组成：`RuntimeService`（Pod/容器生命周期）与 `ImageService`（镜像管理）。理解接口方法有助于把 `kubectl describe` 现象精准映射到运行时层。

## 架构与传输

```
kubelet ──gRPC(unix socket)──> CRI shim (containerd/CRI-O)
                                  ├─ RuntimeService
                                  └─ ImageService
```

- 传输：Unix domain socket，默认 `/run/containerd/containerd.sock`
- 协议：gRPC + Protobuf，启用流式（`Exec`/`Attach`/`PortForward` 通过 `Streaming RPC`）
- 版本协商：`Version()` 返回 `RuntimeHandler` 列表，对应 RuntimeClass

## ImageService 关键方法

| RPC 方法 | 作用 | 排障映射 |
|---|---|---|
| `ListImages` | 列出本地镜像 | `crictl images` |
| `PullImage` | 拉取镜像，含 auth/registry | `ImagePullBackOff` |
| `RemoveImage` | 删除镜像层 | `crictl rmi` |
| `ImageStatus` | 查询镜像 digest/大小 | `crictl inspecti` |
| `ImageFsInfo` | 镜像存储使用量 | DiskPressure 排查 |

`PullImage` 请求关键字段：`image.image`（引用）、`image.auth`（仓库凭证）、`image.sandbox_config`（用于解析镜像凭据的沙箱）。拉取失败时 kubelet 重试并最终进入 `ImagePullBackOff`。

## RuntimeService 关键方法

| RPC 方法 | 作用 | 触发场景 |
|---|---|---|
| `RunPodSandbox` | 创建 pause/infra 容器 | Pod 调度 |
| `StopPodSandbox` | 停止沙箱 | Pod 删除 |
| `RemovePodSandbox` | 删除沙箱 | GC |
| `CreateContainer` | 在沙箱内创建容器 | ContainerCreating |
| `StartContainer` | 启动容器 | 进入 Running |
| `StopContainer` | 优雅停止 | 删除/驱逐 |
| `ContainerStatus` | 容器状态/退出码 | `crictl inspect` |
| `ListPodSandbox` | 列出沙箱 | `crictl pods` |

## 典型调用时序：Pod 启动

```
kubelet SyncPod
 ├─ RunPodSandbox      → 创建 pause 容器、配置 Pod 网络（CNI）
 ├─ PullImage (per)    → 拉取业务镜像
 ├─ CreateContainer    → 生成 OCI bundle、rootfs
 ├─ StartContainer     → 调用 runc 启动
 └─ ContainerStatus    → 回填 Running
```

Pod 卡在 `ContainerCreating` 时，按此时序逐段定位：sandbox 是否就绪（`crictl pods`）→ 镜像是否拉到（`crictl images`）→ shim 是否创建。

## 直接调试 CRI

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# crictl 本质是 CRI gRPC 客户端
crictl --runtime-endpoint unix:///run/containerd/containerd.sock pods
crictl version   # 打印 RuntimeApiVersion 与 RuntimeName

# 启用 verbose 看 gRPC 往返
crictl --debug ps
```

## CRI 版本兼容性

| kubelet | CRI v1alpha2 | CRI v1 |
|---|---|---|
| ≤ 1.25 | 默认 | 支持 |
| 1.26+ | 弃用 | 默认 |
| 1.29+ | 移除 | 仅 v1 |

containerd 1.6+ 同时实现 v1 与 v1alpha2；1.7 起默认 v1。升级 kubelet 与 containerd 必须保持 CRI 版本对齐，否则节点会 `NotReady`（`CRI version v1 runtime API is not implemented`）。

## Exec/Attach/PortForward 流式

这三个 RPC 返回 URL，由 kubelet 反向代理到运行时的流服务器。containerd 在 `StreamServerAddress` / `StreamServerPort` 配置：

```toml
[plugins."io.containerd.grpc.v1.cri"]
  stream_server_address = "127.0.0.1"
  stream_server_port = "0"
```

`kubectl exec` 卡住或超时，常因该地址不可达或节点防火墙拦截。

## 生产检查清单

- [ ] kubelet 与 containerd CRI 版本对齐（`crictl info` 核对）
- [ ] `stream_server_address` 在节点本地可达
- [ ] 已通过 `crictl info` 验证 `RuntimeHandler` 列表
- [ ] ImageService 拉取错误已接入告警

## 相关文档

- [[容器运行时/containerd-CRI-O/07-containerd-configuration-deep-guide.md|containerd 配置深度指南]]
- [[容器运行时/containerd-CRI-O/09-container-runtime-lifecycle.md|容器运行时生命周期]]
- [[容器运行时/01-containerd-deep-guide.md|containerd 深度指南]]

<!-- risk-assessed -->
