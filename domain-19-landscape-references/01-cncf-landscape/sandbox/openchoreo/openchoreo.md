---
title: OpenChoreo
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- grafana
- helm
- flux
- elasticsearch
- pdb
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- OpenChoreo 是什么
- 如何 OpenChoreo
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- OpenChoreo
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- monitoring-basics
---

title: OpenChoreo
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- grafana
- helm
- flux
- elasticsearch
- pdb
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
- OpenChoreo 是什么
- 如何 OpenChoreo
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- OpenChoreo
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
# OpenChoreo

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://openchoreo.dev/ |
| **GitHub** | https://github.com/openchoreo/openchoreo |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

OpenChoreo 是一个云原生的内部开发者平台 (IDP) 框架，提供开箱即用的开发者自助服务门户。它基于 Kubernetes 构建，为开发团队提供应用创建、部署、监控的统一界面，同时让平台团队可以通过声明式配置定义黄金路径 (Golden Path) 和治理策略。OpenChoreo 旨在简化 Platform Engineering 的实施复杂度。

### 核心特性

- **开发者门户**: 自助式 Web UI，开发者可独立创建、部署和管理应用
- **应用模板**: 预置的应用蓝图（Scaffold），快速创建符合规范的新项目
- **黄金路径**: 平台团队定义标准化的开发和部署流程
- **多环境管理**: 统一管理开发、测试、生产等多环境
- **可观测性集成**: 内置日志、指标、追踪的统一视图
- **GitOps 原生**: 与 Argo CD / Flux 等 GitOps 工具深度集成

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                 OpenChoreo Platform                    │
│                                                       │
│  ┌──────────────────────────────────────────────┐    │
│  │          Developer Portal (Web UI)            │    │
│  │  ┌───────────┐ ┌───────────┐ ┌────────────┐ │    │
│  │  │ App       │ │ Env       │ │ Observability│ │    │
│  │  │ Catalog   │ │ Manager   │ │ Dashboard  │ │    │
│  │  └───────────┘ └───────────┘ └────────────┘ │    │
│  └─────────────────────┬────────────────────────┘    │
│                        │ API                          │
│  ┌─────────────────────▼────────────────────────┐    │
│  │          OpenChoreo Control Plane             │    │
│  │  ┌──────────────┐ ┌────────────────────────┐ │    │
│  │  │ Application  │ │ Environment Controller │ │    │
│  │  │ Controller   │ │                        │ │    │
│  │  └──────┬───────┘ └───────────┬────────────┘ │    │
│  │  ┌──────▼───────┐ ┌───────────▼────────────┐ │    │
│  │  │ Template     │ │ Policy Engine          │ │    │
│  │  │ Engine       │ │ (Golden Path Rules)    │ │    │
│  │  └──────────────┘ └────────────────────────┘ │    │
│  └─────────────────────┬────────────────────────┘    │
└────────────────────────┼─────────────────────────────┘
                         │
        ┌────────────────┼────────────────┐
        │                │                │
  ┌─────▼────┐    ┌──────▼─────┐   ┌─────▼──────┐
  │ Git Repo │    │ GitOps     │   │ Kubernetes │
  │ (Source) │    │ (Argo/Flux)│   │ Clusters   │
  └──────────┘    └────────────┘   └────────────┘
```

---

## 快速开始

### 安装 OpenChoreo

```bash
# 使用 Helm 安装
helm repo add openchoreo https://openchoreo.dev/charts
helm install openchoreo openchoreo/openchoreo \
  --namespace openchoreo-system \
  --create-namespace

# 访问 Portal
kubectl port-forward -n openchoreo-system svc/openchoreo-portal 8080:80
# 打开 http://localhost:8080
```

### 定义应用模板

```yaml
# app-template.yaml
apiVersion: openchoreo.dev/v1alpha1
kind: ApplicationTemplate
metadata:
  name: microservice-go
  namespace: openchoreo-system
spec:
  displayName: "Go Microservice"
  description: "Standard Go microservice template with health checks and metrics"
  
  # Git 模板仓库
  scaffold:
    repo: https://github.com/openchoreo/templates
    path: microservice-go
    branch: main
  
  # 默认资源配置
  defaults:
    resources:
      cpu: "500m"
      memory: "512Mi"
    replicas: 2
  
  # 必须包含的组件
  requiredComponents:
    - healthCheck
    - metrics
  
  # 参数化配置
  parameters:
    - name: serviceName
      description: "Service name"
      required: true
    - name: port
      description: "Service port"
      default: "8080"
