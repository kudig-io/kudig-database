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

## 安装与配置

```bash
# 前置条件：安装 Tekton Pipelines
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/latest/release.yaml

# 安装 Shipwright Operator
kubectl apply -f https://github.com/shipwright-io/build/releases/latest/download/release.yaml
kubectl get pods -n shipwright-build
```

### Build CRD 配置

```yaml
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
  timeout: 15m
  resources:
    limits:
      cpu: "2"
      memory: 2Gi
---
apiVersion: shipwright.io/v1beta1
kind: BuildRun
metadata:
  name: buildpacks-nodejs-run
spec:
  build:
    name: buildpacks-nodejs
```

```bash
# 查看构建状态
kubectl get buildrun buildpacks-nodejs-run -w
kubectl logs buildrun-buildpacks-nodejs-run-pod -f
```

## 运维操作

```bash
# 🟢 查看构建状态
kubectl get builds,buildruns -A
kubectl describe buildrun <name>

# 🟢 查看构建日志
kubectl logs buildrun-<name>-pod -f

# 🟡 触发新构建
kubectl apply -f buildrun.yaml

# 🟡 更新构建策略
kubectl apply -f build-updated.yaml

# 🔴 删除构建及历史
kubectl delete build buildpacks-nodejs
kubectl delete buildrun --all
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| BuildRun 失败 | Git 仓库不可达 | `kubectl describe buildrun` | 检查 Git URL 和凭据 |
| 镜像推送失败 | Registry 凭据错误 | `kubectl logs buildrun-pod` | 更新 Secret |
| 构建超时 | 资源不足 | `kubectl top pod` | 增加 resources |
| 策略不存在 | ClusterBuildStrategy 未安装 | `kubectl get clusterbuildstrategy` | 安装对应策略 |
| Tekton 任务失败 | Pipeline 版本不兼容 | `kubectl logs -n tekton-pipelines` | 升级 Tekton |

```
排查流程:
├── 构建失败
│   ├── kubectl describe buildrun → 查看 Conditions
│   ├── kubectl logs buildrun-pod → 构建日志
│   └── 确认 Git/Registry 凭据有效
├── 策略问题
│   ├── kubectl get clusterbuildstrategy → 确认可用
│   └── 检查 Build spec.strategy 名称匹配
└── 性能问题
    ├── 检查构建 Pod 资源使用
    ├── 优化 Dockerfile/构建缓存
    └── 调整 timeout 和 resources
```

## 生产案例

### 案例 1: 统一多语言构建平台

- **场景**: 团队使用 Java/Node/Go 多语言，构建工具分散
- **方案**: 部署 Shipwright，为每种语言配置对应的 BuildStrategy(Buildpacks/Kaniko/ko)；开发者只需提交 Build CR
- **效果**: 构建流程统一，新服务接入从 1 天缩短到 30min

### 案例 2: CI/CD 构建加速

- **场景**: Docker 构建平均 8min，影响发布效率
- **方案**: 使用 Buildpacks 策略替代 Dockerfile；配置构建缓存 PVC；并行构建多组件
- **效果**: 构建时间从 8min 降低到 2min，缓存命中率 85%

## 对比

| 特性 | Shipwright | Tekton | Kaniko | ko | 适用场景 |
|------|-----------|--------|--------|-----|----------|
| 多策略 | ✅ | ⚠️ 需手动 | ❌ 单一 | ❌ 单一 | 多语言 |
| K8s 原生 | ✅ CRD | ✅ CRD | ✡ Pod | ⚠️ CLI | 云原生 |
| 构建抽象 | ✅ | ❌ | ❌ | ❌ | 统一接口 |
| 缓存 | ✅ | ⚠️ | ⚠️ | ✅ | 加速 |
| CNCF 状态 | Sandbox | Graduated | 非 CNCF | Sandbox | 生态 |

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
