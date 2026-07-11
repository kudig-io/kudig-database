---
title: Score (entities)
description: '## 概述'
summary: 'Score 是一个与平台无关的工作负载规范，使开发者能够用统一的格式描述其工作负载需求（容器、资源依赖、环境变量等），然后由 Score 实现工具（score-compose, score-k8s, score-humanitec）将规范翻译为目标平台的原生配置。'
category: entities
tags:
- k8s
- cncf
- orchestration
- score
- crd
- operator
- kubelet
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Score 是什么
- 如何 Score
trigger_keywords:
- Score
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Score

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Score 是由 Humanitec 开发的工作负载规范，2023 年进入 CNCF Sandbox。它不是运行时工具，而是一个**平台无关的声明式工作负载描述格式**。开发者编写一份 `score.yaml` 描述工作负载的容器、端口、环境变量和资源依赖，然后通过 Score CLI（`score-compose`、`score-k8s`、`score-humanitec`）将其翻译为目标平台的原生配置（Docker Compose YAML、Helm values、Humanitec Deployment）。

Score 的核心理念是**关注点分离**：开发者只描述"我需要什么"（如一个 Redis、一个 PostgreSQL），而平台团队决定"如何提供"（本地用 Docker Redis、测试用托管 RDS、生产用 CloudSQL）。这避免了开发者学习每个平台的特定配置语法。

## Key Features

- **平台无关规范**：一份 `score.yaml` 生成多平台配置（Compose/K8s/Humanitec）
- **资源抽象**：通过 `resources` 声明依赖（如 `redis`、`postgres`），由 Provisioner 决定具体实现
- **环境变量引用**：通过 `${resources.xxx.host}` 引用资源属性，保持可移植性
- **多文件支持**：通过 `score.yaml` + `overrides.yaml` 实现环境差异化配置
- **CLI 工具链**：`score-compose`、`score-k8s`、`score-terraform` 等多目标生成器
- **Git 友好**：单一 YAML 文件，易于版本控制和代码审查

## Architecture

Score 架构包含两部分：**Score Spec**（`score.yaml`，开发者编写的声明式工作负载描述）和 **Implementation/Provisioner**（平台特定的资源供给器）。Score CLI 读取 `score.yaml`，根据目标平台生成对应的配置文件。对于资源依赖（`resources`），每个平台有自己的 Provisioner 决定如何创建——例如 `score-compose` 将 `redis` 资源映射为 Docker Compose 中的 Redis 服务，而 `score-k8s` 可能映射为 Helm Chart 安装的 Redis。

## K8s 集成

通过 `score-k8s` CLI，`score.yaml` 被翻译为 Kubernetes 原生资源（Deployment、Service、ConfigMap 等）或 Helm values。资源依赖通过 Provisioner 映射为具体的实现（如 `postgres` → CloudNativePG 或 Bitnami Helm Chart）。也支持通过 Humanitec Platform Orchestrator 在 K8s 中实现自动化资源供给和匹配。

## 生产部署要点

- **资源抽象**：使用 resources 声明依赖，让平台团队决定具体实现
- **环境变量**：通过 `${resources.xxx}` 引用资源属性，保持可移植性
- **本地开发**：使用 score-compose 进行本地开发，score-k8s 部署到集群
- **团队协作**：开发者专注 Score 规范，平台团队维护 provisioners
- **版本控制**：将 score.yaml 纳入 Git 管理

## 生产场景

1. **本地→K8s 一致性**：开发者用 `score-compose` 本地调试，用 `score-k8s` 部署到集群
2. **多环境部署**：同一 `score.yaml` 配合不同 provisioners 部署到 dev/staging/prod
3. **平台迁移**：从 Compose 迁移到 K8s 时，只需更换 Score CLI target
4. **内部开发者平台**：Score 作为 IDP 的工作负载入口格式

## 安装

```bash
# 安装 Score CLI
brew install score-spec/tap/score-compose
brew install score-spec/tap/score-k8s

# 编写 score.yaml
cat > score.yaml <<EOF
apiVersion: score.dev/v1b1
metadata:
  name: my-app
containers:
  app:
    image: nginx:latest
    variables:
      REDIS_HOST: \${resources.redis.host}
resources:
  redis:
    type: redis
service:
  ports:
    http:
      port: 8080
      targetPort: 80
EOF

# 生成 Docker Compose 配置
score-compose generate score.yaml

# 生成 Kubernetes 配置
score-k8s generate score.yaml
```

## 对比

| 特性 | Score | Helm | Kustomize | Compose |
|------|-------|------|-----------|---------|
| 平台无关 | ✅ | ❌ K8s only | ❌ K8s only | ❌ Docker only |
| 资源抽象 | ✅ | ❌ | ❌ | ❌ |
| 开发者友好 | ✅ | ⚠️ | ⚠️ | ✅ |
| 多平台输出 | ✅ | ❌ | ❌ | ❌ |

## 参考链接

- [[概念/kubernetes-architecture-overview.md|kubernetes-architecture-overview]]

## Related

- [[meshery]] — Meshery
- [[knative]] — Knative
- [[konveyor]] — Konveyor
- [[bfe]] — BFE
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- score
- [[概念/scheduling-algorithm.md|[[Scheduling Algorithm|Scheduling Algorithm]]]] — Cross-reference
- [[技能/kubelet-eviction-mechanism.md|kubelet 资源驱逐机制]] — Cross-reference
- [[技能/Symptom Vector Matching Engine.md|Symptom Vector Matching Engine]] — Cross-reference
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference


<!-- risk-assessed -->