```

### 创建应用

```yaml
# my-app.yaml
apiVersion: openchoreo.dev/v1alpha1
kind: Application
metadata:
  name: user-service
  namespace: team-alpha
spec:
  template: microservice-go
  
  parameters:
    serviceName: user-service
    port: "8080"
  
  source:
    git:
      repo: https://github.com/team-alpha/user-service
      branch: main
  
  environments:
    - name: dev
      cluster: dev-cluster
      namespace: user-service-dev
    - name: prod
      cluster: prod-cluster
      namespace: user-service-prod
```

```bash
kubectl apply -f my-app.yaml
```

---

## 高级功能

### 黄金路径规则

```yaml
# golden-path.yaml
apiVersion: openchoreo.dev/v1alpha1
kind: GoldenPathPolicy
metadata:
  name: production-standards
spec:
  # 应用到所有生产环境部署
  selector:
    environmentType: production
  
  rules:
    # 必须有资源限制
    - name: resource-limits-required
      check: |
        spec.resources.limits.cpu != null &&
        spec.resources.limits.memory != null
      message: "Production apps must have resource limits"
    
    # 最小副本数
    - name: min-replicas
      check: "spec.replicas >= 3"
      message: "Production requires at least 3 replicas"
    
    # 必须有 PDB
    - name: pdb-required
      check: "spec.podDisruptionBudget != null"
      message: "Production requires PodDisruptionBudget"
    
    # 必须启用监控
    - name: monitoring-enabled
      check: "spec.monitoring.enabled == true"
      message: "Monitoring must be enabled in production"
```

### 环境晋升 (Promotion)

```yaml
# promotion-pipeline.yaml
apiVersion: openchoreo.dev/v1alpha1
kind: PromotionPipeline
metadata:
  name: standard-promotion
spec:
  stages:
    - name: dev
      autoPromote: false
      tests:
        - unit-tests
        - integration-tests
    
    - name: staging
      autoPromote: true
      waitForApproval: false
      tests:
        - smoke-tests
        - performance-tests
      duration: 24h  # 观察期
    
    - name: production
      autoPromote: false
      waitForApproval: true
      approvers:
        - role: tech-lead
        - role: sre
      canary:
        enabled: true
        steps:
          - weight: 10
            duration: 1h
          - weight: 50
            duration: 2h
          - weight: 100
```

### 开发者门户自定义

```yaml
# portal-config.yaml
apiVersion: openchoreo.dev/v1alpha1
kind: PortalConfiguration
metadata:
  name: default
spec:
  branding:
    title: "ACME Developer Platform"
    logo: "/assets/logo.png"
  
  features:
    appCatalog: true
    environmentView: true
    cicdPipelines: true
    observability: true
    costAnalysis: true
  
  integrations:
    git:
      - provider: github
        org: acme-corp
    monitoring:
      grafana:
        url: https://grafana.acme.com
    logging:
      elasticsearch:
        url: https://es.acme.com
```

---

## 与其他方案对比

| 特性 | OpenChoreo | Backstage | Port | Humanitec |
|:---|:---|:---|:---|:---|
| 定位 | 完整 IDP | 开发者门户 | 开发者门户 | 平台编排 |
| 应用模板 | 内置 | 插件 | 内置 | 内置 |
| GitOps | 深度集成 | 插件 | 集成 | 内置 |
| 黄金路径 | 策略驱动 | TechDocs | Scorecards | 内置 |
| K8s 原生 | CRD 驱动 | 独立服务 | SaaS | SaaS |
| 开源 | Apache-2.0 | Apache-2.0 | 商业 | 商业 |

---

## 最佳实践

1. **模板标准化**: 为不同技术栈创建标准化的应用模板
2. **渐进式策略**: 从宽松的黄金路径规则开始，逐步收紧
3. **自助为主**: 尽量让开发者通过 Portal 完成所有操作，减少工单
4. **可观测性**: 确保每个应用都有统一的监控和日志入口
5. **版本控制**: 所有平台配置都纳入 Git 版本控制

---

## 参考资源

- [OpenChoreo 官方文档](https://openchoreo.dev/docs/)
- [OpenChoreo GitHub](https://github.com/openchoreo/openchoreo)
- [Platform Engineering 指南](https://platformengineering.org/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/gitops.md|gitops]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/cncf-cicd|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
