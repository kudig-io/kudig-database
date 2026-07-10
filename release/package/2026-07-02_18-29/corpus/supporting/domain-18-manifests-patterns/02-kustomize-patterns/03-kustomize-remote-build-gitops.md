---
title: Kustomize 高级特性与 GitOps 集成
description: 'Remote Build、OCI-based Kustomization、ArgoCD 集成与 Git 子模块引用'
summary: 'Remote Build、OCI-based Kustomization、ArgoCD 集成与 Git 子模块引用'
category: manifests-patterns
tags:
- kustomize
- remote-build
- oci
- argocd
- gitops
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Kustomize Remote Build 是什么
- 如何在 ArgoCD 中使用 Kustomize
- Kustomize OCI 集成如何工作
trigger_keywords:
- kustomize
- remote-build
- oci
- argocd
- gitops
- git-submodule
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Kustomize 高级特性与 GitOps 集成

## 1. Remote Build

### 1.1 远程引用基础

```yaml
# 引用远程 Git 仓库中的 kustomization
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

resources:
  # GitHub 仓库
  - https://github.com/org/repo.git//path/to/kustomization?ref=v1.0.0

  # GitLab 仓库
  - https://gitlab.com/org/repo.git//path?ref=main

  # 私有仓库（需要认证）
  - https://github.com/org/private-repo.git//path?ref=v1.0.0
```

### 1.2 远程引用语法

```
# 格式
<repo-url>//<path>?<query-params>

# 查询参数
ref=<branch/tag/commit>    # Git 引用
version=<semver>           # 语义版本
submodule=<true/false>     # 是否包含子模块

# 示例
https://github.com/kubernetes-sigs/kustomize.git//examples/multibases?ref=v5.0.0
```

### 1.3 远程引用最佳实践

```yaml
# 推荐：使用标签（不可变）
resources:
  - https://github.com/org/repo.git//base?ref=v1.2.3

# 不推荐：使用分支（可变）
resources:
  - https://github.com/org/repo.git//base?ref=main

# 推荐：使用 commit hash（精确）
resources:
  - https://github.com/org/repo.git//base?ref=abc123def456
```

## 2. OCI-based Kustomization

### 2.1 OCI 镜像推送

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 构建并推送 kustomization 到 OCI 仓库
kustomize build overlays/prod | kubectl apply --dry-run=client -f -

# 使用 crane 推送 OCI 镜像
crane append -f <(kustomize build overlays/prod) \
  -t registry.example.com/kustomize/my-app:v1.0.0 \
  --new_tag registry.example.com/kustomize/my-app:v1.0.0

# 使用 oras 推送
oras push registry.example.com/kustomize/my-app:v1.0.0 \
  --manifest-config /dev/null:application/vnd.kustomize.config.v1+json \
  overlays/prod/kustomization.yaml:application/vnd.kustomize.file
```
### 2.2 OCI 引用

```yaml
# 从 OCI 仓库引用
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

resources:
  - oci://registry.example.com/kustomize/my-app:v1.0.0
```

### 2.3 OCI 版本管理

```bash
# 版本标签策略
registry.example.com/kustomize/my-app:v1.0.0    # 语义版本
registry.example.com/kustomize/my-app:latest     # 最新版本
registry.example.com/kustomize/my-app:sha-abc123 # Git commit

# 清理旧版本
crane ls registry.example.com/kustomize/my-app
```

## 3. ArgoCD 集成

### 3.1 ArgoCD Application 配置

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: my-app-prod
  namespace: argocd
  finalizers:
    - resources-finalizer.argocd.argoproj.io
spec:
  project: default
  source:
    repoURL: https://github.com/org/k8s-manifests.git
    targetRevision: main
    path: overlays/prod
    # Kustomize 特定配置
    kustomize:
      # 镜像覆盖
      images:
        - registry.example.com/my-app:v1.2.3
      # 名称前缀
      namePrefix: prod-
      # 通用标签
      commonLabels:
        environment: production
      # 通用注解
      commonAnnotations:
        company.example.com/owner: platform-team
      # 命名空间
      namespace: my-app-prod
  destination:
    server: https://kubernetes.default.svc
    namespace: my-app-prod
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    syncOptions:
      - CreateNamespace=true
      - PrunePropagationPolicy=foreground
    retry:
      limit: 5
      backoff:
        duration: 5s
        factor: 2
        maxDuration: 3m
```

### 3.2 ArgoCD ApplicationSet

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: my-app
  namespace: argocd
spec:
  generators:
    - list:
        elements:
          - env: dev
            revision: develop
            namespace: my-app-dev
          - env: staging
            revision: release
            namespace: my-app-staging
          - env: prod
            revision: main
            namespace: my-app-prod
  template:
    metadata:
      name: "my-app-{{env}}"
    spec:
      project: default
      source:
        repoURL: https://github.com/org/k8s-manifests.git
        targetRevision: "{{revision}}"
        path: "overlays/{{env}}"
        kustomize:
          namePrefix: "{{env}}-"
          commonLabels:
            environment: "{{env}}"
      destination:
        server: https://kubernetes.default.svc
        namespace: "{{namespace}}"
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
```

### 3.3 ArgoCD Kustomize 参数覆盖

```yaml
# 通过 ArgoCD 覆盖 kustomize 参数
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: my-app
spec:
  source:
    kustomize:
      # 覆盖镜像
      images:
        - my-app=registry.example.com/my-app:v2.0.0
      # 覆盖名称前缀
      namePrefix: staging-
      # 覆盖命名空间
      namespace: my-app-staging
      # 覆盖通用标签
      commonLabels:
        environment: staging
      # 覆盖通用注解
      commonAnnotations:
        last-deployed: "2026-07-02"
