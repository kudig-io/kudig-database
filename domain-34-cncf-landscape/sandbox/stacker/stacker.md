---
title: Stacker
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Stacker 是什么
- 如何 Stacker
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Stacker
- cncf
- landscape
---

# Stacker

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **GitHub** | https://github.com/project-stacker/stacker |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Stacker 是一个无需特权即可构建 OCI 容器镜像的工具。它使用声明式的 YAML 文件（stacker.yaml）定义镜像层，通过 overlay 文件系统构建镜像，无需 Docker daemon 或 root 权限。Stacker 支持可复现构建、内容寻址层缓存和多阶段构建，特别适合 CI/CD 流水线中的安全镜像构建。

### 核心特性

- **无特权构建**: 使用 user namespace 在非 root 环境下构建镜像
- **声明式定义**: 使用 stacker.yaml 声明式定义镜像内容
- **可复现构建**: 相同输入产生相同输出，确保构建可复现
- **层缓存**: 基于内容寻址的层缓存，加速重复构建
- **OCI 标准**: 生成符合 OCI 标准的容器镜像
- **替换导入**: 支持从 Docker/OCI 镜像、HTTP URL、本地文件等多种来源导入

---

## 快速开始

### 安装

```bash
# 下载二进制
curl -LO https://github.com/project-stacker/stacker/releases/latest/download/stacker
chmod +x stacker
sudo mv stacker /usr/local/bin/
```

### 构建镜像

```yaml
# stacker.yaml
base:
  from:
    type: docker
    url: docker://ubuntu:22.04
  run: |
    apt-get update
    apt-get install -y nginx
    rm -rf /var/lib/apt/lists/*
  entrypoint: /usr/sbin/nginx
  cmd: -g 'daemon off;'

app:
  from:
    type: built
    tag: base
  import:
    - path: ./app/
      dest: /opt/app/
    - path: ./config/nginx.conf
      dest: /etc/nginx/nginx.conf
  run: |
    chmod +x /opt/app/start.sh
  entrypoint: /opt/app/start.sh
```

```bash
# 构建
stacker build

# 推送到注册中心
stacker publish --url docker://registry.example.com/myapp --tag latest
```

### 多阶段构建

```yaml
# 构建阶段
builder:
  from:
    type: docker
    url: docker://golang:1.22
  import:
    - path: ./src/
      dest: /go/src/app/
  run: |
    cd /go/src/app
    CGO_ENABLED=0 go build -o /app

# 运行阶段
runtime:
  from:
    type: docker
    url: docker://gcr.io/distroless/static:latest
  import:
    - path: stacker://builder/app
      dest: /app
  entrypoint: /app
```

---

## 与其他方案对比

| 特性 | Stacker | Docker | Buildah | Kaniko |
|:---|:---|:---|:---|:---|
| 特权要求 | 无需 root | 需要 daemon | 可无 root | 可无 root |
| 定义格式 | YAML | Dockerfile | Dockerfile/CLI | Dockerfile |
| Daemon | 不需要 | 需要 | 不需要 | 不需要 |
| 可复现性 | 高 | 中 | 中 | 中 |
| 适用场景 | 安全 CI/CD | 通用 | CI/CD | K8s 内构建 |

---

## 最佳实践

1. **非特权环境**: 在 CI/CD 中使用非 root 用户运行 stacker 构建
2. **层缓存**: 利用 stacker 的层缓存加速 CI/CD 流水线中的重复构建
3. **多阶段**: 使用多阶段构建减小最终镜像体积
4. **锁定版本**: 在 from 中使用摘要而非标签锁定基础镜像版本
5. **签名**: 构建后对镜像签名，确保供应链安全

---

## 参考资源

- [Stacker GitHub](https://github.com/project-stacker/stacker)
- [Stacker 文档](https://github.com/project-stacker/stacker/blob/main/doc/stacker_yaml.md)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
