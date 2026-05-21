---
title: Carvel
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- helm
- docker
- ingress
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Carvel 是什么
- 如何 Carvel
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Carvel
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- tls-basics
---

title: Carvel
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- docker
- ingress
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Carvel 是什么
- 如何 Carvel
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Carvel
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

# Carvel

> **成熟度**: Sandbox | **加入时间**: 2022-09 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://carvel.dev |
| **GitHub** | https://github.com/carvel-dev |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | App Definition & Build |
| **维护组织** | VMware (Broadcom) |

---

## 项目概述

Carvel 是一组专注于 Kubernetes 应用构建、配置和部署的工具集。它采用 Unix 哲学，每个工具专注于单一任务并可组合使用。主要包括 ytt (YAML 模板)、kbld (镜像构建)、kapp (应用部署)、imgpkg (OCI 镜像打包)、vendir (依赖管理) 和 kapp-controller (GitOps)。

---

## 核心工具

| 工具 | 功能 |
|:---|:---|
| **ytt** | YAML 模板和覆盖层引擎 |
| **kbld** | 镜像构建和解析 |
| **kapp** | 应用生命周期管理 |
| **imgpkg** | OCI 镜像打包工具 |
| **vendir** | 依赖获取和同步 |
| **kapp-controller** | Package 和 GitOps 控制器 |
| **secretgen-controller** | Secret 生成控制器 |

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                      Carvel Toolchain                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    Development Phase                      │   │
│  │                                                           │   │
│  │  ┌─────────────┐     ┌─────────────┐     ┌────────────┐ │   │
│  │  │   Source    │────►│     ytt     │────►│   kbld    │  │   │
│  │  │   YAML +    │     │  (Template  │     │  (Image   │  │   │
│  │  │   Starlark  │     │   Engine)   │     │  Builder) │  │   │
│  │  └─────────────┘     └─────────────┘     └─────────────┘ │   │
│  │                                                           │   │
│  │  ┌─────────────┐     ┌─────────────┐                     │   │
│  │  │   vendir    │────►│  External   │                     │   │
│  │  │  (Fetch     │     │  Deps (Helm,│                     │   │
│  │  │   Deps)     │     │   Git, etc) │                     │   │
│  │  └─────────────┘     └─────────────┘                     │   │
│  └─────────────────────────────────────────────────────────┘   │
│                              │                                   │
│                              ▼                                   │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   Packaging Phase                         │   │
│  │                                                           │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │                    imgpkg                            │ │   │
│  │  │  ┌─────────────────────────────────────────────┐   │ │   │
│  │  │  │              Bundle (OCI Image)              │   │ │   │
│  │  │  │  ┌─────────────┐  ┌─────────────────────┐  │   │ │   │
│  │  │  │  │ .imgpkg/    │  │   config/           │  │   │ │   │
│  │  │  │  │ bundle.yml  │  │   (YAML files)      │  │   │ │   │
│  │  │  │  │ images.yml  │  │                     │  │   │ │   │
│  │  │  │  └─────────────┘  └─────────────────────┘  │   │ │   │
│  │  │  └─────────────────────────────────────────────┘   │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────┬──────────────────────────┘   │
│                                 │                               │
│                                 ▼                               │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   Deployment Phase                        │   │
│  │                                                           │   │
│  │  ┌──────────────────────────────┐  ┌───────────────────┐ │   │
│  │  │           kapp               │  │  kapp-controller  │ │   │
│  │  │  ┌────────────────────────┐  │  │  ┌─────────────┐  │ │   │
│  │  │  │   App = Set of        │  │  │  │   Package   │  │ │   │
│  │  │  │   Resources with      │  │  │  │ Repository  │  │ │   │
│  │  │  │   Ordered Deploy      │  │  │  │             │  │ │   │
│  │  │  └────────────────────────┘  │  │  │ PackageRepo │  │ │   │
│  │  │  ┌────────────────────────┐  │  │  │ Package     │  │ │   │
│  │  │  │ • Diff before apply   │  │  │  │ PackageInst │  │ │   │
│  │  │  │ • Wait for readiness  │  │  │  └─────────────┘  │ │   │
│  │  │  │ • Resource ownership  │  │  │                   │ │   │
│  │  │  └────────────────────────┘  │  │  ┌─────────────┐  │ │   │
│  │  └──────────────────────────────┘  │  │     App     │  │ │   │
│  │                                     │  │  (GitOps)   │  │ │   │
│  │                                     │  └─────────────┘  │ │   │
│  │                                     └───────────────────┘ │   │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

