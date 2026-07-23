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

## CRI 配置示例

```toml
# /etc/containerd/config.toml - CRI 插件配置
[plugins."io.containerd.grpc.v1.cri"]
  # stream server 配置
  stream_server_address = "127.0.0.1"
  stream_server_port = "0"
  # 沙箱镜像
  sandbox_image = "registry.k8s.io/pause:3.9"
  # 运行时配置
  [plugins."io.containerd.grpc.v1.cri".containerd]
    default_runtime_name = "runc"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
      runtime_type = "io.containerd.runc.v2"
  # 镜像拉取配置
  [plugins."io.containerd.grpc.v1.cri".registry]
    max_concurrent_downloads = 10
```

## 性能调优

| 场景 | 优化方向 | 具体操作 |
|------|----------|----------|
| Pod 创建慢 | 预拉取 pause 镜像 | 节点初始化时预加载 |
| 镜像拉取超时 | 调大超时时间 | kubelet --runtime-request-timeout=5m |
| 高并发创建 | 调整并发度 | max_concurrent_downloads |
| exec 延迟高 | 检查 stream server | 确认本地地址可达 |
| 状态同步慢 | 调整 kubelet 同步周期 | --sync-frequency 参数 |
| gRPC 连接失败 | 检查 socket 权限 | 确认 /run/containerd/containerd.sock |

## 监控指标

| 指标 | 含义 | 告警阈值 |
|------|------|----------|
| cri_request_duration_seconds | CRI 调用延迟 | P99 > 5s |
| cri_request_errors_total | CRI 调用错误 | > 10/min |
| image_pull_duration_seconds | 镜像拉取耗时 | P99 > 120s |
| container_create_duration | 容器创建耗时 | P99 > 10s |
| sandbox_create_duration | 沙箱创建耗时 | P99 > 5s |
| running_containers | 运行中容器数 | > 节点容量 90% |

## 安全加固

| 维度 | 建议 | 说明 |
|------|------|------|
| socket 权限 | 仅 root 可访问 | chmod 600 containerd.sock |
| stream server | 使用 TLS | 避免明文传输 |
| RBAC | 限制 crictl 访问 | 仅运维人员可用 |
| 审计 | 记录 CRI 调用 | 便于安全审计 |
| 网络 | stream server 本地监听 | 避免远程访问 |

## 迁移指南

| 从 | 到 | 关键步骤 |
|------|------|----------|
| CRI v1alpha2 | CRI v1 | 升级 containerd 1.7+→升级 kubelet |
| Docker shim | 直接 CRI | 移除 dockershim→配置 containerd |
| 单运行时 | 多运行时 | 配置多个 runtime_type |

## 检查清单

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| CRI 版本 | `crictl version` | v1 |
| 接口连通 | `crictl info` | 返回 JSON |
| 运行时列表 | `crictl info | jq .config` | 包含预期 runtime |
| 镜像拉取 | `crictl pull <image>` | 成功 |
| 容器创建 | `crictl runp` | 成功 |
| stream server | `curl -k https://localhost:10010/info` | 可达 |
| 日志 | `journalctl -u containerd` | 无错误 |

## 版本历史

| 版本 | 时间 | 关键变化 |
|------|------|----------|
| v1alpha2 | K8s 1.7-1.26 | 初始 CRI 接口 |
| v1 | K8s 1.27+ | 移除废弃字段，API 稳定 |
| v1 (containerd 2.0) | K8s 1.32+ | 新插件架构 |

## 架构对比

```text
CRI 架构层次：

kubelet
  └── CRI gRPC Client
       └── /run/containerd/containerd.sock
            └── containerd CRI Plugin
                 ├── RuntimeService
                 │    ├── RunPodSandbox
                 │    ├── CreateContainer
                 │    ├── StartContainer
                 │    └── Exec/Attach
                 └── ImageService
                      ├── PullImage
                      ├── ListImages
                      └── RemoveImage
```

## 容量规划

| 场景 | 建议配置 | 说明 |
|------|----------|------|
| 小集群 | 默认超时 | 足够 |
| 大镜像 | timeout=5m | 避免拉取超时 |
| 高并发 | max_concurrent=10 | 并行拉取 |
| 排障 | debug 日志 | 临时开启 |

## 检查清单（补充）

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| gRPC 延迟 | `crictl info` 计时 | < 1s |
| 镜像拉取 | `crictl pull` 计时 | < 60s |
| 容器创建 | `crictl runp` 计时 | < 5s |
| exec | `crictl exec` | 成功 |
| 日志 | `journalctl -u containerd` | 无 gRPC 错误 |

## 常见问题 FAQ（补充）

| 问题 | 解答 |
|------|------|
| CRI 与 OCI 的关系？ | CRI 定义 K8s 与运行时的接口，OCI 定义容器镜像和运行时规范 |
| crictl 与 kubectl 区别？ | crictl 直接调用 CRI，kubectl 通过 kubelet 间接调用 |
| 如何查看 CRI 版本？ | `crictl version` 或 `crictl info` 查看 runtime 信息 |
| CRI 支持哪些操作？ | 镜像管理、Pod sandbox、容器生命周期、exec/attach/port-forward |
| 如何调试 CRI 调用？ | 设置 containerd 日志级别为 debug，观察 gRPC 调用 |
| streaming 如何实现？ | kubelet 启动 streaming server，CRI 返回 URL 重定向 |
| CRI 性能瓶颈在哪？ | 镜像拉取和容器创建是主要延迟来源 |
| 如何监控 CRI 调用？ | containerd metrics 插件 + Prometheus 拉取 |

## 性能调优参数

| 参数 | 默认值 | 生产建议 | 说明 |
|------|--------|----------|------|
| `max_concurrent_downloads` | 3 | 5-10 | 并行镜像拉取数 |
| `image_pull_progress_timeout` | 1m | 5m | 大镜像拉取超时 |
| `sandbox_image` | pause:3.9 | 与集群一致 | Pod sandbox 基础镜像 |
| `enable_selinux` | false | 按需 | SELinux 环境启用 |
| `max_container_log_line_size` | 16384 | 32768 | 日志行大小限制 |
| `stream_server_address` | 127.0.0.1 | 节点 IP | streaming 服务监听地址 |
| `enable_tls_streaming` | false | 生产启用 | streaming TLS 加密 |
| `stats_collect_period` | 10s | 10-30s | 容器统计采集间隔 |

## 相关文档

- [[容器运行时/containerd-CRI-O/07-containerd-configuration-deep-guide.md|containerd 配置深度指南]]
- [[容器运行时/containerd-CRI-O/09-container-runtime-lifecycle.md|容器运行时生命周期]]
- [[容器运行时/01-containerd-deep-guide.md|containerd 深度指南]]

<!-- risk-assessed -->
