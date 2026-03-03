# Buildpacks

> **成熟度**: Incubating | **加入时间**: 2018-10 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://buildpacks.io |
| **GitHub** | https://github.com/buildpacks |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | App Definition & Image Build |

---

## 项目概述

Cloud Native Buildpacks (CNB) 将应用源代码转换为 OCI 容器镜像，无需编写 Dockerfile。它自动检测应用类型、安装依赖、配置运行环境，简化了容器化流程并提高镜像安全性。

## 核心特性

- **自动检测**: 智能识别应用语言和框架
- **模块化**: Buildpack 可组合、可复用
- **Rebasing**: 无需重新构建即可更新基础镜像
- **SBOM 生成**: 自动生成软件物料清单
- **缓存优化**: 分层缓存加速构建
- **多平台**: 支持 AMD64、ARM64 等架构
- **标准规范**: OCI 兼容的镜像格式

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                   Cloud Native Buildpacks                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                      Build Process                         │ │
│  │                                                            │ │
│  │  ┌──────────┐    ┌──────────────┐    ┌────────────────┐  │ │
│  │  │  Source  │───▶│   Detect     │───▶│    Build       │  │ │
│  │  │   Code   │    │   Phase      │    │    Phase       │  │ │
│  │  └──────────┘    └──────────────┘    └───────┬────────┘  │ │
│  │                                              │            │ │
│  │                                              ▼            │ │
│  │                         ┌────────────────────────────┐   │ │
│  │                         │       Export Phase        │   │ │
│  │                         │    (OCI Image Layers)     │   │ │
│  │                         └────────────────────────────┘   │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                      Builder Structure                     │ │
│  │                                                            │ │
│  │  ┌─────────────────────────────────────────────────────┐  │ │
│  │  │                    Builder Image                    │  │ │
│  │  │  ┌───────────────┐                                 │  │ │
│  │  │  │  Build Image  │  运行 detect/build              │  │ │
│  │  │  └───────────────┘                                 │  │ │
│  │  │  ┌───────────────┐                                 │  │ │
│  │  │  │  Run Image    │  最终运行环境                   │  │ │
│  │  │  └───────────────┘                                 │  │ │
│  │  │  ┌───────────────┐                                 │  │ │
│  │  │  │  Buildpacks   │  Node, Python, Java, Go...     │  │ │
│  │  │  │  (Ordered)    │                                 │  │ │
│  │  │  └───────────────┘                                 │  │ │
│  │  └─────────────────────────────────────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                     Output Image                           │ │
│  │  ┌─────────────────────────────────────────────────────┐  │ │
│  │  │  Run Image (OS + Runtime)                           │  │ │
│  │  │  ─────────────────────────────────────              │  │ │
│  │  │  App Dependencies Layer                             │  │ │
│  │  │  ─────────────────────────────────────              │  │ │
│  │  │  App Source Layer                                   │  │ │
│  │  │  ─────────────────────────────────────              │  │ │
│  │  │  Launcher + Metadata                                │  │ │
│  │  └─────────────────────────────────────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装 Pack CLI

```bash
# macOS
brew install buildpacks/tap/pack

# Linux
(curl -sSL "https://github.com/buildpacks/pack/releases/download/v0.32.1/pack-v0.32.1-linux.tgz" | sudo tar -C /usr/local/bin/ --no-same-owner -xzv pack)

# Windows
scoop install pack

# 验证安装
pack version
```

### 构建第一个应用

```bash
# Node.js 应用
pack build my-node-app --builder paketobuildpacks/builder:base

# Python 应用
pack build my-python-app --builder paketobuildpacks/builder:base

# Go 应用
pack build my-go-app --builder paketobuildpacks/builder:base

# Java (Spring Boot)
pack build my-java-app --builder paketobuildpacks/builder:base
```

### 使用特定 Builder

```bash
# Paketo Buildpacks (推荐)
pack build myapp --builder paketobuildpacks/builder:base
pack build myapp --builder paketobuildpacks/builder:full
pack build myapp --builder paketobuildpacks/builder:tiny

# Google Cloud Buildpacks
pack build myapp --builder gcr.io/buildpacks/builder:v1

# Heroku Buildpacks
pack build myapp --builder heroku/builder:22
```

---

## 项目配置

### project.toml

```toml
# project.toml
[_]
schema-version = "0.2"

[io.buildpacks]
builder = "paketobuildpacks/builder:base"

[[io.buildpacks.build.env]]
name = "BP_NODE_VERSION"
value = "18.*"

[[io.buildpacks.build.env]]
name = "BP_NODE_RUN_SCRIPTS"
value = "build"

[io.buildpacks.build]
exclude = [
  "*.md",
  ".git",
  "test",
  "docs"
]
```

### 环境变量配置

