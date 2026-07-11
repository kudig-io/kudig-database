---
title: Dockerfile 镜像层缓存与体积优化
description: 镜像分层缓存原理、层合并、.dockerignore、多阶段构建与 BuildKit cache mount 实战
summary: 镜像分层缓存原理、层合并、.dockerignore、多阶段构建与 BuildKit cache mount 实战
category: container-runtime
tags:
- containerd
- cri
- runtime
- dockerfile
- layer-cache
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

# Dockerfile 镜像层缓存与体积优化

## 概述

容器镜像由若干只读层（layer）叠加。每条 Dockerfile 指令产生一层，缓存命中策略决定构建速度；层数与每层大小决定最终镜像体积、拉取时间、节点磁盘占用。优化层 = 更快构建 + 更快拉取 + 更省磁盘 + 更少漏洞面。

## 分层与缓存命中规则

BuildKit/Docker 缓存按指令顺序逐层校验：某层输入（基镜像 digest、指令文本、拷贝文件内容 hash）变化，该层及**之后所有层**缓存失效。

```
FROM base            # 缓存命中
COPY go.mod ./       # ← go.mod 变，此层起全部失效
RUN go mod download  # 失效
COPY . .             # 失效
RUN go build         # 失效
```

**核心原则：把变化频率低的放前面，变化频率高的放后面。**

## 反模式 vs 正确顺序

```dockerfile
# ❌ 反模式：源码一变就重下依赖
COPY . .
RUN npm ci

# ✅ 正确：先拷锁文件，依赖缓存稳定
COPY package*.json ./
RUN npm ci
COPY . .
```

```dockerfile
# ❌ 反模式：每条 RUN 一层，多了无意义层
RUN apt-get update
RUN apt-get install -y curl
RUN rm -rf /var/lib/apt/lists/*

# ✅ 正确：合并为一层，清理在同一层完成（否则中间层仍含 apt 缓存）
RUN apt-get update && apt-get install -y --no-install-recommends curl \
    && rm -rf /var/lib/apt/lists/*
```

## .dockerignore

构建上下文（context）整体发送给 daemon/buildkit。不忽略 `node_modules`、`.git`、`dist` 会让上下文膨胀、缓存键漂移。

```gitignore
# .dockerignore
.git
.gitignore
node_modules
dist
*.md
.env*
Dockerfile*
```

``` bash
# 🟢 只读：查看上下文体积
du -sh .
```

## 多阶段构建

用多阶段把编译工具链留在 builder，最终镜像只 COPY 产物：

```dockerfile
FROM maven:3.9-eclipse-temurin-17 AS build
WORKDIR /app
COPY pom.xml .
RUN mvn dependency:go-offline           # 依赖先缓存
COPY src ./src
RUN mvn clean package -DskipTests

FROM eclipse-temurin:17-jre-alpine
COPY --from=build /app/target/app.jar /app.jar
ENTRYPOINT ["java","-jar","/app.jar"]
```

体积从 ~800MB（含 Maven）降到 ~200MB（仅 JRE + jar）。

## BuildKit cache mount（高级）

`--mount=type=cache` 把包管理器缓存挂为持久卷，跨构建复用，且**不进镜像层**：

```dockerfile
# syntax=docker/dockerfile:1.7
RUN --mount=type=cache,target=/root/.m2 \
    mvn clean package -DskipTests

RUN --mount=type=cache,target=/var/cache/apt \
    apt-get update && apt-get install -y curl
```

``` bash
# 🟡 中风险：启用 BuildKit
DOCKER_BUILDKIT=1 docker build -t app:v1 .
```

cache mount 是 BuildKit 专属，Kaniko 不支持（见 [[容器运行时/镜像构建/03-kaniko-ko-build-guide]]）。

## 镜像体积分析

``` bash
# 🟢 只读：分层体积剖析
dive registry.example.com/app:v1
# 或
docker history --no-trunc --format '{{.Size}}\t{{.CreatedBy}}' app:v1
```

`dive` 标出每层浪费（wasted bytes），定位该合并/清理的层。

## 优化效果基线

| 优化前 | 优化手段 | 优化后 |
|---|---|---|
| 1.2GB | 多阶段 + JRE alpine | 220MB |
| + 8min 构建 | 依赖先缓存 + cache mount | 1.5min |
| + 每层 apt 残留 | 合并 RUN + 同层清理 | 无残留 |

## 生产检查清单

- [ ] 提供 `.dockerignore`，构建上下文 < 10MB
- [ ] 依赖安装层（go.mod/package.json/pom.xml）先于源码拷贝
- [ ] 使用多阶段构建，最终镜像不含编译工具链
- [ ] 启用 BuildKit `--mount=type=cache`
- [ ] `dive` 检查无显著 wasted bytes，HIGH/CRITICAL CVE 已清

## 相关文档

- [[容器运行时/镜像构建/01-buildkit-production-guide.md|BuildKit 生产指南]]
- [[容器运行时/镜像构建/05-distroless-minimal-images.md|Distroless 极简镜像]]
- [[容器运行时/镜像构建/03-kaniko-ko-build-guide.md|Kaniko 与 ko]]

<!-- risk-assessed -->
