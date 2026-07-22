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

## 故障排查

| 问题 | 可能原因 | 诊断命令 | 解决方案 |
|------|----------|----------|----------|
| Pod 卡在 ContainerCreating | CRI 调用超时 | `crictl pods --state ready` | 检查 containerd 日志，确认 shim 进程状态 |
| ImagePull 失败 | ImageService 连接异常 | `crictl pull <image> -v` | 检查 registry 认证和网络连通性 |
| kubelet 报 CRI version mismatch | 版本不兼容 | `crictl version` | 升级 containerd 或调整 kubelet 配置 |
| Exec/Attach 失败 | stream server 不可达 | `curl -k https://localhost:10010/info` | 检查 stream_server_address 配置 |
| 容器频繁 OOM | cgroup 配置异常 | `crictl inspect <id> | jq .info.runtimeSpec.linux.resources` | 确认 memory limit 设置正确 |
| RunPodSandbox 超时 | 网络插件未就绪 | `journalctl -u containerd -f` | 检查 CNI 插件安装和配置 |
| ListContainers 返回空 | CRI 连接断开 | `systemctl status containerd` | 重启 containerd 服务 |
| StopContainer 无响应 | shim 进程挂起 | `ps aux | grep shim` | kill shim 进程，containerd 会自动清理 |

## CRI 版本兼容性矩阵

| Kubernetes 版本 | CRI API 版本 | containerd 最低版本 | CRI-O 最低版本 |
|----------------|-------------|-------------------|----------------|
| 1.26 | v1alpha2 | 1.6.x | 1.26.x |
| 1.27 | v1 | 1.7.x | 1.27.x |
| 1.28 | v1 | 1.7.x | 1.28.x |
| 1.29 | v1 | 1.7.x | 1.29.x |
| 1.30 | v1 | 1.7.x | 1.30.x |
| 1.31 | v1 | 1.7.x | 1.31.x |
| 1.32 | v1 | 2.0.x | 1.32.x |

## 生产最佳实践

| 维度 | 建议 | 说明 |
|------|------|------|
| 版本对齐 | kubelet 与 containerd CRI 版本严格匹配 | 避免 v1alpha2/v1 混用 |
| 超时配置 | 设置合理的 CRI 调用超时 | 默认 2min，大镜像拉取需调大 |
| 监控 | 监控 CRI 调用延迟 P99 | 超过 5s 告警 |
| 日志 | 开启 CRI 调用日志 | 便于问题追溯 |
| 升级 | 先升级 containerd 再升级 kubelet | 保证向后兼容 |
| 备份 | 升级前备份 /etc/containerd/config.toml | 便于回滚 |
| 测试 | 升级后执行 crictl 全量验证 | 确保所有接口正常 |
| 网络 | stream server 使用节点本地地址 | 避免跨节点访问问题 |

## CRI 接口调用时序

```text
Pod 创建完整时序：
kubelet → RunPodSandbox() → 创建 pause 容器 + 网络命名空间
kubelet → PullImage() → 拉取容器镜像
kubelet → CreateContainer() → 创建容器（未启动）
kubelet → StartContainer() → 启动容器
kubelet → ListContainers() → 定期状态同步
kubelet → StopContainer() → 停止容器
kubelet → RemoveContainer() → 清理容器
kubelet → StopPodSandbox() → 停止 Pod 沙箱
kubelet → RemovePodSandbox() → 清理 Pod 沙箱
```

## 相关工具

| 工具 | 用途 | 安装/使用 |
|------|------|----------|
| crictl | CRI 命令行调试 | 随 kubelet 分发，`crictl info` |
| grpcurl | gRPC 接口直接调用 | `brew install grpcurl` |
| cri-tools | CRI 测试套件 | `go install github.com/kubernetes-sigs/cri-tools/cmd/crictl@latest` |
| containerd-shim-runc-v2 | 默认 shim 实现 | 随 containerd 安装 |
| ctr | containerd 原生 CLI | 随 containerd 安装 |
| nerdctl | Docker 兼容 CLI | 单独安装 |

## 常见问题 FAQ

| 问题 | 解答 |
|------|------|
| CRI v1alpha2 和 v1 有何区别？ | v1 移除了部分废弃字段，API 更稳定，K8s 1.27+ 强制 v1 |
| 如何查看 CRI 调用日志？ | `journalctl -u containerd` 过滤 `grpc` 关键字 |
| crictl 和 kubectl 的关系？ | crictl 直接调用 CRI，kubectl 通过 API Server → kubelet → CRI |
| 如何测试 CRI 接口连通性？ | `crictl info` 返回 JSON 即表示连通 |
| RuntimeHandler 是什么？ | 指定运行时实现（runc/kata/gvisor），通过 RuntimeClass 选择 |
| stream server 的作用？ | 处理 exec/attach/port-forward 的 WebSocket 流 |
| 如何升级 CRI 版本？ | 升级 containerd 到对应版本，kubelet 自动协商 |
| CRI 调用超时如何调整？ | kubelet `--runtime-request-timeout` 参数 |

## 相关文档

- [[容器运行时/containerd-CRI-O/07-containerd-configuration-deep-guide.md|containerd 配置深度指南]]
- [[容器运行时/containerd-CRI-O/09-container-runtime-lifecycle.md|容器运行时生命周期]]
- [[容器运行时/01-containerd-deep-guide.md|containerd 深度指南]]

<!-- risk-assessed -->
