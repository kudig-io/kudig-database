---
title: nerdctl 生产指南
description: nerdctl 作为 docker CLI 替代方案，含 compose、加密镜像、镜像签名与命名空间管理
summary: nerdctl 作为 docker CLI 替代方案，含 compose、加密镜像、镜像签名与命名空间管理
category: container-runtime
tags:
- containerd
- cri
- runtime
- nerdctl
- compose
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

# nerdctl 生产指南

## 概述

`nerdctl` 是 containerd 官方的 Docker 兼容 CLI，命令语法与 `docker` 几乎一致，但直接对接 containerd API（不经过 CRI）。它解决了 K8s 节点上 `docker` 不再可用、而 `ctr` 又过于底层的问题，并额外支持 `compose`、镜像加密（imgcrypt）、签名（cosign/notation）、懒加载（stargz）等现代化能力。

## nerdctl vs ctr vs crictl

| 工具 | 对接 API | 适用场景 | 特性丰富度 |
|---|---|---|---|
| `crictl` | CRI | K8s Pod/容器排障 | 低（仅集群视图） |
| `ctr` | containerd native | 底层镜像/content 管理 | 中 |
| `nerdctl` | containerd native | 开发/运维、Docker 替代 | 高（compose/build） |

`crictl` 只能看到 K8s 创建的资源（k8s.io namespace）；`nerdctl` 可跨 namespace 操作，适合在节点上做 docker 替代。

## 安装

``` bash
# 🟢 只读/安装
# 从官方 release 下载二进制（含 buildkit/compose 支持）
NERDCTL_VERSION=1.7.7
curl -sL https://github.com/containerd/nerdctl/releases/download/v${NERDCTL_VERSION}/nerdctl-${NERDCTL_VERSION}-linux-amd64.tar.gz \
  | sudo tar xz -C /usr/local/bin nerdctl
nerdctl --version
```

## 常用命令（Docker 用户无缝切换）

``` bash
# 🟢 只读 / 🟡 拉取（低风险）
nerdctl -n k8s.io ps -a          # 等价 docker ps（注意 -n 指定 namespace）
nerdctl -n k8s.io images
nerdctl pull nginx:alpine
nerdctl run --rm -it --name test alpine sh
nerdctl logs test
nerdctl exec -it test sh
nerdctl stop test && nerdctl rm test
```

> 提示：K8s 资源在 `k8s.io` namespace；不指定 `-n` 默认 `default`。排障 K8s Pod 时务必加 `-n k8s.io`。

## compose 支持

nerdctl 内置 `nerdctl compose`，兼容 `docker-compose.yaml`，底层调用 containerd + BuildKit + CNI（无需 Docker daemon）。

``` bash
# 🟡 中风险：会创建容器与网络
nerdctl compose -f compose.yaml up -d
nerdctl compose -f compose.yaml ps
nerdctl compose -f compose.yaml down
```

依赖：节点需安装 `buildkit`（nerdctl-full 包已内置）与 CNI 插件（`/opt/cni/bin`）。

## 镜像构建

``` bash
# 🟡 中风险：构建产物
nerdctl build -t myapp:v1 .
nerdctl build --platform linux/amd64,linux/arm64 -t myapp:v1 .   # 多架构
nerdctl build --output type=oci,dest=app.tar .                   # OCI 归档
```

底层默认 BuildKit，支持 BuildKit 前端（`# syntax=`）、`--mount=type=cache` 等高级特性。

## 镜像加密与签名

``` bash
# 加密镜像（imgcrypt），运行时按需解密
nerdctl image encrypt --recipient=jwe:mypub.pem app:v1 app:enc
nerdctl run --key=key.bin --dec-recipient=jwe:mypub.pem app:enc

# 签名（cosign）
cosign sign --key cosign.key registry.example.com/app:v1
nerdctl run --verify=cosign --certificate-identity-regexp='.*' app:v1
```

适用于专有云敏感镜像分发与供应链安全审计。

## 命名空间管理

