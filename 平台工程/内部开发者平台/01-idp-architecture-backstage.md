---
title: Internal Developer Platform (IDP) Architecture
description: 内部开发者平台架构 — Backstage/Cortex 选型、Golden Path 设计、自助服务、平台工程团队模式
summary: 企业级内部开发者平台的架构设计与落地实践，涵盖服务目录、脚手架、自助运维
category: practice
tags:
- idp
- backstage
- platform-engineering
- golden-path
- developer-experience
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: platform
---
# 内部开发者平台 (IDP) 架构

> 构建以开发者体验为核心的内部平台，降低认知负载，加速交付。

## IDP 核心理念

| 原则 | 说明 |
|------|------|
| 自助服务 | 开发者无需运维介入即可完成 80% 操作 |
| Golden Path | 提供最佳实践模板，而非强制约束 |
| 抽象复杂性 | 隐藏基础设施细节，暴露业务语义 |
| 平台即产品 | 内部开发者是用户，平台团队是产品团队 |
| 渐进式采用 | 不强制迁移，通过价值吸引采用 |

## 平台架构分层

```
┌─────────────────────────────────────────────────┐
│           Developer Portal (Backstage)           │
│  ┌─────────┐ ┌─────────┐ ┌─────────────────┐   │
│  │ Service │ │Software │ │  Self-Service   │   │
│  │Catalog  │ │Templates│ │  Actions        │   │
│  └─────────┘ └─────────┘ └─────────────────┘   │
├─────────────────────────────────────────────────┤
│           Platform Orchestration Layer           │
│  ┌─────────┐ ┌─────────┐ ┌─────────────────┐   │
│  │ CI/CD   │ │ Infra   │ │  Policy         │   │
│  │Pipelines│ │ as Code │ │  Engine         │   │
│  └─────────┘ └─────────┘ └─────────────────┘   │
├─────────────────────────────────────────────────┤
│           Infrastructure Layer                   │
│  ┌─────────┐ ┌─────────┐ ┌─────────────────┐   │
│  │Kubernetes│ │ Cloud   │ │  Observability  │   │
│  │Clusters  │ │Services │ │  Stack          │   │
│  └─────────┘ └─────────┘ └─────────────────┘   │
└─────────────────────────────────────────────────┘
```

## Backstage 部署与配置

### Helm 部署

```yaml
# backstage-values.yaml
global:
  dynamic:
    includes:
      - dynamic-plugins.default.yaml

upstream:
  backstage:
    appConfig:
      app:
        baseUrl: https://developer.example.com
      backend:
        baseUrl: https://developer.example.com
        database:
          client: pg
          connection:
            host: postgresql
            port: 5432
            user: backstage
            password: ${POSTGRES_PASSWORD}
      catalog:
        locations:
          - type: url
            target: https://github.com/org/platform/blob/main/catalog-info.yaml
          - type: url
            target: https://github.com/org/templates/blob/main/all-templates.yaml
      integrations:
        github:
          - host: github.com
            token: ${GITHUB_TOKEN}
      scaffolder:
        defaultAuthor:
          name: Platform Team
          email: platform@example.com

  postgresql:
    enabled: true
    auth:
      password: backstage-pass
```

### 服务目录 (Software Catalog)

```yaml
# catalog-info.yaml — 服务注册
apiVersion: backstage.io/v1alpha1
kind: Component
metadata:
  name: order-service
  description: 订单处理微服务
  annotations:
    github.com/project-slug: org/order-service
    backstage.io/techdocs-ref: dir:.
    argocd/app-name: order-service
    grafana/dashboard-selector: "tags @> 'order'"
spec:
  type: service
  lifecycle: production
  owner: team-commerce
  system: e-commerce
  providesApis:
    - order-api
  consumesApis:
    - payment-api
    - inventory-api
  dependsOn:
    - resource:postgresql/orders-db
    - resource:kafka/order-events
---
apiVersion: backstage.io/v1alpha1
kind: API
metadata:
  name: order-api
  description: 订单服务 REST API
spec:
  type: openapi
  lifecycle: production
  owner: team-commerce
  definition: |
    openapi: "3.0.0"
    info:
      title: Order API
      version: 1.0.0
    paths:
      /orders:
        post:
          summary: Create order
```

### 软件模板 (Scaffolder)

