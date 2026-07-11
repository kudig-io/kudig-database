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

## 相关文档

- [[容器运行时/containerd-CRI-O/01-containerd-production-operations.md|containerd 生产运维]]
- [[容器运行时/镜像构建/01-buildkit-production-guide.md|BuildKit 生产指南]]
- [[容器运行时/运行时迁移/01-docker-to-containerd-migration.md|Docker 到 containerd 迁移]]

<!-- risk-assessed -->
