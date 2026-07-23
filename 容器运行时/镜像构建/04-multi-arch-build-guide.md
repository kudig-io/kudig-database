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

## 故障排查

| 问题 | 可能原因 | 诊断命令 | 解决方案 |
|------|----------|----------|----------|
| QEMU 构建极慢 | 模拟执行性能低 | `docker buildx ls` | 使用原生多节点 builder |
| manifest 缺少架构 | 构建未指定 platform | `docker manifest inspect <image>` | 确认 --platform 参数 |
| arm64 构建失败 | 基础镜像不支持 | `docker pull --platform linux/arm64 <base>` | 确认 base image 支持目标架构 |
| buildx 未安装 | Docker 版本过旧 | `docker buildx version` | 升级 Docker 或安装 buildx 插件 |
| 缓存未命中 | 架构不同缓存不共享 | `docker buildx build --cache-from` | 使用 per-platform 缓存 |
| 交叉编译失败 | CGO 依赖 | `CGO_ENABLED=0 go build` | 禁用 CGO 或使用交叉工具链 |
| 推送失败 | registry 不支持 manifest list | `crane manifest <image>` | 升级 registry 或使用 OCI index |
| 运行时架构不匹配 | 拉取了错误架构 | `uname -m` vs `docker inspect` | 确认节点架构与镜像匹配 |

## 多架构构建方式对比

| 方式 | 性能 | 复杂度 | 适用场景 |
|------|------|--------|----------|
| QEMU 模拟 | 低 | 低 | 开发/小规模 CI |
| 原生多节点 builder | 高 | 中 | 大规模 CI/CD |
| 交叉编译 | 高 | 中 | Go/Rust 静态二进制 |
| 远程 builder | 高 | 高 | 企业级多区域构建 |

## 生产最佳实践

| 维度 | 建议 | 说明 |
|------|------|------|
| 构建 | 大规模 CI 使用原生多节点 builder | 避免 QEMU 性能瓶颈 |
| 缓存 | 启用 BuildKit cache-from/to | 加速重复构建 |
| 基础镜像 | 确认 base image 支持目标架构 | alpine/debian 均支持 |
| 验证 | 构建后验证 manifest 包含所有架构 | `imagetools inspect` |
| 标签 | 添加 OCI 架构标签 | 便于自动化管理 |
| 测试 | 每个架构独立测试 | 避免架构特定 bug |
| 发布 | 使用 manifest list 或 OCI index | 自动拉取对应架构 |
| 监控 | 监控构建时间和失败率 | 按架构分别统计 |

## 相关工具

| 工具 | 用途 | 安装/使用 |
|------|------|----------|
| docker buildx | 多架构构建 | Docker 内置插件 |
| crane | manifest 检查 | `go install github.com/google/go-containerregistry/cmd/crane@latest` |
| regctl | registry 操作 | `brew install regclient` |
| imagetools | 镜像工具 | `docker buildx imagetools inspect` |
| ko | Go 多架构构建 | `go install github.com/google/ko@latest` |
| buildctl | BuildKit CLI | 随 BuildKit 安装 |

## 常见问题 FAQ

| 问题 | 解答 |
|------|------|
| 如何查看镜像支持的架构？ | `docker manifest inspect <image>` 或 `crane manifest` |
| QEMU 和原生构建如何选择？ | 小规模用 QEMU，大规模 CI 用原生 |
| Go 交叉编译需要 QEMU 吗？ | 不需要，CGO_ENABLED=0 + GOARCH 即可 |
| manifest list 和 OCI index 的区别？ | 功能相同，OCI index 是 OCI 标准 |
| 如何添加新架构支持？ | 在 --platform 中添加新架构，确认 base 支持 |
| 构建缓存如何跨架构共享？ | 不能共享，每个架构独立缓存 |
| Apple Silicon 如何构建 amd64？ | `docker buildx build --platform linux/amd64` |
| 如何自动化多架构发布？ | CI 中 buildx + manifest push + tag |

## 多架构构建配置示例

```yaml
# GitHub Actions 多架构构建示例
name: Multi-arch Build
on: [push]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Set up QEMU
        uses: docker/setup-qemu-action@v3
      - name: Set up Docker Buildx
        uses: docker/setup-buildx-action@v3
      - name: Build and push
        uses: docker/build-push-action@v5
        with:
          context: .
          platforms: linux/amd64,linux/arm64
          push: true
          tags: registry.internal/app:${{ github.sha }}
          cache-from: type=registry,ref=registry.internal/app:cache
          cache-to: type=registry,ref=registry.internal/app:cache,mode=max
```

## 性能调优

