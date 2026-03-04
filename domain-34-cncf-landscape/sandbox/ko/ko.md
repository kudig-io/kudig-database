# ko

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://ko.build/ |
| **GitHub** | https://github.com/ko-build/ko |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

ko 是一个快速的 Go 应用容器镜像构建和部署工具。它无需 Docker 或 Dockerfile，直接从 Go 源码构建 OCI 兼容的容器镜像，并推送到容器注册表。ko 的核心理念是简化 Go 应用的容器化流程，实现极速构建和部署。

### 核心特性

- **无需 Docker**: 不依赖 Docker daemon，直接构建 OCI 镜像
- **极速构建**: 利用 Go 的增量编译，镜像构建通常在秒级完成
- **最小镜像**: 基于 distroless 基础镜像，极小攻击面
- **YAML 集成**: 自动替换 YAML 中的 Go import path 为镜像引用
- **多平台构建**: 支持 linux/amd64, linux/arm64 等多平台镜像
- **SBOM 生成**: 自动生成 SPDX 格式的 SBOM（软件物料清单）
- **Sigstore 签名**: 集成 cosign 进行镜像签名

---

## 快速开始

### 安装

```bash
# macOS
brew install ko

# Go install
go install github.com/google/ko@latest

# 设置目标仓库
export KO_DOCKER_REPO=registry.example.com/my-project
```

### 构建和推送镜像

```bash
# 构建并推送单个 Go 应用
ko build ./cmd/myapp

# 构建本地镜像（不推送）
ko build ./cmd/myapp --local

# 多平台构建
ko build ./cmd/myapp --platform=linux/amd64,linux/arm64

# 指定基础镜像
ko build ./cmd/myapp --base-import-paths
```

### 与 Kubernetes 集成

```bash
# 自动替换 YAML 中的镜像引用并 apply
ko apply -f deployment.yaml

# 解析 YAML 但不 apply（预览）
ko resolve -f deployment.yaml

# 删除
ko delete -f deployment.yaml
```

```yaml
# deployment.yaml - 使用 Go import path 作为镜像引用
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
spec:
  replicas: 3
  selector:
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
    spec:
      containers:
        - name: myapp
          # ko 会自动替换为构建后的镜像 digest
          image: ko://github.com/my-org/my-project/cmd/myapp
          ports:
            - containerPort: 8080
```

---

## 配置详解

### .ko.yaml 配置

```yaml
# .ko.yaml
defaultBaseImage: cgr.dev/chainguard/static:latest

baseImageOverrides:
  github.com/my-org/my-project/cmd/myapp: cgr.dev/chainguard/go:latest
  github.com/my-org/my-project/cmd/worker: gcr.io/distroless/base:nonroot

builds:
  - id: myapp
    dir: ./cmd/myapp
    main: .
    env:
      - CGO_ENABLED=0
    flags:
      - -trimpath
    ldflags:
      - -s -w
      - -X main.version={{.Env.VERSION}}
```

### 环境变量

| 变量 | 说明 |
|:---|:---|
| `KO_DOCKER_REPO` | 目标容器注册表地址 |
| `KO_DEFAULTBASEIMAGE` | 默认基础镜像 |
| `GOFLAGS` | 传递给 `go build` 的标志 |
| `KO_CONFIG_PATH` | .ko.yaml 配置文件路径 |

### CI/CD 集成

```yaml
# GitHub Actions
name: Build and Deploy
on: [push]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-go@v5
        with:
          go-version: '1.22'
      - uses: ko-build/setup-ko@v0.7
      - run: ko build ./cmd/myapp --platform=linux/amd64,linux/arm64
        env:
          KO_DOCKER_REPO: ghcr.io/${{ github.repository }}
```

---

## 高级功能

### SBOM 和签名

```bash
# 构建并生成 SBOM
ko build ./cmd/myapp --sbom=spdx

# 使用 cosign 签名
ko build ./cmd/myapp --image-refs=image-refs.txt
cosign sign $(cat image-refs.txt)
```

### 静态资源打包

```yaml
# .ko.yaml - 包含静态文件
builds:
  - id: web-server
    dir: ./cmd/web
    main: .
    # ko 不直接支持 COPY，使用 Go embed 替代
```

```go
// 使用 Go embed 嵌入静态文件
import "embed"

//go:embed static/*
var staticFiles embed.FS
```

---

## 与其他构建工具对比

| 特性 | ko | Docker Build | Buildpacks | Jib |
|:---|:---|:---|:---|:---|
| **语言** | Go only | 通用 | 多语言 | Java only |
| **需要 Docker** | 否 | 是 | 否 | 否 |
| **构建速度** | 极快 | 中等 | 慢 | 快 |
| **镜像大小** | 最小 | 取决于 Dockerfile | 中等 | 小 |
| **SBOM** | 内置 | 手动 | 内置 | 无 |
| **多平台** | 支持 | buildx | 支持 | 支持 |

---

## 最佳实践

1. **基础镜像**: 使用 distroless 或 chainguard/static 作为基础镜像减少攻击面
2. **多平台**: 生产构建使用 `--platform=linux/amd64,linux/arm64` 支持多架构
3. **CI 集成**: 在 CI/CD 中使用 ko 替代 Docker build，消除 Docker daemon 依赖
4. **镜像签名**: 启用 SBOM 生成和 cosign 签名，确保供应链安全
5. **YAML 管理**: 使用 `ko://` 前缀引用 Go 应用，简化镜像版本管理
6. **编译优化**: 在 .ko.yaml 中配置 `-trimpath -s -w` 减小二进制大小

---

## 参考资源

- [ko 官方文档](https://ko.build/)
- [ko GitHub](https://github.com/ko-build/ko)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