``` bash
# 🟢 只读
nerdctl namespace list
nerdctl namespace create ci
nerdctl -n ci pull busybox
nerdctl namespace remove ci      # 🟡 namespace 需为空才能删
```

建议：CI/构建与 K8s 工作负载分属不同 namespace（`ci` vs `k8s.io`），避免 GC 误删构建产物。

## 生产注意

- `nerdctl run` 创建的容器**不被 kubelet 管理**，不进 Pod 视图，仅用于节点级运维/构建。
- 在 K8s 节点上慎用 `nerdctl rm -f`，可能误删非 K8s namespace 的调试容器（但不会影响 k8s.io 下的 Pod 容器，Pod 容器由 kubelet 持有）。
- 构建 Pod 优先在专用节点池，避免 buildkit 占用业务节点资源。

## 生产检查清单

- [ ] 安装 nerdctl-full（含 buildkit/compose/CNI）
- [ ] K8s 排障命令统一带 `-n k8s.io`
- [ ] CI 构建使用独立 namespace（如 `ci`）
- [ ] 敏感镜像启用加密/签名流程

## 故障排查

| 问题 | 可能原因 | 诊断命令 | 解决方案 |
|------|----------|----------|----------|
| nerdctl 命令无输出 | 未指定正确 namespace | `nerdctl -n k8s.io ps` | K8s 容器必须加 `-n k8s.io` |
| build 失败 | buildkitd 未启动 | `systemctl status buildkit` | 启动 buildkitd 服务 |
| compose up 报错 | CNI 插件未安装 | `ls /opt/cni/bin/` | 安装 nerdctl-full 或手动部署 CNI |
| 镜像拉取失败 | registry 认证缺失 | `nerdctl login <registry>` | 配置 /etc/containerd/certs.d |
| 容器网络不通 | CNI 配置错误 | `cat /etc/cni/net.d/*.conflist` | 检查 CNI 配置文件 |
| rootless 模式失败 | 内核不支持 | `sysctl kernel.unprivileged_userns_clone` | 设置为 1 或使用 rootful |
| 镜像加密失败 | 缺少密钥 | `nerdctl image ls` | 配置 OCI crypt 插件和密钥 |
| volume 挂载失败 | 权限不足 | `nerdctl volume ls` | 检查 volume 目录权限 |

## nerdctl 与 docker 命令对比

| 操作 | docker | nerdctl | 差异说明 |
|------|--------|---------|----------|
| 运行容器 | `docker run` | `nerdctl run` | 基本一致 |
| 构建镜像 | `docker build` | `nerdctl build` | 使用 BuildKit |
| 查看容器 | `docker ps` | `nerdctl ps` | 需指定 namespace |
| 镜像列表 | `docker images` | `nerdctl images` | 基本一致 |
| 网络管理 | `docker network` | `nerdctl network` | 使用 CNI |
| 卷管理 | `docker volume` | `nerdctl volume` | 基本一致 |
| compose | `docker compose` | `nerdctl compose` | 兼容 compose-spec |
| 登录 | `docker login` | `nerdctl login` | 基本一致 |

## 生产最佳实践

| 维度 | 建议 | 说明 |
|------|------|------|
| namespace | K8s 排障统一用 `-n k8s.io` | 避免看不到 K8s 容器 |
| CI 构建 | 使用独立 namespace（如 ci） | 与 K8s 容器隔离 |
| 安装 | 使用 nerdctl-full 包 | 含 buildkit/compose/CNI |
| 安全 | 敏感镜像启用加密/签名 | cosign + notation |
| 网络 | 生产用 bridge 网络 | 避免 host 网络的安全风险 |
| 存储 | 使用命名 volume | 避免 bind mount 权限问题 |
| 日志 | 配置日志轮转 | 避免磁盘占满 |
| 更新 | 随 containerd 一起升级 | 保持版本兼容 |

## 相关工具