```yaml
# template.yaml — 新服务脚手架
apiVersion: scaffolder.backstage.io/v1beta3
kind: Template
metadata:
  name: create-nodejs-service
  title: 创建 Node.js 微服务
  description: 基于 Golden Path 创建生产就绪的 Node.js 微服务
spec:
  owner: platform-team
  type: service
  parameters:
    - title: 服务信息
      required:
        - serviceName
        - owner
      properties:
        serviceName:
          title: 服务名称
          type: string
          pattern: '^[a-z0-9-]+$'
        description:
          title: 描述
          type: string
        owner:
          title: 负责团队
          type: string
          ui:field: OwnerPicker
          ui:options:
            catalogFilter:
              kind: Group
    - title: 基础设施选择
      properties:
        database:
          title: 数据库
          type: string
          enum: [none, postgresql, mongodb, redis]
        messaging:
          title: 消息队列
          type: string
          enum: [none, kafka, rabbitmq]
        visibility:
          title: 可见性
          type: string
          enum: [public, internal]
          default: internal

  steps:
    - id: fetch-template
      action: fetch:template
      input:
        url: ./skeleton
        values:
          serviceName: ${{ parameters.serviceName }}
          owner: ${{ parameters.owner }}
          database: ${{ parameters.database }}

    - id: publish-github
      action: publish:github
      input:
        allowedHosts: ['github.com']
        repoUrl: github.com?owner=org&repo=${{ parameters.serviceName }}
        description: ${{ parameters.description }}

    - id: register-catalog
      action: catalog:register
      input:
        repoContentsUrl: ${{ steps['publish-github'].output.repoContentsUrl }}
        catalogInfoPath: /catalog-info.yaml

    - id: create-argocd-app
      action: argocd:create-app
      input:
        appName: ${{ parameters.serviceName }}
        repoUrl: https://github.com/org/${{ parameters.serviceName }}
        path: deploy/

  output:
    links:
      - title: 仓库地址
        url: ${{ steps['publish-github'].output.remoteUrl }}
      - title: 在目录中查看
        icon: catalog
        entityRef: ${{ steps['register-catalog'].output.entityRef }}
```

## Golden Path 设计

### 服务创建 Golden Path

```
开发者发起 → 选择模板 → 填写参数 → 自动创建：
├── GitHub 仓库（含 CI/CD 配置）
├── catalog-info.yaml（服务注册）
├── Dockerfile（多阶段构建）
├── Helm Chart / Kustomize（部署配置）
├── ArgoCD Application（GitOps 部署）
├── Grafana Dashboard（监控模板）
├── Alert Rules（告警规则）
└── TechDocs（文档骨架）
```

### 数据库申请 Golden Path

```yaml
# 自助数据库申请
apiVersion: scaffolder.backstage.io/v1beta3
kind: Template
metadata:
  name: request-database
  title: 申请数据库实例
spec:
  type: database
  parameters:
    - title: 数据库配置
      properties:
        engine:
          type: string
          enum: [postgresql, mysql, mongodb]
        size:
          type: string
          enum: [small, medium, large]
          description: "small=2C4G, medium=4C8G, large=8C16G"
        environment:
          type: string
          enum: [dev, staging, production]
  steps:
    - id: create-namespace
      action: kubernetes:create-namespace
    - id: deploy-database
      action: kubernetes:apply
      input:
        manifest: |
          apiVersion: postgresql.cnpg.io/v1
          kind: Cluster
          metadata:
            name: ${{ parameters.serviceName }}-db
          spec:
            instances: ${{ parameters.environment == 'production' && 3 || 1 }}
            storage:
              size: ${{ parameters.size == 'large' && '100Gi' || parameters.size == 'medium' && '50Gi' || '20Gi' }}
```

## 平台工程团队模式

### 团队拓扑

| 角色 | 职责 | 比例 |
|------|------|------|
| 平台产品经理 | 需求收集、优先级、路线图 | 1:50 开发者 |
| 平台工程师 | 构建/维护平台组件 | 1:10 开发者 |
| DX 工程师 | 开发者体验、文档、培训 | 1:30 开发者 |
| SRE（嵌入） | 可靠性标准、On-Call 支持 | 1:20 开发者 |

### 成熟度模型

| 级别 | 特征 | 开发者体验 |
|------|------|-----------|
| L0 临时 | 无平台，各自为战 | 差（高认知负载） |
| L1 基础 | CI/CD + 共享集群 | 一般 |
| L2 标准化 | 模板 + 自助服务 + 目录 | 良好 |
| L3 优化 | Golden Path + 自动化 + 度量 | 优秀 |
| L4 智能 | AI 辅助 + 预测 + 自愈 | 卓越 |

## 平台度量（DORA + DX）

```yaml
# 平台效能指标
platform_metrics:
  developer_experience:
    - time_to_first_deploy: "< 1 day"  # 新服务首次部署时间
    - pr_cycle_time: "< 4 hours"       # PR 周期时间
    - change_failure_rate: "< 5%"      # 变更失败率
    - mttr: "< 1 hour"                 # 平均恢复时间
  platform_adoption:
    - catalog_coverage: "> 90%"        # 服务目录覆盖率
    - template_usage: "> 80%"          # 模板使用率
    - self_service_rate: "> 70%"       # 自助服务比例
  reliability:
    - platform_availability: "> 99.9%" # 平台可用性
    - deployment_frequency: "daily"    # 部署频率
```

## 最佳实践

1. **从痛点开始**：先解决开发者最大的摩擦点
2. **内部营销**：让开发者知道平台的存在和价值
3. **反馈循环**：定期收集 NPS 和改进建议
4. **避免强制**：Golden Path 是推荐，不是监狱
5. **文档即产品**：TechDocs 与代码同步更新
6. **渐进式抽象**：先标准化，再自动化，最后智能化

## Related

- [[平台工程/index.md|平台工程总索引]]
- [[清单模式/index.md|清单模式]]
- [[发布变更/index.md|发布变更]]
