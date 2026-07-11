---
title: Distroless 与极简镜像最佳实践
description: Distroless/scratch 镜像减体积、降攻击面，含静态二进制、调试变体与非 root 配置
summary: Distroless/scratch 镜像减体积、降攻击面，含静态二进制、调试变体与非 root 配置
category: container-runtime
tags:
- containerd
- cri
- runtime
- distroless
- security
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

# Distroless 与极简镜像最佳实践

## 概述

传统镜像基于完整 Linux 发行版（Debian/Alpine），携带 shell、包管理器、curl 等工具——这些既是体积，也是攻击面。**Distroless** 镜像只包含运行时（如 JRE、glibc）与应用本身，没有 shell、没有包管理器，显著降低体积与 CVE 面积。**scratch** 更进一步，空镜像，适合静态编译二进制。

## 体积与攻击面对比

| 基础镜像 | 体积 | Shell | 包管理器 | 典型 CVE 数 |
|---|---|---|---|---|
| ubuntu:24.04 | ~78MB | 有 | apt | 多 |
| alpine:3.20 | ~7MB | 有 | apk | 中（musl 兼容问题） |
| distroless/static | ~2MB | 无 | 无 | 极少 |
| distroless/java21 | ~200MB | 无 | 无 | 少 |
| scratch | 0 | 无 | 无 | 0（仅二进制自身） |

> 攻击者拿到 RCE 后通常 `sh` 或 `curl` 横向——distroless 让这两步都不可用。

## Go 静态二进制 + scratch

```dockerfile
FROM golang:1.22 AS build
WORKDIR /src
COPY . .
RUN CGO_ENABLED=0 go build -ldflags="-s -w" -o /app ./cmd/server

FROM scratch
COPY --from=build /app /app
# 必须带 CA 证书，否则 HTTPS 失败
COPY --from=build /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/
# distroless/scratch 需显式 EXPOSE 与 USER
EXPOSE 8080
ENTRYPOINT ["/app"]
```

``` bash
# 🟢 只读：验证最终镜像体积
docker images registry.example.com/app:v1
# 预期 < 20MB
```

## Distroless 变体

| 变体 | 内容 |
|---|---|
| `gcr.io/distroless/static` | 空 + CA 证书 + tzdata（静态二进制） |
| `gcr.io/distroless/base` | + glibc（C 动态链接程序） |
| `gcr.io/distroless/java21` | + JRE |
| `gcr.io/distroless/nodejs20` | + Node.js |
| `...:nonroot` | 以非 root（uid 65532）运行 |
| `...:debug` | 含 busybox shell，用于排障 |

生产用 `:nonroot`，排障临时换 `:debug`。

## 非 root 运行

```dockerfile
FROM gcr.io/distroless/static:nonroot
COPY --chown=nonroot:nonroot app /app
USER nonroot:nonroot
ENTRYPOINT ["/app"]
```

``` yaml
# Pod 层兜底（即使镜像忘设 user）
spec:
  securityContext:
    runAsNonRoot: true
    runAsUser: 65532
    readOnlyRootFilesystem: true
```

## 排障：distroless 没 shell 怎么办

1. 用 `:debug` 变体（含 `sh`）临时构建，`kubectl exec -it` 进去。
2. 用 `ephemeral debug container` 注入带 shell 的 sidecar。
3. 在节点上 `crictl inspect <c>` + `nsenter` 直接进入容器 namespace。

``` bash
# 🟢 只读：节点级进入 distroless 容器 namespace
crictl inspect <container-id> | jq '.info.pid'
sudo nsenter -t <pid> -m -u -i -n -p -- /busybox sh
```

## 与 SBOM / 漏洞扫描结合

distroless 层少、依赖清晰，Trivy/Syft 扫描更快更准，建议 CI 强制：

``` bash
# 🟢 只读
syft registry.example.com/app:v1 -o cyclonedx-json > sbom.json
trivy image --severity HIGH,CRITICAL --exit-code 1 registry.example.com/app:v1
```

## 生产检查清单

- [ ] 基础镜像切换为 distroless 或 scratch
- [ ] 二进制静态编译（`CGO_ENABLED=0`）+ `-ldflags="-s -w"`
- [ ] scratch 镜像携带 CA 证书
- [ ] 以 nonroot（uid 65532）运行，`readOnlyRootFilesystem: true`
- [ ] CI 强制 Trivy 扫描，HIGH/CRITICAL 阻断发布

## 相关文档

- [[容器运行时/镜像构建/04-multi-arch-build-guide.md|多架构构建指南]]
- [[容器运行时/镜像构建/06-image-layer-optimization.md|镜像层优化]]
- [[容器运行时/镜像构建/03-kaniko-ko-build-guide.md|Kaniko 与 ko（ko 默认 distroless）]]

<!-- risk-assessed -->
