# Backstage

> **成熟度**: Incubating | **加入时间**: 2022-03 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://backstage.io |
| **GitHub** | https://github.com/backstage/backstage |
| **文档** | https://backstage.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | TypeScript |
| **CNCF 分类** | App Definition & Development |

---

## 项目概述

### 简介
Backstage 是 Spotify 开源的开发者门户平台，用于构建内部开发者平台(IDP)。它提供统一的软件目录、项目模板、技术文档聚合和可扩展的插件系统，旨在提升开发者体验和工程效率。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2016 | Spotify 内部开发 |
| 2020-03 | 开源发布 |
| 2022-03 | 加入 CNCF Incubating |
| 至今 | 100+ 企业采用 |

### 核心定位
Backstage 是构建内部开发者平台的开源框架，被 Spotify、Netflix、American Airlines 等企业采用，是提升开发者体验的标准解决方案。

---

## 架构设计

### 核心功能

```
┌─────────────────────────────────────────────────────────────────┐
│                    Backstage 核心功能                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                  Software Catalog                           ││
│  │                    (软件目录)                                ││
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐          ││
│  │  │Component│ │  API    │ │ Resource│ │ System  │          ││
│  │  │ 服务    │ │ 接口    │ │ 资源    │ │ 系统    │          ││
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘          ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │               Software Templates                            ││
│  │                 (软件模板)                                   ││
│  │  ┌─────────────────────────────────────────────────────┐   ││
│  │  │ 标准化项目创建 → Scaffolder → Git Repo + CI/CD      │   ││
│  │  └─────────────────────────────────────────────────────┘   ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                   TechDocs                                  ││
│  │                (技术文档聚合)                                ││
│  │  ┌─────────────────────────────────────────────────────┐   ││
│  │  │ Markdown → MkDocs → Backstage 统一展示              │   ││
│  │  └─────────────────────────────────────────────────────┘   ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                Plugin Marketplace                           ││
│  │                  (插件市场)                                  ││
│  │  Kubernetes │ GitHub │ ArgoCD │ PagerDuty │ Datadog │ ... ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 实体模型

```yaml
# catalog-info.yaml
apiVersion: backstage.io/v1alpha1
kind: Component
metadata:
  name: payment-service
  description: Payment processing service
  annotations:
    github.com/project-slug: myorg/payment-service
    backstage.io/techdocs-ref: dir:.
  tags:
    - java
    - spring-boot
spec:
  type: service
  lifecycle: production
  owner: team-payments
  system: payment-platform
  dependsOn:
    - component:user-service
  providesApis:
    - payment-api
```

---

## 快速开始

### 创建 Backstage 应用

```bash
# 创建新应用
npx @backstage/create-app@latest

# 启动开发服务器
cd my-backstage-app
yarn dev
```

### 软件模板示例

```yaml
# template.yaml
apiVersion: scaffolder.backstage.io/v1beta3
kind: Template
metadata:
  name: springboot-service
  title: Spring Boot Service
  description: Create a new Spring Boot microservice
spec:
  owner: platform-team
  type: service
  
  parameters:
    - title: Service Information
      required:
        - name
        - owner
      properties:
        name:
          title: Name
          type: string
        owner:
          title: Owner
          type: string
          ui:field: OwnerPicker
    
    - title: Repository Location
      properties:
        repoUrl:
          title: Repository URL
          type: string
          ui:field: RepoUrlPicker
  
  steps:
    - id: fetch
      name: Fetch Template
      action: fetch:template
      input:
        url: ./skeleton
        values:
          name: ${{ parameters.name }}
    
    - id: publish
      name: Publish to GitHub
      action: publish:github
      input:
        repoUrl: ${{ parameters.repoUrl }}
    
    - id: register
      name: Register in Catalog
      action: catalog:register
      input:
        repoContentsUrl: ${{ steps.publish.output.repoContentsUrl }}
```

---

## 插件生态

| 类别 | 插件 |
|:---|:---|
| **源码管理** | GitHub, GitLab, Bitbucket |
| **CI/CD** | Jenkins, GitHub Actions, ArgoCD, Tekton |
| **监控** | Datadog, Grafana, PagerDuty |
| **Kubernetes** | Kubernetes, Rancher |
| **云服务** | AWS, GCP, Azure |
| **安全** | Snyk, SonarQube |

---

## 参考资源

- [官方文档](https://backstage.io/docs)
- [GitHub Repo](https://github.com/backstage/backstage)
- [CNCF 项目页面](https://www.cncf.io/projects/backstage/)
- [插件市场](https://backstage.io/plugins)

---

**维护者**: Kudig Team | **许可证**: MIT
