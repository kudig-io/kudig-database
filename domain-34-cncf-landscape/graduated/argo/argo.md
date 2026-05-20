---
title: Argo
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- helm
- argocd
- redis
- rbac
- webhook
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Argo 是什么
- 如何 Argo
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Argo
- cncf
- landscape
cross_refs:
- type: fta
  path: ../topic-fta/list/gitops-argocd-fta.md
  label: '故障树: gitops-argocd'
---

# Argo

> **成熟度**: Graduated | **加入时间**: 2020-04 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://argoproj.github.io |
| **GitHub** | https://github.com/argoproj |
| **文档** | https://argo-cd.readthedocs.io |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | App Definition & Development |

---

## 项目概述

### 简介
Argo 是一组 Kubernetes 原生工具，用于运行工作流、管理集群和实现 GitOps 持续交付。

### 核心定位
Argo 项目提供了完整的云原生工作流和 GitOps 解决方案，包括 Argo CD（GitOps 持续交付）、Argo Workflows（工作流引擎）、Argo Events（事件驱动）和 Argo Rollouts（渐进式发布）。

### 发展历程
- **2017**: Applatix 创建 Argo Workflows
- **2018**: Intuit 创建 Argo CD
- **2020-04**: 加入 CNCF 作为孵化项目
- **2022-12**: 成为 CNCF 毕业项目
- **2024**: Argo 项目持续演进

---

## 核心功能

### 主要特性
- **Argo CD**: GitOps 持续交付，声明式应用部署
- **Argo Workflows**: Kubernetes 原生工作流引擎
- **Argo Events**: 事件驱动自动化
- **Argo Rollouts**: 渐进式发布策略（金丝雀、蓝绿）

### 功能架构
```
┌─────────────────────────────────────────────────────────────┐
│                      Argo Projects                          │
│  ┌─────────────────┐ ┌─────────────────┐                   │
│  │    Argo CD      │ │ Argo Workflows  │                   │
│  │   (GitOps CD)   │ │ (Workflow Eng.) │                   │
│  └─────────────────┘ └─────────────────┘                   │
│  ┌─────────────────┐ ┌─────────────────┐                   │
│  │  Argo Events    │ │ Argo Rollouts   │                   │
│  │ (Event Driven)  │ │ (Progressive)   │                   │
│  └─────────────────┘ └─────────────────┘                   │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   Kubernetes Cluster                        │
└─────────────────────────────────────────────────────────────┘
```

---

## 技术架构

### Argo CD 架构
| 组件 | 功能 | 说明 |
|:---|:---|:---|
| API Server | REST/gRPC 服务 | 提供 UI 和 CLI 交互 |
| Repo Server | Git 仓库管理 | 克隆和生成 manifests |
| Application Controller | 应用控制器 | 监控和同步应用状态 |
| Redis | 缓存 | 缓存 Git 仓库数据 |

### Argo Workflows 架构
| 组件 | 功能 | 说明 |
|:---|:---|:---|
| Workflow Controller | 工作流控制器 | 管理工作流执行 |
| Argo Server | API 服务器 | UI 和 API 端点 |
| Archive | 工作流存档 | 历史工作流存储 |

---

## 使用场景

### Argo CD 场景
- **GitOps 部署**: 从 Git 仓库自动同步应用到集群
- **多集群管理**: 统一管理多个 Kubernetes 集群
- **环境管理**: 开发、测试、生产环境的差异化配置

### Argo Workflows 场景
- **CI/CD 流水线**: 容器化的 CI/CD 工作流
- **数据处理**: ETL 和数据管道
- **机器学习**: ML 训练和推理工作流

### Argo Rollouts 场景
- **金丝雀发布**: 渐进式流量切换
- **蓝绿部署**: 零停机切换版本
- **实验和 A/B 测试**: 流量分割实验

---

## 快速开始

### Argo CD 安装
```bash
# 创建命名空间
kubectl create namespace argocd

# 安装 Argo CD
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/stable/manifests/install.yaml

# 获取初始密码
kubectl -n argocd get secret argocd-initial-admin-secret -o jsonpath="{.data.password}" | base64 -d

# 安装 CLI
brew install argocd
```

### 基础配置
```yaml
# Argo CD Application
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: my-app
  namespace: argocd
spec:
  project: default
  source:
    repoURL: https://github.com/myorg/myapp.git
    targetRevision: HEAD
    path: k8s
  destination:
    server: https://kubernetes.default.svc
    namespace: default
  syncPolicy:
    automated:
      prune: true
      selfHeal: true

# Argo Rollout
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: my-rollout
spec:
  replicas: 5
  strategy:
    canary:
      steps:
      - setWeight: 20
      - pause: {duration: 1h}
      - setWeight: 50
      - pause: {duration: 1h}
      - setWeight: 80
      - pause: {duration: 1h}
  selector:
    matchLabels:
      app: my-app
  template:
    metadata:
      labels:
        app: my-app
    spec:
      containers:
      - name: my-app
        image: my-app:v2
```

### 验证测试
```bash
# Argo CD 操作
argocd login localhost:8080
argocd app list
argocd app sync my-app

# 查看同步状态
argocd app get my-app
```

---

## 最佳实践

### 生产环境建议
- 使用 SSO 集成身份认证
- 配置 RBAC 细粒度权限
- 启用 Git webhook 触发同步
- 配置应用健康检查

### 性能优化
- 配置 repo server 缓存
- 合理设置同步间隔
- 使用 ApplicationSet 管理多应用
- 分片大型集群

### 安全加固
- 启用 HTTPS 和 TLS
- 配置 secret 管理（Vault、Sealed Secrets）
- 限制仓库访问权限
- 审计操作日志

---

## 生态集成

### 相关 CNCF 项目
- **Helm**: Chart 部署支持
- **Kustomize**: 配置管理
- **Prometheus**: 指标监控
- **Crossplane**: 基础设施管理

### 常见集成方案
- Argo CD + Helm/Kustomize
- Argo CD + Vault 密钥管理
- Argo Workflows + Argo Events
- Argo Rollouts + Prometheus 分析

---

## 社区与支持

### 社区资源
- Slack: https://argoproj.slack.com
- GitHub Discussions
- Twitter: @argoproj

### 贡献指南
访问 https://github.com/argoproj/argo-cd/blob/master/CONTRIBUTING.md 了解参与方式

---

## 参考资源

- [Argo CD 文档](https://argo-cd.readthedocs.io)
- [Argo Workflows 文档](https://argoproj.github.io/argo-workflows/)
- [GitHub Repo](https://github.com/argoproj)
- [CNCF 项目页面](https://www.cncf.io/projects/argo/)

---

**维护者**: Kudig Team | **许可证**: MIT