| 场景 | 优化方向 | 具体操作 |
|------|----------|----------|
| QEMU 构建慢 | 原生节点 | 使用 ARM 原生 runner |
| 缓存未命中 | per-platform 缓存 | 每个架构独立缓存 key |
| 推送失败 | 重试机制 | CI 配置 retry |
| 基础镜像不支持 | 多架构 base | 确认 base 支持目标架构 |
| 交叉编译失败 | 禁用 CGO | CGO_ENABLED=0 |
| 构建超时 | 并行构建 | 多节点 builder |

## 监控指标

| 指标 | 含义 | 告警阈值 |
|------|------|----------|
| build_duration_seconds | 构建耗时 | P99 > 15min |
| build_failure_rate | 构建失败率 | > 5% |
| image_size_bytes | 镜像体积 | 异常增长 > 20% |
| push_duration_seconds | 推送耗时 | P99 > 5min |
| cache_hit_rate | 缓存命中率 | < 50% |

## 安全加固

| 维度 | 建议 | 说明 |
|------|------|------|
| 基础镜像 | 使用官方多架构镜像 | 避免第三方不可信源 |
| 构建环境 | 隔离构建环境 | 避免供应链攻击 |
| 签名 | 构建后签名 | cosign sign |
| 扫描 | 每个架构独立扫描 | Trivy per-platform |
| 来源 | 记录构建元数据 | SLSA provenance |

## 迁移指南

| 从 | 到 | 关键步骤 |
|------|------|----------|
| 单架构 | 多架构 | 添加 --platform 参数 |
| QEMU | 原生 builder | 配置多节点 builder |
| docker build | buildx | 安装 buildx 插件 |
| 手动 manifest | 自动 | CI 中 buildx --push |

## 检查清单

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| 架构支持 | `docker manifest inspect <image>` | 包含所有目标架构 |
| buildx | `docker buildx version` | 已安装 |
| builder | `docker buildx ls` | 包含目标架构 |
| 构建 | `docker buildx build --platform linux/amd64,linux/arm64 .` | 成功 |
| 推送 | `docker buildx build --push` | 成功 |
| 验证 | `crane manifest <image>` | 多架构 |

## 版本历史

| 版本 | 时间 | 关键变化 |
|------|------|----------|
| Docker 18.09 | 2018 | buildx 实验性 |
| Docker 20.10 | 2020 | buildx 稳定 |
| BuildKit 0.12 | 2023 | 多平台缓存优化 |
| Docker 25.0 | 2024 | buildx 默认 |

## 架构对比

```text
多架构构建流程：

QEMU 模拟：
  buildx → QEMU → 模拟执行 → 镜像
  优点：简单，无需额外硬件
  缺点：慢（5-10x）

原生多节点：
  buildx → node1 (amd64) → 镜像
         → node2 (arm64) → 镜像
         → manifest merge
  优点：快，原生性能
  缺点：需要多架构节点

交叉编译：
  buildx → CGO_ENABLED=0 GOARCH=arm64 → 镜像
  优点：最快，无需模拟
  缺点：仅限静态编译语言
```

## 容量规划

| 场景 | 建议配置 | 说明 |
|------|----------|------|
| 开发 | QEMU | 简单 |
| CI | 原生 builder | 性能 |
| 企业 | 多区域 builder | 就近构建 |
| Go 项目 | 交叉编译 | 最快 |

## 检查清单（补充）

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| QEMU | `docker run --rm --privileged multiarch/qemu-user-static --reset` | 成功 |
| builder | `docker buildx ls` | 包含目标架构 |
| 构建 | `docker buildx build --platform linux/amd64,linux/arm64 .` | 成功 |
| 验证 | `crane manifest <image>` | 多架构 |

## 常见问题 FAQ（补充）

| 问题 | 解答 |
|------|------|
| QEMU 模拟性能如何？ | 比原生慢 5-10x，建议用原生节点或远程 builder |
| 如何验证多架构 manifest？ | `docker manifest inspect` 或 `crane manifest` |
| buildx 与 docker build 区别？ | buildx 支持多平台、缓存导出、远程 builder |
| 如何处理架构特定依赖？ | 使用 `TARGETARCH` 构建参数条件化安装 |
| CI 中如何加速多架构构建？ | 使用原生 ARM 节点 + buildx 远程 builder |
| manifest list 与 image index 区别？ | 同一概念，OCI 规范称 image index |

## 相关文档

- [[容器运行时/镜像构建/01-buildkit-production-guide.md|BuildKit 生产指南]]
- [[容器运行时/镜像构建/05-distroless-minimal-images.md|Distroless 极简镜像]]
- [[容器运行时/镜像构建/03-kaniko-ko-build-guide.md|Kaniko 与 ko 指南]]

<!-- risk-assessed -->