```bash
# Node.js 配置
pack build myapp \
  --env BP_NODE_VERSION="18.*" \
  --env BP_NODE_RUN_SCRIPTS="build"

# Java 配置
pack build myapp \
  --env BP_JVM_VERSION="17" \
  --env BP_MAVEN_BUILD_ARGUMENTS="-DskipTests package"

# Python 配置
pack build myapp \
  --env BP_CPYTHON_VERSION="3.11.*"

# Go 配置
pack build myapp \
  --env BP_GO_VERSION="1.21.*" \
  --env BP_GO_TARGETS="./cmd/server"
```

---

## 高级用法

### Procfile 定义启动命令

```procfile
# Procfile
web: node server.js
worker: node worker.js
```

### 多进程启动

```bash
# 指定进程类型
pack build myapp --default-process worker
```

### Rebase (更新基础镜像)

```bash
# 无需重新构建，仅更新运行时层
pack rebase myapp:latest

# 指定新的 run image
pack rebase myapp:latest --run-image paketobuildpacks/run:base-cnb
```

### 生成 SBOM

```bash
# 查看 SBOM
pack sbom download myapp:latest --output-dir ./sbom

# 支持的格式
pack sbom download myapp:latest --format cyclonedx
pack sbom download myapp:latest --format spdx
pack sbom download myapp:latest --format syft
```

---

## 自定义 Buildpack

### 创建简单 Buildpack

```bash
# 目录结构
my-buildpack/
├── buildpack.toml
├── bin/
│   ├── detect
│   └── build
```

### buildpack.toml

```toml
# buildpack.toml
api = "0.9"

[buildpack]
id = "example/my-buildpack"
version = "0.0.1"
name = "My Custom Buildpack"

[[stacks]]
id = "io.buildpacks.stacks.jammy"
```

### detect 脚本

```bash
#!/usr/bin/env bash
# bin/detect

# 检测是否存在 app.json
if [[ -f "app.json" ]]; then
  echo "My Buildpack"
  exit 0
else
  exit 100
fi
```

### build 脚本

```bash
#!/usr/bin/env bash
# bin/build

set -euo pipefail

layers_dir="$1"
env_dir="$2/env"
plan_path="$3"

# 创建依赖层
deps_layer="${layers_dir}/deps"
mkdir -p "${deps_layer}"

echo -e '[types]\ncache = true\nlaunch = true' > "${deps_layer}.toml"

# 安装依赖
npm install --prefix "${deps_layer}"

# 设置启动命令
cat > "${layers_dir}/launch.toml" << EOF
[[processes]]
type = "web"
command = "node"
args = ["server.js"]
default = true
EOF
```

---

## CI/CD 集成

### GitHub Actions

```yaml
name: Build with Buildpacks

on:
  push:
    branches: [main]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Setup Pack CLI
        uses: buildpacks/github-actions/setup-pack@v5.4.0
      
      - name: Build Image
        run: |
          pack build myapp:${{ github.sha }} \
            --builder paketobuildpacks/builder:base \
            --publish \
            --cache-image myregistry.io/myapp-cache
        env:
          DOCKER_USERNAME: ${{ secrets.DOCKER_USERNAME }}
          DOCKER_PASSWORD: ${{ secrets.DOCKER_PASSWORD }}
```

### GitLab CI

```yaml
build:
  image: buildpacksio/pack:latest
  script:
    - pack build $CI_REGISTRY_IMAGE:$CI_COMMIT_SHA
        --builder paketobuildpacks/builder:base
        --publish
```

---

## Kubernetes 集成 (kpack)

```yaml
# ClusterBuilder
apiVersion: kpack.io/v1alpha2
kind: ClusterBuilder
metadata:
  name: my-builder
spec:
  tag: myregistry.io/builder
  stack:
    name: base
    kind: ClusterStack
  store:
    name: default
    kind: ClusterStore
  order:
    - group:
      - id: paketo-buildpacks/nodejs

---
# Image 资源
apiVersion: kpack.io/v1alpha2
kind: Image
metadata:
  name: my-app
spec:
  tag: myregistry.io/my-app
  builder:
    name: my-builder
    kind: ClusterBuilder
  source:
    git:
      url: https://github.com/example/my-app
      revision: main
```

---

## 最佳实践

1. **选择合适的 Builder**: tiny 用于生产（最小镜像），full 用于调试
2. **利用缓存**: 使用 cache-image 加速 CI/CD 构建
3. **Rebase 更新**: 定期 rebase 以获取安全补丁
4. **SBOM 集成**: 将 SBOM 纳入供应链安全流程
5. **版本锁定**: 使用 project.toml 锁定 buildpack 版本

---

## 参考资源

- [官方文档](https://buildpacks.io/docs)
- [GitHub Repo](https://github.com/buildpacks)
- [Paketo Buildpacks](https://paketo.io/)
- [Pack CLI Reference](https://buildpacks.io/docs/tools/pack/)
- [kpack](https://github.com/pivotal/kpack)

---

**维护者**: Kudig Team | **许可证**: MIT
