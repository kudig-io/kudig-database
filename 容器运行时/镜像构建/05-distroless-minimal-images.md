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

## 故障排查

| 问题 | 可能原因 | 诊断命令 | 解决方案 |
|------|----------|----------|----------|
| 容器启动即崩溃 | 缺少动态链接库 | `ldd /app/binary` | 确保静态编译 CGO_ENABLED=0 |
| TLS 连接失败 | 缺少 CA 证书 | `openssl s_client -connect host:443` | 复制 ca-certificates 到镜像 |
| 无法 exec 调试 | distroless 无 shell | `kubectl debug -it <pod> --image=busybox` | 使用 ephemeral container 调试 |
| 镜像扫描报漏洞 | 基础镜像过旧 | `trivy image <image>` | 更新 gcr.io/distroless 基础镜像 |
| 二进制无法执行 | 架构不匹配 | `file /app/binary` | 确认构建架构与运行架构一致 |
| 权限拒绝 | 以 root 运行但文件权限不对 | `crictl inspect <id>` | 设置 USER 65532 并调整文件权限 |
| 镜像过大 | 未使用多阶段构建 | `docker history <image>` | 使用 multi-stage build |
| DNS 解析失败 | 缺少 /etc/resolv.conf | `nslookup google.com` | 确认 K8s DNS 配置正确 |

## Distroless 基础镜像对比

| 基础镜像 | 大小 | 包含内容 | 适用场景 |
|----------|------|----------|----------|
| gcr.io/distroless/static | ~2MB | CA证书+时区数据 | Go 静态二进制 |
| gcr.io/distroless/base | ~20MB | +glibc+libssl | 需要 glibc 的应用 |
| gcr.io/distroless/cc | ~30MB | +libgcc | C/C++ 应用 |
| gcr.io/distroless/python3 | ~50MB | Python 3 运行时 | Python 应用 |
| gcr.io/distroless/java | ~100MB | JRE | Java 应用 |
| gcr.io/distroless/nodejs | ~80MB | Node.js 运行时 | Node.js 应用 |
| scratch | 0MB | 无任何内容 | 完全静态链接的二进制 |

## 生产最佳实践

| 维度 | 建议 | 说明 |
|------|------|------|
| 编译 | CGO_ENABLED=0 静态编译 | 避免动态库依赖 |
| 用户 | 以 nonroot (uid 65532) 运行 | 最小权限原则 |
| 文件系统 | readOnlyRootFilesystem: true | 防止容器内写入 |
| 扫描 | CI 强制 Trivy 扫描 | HIGH/CRITICAL 阻断发布 |
| 证书 | scratch 镜像携带 CA 证书 | 从 builder 阶段复制 |
| 时区 | 复制 /usr/share/zoneinfo | 确保日志时间正确 |
| 调试 | 使用 kubectl debug | 不要在生产镜像中加 shell |
| 更新 | 定期更新 distroless 基础镜像 | 修复已知漏洞 |

## 多阶段构建示例

```dockerfile
# 构建阶段
FROM golang:1.22-alpine AS builder
WORKDIR /app
COPY go.mod go.sum ./
RUN go mod download
COPY . .
RUN CGO_ENABLED=0 GOOS=linux go build -ldflags="-s -w" -o /app/server .

# 运行阶段
FROM gcr.io/distroless/static-debian12:nonroot
COPY --from=builder /app/server /server
COPY --from=builder /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/
EXPOSE 8080
USER 65532:65532
ENTRYPOINT ["/server"]
```

## 相关工具

| 工具 | 用途 | 安装/使用 |
|------|------|----------|
| Trivy | 镜像漏洞扫描 | `brew install trivy` |
| Grype | 替代扫描工具 | `brew install grype` |
| ko | Go 专用 distroless 构建 | `go install github.com/google/ko@latest` |
| crane | 镜像操作工具 | `go install github.com/google/go-containerregistry/cmd/crane@latest` |
| docker-slim | 自动精简镜像 | `brew install docker-slim` |
| dive | 镜像层分析 | `brew install dive` |

## 常见问题 FAQ

| 问题 | 解答 |
|------|------|
| distroless 和 alpine 如何选择？ | 安全优先选 distroless，需要 shell 调试选 alpine |
| 如何调试 distroless 容器？ | `kubectl debug` 或构建时加入 debug 变体 |
| scratch 和 distroless 的区别？ | scratch 完全空，distroless 包含 CA 证书和时区数据 |
| 如何减小 Go 镜像体积？ | CGO_ENABLED=0 + -ldflags="-s -w" + UPX 压缩 |
| distroless 镜像如何更新？ | 重新构建并推送，使用 :latest 或固定 digest |
| 能否在 distroless 中安装软件？ | 不能，无包管理器，所有依赖必须在构建时包含 |
| 如何确认镜像是否真正无 shell？ | `crane export <image> - | tar -t | grep -E "sh|bash"` |
| nonroot 变体的作用？ | 默认以 uid 65532 运行，无需额外配置 USER |

## 相关文档

- [[容器运行时/镜像构建/04-multi-arch-build-guide.md|多架构构建指南]]
- [[容器运行时/镜像构建/06-image-layer-optimization.md|镜像层优化]]
- [[容器运行时/镜像构建/03-kaniko-ko-build-guide.md|Kaniko 与 ko（ko 默认 distroless）]]

<!-- risk-assessed -->
