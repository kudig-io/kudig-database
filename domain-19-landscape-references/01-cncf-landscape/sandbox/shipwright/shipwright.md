---
title: Shipwright
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- docker
- crd
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Shipwright 是什么
- 如何 Shipwright
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Shipwright
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

title: Shipwright
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- docker
- crd
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Shipwright 是什么
- 如何 Shipwright
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Shipwright
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# Shipwright

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://shipwright.io/ |
| **GitHub** | https://github.com/shipwright-io/build |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Shipwright 是一个在 Kubernetes 上构建容器镜像的框架。它抽象了底层构建工具的差异，通过统一的 CRD API 支持 Buildpacks, Buildah, BuildKit, Kaniko 等多种构建策略。开发者只需定义源码位置和目标镜像，Shipwright 自动管理构建过程。

### 核心特性

- **多策略支持**: Buildpacks, Buildah, BuildKit, Kaniko, ko, S2I
- **声明式 API**: 通过 Build 和 BuildRun CRD 定义和触发构建
- **源码管理**: 支持 Git、本地上传和 Bundle 格式的源码输入
- **安全构建**: 不需要特权容器，支持无 Docker daemon 构建
- **可复用**: BuildStrategy 可在多个 Build 间共享
- **Tekton 集成**: 底层使用 Tekton TaskRun 执行构建任务

---

## 架构设计

```
┌──────────────────────────────────────────────────┐
│               Kubernetes Cluster                  │
│                                                   │
│  ┌─────────────┐    ┌────────────────────────┐   │
│  │ Build CRD   │───►│ Shipwright Controller  │   │
│  │             │    │                         │   │
│  │ BuildRun    │    │  Reconcile Build/       │   │
│  │ CRD        │    │  BuildRun resources     │   │
│  └─────────────┘    └───────────┬────────────┘   │
│                                 │                 │
│  ┌──────────────────────────────┴──────────────┐ │
│  │          ClusterBuildStrategy                │ │
│  │                                              │ │
│  │  ┌─────────┐ ┌────────┐ ┌────────────────┐ │ │
│  │  │Buildpacks│ │Buildah │ │  BuildKit      │ │ │
│  │  └─────────┘ └────────┘ └────────────────┘ │ │
│  │  ┌─────────┐ ┌────────┐ ┌────────────────┐ │ │
│  │  │ Kaniko  │ │  ko    │ │    S2I         │ │ │
│  │  └─────────┘ └────────┘ └────────────────┘ │ │
│  └──────────────────────────────────────────────┘ │
│                         │                         │
│  ┌──────────────────────┴──────────────────────┐ │
│  │         Tekton TaskRun (Execution)           │ │
│  └──────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# 安装 Tekton (前置依赖)
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/latest/release.yaml

# 安装 Shipwright Build Controller
kubectl apply -f https://github.com/shipwright-io/build/releases/latest/download/release.yaml

# 安装构建策略
kubectl apply -f https://github.com/shipwright-io/build/releases/latest/download/sample-strategies.yaml

# 安装 CLI
brew install shipwright-io/cli/shp
```

### 创建 Build

```yaml
apiVersion: shipwright.io/v1beta1
kind: Build
metadata:
  name: my-app-build
spec:
  source:
    type: Git
    git:
      url: https://github.com/my-org/my-app.git
      revision: main
    contextDir: .
  strategy:
    name: buildpacks-v3
    kind: ClusterBuildStrategy
  output:
    image: registry.example.com/my-app:latest
    pushSecret: registry-credentials
  paramValues:
    - name: run-image
      value: paketobuildpacks/run-jammy-base:latest
```

### 触发构建

```bash
# 使用 CLI 触发
shp buildrun create my-app-build-run --buildref-name my-app-build

# 或使用 YAML
kubectl apply -f - <<EOF
apiVersion: shipwright.io/v1beta1
kind: BuildRun
metadata:
  generateName: my-app-build-run-
spec:
  build:
    name: my-app-build
  serviceAccount:
    name: build-sa
EOF

# 查看构建日志
shp buildrun logs my-app-build-run
```

---

## 构建策略

### Buildpacks

```yaml
apiVersion: shipwright.io/v1beta1
kind: Build
metadata:
  name: buildpacks-build
spec:
  source:
    type: Git
    git:
      url: https://github.com/my-org/node-app.git
  strategy:
    name: buildpacks-v3
    kind: ClusterBuildStrategy
  output:
    image: registry.example.com/node-app:latest
```

### Buildah (Dockerfile)

```yaml
apiVersion: shipwright.io/v1beta1
kind: Build
metadata:
  name: dockerfile-build
spec:
  source:
    type: Git
    git:
      url: https://github.com/my-org/go-app.git
  strategy:
    name: buildah
    kind: ClusterBuildStrategy
  paramValues:
    - name: dockerfile
      value: Dockerfile.production
  output:
    image: registry.example.com/go-app:latest
```

### 自定义 BuildStrategy

```yaml
apiVersion: shipwright.io/v1beta1
kind: ClusterBuildStrategy
metadata:
  name: custom-kaniko
spec:
  parameters:
    - name: dockerfile
      description: Path to Dockerfile
      default: Dockerfile
  steps:
    - name: build
      image: gcr.io/kaniko-project/executor:latest
      command:
        - /kaniko/executor
      args:
        - --dockerfile=$(params.dockerfile)
        - --context=$(params.shp-source-context)
        - --destination=$(params.shp-output-image)
        - --snapshot-mode=redo
        - --push-retry=3
      resources:
        limits:
          cpu: "2"
          memory: 4Gi
```

---

## 最佳实践

1. **策略选择**: Go 应用用 ko，通用语言用 Buildpacks，需要精细控制用 Buildah
2. **镜像缓存**: 配置构建缓存加速重复构建
3. **安全**: 使用 Kaniko/Buildah 避免特权容器构建
4. **CI 集成**: 在 CI/CD pipeline 中创建 BuildRun 实现自动化构建
5. **多平台**: 使用支持多平台的策略构建 arm64/amd64 镜像
6. **资源限制**: 为构建 Pod 设置合理的 CPU/内存限制

---

## 参考资源

- [Shipwright 官方文档](https://shipwright.io/docs/)
- [Shipwright GitHub](https://github.com/shipwright-io/build)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[domain-17-system-foundation/topic-cheat-sheet/docker.md|docker]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/cncf-cicd|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