---

## ytt - YAML 模板引擎

### 安装

```bash
# macOS
brew install ytt

# Linux
curl -L https://carvel.dev/install.sh | bash
```

### 基本模板

```yaml
#! config.yml
#@ load("@ytt:data", "data")

apiVersion: apps/v1
kind: Deployment
metadata:
  name: #@ data.values.app_name
spec:
  replicas: #@ data.values.replicas
  selector:
    matchLabels:
      app: #@ data.values.app_name
  template:
    metadata:
      labels:
        app: #@ data.values.app_name
    spec:
      containers:
        - name: app
          image: #@ data.values.image
          ports:
            - containerPort: #@ data.values.port
```

```yaml
#! values.yml
#@data/values
---
app_name: my-app
replicas: 3
image: nginx:latest
port: 80
```

### Overlay 覆盖

```yaml
#! overlays/production.yml
#@ load("@ytt:overlay", "overlay")

#@overlay/match by=overlay.subset({"kind": "Deployment"})
---
spec:
  replicas: 5
  template:
    spec:
      #@overlay/match missing_ok=True
      resources:
        limits:
          cpu: "2"
          memory: "4Gi"
```

### 使用 ytt

```bash
# 渲染模板
ytt -f config.yml -f values.yml

# 应用覆盖层
ytt -f config/ -f overlays/production.yml

# 使用数据值
ytt -f config/ --data-value app_name=production-app

# 从文件加载数据值
ytt -f config/ --data-values-file prod-values.yml
```

---

## kbld - 镜像构建

### 配置文件

```yaml
#! kbld.yml
apiVersion: kbld.k14s.io/v1alpha1
kind: Config
sources:
  - image: my-app
    path: .
    docker:
      build:
        file: Dockerfile
        
destinations:
  - image: my-app
    newImage: registry.example.com/my-app

overrides:
  - image: nginx
    newImage: registry.example.com/nginx@sha256:abc123
```

### 使用 kbld

```bash
# 解析并锁定镜像
ytt -f config/ | kbld -f -

# 构建并推送镜像
ytt -f config/ | kbld -f - --build

# 生成锁文件
kbld -f config/ --lock-output kbld.lock.yml
```

---

## kapp - 应用部署

### 基本部署

```bash
# 部署应用
kapp deploy -a my-app -f config/

# 使用 ytt + kbld + kapp 管道
ytt -f config/ | kbld -f - | kapp deploy -a my-app -f -

# 查看差异
kapp deploy -a my-app -f config/ --diff-changes

# 列出应用
kapp list

# 查看应用详情
kapp inspect -a my-app

# 删除应用
kapp delete -a my-app
```

### kapp 配置注解

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
  annotations:
    # 部署顺序
    kapp.k14s.io/change-group: apps
    kapp.k14s.io/change-rule: "upsert after upserting configmaps"
    
    # 更新策略
    kapp.k14s.io/update-strategy: fallback-on-replace
    
    # 所有权
    kapp.k14s.io/owned-for-deletion: ""
```

---

## imgpkg - Bundle 打包

### 创建 Bundle

```bash
# 目录结构
my-bundle/
├── .imgpkg/
│   └── bundle.yml
└── config/
    ├── deployment.yml
    └── service.yml

# 推送 Bundle
imgpkg push -b registry.example.com/my-bundle:v1.0.0 -f my-bundle/

# 拉取 Bundle
imgpkg pull -b registry.example.com/my-bundle:v1.0.0 -o ./downloaded-bundle

# 复制 Bundle 到另一个仓库
imgpkg copy -b registry.example.com/my-bundle:v1.0.0 \
  --to-repo internal-registry.example.com/my-bundle
```

### Bundle 配置

```yaml
#! .imgpkg/bundle.yml
apiVersion: imgpkg.carvel.dev/v1alpha1
kind: Bundle
metadata:
  name: my-app-bundle
authors:
  - name: Platform Team
    email: platform@example.com
websites:
  - url: https://github.com/example/my-app
