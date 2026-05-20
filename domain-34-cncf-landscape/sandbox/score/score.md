---
title: Score
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- docker
- redis
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Score 是什么
- 如何 Score
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Score
- cncf
- landscape
---

# Score

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://score.dev/ |
| **GitHub** | https://github.com/score-spec/spec |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Score 是一个与平台无关的工作负载规范，使开发者能够用统一的格式描述其工作负载需求（容器、资源依赖、环境变量等），然后由 Score 实现工具（score-compose, score-k8s, score-humanitec）将规范翻译为目标平台的原生配置。

### 核心特性

- **平台无关**: 一次编写工作负载定义，部署到多个平台
- **开发者友好**: 简洁的 YAML 格式，专注于工作负载需求而非平台细节
- **资源抽象**: 声明式定义数据库、缓存、消息队列等资源依赖
- **多平台实现**: Docker Compose, Kubernetes, Humanitec 等
- **分离关注点**: 开发者定义需求，平台团队定义实现

---

## 快速开始

### 安装

```bash
# 安装 score-compose（Docker Compose 实现）
brew install score-spec/tap/score-compose

# 安装 score-k8s（Kubernetes 实现）
brew install score-spec/tap/score-k8s
```

### Score 工作负载定义

```yaml
# score.yaml
apiVersion: score.dev/v1b1
metadata:
  name: my-web-app

containers:
  web:
    image: my-app:latest
    command: ["./server"]
    variables:
      PORT: "8080"
      DB_HOST: "${resources.db.host}"
      DB_PORT: "${resources.db.port}"
      DB_NAME: "${resources.db.name}"
      DB_PASSWORD: "${resources.db.password}"
      REDIS_URL: "redis://${resources.cache.host}:${resources.cache.port}"
    files:
      - target: /etc/app/config.yaml
        mode: "0644"
        content: |
          server:
            port: 8080
          database:
            host: ${resources.db.host}
    resources:
      limits:
        cpu: "500m"
        memory: "512Mi"

service:
  ports:
    web:
      port: 80
      targetPort: 8080

resources:
  db:
    type: postgres
    properties:
      host:
      port:
        default: 5432
      name:
      password:
  cache:
    type: redis
    properties:
      host:
      port:
        default: 6379
```

### 生成 Docker Compose

```bash
score-compose init
score-compose generate score.yaml
docker compose up -d
```

### 生成 Kubernetes 清单

```bash
score-k8s init
score-k8s generate score.yaml
kubectl apply -f manifests/
```

---

## 最佳实践

1. **资源抽象**: 使用 resources 声明依赖，让平台团队决定具体实现
2. **环境变量**: 通过 `${resources.xxx}` 引用资源属性，保持可移植性
3. **本地开发**: 使用 score-compose 进行本地开发，score-k8s 部署到集群
4. **团队协作**: 开发者专注 Score 规范，平台团队维护 provisioners
5. **版本控制**: 将 score.yaml 纳入 Git 管理

---

## 参考资源

- [Score 官方文档](https://score.dev/docs/)
- [Score 规范](https://github.com/score-spec/spec)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
