---
title: buildx 多架构构建指南
description: 使用 buildx/buildkit 构建 amd64/arm64 多架构镜像，含 QEMU、cross-build、ACR 推送与 CI 模式
summary: 使用 buildx/buildkit 构建 amd64/arm64 多架构镜像，含 QEMU、cross-build、ACR 推送与CI 模式
category: container-runtime
tags:
- containerd
- cri
- runtime
- buildx
- multi-arch
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

# buildx 多架构构建指南

## 概述

ARM（倚天/Graviton）节点普及后，镜像必须同时包含 `linux/amd64` 与 `linux/arm64` 架构。`docker buildx` 基于 BuildKit 提供多架构构建能力，产出符合 OCI image index 的 manifest list，运行时按节点架构自动选择正确镜像。

## 三种构建路径

| 方式 | 原理 | 速度 | 适用 |
|---|---|---|---|
| QEMU 用户态模拟 | binfmt_misc 注册交叉 ISA | 慢（5-10x） | 本地/CI 无 ARM 机器 |
| 原生多节点 builder | 多架构 builder 节点池 | 最快 | 大规模生产 CI |
| Cross-build（cross-toolchain） | 交叉编译工具链 | 快 | Go/Rust 静态二进制 |

## 启用 QEMU（无 ARM 节点时）

``` bash
# 🟢 只读/注册（一次性，需内核支持 binfmt_misc）
docker run --privileged --rm tonistiigi/binfmt --install all
docker buildx ls
```

## 创建多架构 builder

``` bash
# 🟡 中风险：创建本地 builder 实例
docker buildx create --name multiarch \
  --driver docker-container \
  --platform linux/amd64,linux/arm64 \
  --use
docker buildx inspect --bootstrap
```

## 构建并推送多架构镜像

``` bash
# 🟡 中风险：构建并推送（必须 --push 才能产出 manifest list）
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t registry.cn-hangzhou.aliyuncs.com/demo/app:v1.2.3 \
  --push .
```

> 仅 `--load` 不支持多架构（本地 docker 一次只能装一个架构），多架构必须 `--push` 到 registry。

## 验证 manifest list

``` bash
# 🟢 只读
docker buildx imagetools inspect \
  registry.cn-hangzhou.aliyuncs.com/demo/app:v1.2.3
# 应同时列出 amd64 与 arm64 manifest
crane manifest registry.cn-hangzhou.aliyuncs.com/demo/app:v1.2.3 | jq .
```

## Dockerfile 多架构要点

```dockerfile
# 用 TARGETARCH 自动选择基础镜像（BuildKit 内置变量）
FROM --platform=$BUILDPLATFORM golang:1.22 AS build
ARG TARGETOS TARGETARCH
WORKDIR /src
COPY . .
RUN CGO_ENABLED=0 GOOS=$TARGETOS GOARCH=$TARGETARCH \
    go build -ldflags="-s -w" -o /app ./cmd/server

FROM --platform=$TARGETPLATFORM alpine:3.20
COPY --from=build /app /usr/local/bin/app
ENTRYPOINT ["app"]
```

关键 BuildKit 变量：

| 变量 | 含义 |
|---|---|
| `BUILDPLATFORM` | 构建机架构（执行 RUN 的环境） |
| `TARGETPLATFORM` | 目标架构（产物） |
| `TARGETOS` / `TARGETARCH` | 拆分值，如 `linux` / `arm64` |

`FROM --platform=$BUILDPLATFORM` 让编译阶段在构建机原生架构跑（用 QEMU 反而慢），最终镜像层才切到 `TARGETPLATFORM`。

## 原生多节点 builder（生产 CI）

``` bash
# 🟡 中风险：把 amd64 与 arm64 构建机加入同一 builder
docker buildx create --name prod-builder --driver remote \
  tcp://build-amd64.internal:2376
docker buildx create --name prod-builder --append --driver remote \
  tcp://build-arm64.internal:2376
```

BuildKit 自动把不同平台的 stage 分发到对应节点，规避 QEMU 性能损耗。ACK 倚天 + x86 混部 CI 推荐此模式。

## CI（GitHub Actions）示例

```yaml
jobs:
  multiarch:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: docker/setup-qemu-action@v3
      - uses: docker/setup-buildx-action@v3
      - uses: docker/login-action@v3
        with:
          registry: registry.cn-hangzhou.aliyuncs.com
          username: ${{ secrets.ACR_USER }}
          password: ${{ secrets.ACR_PASS }}
      - uses: docker/build-push-action@v5
        with:
          platforms: linux/amd64,linux/arm64
          push: true
          tags: registry.cn-hangzhou.aliyuncs.com/demo/app:${{ github.sha }}
          cache-from: type=registry,ref=demo/app:cache
          cache-to: type=registry,ref=demo/app:cache,mode=max
```

## 生产检查清单

- [ ] 镜像 manifest 同时含 amd64 与 arm64（`imagetools inspect` 验证）
- [ ] 大规模 CI 使用原生多节点 builder，避免 QEMU 瓶颈
- [ ] 启用 BuildKit `cache-from/to` 加速重复构建
- [ ] 多架构 base image 自身支持目标架构（如 alpine/arm64）

## 相关文档

- [[容器运行时/镜像构建/01-buildkit-production-guide.md|BuildKit 生产指南]]
- [[容器运行时/镜像构建/05-distroless-minimal-images.md|Distroless 极简镜像]]
- [[容器运行时/镜像构建/03-kaniko-ko-build-guide.md|Kaniko 与 ko 指南]]

<!-- risk-assessed -->
