---
title: DevSpace
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- helm
- docker
- redis
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- DevSpace 是什么
- 如何 DevSpace
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- DevSpace
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- redis-basics
---

title: DevSpace
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
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
- DevSpace 是什么
- 如何 DevSpace
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- DevSpace
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

# DevSpace

> **成熟度**: Sandbox | **加入时间**: 2021-04 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://devspace.sh |
| **GitHub** | https://github.com/devspace-sh/devspace |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | App Definition & Build |
| **维护组织** | Loft Labs |

---

## 项目概述

DevSpace 是一款开源的 Kubernetes 开发工具，旨在简化云原生应用的开发工作流。它提供热重载、实时同步、远程调试等功能，让开发者可以直接在 Kubernetes 集群中开发和测试应用，而无需在本地环境复现复杂的微服务架构。

---

## 核心特性

- **热重载**: 代码更改自动同步并重新部署
- **文件同步**: 双向文件同步到容器
- **端口转发**: 自动管理端口转发
- **远程调试**: 支持 VS Code 和 IDE 远程调试
- **日志流**: 实时聚合多 Pod 日志
- **依赖管理**: 管理服务间依赖顺序
- **配置灵活**: 支持配置文件和变量

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                    DevSpace Architecture                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                  Developer Workstation                    │   │
│  │                                                           │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │                    DevSpace CLI                      │ │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │ │   │
│  │  │  │   Build     │  │   Deploy    │  │   Sync     │  │ │   │
│  │  │  │   Engine    │  │   Engine    │  │   Engine   │  │ │   │
│  │  │  └──────┬──────┘  └──────┬──────┘  └─────┬──────┘  │ │   │
│  │  │         │                │               │          │ │   │
│  │  │  ┌──────▼──────┐  ┌──────▼──────┐  ┌────▼───────┐  │ │   │
│  │  │  │   Docker    │  │   Helm/     │  │  File      │  │ │   │
│  │  │  │   Buildx    │  │   Kubectl   │  │  Watcher   │  │ │   │
│  │  │  └─────────────┘  └─────────────┘  └────────────┘  │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  │                                                           │   │
│  │  ┌───────────────┐  ┌────────────────────────────────┐  │   │
│  │  │ Source Code   │  │      devspace.yaml             │  │   │
│  │  │ ./src         │  │  ┌────────────────────────┐    │  │   │
│  │  │ ./Dockerfile  │  │  │ images:                │    │  │   │
│  │  │               │  │  │ deployments:           │    │  │   │
│  │  │               │  │  │ dev:                   │    │  │   │
│  │  │               │  │  │ profiles:              │    │  │   │
│  │  │               │  │  └────────────────────────┘    │  │   │
│  │  └───────────────┘  └────────────────────────────────┘  │   │
│  └──────────────────────────────┬──────────────────────────┘   │
│                                 │                               │
│                         ┌───────▼───────┐                      │
│                         │  Port Forward │                      │
│                         │  File Sync    │                      │
│                         │  Log Stream   │                      │
│                         └───────┬───────┘                      │
│                                 │                               │
│  ┌──────────────────────────────▼──────────────────────────┐   │
│  │                   Kubernetes Cluster                      │   │
│  │                                                           │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │              Developer Namespace                     │ │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │ │   │
│  │  │  │   App Pod   │  │   DB Pod    │  │  Cache Pod │  │ │   │
│  │  │  │ ┌─────────┐ │  │ ┌─────────┐ │  │ ┌────────┐ │  │ │   │
│  │  │  │ │Container│◄├──┼─┤Container│ │  │ │Container│ │  │ │   │
│  │  │  │ │ + Sync  │ │  │ └─────────┘ │  │ └────────┘ │  │ │   │
│  │  │  │ │ Helper  │ │  └─────────────┘  └────────────┘  │ │   │
│  │  │  │ └─────────┘ │                                    │ │   │
│  │  │  │   ▲         │                                    │ │   │
│  │  │  │   │ Synced  │                                    │ │   │
│  │  │  │   │ Files   │                                    │ │   │
│  │  │  └───┼─────────┘                                    │ │   │
│  │  └──────┼──────────────────────────────────────────────┘ │   │
│  └─────────┼────────────────────────────────────────────────┘  │
│            │                                                    │
│    ┌───────▼───────┐                                           │
│    │ Hot Reload    │                                           │
│    │ (File Change) │                                           │
│    └───────────────┘                                           │
└─────────────────────────────────────────────────────────────────┘
```

| 组件 | 说明 |
|:---|:---|
| **DevSpace CLI** | 命令行工具，管理开发流程 |
| **Build Engine** | 构建镜像，支持 Docker/Kaniko/BuildKit |
| **Deploy Engine** | 部署应用，支持 Helm/Kustomize/Kubectl |
| **Sync Engine** | 文件同步，实时传输代码更改 |
| **devspace.yaml** | 配置文件，定义开发环境 |

---

## 快速开始

### 安装 DevSpace

```bash
# macOS/Linux
curl -L -o devspace https://github.com/devspace-sh/devspace/releases/latest/download/devspace-$(uname -s | tr '[:upper:]' '[:lower:]')-$(uname -m | sed 's/x86_64/amd64/;s/aarch64/arm64/')
chmod +x devspace
sudo mv devspace /usr/local/bin/

