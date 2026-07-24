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

## 安装与配置

```bash
# 安装 Score CLI
brew install score-spec/tap/score-compose
brew install score-spec/tap/score-k8s

# 或使用安装脚本
curl -fsSL https://raw.githubusercontent.com/score-spec/score-compose/main/install.sh | sh
curl -fsSL https://raw.githubusercontent.com/score-spec/score-k8s/main/install.sh | sh

# 验证安装
score-compose version
score-k8s version
```

```yaml
# score.yaml 示例（完整工作负载定义）
apiVersion: score.dev/v1b1
metadata:
  name: payment-service
  labels:
    team: payments
    tier: backend
containers:
  app:
    image: registry.company.com/payment:v2.1.0
    variables:
      REDIS_HOST: ${resources.redis.host}
      REDIS_PORT: ${resources.redis.port}
      DB_HOST: ${resources.postgres.host}
      DB_NAME: ${resources.postgres.name}
      DB_USER: ${resources.postgres.username}
      DB_PASSWORD: ${resources.postgres.password}
    resources:
      limits:
        cpu: "2"
        memory: "4Gi"
      requests:
        cpu: "500m"
        memory: "1Gi"
    readinessProbe:
      httpGet:
        path: /health
        port: 8080
resources:
  redis:
    type: redis
    metadata:
      annotations:
        score.dev/description: "Cache for session data"
  postgres:
    type: postgres
    metadata:
      annotations:
        score.dev/description: "Primary database"
service:
  ports:
    http:
      port: 8080
      targetPort: 8080
    grpc:
      port: 9090
      targetPort: 9090
---
# overrides.yaml（生产环境覆盖）
containers:
  app:
    image: registry.company.com/payment:v2.1.0-prod
    resources:
      limits:
        cpu: "4"
        memory: "8Gi"
```

```bash
# 生成 Docker Compose 配置（本地开发）
score-compose generate score.yaml --override overrides.yaml
docker compose up -d

# 生成 Kubernetes 配置（集群部署）
score-k8s generate score.yaml --override overrides.yaml > k8s-resources.yaml
kubectl apply -f k8s-resources.yaml
```

## 运维操作

```bash
# 🟢 验证 score.yaml 语法
score-compose validate score.yaml

# 🟢 生成并预览 Docker Compose 配置
score-compose generate score.yaml --dry-run

# 🟢 生成并预览 Kubernetes 配置
score-k8s generate score.yaml --dry-run

# 🟡 更新工作负载配置
# 修改 score.yaml 后重新生成
score-k8s generate score.yaml > k8s-resources.yaml
kubectl apply -f k8s-resources.yaml

# 🟢 查看生成的资源
kubectl get deployment,service,configmap -l app=payment-service

# 🟡 回滚到上一版本
kubectl rollout undo deployment/payment-service
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| score.yaml 验证失败 | YAML 语法错误或字段缺失 | `score-compose validate score.yaml` | 修复 YAML 格式和必填字段 |
| 资源依赖未解析 | Provisioner 未配置或资源类型不支持 | `score-k8s generate --verbose` | 检查 provisioner 配置 |
| 生成的 K8s 配置错误 | 资源映射配置不正确 | `score-k8s generate --dry-run` | 调整 provisioner 映射规则 |
| 环境变量未替换 | 资源引用语法错误 | 检查生成的 ConfigMap | 确认 ${resources.xxx} 语法 |
| 多环境配置冲突 | overrides 覆盖顺序错误 | 对比不同环境的生成结果 | 检查 override 文件层次 |

```
排查流程：
├── 生成失败
│   ├── score-compose validate 检查语法
│   ├── 确认所有 resources 类型有对应 provisioner
│   ├── 检查 ${resources.xxx} 引用语法
│   └── 使用 --verbose 查看详细错误
├── 部署失败
│   ├── 检查生成的 K8s YAML 是否有效
│   ├── kubectl apply --dry-run=server 验证
│   ├── 确认镜像可拉取
│   └── 检查资源配额是否足够
└── 环境差异问题
    ├── 对比不同环境的 score.yaml + overrides
    ├── 确认 provisioner 配置一致
    └── 检查环境变量引用是否正确解析
```

## 生产案例

### 案例 1：本地开发到 K8s 一致性部署

- **场景**：开发者本地用 Docker Compose 调试，部署到 K8s 时需要重新编写 Helm Chart，配置经常不一致
- **排查**：本地和 K8s 配置分离，环境变量和资源依赖配置经常遗漏，部署失败率高
- **方案**：使用 Score 统一工作负载定义，score-compose 本地调试，score-k8s 集群部署
- **效果**：配置一致性 100%，部署失败率从 30% 降至 5%，新服务上线时间从 2 天降至 2 小时

### 案例 2：内部开发者平台工作负载入口

- **场景**：平台团队构建 IDP，需要统一的工作负载描述格式，屏蔽底层平台复杂性
- **排查**：开发者需要学习 Helm、Kustomize、Terraform 多种工具，上手成本高
- **方案**：Score 作为 IDP 的工作负载入口格式，开发者只写 score.yaml，平台自动处理部署
- **效果**：开发者上手时间从 1 周降至 1 天，平台团队维护 provisioner 而非每个服务配置

## 对比

| 特性 | Score | Helm | Kustomize | Compose | 适用场景 |
|------|-------|------|-----------|---------|----------|
| 平台无关 | ✅ | ❌ K8s only | ❌ K8s only | ❌ Docker only | 多平台部署 |
| 资源抽象 | ✅ | ❌ | ❌ | ❌ | 关注点分离 |
| 开发者友好 | ✅ | ⚠️ | ⚠️ | ✅ | 降低上手成本 |
| 多平台输出 | ✅ | ❌ | ❌ | ❌ | 本地→云一致性 |
| 生产成熟度 | 中（新项目） | 高 | 高 | 高 | 稳定性要求 |

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
- [[技能/节点/node/运维操作/kubelet-eviction-mechanism.md|kubelet 资源驱逐机制]] — Cross-reference
- [[技能/工作负载/pod/方法论/Symptom Vector Matching Engine.md|Symptom Vector Matching Engine]] — Cross-reference
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference


<!-- risk-assessed -->
