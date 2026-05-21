---
title: Porter
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- helm
- postgresql
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Porter 是什么
- 如何 Porter
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Porter
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- iac-basics
---

title: Porter
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- postgresql
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Porter 是什么
- 如何 Porter
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Porter
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
# Porter

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://porter.sh/ |
| **GitHub** | https://github.com/getporter/porter |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Porter 是一个 CNAB (Cloud Native Application Bundle) 包管理器，用于将复杂的云原生应用及其所有依赖（Helm Charts、Terraform 模块、Kubernetes manifests、脚本等）打包为可分发、可安装的 Bundle。它解决了"我的应用需要先部署数据库，再配置网络，最后部署应用"这类多步骤安装流程的自动化问题，支持安装、升级、卸载的全生命周期管理。

### 核心特性

- **CNAB 规范**: 实现 CNAB 标准，将多工具安装流程打包为单一 Bundle
- **Mixin 系统**: 通过 Mixin 集成 Helm、Terraform、kubectl、Azure 等工具
- **参数与凭证**: 类型安全的参数传递和凭证注入
- **OCI 分发**: 将 Bundle 发布到 OCI 注册中心分发
- **声明式定义**: 使用 porter.yaml 声明式定义 Bundle 内容
- **状态管理**: 跟踪 Bundle 安装状态，支持升级和卸载

---

## 架构设计

```
┌─────────────────────────────────────────────┐
│              Porter Bundle                   │
│                                              │
│  porter.yaml (Bundle 定义)                  │
│  ┌──────────────────────────────────┐       │
│  │  Install / Upgrade / Uninstall   │       │
│  │                                   │       │
│  │  Step 1: Terraform (基础设施)    │       │
│  │  Step 2: Helm (数据库)           │       │
│  │  Step 3: kubectl (应用)          │       │
│  │  Step 4: exec (配置脚本)         │       │
│  └──────────────────────────────────┘       │
│                                              │
│  ┌──────────────────────────────────┐       │
│  │  Mixins (工具集成)                │       │
│  │  helm │ terraform │ kubectl      │       │
│  │  exec │ az │ aws │ gcloud       │       │
│  └──────────────────────────────────┘       │
│                                              │
│  ┌──────────────────────────────────┐       │
│  │  Parameters & Credentials        │       │
│  │  (类型安全的参数和凭证)           │       │
│  └──────────────────────────────────┘       │
└──────────┬──────────────────────────────────┘
           │ porter publish
    ┌──────▼──────┐
    │ OCI Registry│
    └─────────────┘
```

---

## 快速开始

### 安装

```bash
# macOS/Linux
curl -L https://cdn.porter.sh/latest/install-linux.sh | bash

# 或使用 Homebrew
brew install porter

# 验证安装
porter version
```

### 创建 Bundle

```yaml
# porter.yaml
schemaType: Bundle
schemaVersion: 1.0.1
name: my-app
version: 0.1.0
description: "Full-stack application deployment"
registry: ghcr.io/myorg

parameters:
  - name: environment
    type: string
    default: "staging"
    enum: ["staging", "production"]
  - name: replicas
    type: integer
    default: 2
  - name: db-name
    type: string
    default: "myapp"

credentials:
  - name: kubeconfig
    path: /home/nonroot/.kube/config
  - name: db-password
    env: DATABASE_PASSWORD

mixins:
  - helm3:
      repositories:
        bitnami: https://charts.bitnami.com/bitnami
  - kubernetes
  - exec

install:
  - helm3:
      description: "Deploy PostgreSQL"
      name: my-db
      chart: bitnami/postgresql
      version: "13.0.0"
      namespace: "{{ bundle.parameters.environment }}"
      set:
        auth.database: "{{ bundle.parameters.db-name }}"
        auth.password: "{{ bundle.credentials.db-password }}"

  - kubernetes:
      description: "Deploy application"
      manifests:
        - manifests/
      wait: true

  - exec:
      description: "Run database migration"
      command: ./scripts/migrate.sh
      arguments:
        - "{{ bundle.parameters.db-name }}"

upgrade:
  - helm3:
      description: "Upgrade PostgreSQL"
      name: my-db
      chart: bitnami/postgresql
      version: "13.0.0"
      namespace: "{{ bundle.parameters.environment }}"

  - kubernetes:
      description: "Update application"
      manifests:
        - manifests/
      wait: true

uninstall:
  - kubernetes:
      description: "Remove application"
      manifests:
        - manifests/
      wait: true

  - helm3:
      description: "Remove PostgreSQL"
      releases:
        - my-db
```

### 构建、发布和安装

```bash
# 构建 Bundle
porter build

# 发布到 OCI 注册中心
porter publish

# 创建凭证集
porter credentials generate my-creds

# 安装 Bundle
porter install my-app-staging \
  --reference ghcr.io/myorg/my-app:v0.1.0 \
  --credential-set my-creds \
  --param environment=staging \
  --param replicas=3

# 升级
porter upgrade my-app-staging \
  --reference ghcr.io/myorg/my-app:v0.2.0 \
  --param replicas=5

# 卸载
porter uninstall my-app-staging
```

---

## 与其他方案对比

| 特性 | Porter (CNAB) | Helm | Terraform | Ansible |
|:---|:---|:---|:---|:---|
| 打包范围 | 多工具组合 | K8s 资源 | 基础设施 | 通用自动化 |
| 分发方式 | OCI Registry | Helm Repo/OCI | Registry | Galaxy/Git |
| 声明式 | Bundle 声明 | Chart 模板 | HCL | Playbook |
| 状态管理 | Bundle 安装状态 | Release | State 文件 | 无 |
| 多工具集成 | Mixin 系统 | 不支持 | Provider | Module |
| 适用场景 | 复杂多步部署 | K8s 应用 | 基础设施 | 配置管理 |

---

## 最佳实践

1. **Mixin 选择**: 优先使用官方 Mixin，自定义逻辑用 exec Mixin
2. **凭证管理**: 使用 Porter 的凭证集管理敏感信息，不要硬编码在 Bundle 中
3. **参数校验**: 使用 enum 和类型约束确保参数值合法
4. **幂等性**: 确保 install 和 upgrade 步骤是幂等的
5. **版本化**: 为每个 Bundle 版本打标签，确保可复现的部署

---

## 参考资源

- [Porter 官方文档](https://porter.sh/docs/)
- [Porter GitHub](https://github.com/getporter/porter)
- [CNAB 规范](https://cnab.io/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[man/INSTALL.md|INSTALL]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/sql.md|sql]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