# Homebrew
brew install devspace

# Windows (PowerShell)
md -Force "$Env:APPDATA\devspace"; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -URI "https://github.com/devspace-sh/devspace/releases/latest/download/devspace-windows-amd64.exe" -o $Env:APPDATA\devspace\devspace.exe; $env:Path += ";$Env:APPDATA\devspace"; [Environment]::SetEnvironmentVariable("Path", $env:Path, [System.EnvironmentVariableTarget]::User);
```

### 初始化项目

```bash
# 在项目目录中初始化
cd my-project
devspace init

# 选择部署方式：
# 1. Helm Chart
# 2. Kubectl manifests
# 3. Kustomize
# 4. Component Chart (DevSpace 内置)
```

---

## 配置文件

### 基本 devspace.yaml

```yaml
version: v2beta1
name: my-app

# 镜像配置
images:
  app:
    image: my-registry/my-app
    dockerfile: ./Dockerfile
    context: ./
    rebuildStrategy: ignoreContextChanges

# 部署配置
deployments:
  app:
    helm:
      chart:
        name: ./chart
      values:
        image: ${images.app}

# 开发模式配置
dev:
  app:
    imageSelector: ${images.app}
    devImage: node:18-alpine
    
    # 文件同步
    sync:
      - path: ./src:/app/src
        excludePaths:
          - node_modules/
        onUpload:
          restartContainer: true
    
    # 端口转发
    ports:
      - port: "3000"
      - port: "9229"  # Debug port
    
    # 终端
    terminal:
      command: /bin/sh
    
    # 日志
    logs:
      enabled: true

# 依赖
dependencies:
  database:
    git:
      url: https://github.com/example/postgres-chart
      branch: main
    
# 命令
commands:
  migrate:
    command: npm run migrate
  test:
    command: npm test
```

### 多环境配置

```yaml
version: v2beta1
name: my-app

# 变量定义
vars:
  - name: IMAGE_REGISTRY
    default: docker.io
  - name: ENVIRONMENT
    default: dev

# 默认镜像配置
images:
  app:
    image: ${IMAGE_REGISTRY}/my-app

# Profile 配置
profiles:
  - name: production
    patches:
      - op: replace
        path: images.app.image
        value: prod-registry/my-app
      - op: add
        path: deployments.app.helm.values.replicas
        value: 3
    
  - name: staging
    patches:
      - op: replace
        path: images.app.image
        value: staging-registry/my-app

# 使用: devspace dev -p production
```

---

## 开发工作流

### 启动开发模式

```bash
# 启动开发环境
devspace dev

# 使用特定 namespace
devspace dev -n my-namespace

# 使用特定 profile
devspace dev -p staging

# 只构建不部署
devspace build

# 只部署
devspace deploy
```

### 文件同步配置

```yaml
dev:
  app:
    sync:
      # 基本同步
      - path: ./:/app
      
      # 排除文件
      - path: ./src:/app/src
        excludePaths:
          - "**/.git"
          - "**/node_modules"
          - "**/*.log"
        
      # 上传后操作
      - path: ./src:/app/src
        onUpload:
          exec:
            - command: npm
              args: ["run", "compile"]
          restartContainer: false
          
      # 下载同步（从容器到本地）
      - path: ./generated:/app/generated
        disableUpload: true
