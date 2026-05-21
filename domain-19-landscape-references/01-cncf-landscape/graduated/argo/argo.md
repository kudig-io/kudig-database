---
title: Argo
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- prometheus
- helm
- argocd
- redis
- rbac
- webhook
- operator
- ebpf
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Argo 是什么
- 如何 Argo
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Argo
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
- gitops-basics
- ebpf-basics
- redis-basics
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
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/gitops-argocd-fta.md
  label: '故障树: gitops-argocd'
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

## Related

- [[domain-19-landscape-references/04-cncf-fta-index.md|04-cncf-fta-index]]
- [[CONTRIBUTING.md|CONTRIBUTING]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[journal/digest-2026-05-21-full|Wiki 全量知识库摘要 — 2026-05-21]] — Cross-reference
- [[references/kudig-ecosystem-guide|KUDIG 开源生态指南与深度研究指南]] — Cross-reference
- [[references/k8s-advanced-ecosystem|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[references/storage-terms|K8s 存储术语参考]] — Cross-reference
- [[references/KUDIG Tag Dictionary|KUDIG Tag Dictionary]] — Cross-reference
- [[references/k8s-deployment-create|Kubernetes Deployment 创建操作指南]] — Cross-reference
- [[references/release-notes-cicd-gitops|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[references/k8s-production-operations|生产运维：GitOps、FinOps、灾备恢复与变更管理]] — Cross-reference
- [[references/k8s-ai-infra-domain-guide|AI Infrastructure on Kubernetes Domain Guide]] — Cross-reference
- [[references/k8s-platform-extensions|平台运维与扩展生态：Helm、CI/CD、Operator 开发与服务网格]] — Cross-reference
- [[references/platform-engineering-terms|K8s 平台工程术语参考]] — Cross-reference
- [[references/version-upgrade-guide|版本升级指南]] — Cross-reference
- [[concepts/gitops-tool-evolution|GitOps 工具演进]] — Cross-reference
- [[skills/ts-ai-ml-workloads|AI/ML 工作负载排查]] — Cross-reference
- [[skills/deployment-canary-and-bluegreen|金丝雀与蓝绿发布]] — Cross-reference
- [[entities/cncf-cicd|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[entities/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/observability-index|Observability 可观测性知识图谱索引]]
- [[domain-19-landscape-references/topic-index/openkruise-index|OpenKruise 全局索引]]
- [[domain-19-landscape-references/topic-index/helm-index|Helm 全局索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-0.12|argo-cd v0.12 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-0.8|argo-cd v0.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.12|argo-cd v2.12 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.13|argo-cd v2.13 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-1.8|argo-cd v1.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-0.9|argo-cd v0.9 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.4|argo-cd v2.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-1.3|argo-cd v1.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-0.2|argo-cd v0.2 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-1.7|argo-cd v1.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.0|argo-cd v2.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-3.1|argo-cd v3.1 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-0.6|argo-cd v0.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-1.6|argo-cd v1.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-3.0|argo-cd v3.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-0.7|argo-cd v0.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.5|argo-cd v2.5 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-1.2|argo-cd v1.2 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-0.3|argo-cd v0.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-1.5|argo-cd v1.5 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.2|argo-cd v2.2 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-3.3|argo-cd v3.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-0.4|argo-cd v0.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.6|argo-cd v2.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-1.1|argo-cd v1.1 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.7|argo-cd v2.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-1.0|argo-cd v1.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-1.4|argo-cd v1.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.3|argo-cd v2.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-3.2|argo-cd v3.2 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-0.5|argo-cd v0.5 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.8|argo-cd v2.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.10|argo-cd v2.10 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-0.10|argo-cd v0.10 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.14|argo-cd v2.14 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-0.11|argo-cd v0.11 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.9|argo-cd v2.9 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.11|argo-cd v2.11 Release Notes]]
