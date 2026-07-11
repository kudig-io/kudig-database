---
title: KitOps (entities)
description: '## 概述'
summary: 'KitOps 是一个 MLOps/AI 工件打包和版本管理工具，使用 OCI 标准将 AI/ML 项目的所有组件（模型权重、数据集、代码、配置）打包为称为 ModelKit 的 OCI Artifact。它允许数据科学家和 ML 工程师像管理容器镜像一样管理 AI 模型全生命周期的工件，并通过标准容器注册中心进行分发。'
category: entities
tags:
- k8s
- cncf
- image
- kitops
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
- KitOps 是什么
- 如何 KitOps
trigger_keywords:
- KitOps
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KitOps

> **CNCF 状态**: Sandbox | **类别**: Image | **主要语言**: Go

## 概述

KitOps 是由 Jozu（原 Anaconda 团队成员创立）开发的开源 AI/ML 工件打包和版本管理工具，2023 年进入 CNCF Sandbox。它解决了 MLOps 领域的核心痛点：**AI 项目的工件分散管理**——模型权重、训练数据集、推理代码、配置文件、依赖清单存储在不同位置，版本不统一，导致"训练环境能跑、生产环境报错"的可复现性问题。

KitOps 使用 **OCI 标准**将 AI/ML 项目的所有组件打包为称为 **ModelKit** 的 OCI Artifact。一个 ModelKit 包含模型权重、数据集、代码、Kitfile（类似 Dockerfile 的构建描述）和元数据，通过标准容器注册中心（Docker Registry、Harbor、zot 等）分发。这使得 AI 团队可以像管理容器镜像一样管理模型全生命周期的工件，实现版本化、可追溯和可复现。

## Key Features

- **ModelKit 打包**：将模型、数据、代码、配置打包为一个 OCI Artifact
- **Kitfile 规范**：声明式 YAML 描述 ModelKit 的组成部分
- **OCI 兼容**：通过标准 OCI Registry 分发，与容器生态无缝集成
- **分层打包**：大文件（模型权重）独立 OCI 层，利用层缓存加速拉取
- **版本可追溯**：每个 ModelKit 版本绑定特定的代码 commit 和数据版本
- **多目标部署**：同一 ModelKit 可部署到开发环境、推理服务器或边缘设备

## Architecture

KitOps 由 **kit CLI**（命令行工具，打包/推送/拉取 ModelKit）、**Kitfile**（类似 Dockerfile 的构建描述，定义 ModelKit 的组件和层次）和 **OCI Registry 集成**（将 ModelKit 存储为标准 OCI Artifact）组成。`kit pack` 命令根据 Kitfile 将本地文件打包为 ModelKit OCI 镜像，`kit push` 推送到 Registry，`kit pull` 拉取到部署环境。Kitfile 中的每个组件（model、code、dataset、docs）对应独立的 OCI 层。

## K8s 集成

KitOps 与 Kubernetes 通过推理框架（KServe、vLLM、Triton）集成。ModelKit 拉取到 K8s 节点后，推理框架加载模型权重启动推理服务。也支持通过 init container 在 Pod 启动时拉取 ModelKit，避免将大文件构建到镜像中。

## 生产部署要点

- **版本策略**：使用语义版本标签管理 ModelKit，保持可追溯性
- **分层打包**：将大文件（模型权重）放在独立层，利用 OCI 层缓存加速拉取
- **元数据完善**：在 Kitfile 中详细记录模型参数、训练配置和评估指标
- **CI/CD 集成**：训练完成后自动打包推送 ModelKit
- **安全扫描**：对 ModelKit 中的代码部分进行安全扫描

## 生产场景

1. **模型版本管理**：每个训练版本打包为 ModelKit，版本化存储到 Registry
2. **跨环境部署**：同一 ModelKit 从开发→测试→生产环境一致部署
3. **模型审计**：通过 ModelKit 的 OCI 签名验证模型完整性，满足合规审计
4. **团队协作**：数据科学家通过 Registry 共享 ModelKit，推理工程师拉取部署

## 安装

```bash
# 安装 kit CLI
brew tap kitops/kitops
brew install kitops
# 或
curl -fsSL https://github.com/kitops-ml/kitops/releases/latest/download/kit-linux-amd64 -o kit
chmod +x kit && sudo mv kit /usr/local/bin/

# 创建 Kitfile
cat > Kitfile <<EOF
manifestVersion: 1.0.0
package:
  name: sentiment-bert
  version: 1.0.0
  description: BERT sentiment analysis model
model:
  name: bert-base-sentiment
  path: ./model/
  framework: pytorch
code:
  - path: ./src/inference.py
    description: Inference script
datasets:
  - name: training-data
    path: ./data/train.csv
docs:
  - path: ./README.md
EOF

# 打包 ModelKit
kit pack .
# 推送到 OCI Registry
kit push sentiment-bert:1.0.0 registry.example.com/models/sentiment-bert:1.0.0
# 在部署环境拉取
kit pull registry.example.com/models/sentiment-bert:1.0.0
kit unpack sentiment-bert:1.0.0 --destdir /app/model
```

## 对比

| 特性 | KitOps | MLflow | DVC | Weights & Biases |
|------|--------|--------|-----|-----------------|
| OCI 打包 | ✅ | ❌ | ❌ | ❌ |
| 模型+代码+数据一体 | ✅ | ⚠️ | ⚠️ | ❌ |
| Registry 分发 | ✅ 标准 OCI | ❌ | ⚠️ | ❌ |
| 开源 | ✅ | ✅ | ✅ | ❌ |

## 参考链接

- [[概念/secrets-management.md|secrets-management]]
- [[概念/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[slimfaas]] — SlimFaas
- [[tuf]] — TUF
- [[kcl]] — KCL (Kusion Configuration Language)
- [[kube-vip]] — kube-vip
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kitops
- [[实体/slimtoolkit.md|[[SlimToolkit|SlimToolkit]]]]
- [[实体/modelpack.md|ModelPack]]
- [[实体/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