```

### 3.4 ArgoCD 与 Kustomize Components

```yaml
# ArgoCD 支持 Kustomize Components
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: my-app-prod
spec:
  source:
    kustomize:
      # 启用 components
      components:
        - components/monitoring
        - components/logging
```

## 4. Git 子模块引用

### 4.1 子模块配置

```bash
# 初始化子模块
git submodule add https://github.com/org/k8s-base.git base
git submodule add https://github.com/org/k8s-components.git components

# 更新子模块
git submodule update --init --recursive
git submodule update --remote
```

### 4.2 子模块目录结构

```
my-app/
├── .gitmodules
├── base/                 # 子模块
│   ├── kustomization.yaml
│   └── deployment.yaml
├── components/          # 子模块
│   ├── monitoring/
│   └── logging/
├── overlays/
│   ├── dev/
│   │   └── kustomization.yaml
│   └── prod/
│       └── kustomization.yaml
└── kustomization.yaml
```

```yaml
# .gitmodules
[submodule "base"]
    path = base
    url = https://github.com/org/k8s-base.git
    branch = main

[submodule "components"]
    path = components
    url = https://github.com/org/k8s-components.git
    branch = main
```

### 4.3 子模块版本管理

```bash
# 锁定子模块版本
cd base
git checkout v1.0.0
cd ..
git add base
git commit -m "Lock base submodule to v1.0.0"

# 更新子模块版本
cd base
git fetch
git checkout v1.1.0
cd ..
git add base
git commit -m "Update base submodule to v1.1.0"
```

### 4.4 子模块在 CI/CD 中的使用

```yaml
# GitHub Actions
name: Deploy
on:
  push:
    branches: [main]

jobs:
  deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Build Kustomize
        run: kustomize build overlays/prod

      - name: Deploy
        run: kubectl apply -k overlays/prod
```

## 5. Kustomize 插件系统

### 5.1 内置插件

```yaml
# 使用内置插件
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

# Replacement Transformer（内置）
replacements:
  - source:
      kind: ConfigMap
      name: app-config
      fieldPath: data.VERSION
    targets:
      - select:
          kind: Deployment
        fieldPaths:
          - spec.template.metadata.labels.version
```

### 5.2 自定义插件（External）

```yaml
# 自定义插件配置
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

generators:
  - plugin.yaml

# plugin.yaml
apiVersion: kustomize.config.k8s.io/v1
kind: KustomizationPlugin
metadata:
  name: custom-generator
pluginType: exec
args:
  - --input=config.yaml
```

## 6. 多仓库 GitOps 模式

### 6.1 应用仓库与配置仓库分离

```
应用代码仓库：
  src/
    main.py
    Dockerfile
  k8s/
    base/
      kustomization.yaml
      deployment.yaml
    overlays/
      dev/
      prod/

配置仓库（GitOps）：
  apps/
    my-app/
      overlays/
        dev/
        prod/
    other-app/
      overlays/
        dev/
        prod/
```

### 6.2 配置仓库 Kustomization

```yaml
# config-repo/apps/my-app/overlays/prod/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

resources:
  # 引用应用仓库的 base
  - https://github.com/org/app-repo.git//k8s/base?ref=v1.2.3

# 应用 prod 特定配置
namespace: my-app-prod
namePrefix: prod-

patches:
  - target:
      kind: Deployment
      name: my-app
    patch: |-
      apiVersion: apps/v1
      kind: Deployment
      metadata:
        name: my-app
      spec:
        replicas: 3

images:
  - name: my-app
    newName: registry.example.com/my-app
    newTag: v1.2.3
```

## 7. 高级构建技巧

### 7.1 构建输出控制

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 仅输出特定资源类型
kustomize build overlays/prod | kubectl get -f -

# 输出为 JSON
kustomize build overlays/prod -o json

# 输出到文件
kustomize build overlays/prod -o prod.yaml

# 与 kubectl 集成
kubectl apply -k overlays/prod
kubectl diff -k overlays/prod
kubectl delete -k overlays/prod
```
### 7.2 构建验证

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 验证 kustomization 语法
kustomize build overlays/prod > /dev/null

# 验证资源 schema
kubectl apply --dry-run=client -k overlays/prod

# 验证资源清单
kustomize build overlays/prod | kubectl apply --dry-run=server -f -
```
### 7.3 性能优化

```yaml
# 使用 kustomize 构建缓存
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

# 禁用不必要的转换器
sortOptions:
  order: fifo

# 避免深度嵌套
# 推荐：最多 3 层
# base → overlays/env → overlays/env/region
```

## 8. 常见问题排查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 问题 1：远程引用失败
# 检查 Git 访问权限
git ls-remote https://github.com/org/repo.git

# 问题 2：子模块未初始化
git submodule update --init --recursive

# 问题 3：ArgoCD 同步失败
# 检查 ArgoCD 日志
kubectl logs -n argocd deployment/argocd-repo-server

# 问题 4：OCI 镜像拉取失败
crane manifest registry.example.com/kustomize/my-app:v1.0.0
```
---

## Related

- [[domain-18-manifests-patterns/Kustomize模式/01-kustomize-base-overlay-structure|Kustomize Base/Overlay 分层架构]]
- [[domain-18-manifests-patterns/Kustomize模式/02-kustomize-transformers-reference|Kustomize Transformers 参考]]

## See Also

- [Kustomize Remote Build](https://kubectl.docs.kubernetes.io/references/kustomize/)
- [ArgoCD Kustomize](https://argo-cd.readthedocs.io/en/stable/user-guide/kustomize/)


<!-- risk-assessed -->