```

---

## vendir - 依赖管理

### vendir 配置

```yaml
#! vendir.yml
apiVersion: vendir.k14s.io/v1alpha1
kind: Config
directories:
  - path: vendor/helm
    contents:
      - path: nginx-ingress
        helmChart:
          name: ingress-nginx
          version: 4.8.0
          repository:
            url: https://kubernetes.github.io/ingress-nginx
            
  - path: vendor/git
    contents:
      - path: common-lib
        git:
          url: https://github.com/example/common-lib
          ref: v1.2.0
          
  - path: vendor/github
    contents:
      - path: crds
        githubRelease:
          slug: cert-manager/cert-manager
          tag: v1.13.0
          assetNames: ["cert-manager.crds.yaml"]
```

### 使用 vendir

```bash
# 同步依赖
vendir sync

# 查看锁文件
cat vendir.lock.yml
```

---

## kapp-controller - GitOps

### 安装 kapp-controller

```bash
kubectl apply -f https://github.com/carvel-dev/kapp-controller/releases/latest/download/release.yml
```

### Package Repository

```yaml
apiVersion: packaging.carvel.dev/v1alpha1
kind: PackageRepository
metadata:
  name: my-packages
  namespace: kapp-controller-packaging-global
spec:
  fetch:
    imgpkgBundle:
      image: registry.example.com/my-packages:latest
```

### Package 定义

```yaml
apiVersion: data.packaging.carvel.dev/v1alpha1
kind: Package
metadata:
  name: nginx.example.com.1.0.0
spec:
  refName: nginx.example.com
  version: 1.0.0
  releaseNotes: "Initial release"
  template:
    spec:
      fetch:
        - imgpkgBundle:
            image: registry.example.com/nginx-bundle:v1.0.0
      template:
        - ytt:
            paths:
              - config/
        - kbld:
            paths:
              - "-"
              - .imgpkg/images.yml
      deploy:
        - kapp: {}
```

### Package Install

```yaml
apiVersion: packaging.carvel.dev/v1alpha1
kind: PackageInstall
metadata:
  name: nginx
  namespace: default
spec:
  serviceAccountName: default-sa
  packageRef:
    refName: nginx.example.com
    versionSelection:
      constraints: ">=1.0.0"
  values:
    - secretRef:
        name: nginx-values

---
apiVersion: v1
kind: Secret
metadata:
  name: nginx-values
stringData:
  values.yml: |
    replicas: 3
    service_type: LoadBalancer
```

### App (GitOps)

```yaml
apiVersion: kappctrl.k14s.io/v1alpha1
kind: App
metadata:
  name: my-app
  namespace: default
spec:
  serviceAccountName: default-sa
  fetch:
    - git:
        url: https://github.com/example/my-app
        ref: origin/main
        subPath: deploy
  template:
    - ytt: {}
    - kbld: {}
  deploy:
    - kapp: {}
  syncPeriod: 5m
```

---

## 完整工作流示例

```bash
# 1. 使用 vendir 获取依赖
vendir sync

# 2. 使用 ytt 渲染模板
ytt -f config/ -f values/production.yml -f vendor/ > rendered.yml

# 3. 使用 kbld 解析镜像
kbld -f rendered.yml > resolved.yml

# 4. 使用 kapp 部署
kapp deploy -a my-app -f resolved.yml

# 或者组合使用
ytt -f config/ -f values/production.yml | \
  kbld -f - | \
  kapp deploy -a my-app -f -
```

---

## 最佳实践

1. **模块化配置**: 使用 ytt overlay 分离环境配置
2. **镜像锁定**: 使用 kbld 锁定镜像 digest
3. **Bundle 打包**: 使用 imgpkg 打包可重定位的应用
4. **依赖管理**: 使用 vendir 统一管理外部依赖
5. **GitOps**: 使用 kapp-controller 实现声明式部署
6. **版本控制**: 锁文件纳入版本控制

---

## 参考资源

- [官方文档](https://carvel.dev/)
- [ytt 文档](https://carvel.dev/ytt/)
- [kapp 文档](https://carvel.dev/kapp/)
- [kapp-controller](https://carvel.dev/kapp-controller/)
- [GitHub](https://github.com/carvel-dev)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[man/INSTALL.md|INSTALL]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/gitops.md|gitops]]
- [[entities/cncf-cicd|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
