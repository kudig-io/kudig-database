---
title: werf [entities]
description: '## 概述'
summary: 'werf 是一个一致且可复现的 CI/CD 交付工具，将 Git 作为唯一真相来源，集成了镜像构建、镜像发布、Helm 部署和清理策略。werf 提供从源码到部署的完整流水线，特别强调构建的可复现性和基于内容的标签策略。'
category: entities
tags:
- k8s
- cncf
- ci-cd
- werf
- helm
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- werf 是什么
- 如何 werf
trigger_keywords:
- werf
prerequisites:
- kubectl-basics
- helm-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# werf

> **CNCF 状态**: Sandbox | **类别**: CI/CD | **主要语言**: Go

## 概述

werf 是由 Flant（现为 1inch 开发者团队维护）开发的完整 CI/CD 交付工具，2021 年进入 CNCF Sandbox。它将 Git 作为**唯一真相来源（Single Source of Truth）**，集成镜像构建、镜像发布、Helm 部署和自动化清理为一体。werf 的核心理念是 **Giterminism（Git 终极主义）**——`werf.yaml` 中所有配置必须来自 Git 仓库，确保每次构建完全可复现。

werf 独特的**基于内容的标签策略（Content-based Tagging）**自动根据镜像层内容生成镜像标签（如 `8dfc5a1f3e6b4c2d`），确保相同代码总是产生相同镜像，彻底消除 `latest` 标签的歧义。它还提供增强的 Helm（werf helm），支持等待资源就绪、资源跟踪和回滚。

## Key Features

- **Giterminism**：所有构建配置来自 Git，确保可复现构建
- **基于内容的标签**：自动根据镜像内容生成确定性标签，消除 `latest` 歧义
- **多阶段构建**：支持声明式 Stage 定义和精确的缓存依赖控制
- **增强 Helm**：扩展的 Helm 部署，支持资源就绪等待和详细跟踪
- **自动清理**：定期清理未使用的镜像和 Helm Release，节省存储成本
- **收敛部署**：每次部署都是收敛操作，自动修正状态漂移

## Architecture

werf 由 **werf CLI**（单一二进制工具，集成构建、发布、部署、清理命令）、**werf.yaml**（构建配置，定义 Stage、镜像和部署）、**werf Helm**（扩展的 Helm 引擎）和 **werf Cleanup**（基于策略的镜像和 Release 清理器）组成。构建阶段（Stage）使用内容寻址缓存——只有当 Stage 的输入（源码/依赖）变化时才重新构建。部署阶段使用增强的 Helm，自动等待所有资源就绪并跟踪状态。

## K8s 集成

werf 通过增强的 Helm 与 Kubernetes 集成。`werf deploy` 命令执行 Helm 部署，自动等待所有 Deployment、Job、DaemonSet 达到 Ready 状态。支持多集群部署（通过 `--kube-config`）。也提供 werf Converge 模式——在 CI/CD 中一次命令完成构建+发布+部署+清理全流程。

## 生产部署要点

- **Giterminism**：保持 werf.yaml 中所有配置来自 Git，确保构建可复现
- **Stage 依赖**：使用 stageDependencies 精确控制缓存失效范围
- **基于内容的标签**：使用默认的 content-based 标签策略确保部署与构建一致
- **自动清理**：在 CI 中定期运行 `werf cleanup` 清理未使用的镜像
- **Helm values 分离**：为不同环境维护独立的 values 文件
- **资源跟踪**：利用 werf 的增强 Helm 部署监控资源就绪状态

## 生产场景

1. **GitOps CI/CD 流水线**：Git push 触发 werf converge，自动构建→部署→清理
2. **多环境部署**：一份 werf.yaml 配合不同 values 文件部署到 dev/staging/prod
3. **可复现构建**：同一 Git commit 总是产生相同镜像，便于回滚和审计
4. **镜像生命周期管理**：定期清理旧镜像，控制 Registry 存储成本

## 安装

```bash
# 安装 werf CLI
curl -sSL https://werf.io/install.sh | bash
# 或
brew install werf

# 创建 werf.yaml
cat > werf.yaml <<EOF
project: my-app
configVersion: 1
---
image: app
from: golang:1.21
git:
  - add: /app
    to: /app
shell:
  build:
    - cd /app && go build -o /app/main ./cmd/server
---
image: frontend
from: node:20
git:
  - add: /frontend
    to: /app
shell:
  build:
    - cd /app && npm ci && npm run build
EOF

# 一键构建+部署（Converge）
werf converge --repo registry.example.com/myapp \
  --kube-config ~/.kube/config \
  --release myapp --namespace myapp
```

## 对比

| 特性 | werf | Helm | ArgoCD | BuildKit |
|------|------|------|--------|---------|
| 构建+部署一体 | ✅ | ❌ 部署 only | ❌ 部署 only | ❌ 构建 only |
| 基于内容标签 | ✅ | ❌ | ❌ | ❌ |
| Giterminism | ✅ | ❌ | ❌ | ❌ |
| 自动清理 | ✅ | ❌ | ❌ | ❌ |

## 参考链接

- [[deployment]]
- [[概念/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[pipecd]] — PipeCD
- [[hami]] — HAMI
- [[open-policy-containers]] — [[实体/open-policy-containers.md|Open Policy Containers (OPCR)]]
- [[helm]] — Helm
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- werf
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
