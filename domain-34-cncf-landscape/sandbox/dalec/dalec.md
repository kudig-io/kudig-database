---
title: Dalec (Declarative Application Linux Environment Creator)
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- docker
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Dalec (Declarative Application Linux Environment Creator) 是什么
- 如何 Dalec (Declarative Application Linux Environment Creator)
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Dalec
- Declarative
- Application
- Linux
- Environment
- Creator
- cncf
- landscape
---

# Dalec (Declarative Application Linux Environment Creator)

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **GitHub** | https://github.com/Azure/dalec |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Dalec 是一个声明式的 Linux 系统包构建工具，通过简洁的 YAML 规范定义如何构建 RPM、DEB 等 Linux 包，而无需手动编写 spec 文件或 debian/rules。它基于 BuildKit 构建，能够交叉编译多架构包，支持自动依赖管理、补丁应用、签名等功能。Dalec 特别适合需要将软件打包为多个发行版格式的场景。

### 核心特性

- **声明式配置**: 使用 YAML 描述包构建规范，替代传统 spec/rules 文件
- **多发行版支持**: 从单一定义生成 RPM (RHEL/Fedora)、DEB (Debian/Ubuntu) 等
- **BuildKit 驱动**: 利用 BuildKit 实现高效缓存和并行构建
- **交叉编译**: 支持构建多架构包 (amd64, arm64 等)
- **补丁管理**: 内置补丁应用和源码修改支持
- **依赖解析**: 自动处理构建依赖和运行时依赖
- **签名集成**: 支持 GPG 签名打包

---

## 架构设计

```
┌─────────────────────────────────────────────┐
│              Dalec 构建流程                   │
│                                              │
│  ┌──────────────┐                           │
│  │ dalec.yaml   │  声明式包定义              │
│  │ (源码/依赖/  │                           │
│  │  补丁/配置)  │                           │
│  └──────┬───────┘                           │
│         │                                    │
│  ┌──────▼───────┐                           │
│  │ Dalec Engine │                           │
│  │ (YAML 解析 / │                           │
│  │  DAG 生成)   │                           │
│  └──────┬───────┘                           │
│         │                                    │
│  ┌──────▼───────────────────────────┐       │
│  │        BuildKit Backend          │       │
│  │  ┌──────────┐  ┌──────────┐    │       │
│  │  │ RPM 构建 │  │ DEB 构建 │    │       │
│  │  │ (rpmbuild)│  │ (dpkg)  │    │       │
│  │  └────┬─────┘  └────┬─────┘    │       │
│  └───────┼──────────────┼──────────┘       │
│          │              │                    │
│  ┌───────▼──┐    ┌──────▼───┐              │
│  │ .rpm 包  │    │ .deb 包  │              │
│  └──────────┘    └──────────┘              │
└─────────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# Dalec 作为 BuildKit 前端运行，无需单独安装
# 确保有 Docker 或 BuildKit 可用

# 使用 Docker
docker buildx build -f dalec.yaml --target=mariner2/rpm .

# 或直接使用 BuildKit
buildctl build \
  --frontend=gateway.v0 \
  --opt source=ghcr.io/azure/dalec/frontend:latest \
  --local context=. \
  --opt target=azlinux3/rpm
```

### 包定义示例

```yaml
# dalec.yaml
# syntax=ghcr.io/azure/dalec/frontend:latest

name: my-tool
version: 1.2.3
revision: "1"
description: My custom tool for data processing
website: https://github.com/myorg/my-tool
vendor: MyOrg
license: Apache-2.0
packager: MyOrg Build Team

sources:
  my-tool:
    git:
      url: https://github.com/myorg/my-tool.git
      commit: "abc123def456"
    # 或使用 HTTP 源
    # http:
    #   url: https://github.com/myorg/my-tool/archive/v1.2.3.tar.gz

  config-files:
    inline:
      - destination: my-tool.conf
        content: |
          [server]
          port = 8080
          log_level = info

patches:
  - source: fix-build.patch

build:
  deps:
    golang:
      version: ">=1.21"
    make: {}
  env:
    CGO_ENABLED: "0"
    GOFLAGS: "-trimpath"
  steps:
    - command: |
        cd my-tool
        make build
        install -D -m 755 bin/my-tool ${DESTDIR}/usr/bin/my-tool
    - command: |
        install -D -m 644 config-files/my-tool.conf \
          ${DESTDIR}/etc/my-tool/my-tool.conf

artifacts:
  binaries:
    my-tool:
      subpath: usr/bin/my-tool
  config_files:
    my-tool.conf:
      subpath: etc/my-tool/my-tool.conf

  systemd:
    units:
      my-tool.service:
        name: my-tool.service
        contents: |
          [Unit]
          Description=My Tool Service
          After=network.target

          [Service]
          Type=simple
          ExecStart=/usr/bin/my-tool serve
          Restart=always

          [Install]
          WantedBy=multi-user.target
        enable: true

dependencies:
  runtime:
    glibc: {}
    openssl-libs: {}
```

### 构建多种格式

```bash
# 构建 Azure Linux (Mariner) RPM
docker buildx build -f dalec.yaml --target=azlinux3/rpm --output=type=local,dest=./out .

# 构建 RHEL 9 RPM
docker buildx build -f dalec.yaml --target=rhel9/rpm --output=type=local,dest=./out .

# 构建 Ubuntu 22.04 DEB
docker buildx build -f dalec.yaml --target=jammy/deb --output=type=local,dest=./out .

# 构建包含包的容器镜像
docker buildx build -f dalec.yaml --target=azlinux3/container -t myorg/my-tool:latest .
```

---

## 高级功能

### 测试阶段

```yaml
tests:
  - name: binary-exists
    steps:
      - command: test -f /usr/bin/my-tool
  - name: version-check
    steps:
      - command: /usr/bin/my-tool --version
        stdout:
          contains:
            - "1.2.3"
  - name: config-exists
    steps:
      - command: test -f /etc/my-tool/my-tool.conf
```

### 多架构构建

```bash
# 构建 arm64 包
docker buildx build -f dalec.yaml \
  --target=azlinux3/rpm \
  --platform=linux/arm64 \
  --output=type=local,dest=./out .
```

---

## 与其他方案对比

| 特性 | Dalec | rpmbuild | dpkg-buildpackage | Nix |
|:---|:---|:---|:---|:---|
| 配置格式 | YAML | spec 文件 | debian/* | Nix 表达式 |
| 学习曲线 | 低 | 高 | 高 | 高 |
| 多发行版 | 单一定义 | RPM 仅 | DEB 仅 | 跨平台 |
| 缓存 | BuildKit 缓存 | 手动 | 手动 | Nix Store |
| 交叉编译 | 内置 | 复杂 | 复杂 | 支持 |
| 可复现性 | 高 (内容寻址) | 中 | 中 | 极高 |

---

## 最佳实践

1. **源码固定**: 使用 Git commit hash 而非分支名固定源码版本
2. **依赖最小化**: 只声明实际需要的运行时依赖
3. **测试覆盖**: 为每个包编写基本测试确保安装和运行正常
4. **CI/CD 集成**: 将 Dalec 构建集成到流水线，自动构建多发行版包
5. **版本策略**: 使用 revision 字段区分同一上游版本的不同打包版本

---

## 参考资源

- [Dalec GitHub](https://github.com/Azure/dalec)
- [Dalec 示例](https://github.com/Azure/dalec/tree/main/test/fixtures)
- [BuildKit 文档](https://github.com/moby/buildkit)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