| 工具 | 用途 | 安装/使用 |
|------|------|----------|
| nerdctl | Docker 兼容 CLI | 随 nerdctl-full 安装 |
| buildkitd | 镜像构建引擎 | 随 nerdctl-full 安装 |
| CNI plugins | 容器网络 | 随 nerdctl-full 安装 |
| cosign | 镜像签名 | `go install github.com/sigstore/cosign/v2/cmd/cosign@latest` |
| notation | 镜像签名(替代) | `brew install notation` |
| crane | 镜像操作 | `go install github.com/google/go-containerregistry/cmd/crane@latest` |

## 常见问题 FAQ

| 问题 | 解答 |
|------|------|
| nerdctl 和 ctr 的区别？ | nerdctl 兼容 Docker CLI，ctr 是 containerd 原生 CLI |
| 为什么看不到 K8s 容器？ | 需加 `-n k8s.io` 参数 |
| nerdctl 能否替代 Docker？ | 可以，生产环境推荐迁移到 nerdctl |
| rootless 模式有何限制？ | 部分网络功能受限，性能略低 |
| 如何配置镜像加速？ | 编辑 /etc/containerd/certs.d/ 配置 mirror |
| compose 兼容性如何？ | 完全兼容 compose-spec |
| 如何启用 GPU 支持？ | 安装 nvidia-container-toolkit |
| 如何迁移 docker-compose 项目？ | 直接使用 `nerdctl compose up` |

## nerdctl 配置示例

```toml
# /etc/nerdctl/nerdctl.toml
namespace = "default"
snapshotter = "overlayfs"
cgroup_manager = "systemd"
data_root = "/var/lib/nerdctl"

# 镜像构建配置
[build]
  builder = "default"
  cache_from = ["type=registry,ref=registry.internal/cache"]
  cache_to = ["type=registry,ref=registry.internal/cache,mode=max"]

# Registry 配置
[registry]
  insecure = false
  [registry.mirrors."docker.io"]
    endpoint = ["https://mirror.internal:5000"]
```

## 性能调优

| 场景 | 优化方向 | 具体操作 |
|------|----------|----------|
| 构建速度慢 | 启用 BuildKit 缓存 | `nerdctl build --cache-from` |
| 拉取速度慢 | 配置 mirror | 编辑 certs.d 目录 |
| 容器启动慢 | 减少镜像层数 | 优化 Dockerfile |
| 网络性能差 | 使用 host 网络 | 仅安全场景使用 |
| 存储性能差 | 使用 SSD | 将数据目录挂载到 SSD |
| 并发构建 | 多 builder 实例 | 配置 buildkitd 并发数 |

## 版本兼容性

| nerdctl 版本 | containerd 版本 | BuildKit 版本 | 说明 |
|-------------|----------------|--------------|------|
| 1.7.x | 1.7.x | 0.12.x | 稳定版 |
| 2.0.x | 2.0.x | 0.13.x | 新架构 |
| 1.6.x | 1.6.x | 0.11.x | 旧版兼容 |

## 监控指标

| 指标 | 含义 | 告警阈值 |
|------|------|----------|
| container_count | 容器总数 | > 节点容量 80% |
| image_count | 镜像总数 | > 500 |
| build_duration | 构建耗时 | P99 > 10min |
| pull_errors | 拉取失败次数 | > 5/min |
| disk_usage | 磁盘使用率 | > 80% |

## 安全加固

| 维度 | 建议 | 说明 |
|------|------|------|
| 镜像签名 | 启用 cosign/notation | 供应链验证 |
| 网络 | 生产用 bridge | 避免 host 网络 |
| 存储 | 命名 volume | 避免 bind mount 权限问题 |
| 日志 | 配置轮转 | 避免磁盘占满 |
| 更新 | 随 containerd 升级 | 保持版本兼容 |
| 权限 | 限制 socket 访问 | 仅授权用户 |

## 迁移指南

| 从 | 到 | 关键步骤 |
|------|------|----------|
| docker | nerdctl | 安装 nerdctl-full→替换命令→验证 |
| docker-compose | nerdctl compose | 直接使用，兼容 compose-spec |
| docker build | nerdctl build | 使用 BuildKit，基本一致 |
| docker network | nerdctl network | 使用 CNI，配置略有不同 |