```

### 热重载配置

```yaml
dev:
  app:
    # Node.js 示例
    command: ["npm", "run", "dev"]
    
    sync:
      - path: ./src:/app/src
        onUpload:
          restartContainer: false  # 由 nodemon 处理重载
          
    # Go 示例 (使用 Air)
    # command: ["air"]
    
    # Python 示例
    # command: ["python", "-m", "flask", "run", "--reload"]
```

---

## 端口转发和调试

### 端口转发配置

```yaml
dev:
  app:
    ports:
      - port: "3000"        # 应用端口
      - port: "9229"        # Node.js 调试端口
      - port: "5432:5432"   # 数据库端口映射
        
    # 反向端口转发 (本地服务暴露到集群)
    reversePorts:
      - port: "5000"        # 本地 mock 服务
```

### VS Code 调试配置

```json
// .vscode/launch.json
{
  "version": "0.2.0",
  "configurations": [
    {
      "type": "node",
      "request": "attach",
      "name": "DevSpace Debug",
      "port": 9229,
      "remoteRoot": "/app",
      "localRoot": "${workspaceFolder}"
    }
  ]
}
```

---

## 依赖管理

### 服务依赖配置

```yaml
dependencies:
  # Helm Chart 依赖
  redis:
    helm:
      chart:
        name: redis
        repo: https://charts.bitnami.com/bitnami
      values:
        auth:
          enabled: false
          
  # Git 仓库依赖
  shared-lib:
    git:
      url: https://github.com/company/shared-lib
      branch: main
      subPath: chart
      
  # 本地依赖
  common:
    path: ../common-service
    
# 依赖顺序控制
dependencyOrder:
  - redis
  - common
  - shared-lib
```

---

## Pipeline 配置

### 自定义 Pipeline

```yaml
pipelines:
  dev:
    flags:
      - name: skip-build
        short: s
        type: bool
    run: |
      if ! is_flag skip-build; then
        run_pipelines build
      fi
      run_pipelines deploy
      start_dev app

  build:
    run: |
      build_images app
      
  deploy:
    run: |
      create_deployments app
      
  test:
    run: |
      exec_container --image-selector ${images.app} -- npm test
```

### 钩子配置

```yaml
hooks:
  - name: pre-deploy
    events: ["before:deploy"]
    command: |
      echo "Running pre-deploy hook"
      ./scripts/prepare.sh
      
  - name: post-deploy
    events: ["after:deploy"]
    command: |
      echo "Deployment completed"
      ./scripts/verify.sh
      
  - name: on-error
    events: ["error"]
    command: |
      echo "Error occurred, running cleanup"
      ./scripts/cleanup.sh
```

---

## 常用命令

```bash
# 开发环境
devspace dev                    # 启动开发模式
devspace dev -p production      # 使用 production profile
devspace dev --skip-build       # 跳过构建

# 构建和部署
devspace build                  # 只构建镜像
devspace deploy                 # 部署应用
devspace purge                  # 删除部署

# 容器操作
devspace enter                  # 进入容器
devspace logs                   # 查看日志
devspace logs -f                # 实时日志

# 同步和端口
devspace sync                   # 启动文件同步
devspace attach                 # 附加到容器

# 工具
devspace list namespaces        # 列出命名空间
devspace use namespace dev      # 切换命名空间
devspace ui                     # 启动 Web UI
```

---

## 最佳实践

1. **镜像策略**: 使用 `rebuildStrategy` 减少不必要的构建
2. **同步排除**: 排除 node_modules、.git 等大目录
3. **Profile 管理**: 为不同环境创建独立 profile
4. **依赖顺序**: 明确定义服务启动顺序
5. **资源限制**: 在集群中为开发 Pod 设置资源限制
6. **清理策略**: 定期使用 `devspace purge` 清理旧资源

---

## 参考资源

- [官方文档](https://devspace.sh/docs)
- [GitHub Repo](https://github.com/devspace-sh/devspace)
- [配置参考](https://devspace.sh/docs/configuration/reference)
- [示例项目](https://github.com/devspace-sh/devspace/tree/main/examples)
- [Loft DevSpace](https://loft.sh)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[domain-17-system-foundation/topic-cheat-sheet/docker.md|docker]]
- [[entities/cncf-runtime|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
