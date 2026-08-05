---
title: Backstage 内部开发者平台 (IDP) 构建指南
description: '# Backstage 内部开发者平台 (IDP) 构建指南'
summary: 'pagerduty.com/integration-key: "<INTEGRATION_KEY>"'
category: platform-engineering
tags:
- k8s
- platform-engineering
- developer-experience
- idp
- prometheus
- grafana
- helm
- docker
- opa
- redis
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- SRE
- 架构师
estimated_read_time: 5min
intent_queries:
- Backstage 内部开发者平台 (IDP) 构建指南 是什么
- 如何 Backstage 内部开发者平台 (IDP) 构建指南
- Kubernetes 36 platform engineering 最佳实践
trigger_keywords:
- Backstage
- 内部开发者平台
- IDP
- 构建指南
- platform
- engineering
prerequisites:
- kubectl-basics
- platform-engineering-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- redis-basics
- mysql-basics
- policy-basics
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




# [[backstage|Backstage]] 内部开发者平台 (IDP) 构建指南

> **适用版本**: Backstage v1.36.0  
> **最后更新**: 2026-04-24  
> **难度**: 中级

---

<!-- chunk: 📋 目录 -->## 📋 目录

- [一、Backstage 核心架构](#一backstage-核心架构)
- [二、快速启动](#二快速启动)
- [三、Software Catalog 服务目录](#三software-catalog-服务目录)
- [四、Software Templates 自助服务](#四software-templates-自助服务)
- [五、TechDocs 文档即代码](#五techdocs-文档即代码)
- [六、插件生态集成](#六插件生态集成)
- [七、认证与多租户](#七认证与多租户)
- [八、生产部署](#八生产部署)

---

<!-- chunk: 一、Backstage 核心架构 -->## 一、Backstage 核心架构

```
Backstage 架构
├── 前端 (React + TypeScript)
│   ├── 插件系统 (Plugin Framework)
│   ├── 主题与品牌定制
│   └── 统一导航与搜索
├── 后端 (Node.js)
│   ├── Plugin Backend APIs
│   ├── 数据库连接 (PostgreSQL/SQLite)
│   └── 缓存与任务调度
└── 核心系统
    ├── Software Catalog (实体关系图)
    ├── Software Templates (Scaffolder)
    ├── TechDocs (MkDocs 渲染)
    ├── Search (多源聚合搜索)
    └── Permission Framework (权限框架)
```

---

<!-- chunk: 二、快速启动 -->## 二、快速启动

```bash
# 使用 npx 创建应用
npx @backstage/create-app@latest
# 输入应用名称: my-idp
# 选择数据库: PostgreSQL (生产) / SQLite (开发)

cd my-idp
yarn dev
# 访问 http://localhost:3000
```

## 生产级 app-config.yaml

```yaml
app:
  title: My Company IDP
  baseUrl: https://idp.example.com

backend:
  baseUrl: https://idp.example.com
  listen:
    port: 7007
  database:
    client: better-sqlite3
    connection: ':memory:'
    # 生产环境使用 PostgreSQL:
    # client: pg
    # connection:
    #   host: ${POSTGRES_HOST}
    #   port: ${POSTGRES_PORT}
    #   user: ${POSTGRES_USER}
    #   password: ${POSTGRES_PASSWORD}
  # 使用 Redis 缓存
  cache:
    store: redis
    connection: redis://redis:6379

auth:
  environment: production
  providers:
    github:
      production:
        clientId: ${GITHUB_CLIENT_ID}
        clientSecret: ${GITHUB_CLIENT_SECRET}
    google:
      production:
        clientId: ${GOOGLE_CLIENT_ID}
        clientSecret: ${GOOGLE_CLIENT_SECRET}
        signIn:
          resolvers:
            - resolver: emailMatchingUserEntityProfileEmail

catalog:
  rules:
    - allow: [Component, System, API, Resource, Location]
  locations:
    # 自动发现 GitHub 组织仓库
    - type: url
      target: https://github.com/my-org/backstage-catalog/blob/main/catalog-info.yaml
      rules:
        - allow: [Component, System, API, Resource]
    - type: url
      target: https://github.com/my-org/backstage-templates/blob/main/template.yaml
      rules:
        - allow: [Template]

integrations:
  github:
    - host: github.com
      token: ${GITHUB_TOKEN}
      apps:
        - appId: ${GITHUB_APP_ID}
          clientId: ${GITHUB_APP_CLIENT_ID}
          clientSecret: ${GITHUB_APP_CLIENT_SECRET}
          webhookSecret: ${GITHUB_APP_WEBHOOK_SECRET}
          privateKey: |
            ${GITHUB_APP_PRIVATE_KEY}

scaffolder:
  # 模板执行并发数
  concurrentTasksLimit: 10

 techdocs:
   builder: 'local' # 或 'external' (使用 CI/CD 构建)
   generator:
     runIn: 'local' # 或 'docker'
   publisher:
     type: 'local' # 或 'awsS3', 'googleGcs', 'azureBlobStorage'
```

---

<!-- chunk: 三、Software Catalog 服务目录 -->## 三、Software Catalog 服务目录

## 3.1 实体定义 (catalog-info.yaml)

```yaml
apiVersion: backstage.io/v1alpha1
kind: Component
metadata:
  name: payment-service
  description: 支付核心服务
  tags:
    - java
    - spring-boot
    - microservice
  annotations:
    github.com/project-slug: my-org/payment-service
    backstage.io/techdocs-ref: dir:.
    grafana/dashboard-selector: "tags @> ['payment']"
    pagerduty.com/integration-key: "<INTEGRATION_KEY>"
    snyk.io/org-name: my-org
spec:
  type: service
  lifecycle: production
  owner: team-payments
  system: payment-platform
  dependsOn:
    - resource:postgres-payment-db
    - component:notification-service
  providesApis:
    - payment-api
---
apiVersion: backstage.io/v1alpha1
kind: Resource
metadata:
  name: postgres-payment-db
  description: 支付数据库
  tags:
    - postgres
    - database
spec:
  type: database
  owner: dba-team
  system: payment-platform
  dependencyOf:
    - component:payment-service
---
apiVersion: backstage.io/v1alpha1
kind: API
metadata:
  name: payment-api
  description: 支付 REST API
  tags:
    - rest
    - openapi
spec:
  type: openapi
  lifecycle: production
  owner: team-payments
  system: payment-platform
  definition:
    $text: https://github.com/my-org/payment-service/blob/main/openapi.yaml
```

## 3.2 自动发现配置

```yaml
# app-config.production.yaml
catalog:
  providers:
    githubOrg:
      myOrg:
        organization: 'my-org'
        catalogPath: '/catalog-info.yaml'
        filters:
          branch: 'main'
          repository: '.*'
        schedule:
          frequency: { minutes: 30 }
          timeout: { minutes: 3 }
```

---

<!-- chunk: 四、Software Templates 自助服务 -->## 四、Software Templates 自助服务

## 4.1 模板定义

```yaml
apiVersion: scaffolder.backstage.io/v1beta3
kind: Template
metadata:
  name: microservice-template
  title: Spring Boot Microservice
  description: 创建标准化的 Spring Boot 微服务
  tags:
    - spring-boot
    - java
    - recommended
spec:
  owner: platform-team
  type: service
  parameters:
    - title: 服务信息
      required:
        - name
        - owner
      properties:
        name:
          title: 服务名称
          type: string
          description: 唯一的微服务名称
          ui:autofocus: true
        owner:
          title: 团队
          type: string
          ui:field: OwnerPicker
          ui:options:
            allowedKinds:
              - Group
        description:
          title: 描述
          type: string
          description: 服务用途简述
    - title: 技术栈
      properties:
        javaVersion:
          title: Java 版本
          type: string
          enum: ['17', '21']
          default: '21'
        database:
          title: 数据库
          type: string
          enum: ['PostgreSQL', 'MySQL', 'MongoDB', 'None']
          default: 'PostgreSQL'
  steps:
    - id: fetch-base
      name: 获取模板
      action: fetch:template
      input:
        url: ./skeleton
        values:
          name: ${{ parameters.name }}
          owner: ${{ parameters.owner }}
          description: ${{ parameters.description }}
          javaVersion: ${{ parameters.javaVersion }}
          database: ${{ parameters.database }}
    - id: publish
      name: 发布到 GitHub
      action: publish:github
      input:
        allowedHosts: ['github.com']
        description: ${{ parameters.description }}
        repoUrl: github.com?owner=my-org&repo=${{ parameters.name }}
        defaultBranch: main
        repoVisibility: internal
    - id: register
      name: 注册到 Catalog
      action: catalog:register
      input:
        repoContentsUrl: ${{ steps.publish.output.repoContentsUrl }}
        catalogInfoPath: '/catalog-info.yaml'
  output:
    links:
      - title: 仓库
        url: ${{ steps.publish.output.remoteUrl }}
      - title: 目录
        icon: catalog
        entityRef: ${{ steps.register.output.entityRef }}
```

---

<!-- chunk: 五、TechDocs 文档即代码 -->## 五、TechDocs 文档即代码

```yaml
# mkdocs.yaml
site_name: 'Payment Service Docs'
nav:
  - Home: index.md
  - Architecture: architecture.md
  - API Reference: api.md
  - Runbooks: runbooks/
  - Onboarding: onboarding.md

plugins:
  - techdocs-core
```

**目录结构**
```
docs/
├── index.md
├── architecture.md
├── api.md
├── runbooks/
│   ├── incident-response.md
│   └── failover.md
└── onboarding.md
```

---

<!-- chunk: 六、插件生态集成 -->## 六、插件生态集成

## 6.1 核心生产插件

| 插件 | 作用 | 安装 |
|:---|:---|:---|
| `@backstage/plugin-[[Kubernetes|kubernetes]]` | K8s 资源可视化 | `yarn add` + 后端配置 |
| `@backstage/plugin-argo-cd` | [[argo\|Argo]] CD 应用状态 | `yarn add` |
| `@backstage/plugin-[[Prometheus|prometheus]]` | Prometheus 指标 | `yarn add` |
| `@backstage/plugin-grafana` | Grafana 仪表盘 | `yarn add` |
| `@backstage/plugin-sonarqube` | 代码质量看板 | `yarn add` |
| `@backstage/plugin-jira` | Jira 集成 | `yarn add` |
| `@backstage/plugin-pagerduty` | 值班与告警 | `yarn add` |
| `@backstage/plugin-cost-insights` | 成本洞察 | `yarn add` |
| `@roadiehq/backstage-plugin-github-pull-requests` | PR 看板 | `yarn add` |
| `@roadiehq/backstage-plugin-security-insights` | 安全洞察 | `yarn add` |
| `@roadiehq/backstage-plugin-argo-cd` | Argo CD 增强 | `yarn add` |
| `@k-phoen/backstage-plugin-opsgenie` | OpsGenie 集成 | `yarn add` |
| `@backstage/plugin-sentry` | 错误追踪 | `yarn add` |
| `@backstage/plugin-lighthouse` | Web 性能 | `yarn add` |
| `@backstage/plugin-airbrake` | 错误监控 | `yarn add` |
| `@backstage/plugin-badges` | 状态徽章 | `yarn add` |

## 6.2 K8s 插件配置

```yaml
# app-config.yaml
kubernetes:
  serviceLocatorMethod:
    type: 'multiTenant'
  clusterLocatorMethods:
    - type: 'config'
      clusters:
        - url: https://k8s-api.example.com
          name: production
          authProvider: 'serviceAccount'
          skipTLSVerify: false
          skipMetricsLookup: false
          serviceAccountToken: ${K8S_SA_TOKEN}
          dashboardUrl: https://k8s-dashboard.example.com
          dashboardApp: standard
```

---

<!-- chunk: 七、认证与多租户 -->## 七、认证与多租户

## 7.1 GitHub OAuth + 组织成员

```typescript
// packages/backend/src/plugins/auth.ts
import { createOAuthProviderIntegration } from '@backstage/plugin-auth-backend';

export default createOAuthProviderIntegration({
  provider: {
    github: {
      production: {
        signIn: {
          resolvers: [
            {
              resolver: 'emailMatchingUserEntityProfileEmail',
            },
          ],
        },
      },
    },
  },
});
```

## 7.2 权限框架 (Permissions Framework)

```yaml
# app-config.yaml
permission:
  enabled: true
```

```typescript
// 自定义权限策略
// packages/backend/src/plugins/permission.ts
import { createBackendModule } from '@backstage/backend-defaults';
import { policyExtensionPoint } from '@backstage/plugin-permission-node/alpha';

export default createBackendModule({
  pluginId: 'permission',
  moduleId: 'custom-policy',
  register(reg) {
    reg.registerInit({
      deps: { policy: policyExtensionPoint },
      async init({ policy }) {
        policy.setPolicy(async (request) => {
          if (request.permission.name === 'catalog.entity.read') {
            return { result: AuthorizeResult.ALLOW };
          }
          return { result: AuthorizeResult.DENY };
        });
      },
    });
  },
});
```

---

<!-- chunk: 八、生产部署 -->## 八、生产部署

## 8.1 Docker 构建

```dockerfile
# packages/backend/Dockerfile
FROM node:20-bookworm-slim
WORKDIR /app
COPY yarn.lock package.json packages/backend/dist/skeleton.tar.gz ./
RUN tar xzf skeleton.tar.gz && rm skeleton.tar.gz
RUN yarn install --frozen-lockfile --production --network-timeout 300000
COPY packages/backend/dist/bundle.tar.gz ./
RUN tar xzf bundle.tar.gz && rm bundle.tar.gz
CMD ["node", "packages/backend", "--config", "app-config.yaml", "--config", "app-config.production.yaml"]
```

## 8.2 Helm 部署

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 社区 Helm Chart (非官方)
helm repo add backstage https://backstage.github.io/charts
helm install backstage backstage/backstage \
  --namespace backstage \
  --create-namespace \
  --set backstage.image.tag=latest \
  --set postgresql.enabled=true
```
## 8.3 生产 Checklist

| 检查项 | 建议 |
|:---|:---|
| 数据库 | 使用 PostgreSQL，定期备份 |
| 缓存 | 配置 Redis 缓存 |
| 认证 | 启用 SSO (GitHub/Google/Okta) |
| Catalog 发现 | 配置自动同步，避免手动维护 |
| TechDocs | 使用 S3/GCS 存储，CI/CD 预构建 |
| 插件版本 | 锁定版本，定期升级 |
| 监控 | 启用 Prometheus 指标导出 |
| 日志 | 结构化 JSON 日志 |
| 权限 | 启用 Permission Framework |
| 搜索 | 配置 Elasticsearch/OpenSearch |

---

<!-- chunk: 参考链接 -->## 参考链接

- [Backstage 官方文档](https://backstage.io/docs/)
- [Backstage Plugins 市场](https://backstage.io/plugins/)
- [Backstage 创建应用指南](https://backstage.io/docs/getting-started/)
- [Software Catalog 定义](https://backstage.io/docs/features/software-catalog/descriptor-format/)
- [Software Templates](https://backstage.io/docs/features/software-templates/writing-templates/)
- [TechDocs](https://backstage.io/docs/features/techdocs/getting-started/)
- [Permissions Framework](https://backstage.io/docs/permissions/overview/)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- 平台工程 MOC
- [[10-平台工程/README.md|Domain 07: 平台工程 (Platform Engineering)]]
- Domain-36 平台工程 — 开源项目索引
- 平台工程概述与成熟度模型
- 内部开发者平台设计原则
- Backstage 部署与配置
- Backstage 软件目录与 TechDocs
- Backstage 脚手架与模板系统
- Kratix 平台即代码 (Kratix Platform as Code)
- Crossplane 平台组合 (Crossplane Platform Composition)
- Golden Paths 黄金路径设计 (Golden Paths Design Patterns)
- 开发者体验度量 (Developer Experience Metrics)

## See Also

- 10-platform-team-topology
- 11-vercel-frontend-deployment-platform
- 01-platform-engineering-overview
- 02-idp-design-principles


<!-- risk-assessed -->