## 检查清单

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| 版本 | `nerdctl --version` | 与 containerd 匹配 |
| namespace | `nerdctl -n k8s.io ps` | 可见 K8s 容器 |
| buildkit | `systemctl status buildkit` | active |
| CNI | `ls /opt/cni/bin/` | 插件齐全 |
| 网络 | `nerdctl network ls` | bridge 存在 |
| 构建 | `nerdctl build .` | 成功 |

## 架构对比

```text
nerdctl 架构层次：

nerdctl CLI
  └── containerd API (gRPC)
       ├── CRI 插件 (K8s 容器)
       ├── 默认 namespace (nerdctl 容器)
       └── 自定义 namespace (CI 构建)

与 Docker 对比：
docker CLI → dockerd → containerd → runc
nerdctl CLI → containerd → runc (更短路径)
```

## 容量规划

| 场景 | 建议配置 | 说明 |
|------|----------|------|
| 开发机 | 默认配置 | 足够日常使用 |
| CI 节点 | 独立 namespace + SSD | 构建性能 |
| K8s 节点 | -n k8s.io | 排障用 |
| 生产服务器 | rootful + bridge | 安全和性能 |

## 版本历史

| 版本 | 时间 | 关键变化 |
|------|------|----------|
| nerdctl 0.1 | 2021 | 初始发布 |
| nerdctl 1.0 | 2022 | 生产稳定 |
| nerdctl 1.7 | 2023 | compose 完善 |
| nerdctl 2.0 | 2024 | containerd 2.0 支持 |

## 常见问题 FAQ（补充）

| 问题 | 解答 |
|------|------|
| nerdctl 与 docker CLI 兼容性如何？ | 命令语法基本一致，但部分 flag 如 `--link` 不支持 |
| 如何查看容器日志？ | `nerdctl logs -f <container>`，与 docker 一致 |
| compose 文件兼容性？ | 支持 Compose Spec，但部分 Docker 特有字段不支持 |
| 如何配置镜像加速？ | 修改 `/etc/containerd/certs.d/` 下的 hosts.toml |
| rootless 模式有何限制？ | 无法绑定 < 1024 端口，网络性能略低 |
| 如何导出/导入镜像？ | `nerdctl save/load`，与 docker 格式兼容 |
| 与 crictl 区别？ | nerdctl 面向开发者，crictl 面向 K8s 运维 |
| 如何清理无用镜像？ | `nerdctl image prune` 或 `nerdctl system prune` |
| 支持哪些网络驱动？ | bridge、host、none、macvlan、ipvlan |
| 如何调试网络问题？ | `nerdctl network inspect` + `ip netns exec` |

## 生产部署建议

| 场景 | 推荐配置 | 注意事项 |
|------|----------|----------|
| 开发环境 | rootless + bridge | 安全性优先，无需 root |
| CI/CD | rootful + host 网络 | 性能优先，配合 BuildKit |
| 生产节点 | 仅用 crictl | 避免 nerdctl 直接操作生产容器 |
| 镜像构建 | nerdctl build + BuildKit | 支持多阶段、缓存导出 |
| 多节点测试 | nerdctl compose | 本地模拟多服务编排 |
| 安全测试 | rootless + gVisor | 隔离测试环境 |

## 性能调优参数

| 参数 | 默认值 | 生产建议 | 说明 |
|------|--------|----------|------|
| `--cpus` | 无限制 | 根据服务设置 | CPU 核数限制 |
| `--memory` | 无限制 | 必须设置 | 内存上限 |
| `--pids-limit` | 无限制 | 1024 | 防止 fork 炸弹 |
| `--ulimit nofile` | 1024 | 65535 | 高并发服务必须调大 |
| `--oom-score-adj` | 0 | -500 | 重要服务降低 OOM 优先级 |

## 相关文档

- [[容器运行时/containerd-CRI-O/01-containerd-production-operations.md|containerd 生产运维]]
- [[容器运行时/镜像构建/01-buildkit-production-guide.md|BuildKit 生产指南]]
- [[容器运行时/运行时迁移/01-docker-to-containerd-migration.md|Docker 到 containerd 迁移]]

<!-- risk-assessed -->
