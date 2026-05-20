---
title: werf
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
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
- werf 是什么
- 如何 werf
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- werf
- cncf
- landscape
---

# werf

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://werf.io/ |
| **GitHub** | https://github.com/werf/werf |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

werf 是一个一致且可复现的 CI/CD 交付工具，将 Git 作为唯一真相来源，集成了镜像构建、镜像发布、Helm 部署和清理策略。werf 提供从源码到部署的完整流水线，特别强调构建的可复现性和基于内容的标签策略。

### 核心特性

- **Git 感知构建**: 基于 Git 历史的增量构建和缓存优化
- **基于内容的标签**: 镜像 tag 基于文件内容 hash，确保可复现性
- **Helm 部署**: 内置增强的 Helm 部署，支持资源跟踪和就绪检查
- **清理策略**: 基于 Git 分支、标签和提交历史的自动镜像清理
- **Buildah/Dockerfile**: 支持 Dockerfile 和 Stapel（自有构建语法）
- **Giterminism**: 严格保证构建结果仅取决于 Git 仓库内容
- **多注册表**: 支持多容器注册表的镜像分发

---

## 快速开始

### 安装

```bash
# macOS/Linux
curl -sSL https://werf.io/install.sh | bash -s -- --ci

# 验证安装
werf version
```

### 项目配置

```yaml
# werf.yaml
project: my-web-app
configVersion: 1

---
image: backend
dockerfile: Dockerfile
context: ./backend

---
image: frontend
dockerfile: Dockerfile
context: ./frontend
args:
  API_URL: "https://api.example.com"
```

### 构建和部署

```bash
# 构建镜像
werf build

# 部署到 Kubernetes（使用 Helm chart）
werf converge --repo registry.example.com/my-app --env production

# 清理旧镜像
werf cleanup --repo registry.example.com/my-app
```

---

## 配置详解

### Helm Chart 集成

```yaml
# .helm/values.yaml
backend:
  replicas: 3
  resources:
    requests:
      cpu: 200m
      memory: 256Mi

frontend:
  replicas: 2
```

```yaml
# .helm/templates/deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: backend
spec:
  replicas: {{ .Values.backend.replicas }}
  selector:
    matchLabels:
      app: backend
  template:
    metadata:
      labels:
        app: backend
    spec:
      containers:
        - name: backend
          # werf 自动注入镜像地址和 digest
          image: {{ .Values.werf.image.backend }}
          resources:
            {{- toYaml .Values.backend.resources | nindent 12 }}
```

### Stapel 构建语法

```yaml
# werf.yaml - 使用 Stapel 语法
image: app
from: ubuntu:22.04
git:
  - add: /src
    to: /app
    stageDependencies:
      install:
        - package.json
        - package-lock.json
      setup:
        - "**/*.ts"
shell:
  install:
    - apt-get update && apt-get install -y nodejs npm
    - cd /app && npm ci
  setup:
    - cd /app && npm run build
```

### 多环境配置

```bash
# 不同环境的部署
werf converge --repo registry.example.com/my-app --env staging
werf converge --repo registry.example.com/my-app --env production

# 环境特定的 values
# .helm/values-staging.yaml
# .helm/values-production.yaml
```

### CI/CD 集成 (GitLab)

```yaml
# .gitlab-ci.yml
stages:
  - build
  - deploy

build:
  stage: build
  script:
    - werf build --repo $CI_REGISTRY_IMAGE
  tags:
    - werf

deploy_production:
  stage: deploy
  script:
    - werf converge --repo $CI_REGISTRY_IMAGE --env production
  environment:
    name: production
  when: manual
  only:
    - main
```

---

## 清理策略

```bash
# 基于 Git 策略的清理
werf cleanup --repo registry.example.com/my-app

# 清理策略配置
# werf.yaml
cleanup:
  keepPolicies:
    - references:
        branch: /^main$/
      imagesPerReference:
        last: 10
    - references:
        tag: /^v\d+\.\d+\.\d+$/
      imagesPerReference:
        last: 5
    - references:
        branch: /.*/
      imagesPerReference:
        last: 2
        in: 168h  # 7 天
```

---

## 最佳实践

1. **Giterminism**: 保持 werf.yaml 中所有配置来自 Git，确保构建可复现
2. **Stage 依赖**: 使用 stageDependencies 精确控制缓存失效范围
3. **基于内容的标签**: 使用默认的 content-based 标签策略确保部署与构建一致
4. **自动清理**: 在 CI 中定期运行 `werf cleanup` 清理未使用的镜像
5. **Helm values 分离**: 为不同环境维护独立的 values 文件
6. **资源跟踪**: 利用 werf 的增强 Helm 部署监控资源就绪状态

---

## 参考资源

- [werf 官方文档](https://werf.io/docs/)
- [werf GitHub](https://github.com/werf/werf)
- [werf 教程](https://werf.io/guides/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
