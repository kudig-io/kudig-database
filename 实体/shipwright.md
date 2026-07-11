---
title: Shipwright (entities)
description: '## 概述'
summary: 'Shipwright 是一个在 Kubernetes 上构建容器镜像的框架。'
category: entities
tags:
- k8s
- cncf
- ci-cd
- shipwright
- cri-o
- crd
- operator
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Shipwright 是什么
- 如何 Shipwright
trigger_keywords:
- Shipwright
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Shipwright

> **CNCF 状态**: Sandbox | **类别**: CI/CD | **主要语言**: Go

## 概述

Shipwright 是一个在 Kubernetes 上构建容器镜像的框架，由 Red Hat 开发，2022 年加入 CNCF 沙箱。它抽象了底层构建工具的差异，通过统一的 CRD API 支持 Buildpacks、Buildah、BuildKit、Kaniko、ko 等多种构建策略。开发者只需定义源码位置（Git 仓库）和目标镜像（Registry 地址），Shipwright 自动管理构建过程——拉取源码、执行构建、推送镜像。与 Tekton 等通用 CI 流水线相比，Shipwright 专注于容器镜像构建这一特定领域，提供了更简洁的 API 和更完善的构建策略集成。它还支持构建策略的参数化配置和构建缓存。

## 核心能力

- **多构建策略**: 支持 Buildpacks、Buildah、BuildKit、Kaniko、ko、Source-to-Image (s2i) 等
- **统一 API**: 通过 BuildStrategy CRD 抽象不同构建工具的差异
- **Git 源码集成**: 支持从 Git 仓库拉取源码，支持分支/标签/PR
- **镜像推送**: 自动将构建结果推送到目标 Registry
- **参数化**: 构建策略支持参数化配置（Dockerfile 路径、构建参数等）
- **Tekton 集成**: 底层使用 Tekton Task 执行构建步骤

## 架构

Shipwright 基于 Tekton + CRD 模式：

- **Shipwright Controller**: 核心 Controller，监听 Build 和 BuildRun CRD
- **Build CRD**: 定义构建配置（源码仓库、目标镜像、构建策略、参数）
- **BuildRun CRD**: 触发构建执行的资源
- **BuildStrategy CRD**: 定义构建策略模板（Buildpacks/Buildah/BuildKit 等）
- **Tekton Task**: Shipwright 将 BuildRun 翻译为 Tekton Task 执行
- **Tekton Pod**: 实际执行构建的 Pod（拉取源码、构建镜像、推送）

构建流程：`BuildRun → Controller → Tekton Task → Pod → (git clone → build → push) → Registry`

## K8s 集成

Shipwright 以 Operator 模式部署在 Kubernetes 集群中，依赖 Tekton Pipelines 作为底层执行引擎。Build CRD 定义构建配置，BuildRun CRD 触发构建。Controller 将 BuildRun 翻译为 Tekton TaskRun，Tekton 创建 Pod 执行构建步骤（Git clone、镜像构建、Registry push）。Registry 凭据通过 Kubernetes Secret 管理。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的标准 CRD+Controller+Pod 模式和 Tekton 生态完全集成。

## 生产场景

1. **CI/CD 镜像构建**: 在 Tekton 流水线中使用 Shipwright 替代手写构建步骤
2. **多语言项目构建**: 不同语言项目使用不同的 BuildStrategy（Go 用 ko，Java 用 Buildpacks）
3. **无 Dockerfile 构建**: 使用 Buildpacks 策略，开发者无需编写 Dockerfile
4. **集群内构建**: 构建在集群内执行，利用集群的计算资源，无需外部 CI runner

## 安装

```bash
# 前置条件：安装 Tekton Pipelines
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/latest/release.yaml

# 安装 Shipwright Operator
kubectl apply -f https://github.com/shipwright-io/build/releases/latest/download/release.yaml

# 创建构建（使用 Buildpacks）
kubectl apply -f - <<EOF
apiVersion: shipwright.io/v1beta1
kind: Build
metadata:
  name: buildpacks-nodejs
spec:
  source:
    type: Git
    git:
      url: https://github.com/shipwright-io/sample-nodejs
    contextDir: source-build
  strategy:
    name: buildpacks-v3
    kind: ClusterBuildStrategy
  output:
    image: my-registry.io/myorg/nodejs-app:latest
    credentials:
      name: registry-credentials
---
apiVersion: shipwright.io/v1beta1
kind: BuildRun
metadata:
  name: buildpacks-nodejs-run
spec:
  build:
    name: buildpacks-nodejs
EOF

# 查看构建状态
kubectl get buildrun buildpacks-nodejs-run -w
kubectl logs buildrun-buildpacks-nodejs-run-pod -f
```

## 对比

| 特性 | Shipwright | Tekton | Kaniko | ko |
|------|-----------|--------|--------|-----|
| 多策略 | ✅ | ⚠️ 需手动 | ❌ 单一 | ❌ 单一 |
| K8s 原生 | ✅ CRD | ✅ CRD | ✡ Pod | ⚠️ CLI |
| 构建抽象 | ✅ | ❌ | ❌ | ❌ |
| CNCF 状态 | Sandbox | Graduated | 非 CNCF | Sandbox |

## 架构定位

在 CNCF 生态中，Shipwright 属于 **CI/CD** 类别，为云原生应用提供统一的容器镜像构建框架。

## 参考链接

- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/storage-model.md|storage-model]]
- [[概念/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[kubescape]] — Kubescape
- [[cedar]] — Cedar
- [[cri-o]] — CRI-O
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[buildpacks]] — Cloud Native Buildpacks

- shipwright
- [[实体/atlantis.md|Atlantis]]
- [[实体/dalec.md|Dalec]]
- [[实体/werf.md|werf]]
- [[实体/pipecd.md|PipeCD]]
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
