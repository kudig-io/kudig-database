---
title: "Backstage.io 2025：插件生态、v1.28+ 新特性与企业落地"
description: "Backstage.io 1.28+ 重要更新、2025 年插件生态全景、New Backend System 迁移、Scaffolder Templates 高级实践与企业级 IDP 落地经验"
summary: "深度覆盖 Backstage 1.28+ New Backend System 正式 GA、前端插件架构重构、RBAC 增强、Catalog Entity Validation 改进；2025 年核心插件推荐（Kubernetes/ArgoCD/TechDocs/Scaffolder/Copilot）；企业 Backstage 落地路径与常见陷阱"
category: practice
tags:
- backstage
- idp
- platform-engineering
- plugin-ecosystem
- scaffolder
- service-catalog
- techdocs
- rbac
- developer-portal
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- DevOps 工程师
- 架构师
estimated_read_time: 20min
intent_queries:
- "Backstage New Backend System 如何迁移"
- "Backstage 2025 最佳插件有哪些"
- "Backstage 企业落地如何规划"
- "Backstage RBAC 如何配置"
trigger_keywords:
- Backstage
- New Backend System
- Scaffolder
- TechDocs
- Service Catalog
- RBAC
prerequisites:
- nodejs-basics
- kubernetes-basics
- idp-basics
sources:
- https://backstage.io/docs/
- https://github.com/backstage/backstage
- https://roadie.io/backstage/
- https://www.cncf.io/projects/backstage/
---

# Backstage.io 2025：插件生态、v1.28+ 新特性与企业落地

> Backstage 已成为 CNCF 孵化项目中增长最快的项目之一（2024 年贡献者数量同比增长 40%），2025 年 New Backend System 正式 GA 标志着架构现代化完成。

## Backstage 1.28+ 重要更新

### New Backend System 正式 GA

New Backend System（NBS）是 Backstage 自 1.20 开始推进的后端架构重构，1.28 宣告正式 GA 并成为推荐方案。

**架构对比：**

```
旧后端（Legacy Backend）          新后端（New Backend System）
─────────────────────────────────────────────────────────────
const backend = createBackend();   import { createBackend } from
  // 手动注册每个插件               '@backstage/backend-defaults';
  backend.add(                     const backend = createBackend();
    import('@backstage/plugin-catalog-backend'));    backend.add(
  backend.add(                       import('@backstage/plugin-catalog-backend/alpha'));
    import('@backstage/plugin-auth-backend'));       backend.start();

特点：                            特点：
• 显式 createEnv 模板             • 依赖注入（DI）容器
• 手动 wiring                    • 插件自动发现
• 难以扩展                       • 类型安全的扩展点
• 共享 env 对象                  • 独立插件生命周期
```

**迁移到 New Backend System：**

```typescript
// packages/backend/src/index.ts（新后端）
import { createBackend } from '@backstage/backend-defaults';
import { PackageRoles } from '@backstage/cli-node';

const backend = createBackend();

// 核心插件
backend.add(import('@backstage/plugin-app-backend/alpha'));
backend.add(import('@backstage/plugin-catalog-backend/alpha'));
backend.add(import('@backstage/plugin-catalog-backend-module-scaffolder-entity-model'));
backend.add(import('@backstage/plugin-scaffolder-backend/alpha'));
backend.add(import('@backstage/plugin-scaffolder-backend-module-github'));
backend.add(import('@backstage/plugin-auth-backend'));
backend.add(import('@backstage/plugin-auth-backend-module-github-provider'));
backend.add(import('@backstage/plugin-techdocs-backend/alpha'));

// 自定义扩展（新 API）
backend.add(import('./extensions/customCatalogProcessor'));
backend.add(import('./extensions/customScaffolderActions'));

backend.start();
```

```typescript
// 自定义 Catalog Processor（新 API）
import {
  coreServices,
  createBackendModule,
} from '@backstage/backend-plugin-api';
import {
  catalogProcessingExtensionPoint,
} from '@backstage/plugin-catalog-node/alpha';
import { CustomEntityProcessor } from './CustomEntityProcessor';

export const catalogModuleCustomProcessor = createBackendModule({
  pluginId: 'catalog',
  moduleId: 'custom-processor',
  register(reg) {
    reg.registerInit({
      deps: {
        catalog: catalogProcessingExtensionPoint,
        logger: coreServices.logger,
      },
      async init({ catalog, logger }) {
        catalog.addProcessor(new CustomEntityProcessor(logger));
      },
    });
  },
});
```

### RBAC 增强（1.26+）

```yaml
# Permission Framework 配置
# app-config.yaml
permission:
  enabled: true
  # 使用内置 RBAC 插件
  rbac:
    admin:
      users:
        - name: user:default/platform-admin
      groups:
        - name: group:default/platform-team

# 策略定义（packages/backend/src/permissions.ts）
```

```typescript
// 细粒度权限策略
import {
  PolicyQuery,
  PolicyQueryUser,
  PolicyDecision,
  AuthorizeResult,
} from '@backstage/plugin-permission-common';
import {
  catalogConditions,
  createCatalogConditionalDecision,
} from '@backstage/plugin-catalog-backend/alpha';

export class CustomPermissionPolicy implements PermissionPolicy {
  async handle(
    request: PolicyQuery,
    user?: PolicyQueryUser,
  ): Promise<PolicyDecision> {
    // 只能修改自己团队的实体
    if (
      request.permission.name === 'catalog.entity.delete' &&
      isResourcePermission(request.permission, 'catalog-entity')
    ) {
      return createCatalogConditionalDecision(
        request.permission,
        catalogConditions.isEntityOwner({
          claims: user?.info.ownershipEntityRefs ?? [],
        }),
      );
    }

    // 读取权限：所有认证用户
    if (request.permission.attributes.action === 'read') {
      return { result: AuthorizeResult.ALLOW };
    }

    // 默认拒绝
    return { result: AuthorizeResult.DENY };
  }
}
```

### Entity Validation 增强

```yaml
# 严格 Entity 验证配置
catalog:
  rules:
    - allow: [Component, Service, API, Group, User, Resource, System, Domain, Location]
  providers:
    github:
      myGithubOrg:
        organization: my-company
        catalogPath: '/catalog-info.yaml'
        filters:
          branch: main
          repository: '.*'             # 所有仓库
        schedule:
          frequency: { minutes: 30 }
          timeout: { minutes: 3 }
  # 自定义验证
  validators:
    - type: 'custom'
      path: './src/validators/security-standards'
```

---

## 2025 核心插件生态

### 必装插件清单

| 插件 | 类型 | 功能 | 成熟度 |
|------|------|------|--------|
| `@backstage/plugin-kubernetes` | 官方 | K8s 资源可视化 | GA |
| `@backstage/plugin-techdocs` | 官方 | 文档即代码 | GA |
| `@backstage/plugin-scaffolder` | 官方 | 服务脚手架 | GA |
| `@roadiehq/backstage-plugin-argo-cd` | 社区 | Argo CD 集成 | GA |
| `@backstage/plugin-cost-insights` | 官方 | 成本可见性 | Beta |
| `@backstage/plugin-github-actions` | 官方 | GitHub Actions 集成 | GA |
| `backstage-plugin-flux` | 社区 | Flux GitOps 集成 | GA |
| `@roadiehq/backstage-plugin-datadog` | 社区 | Datadog 监控 | GA |
| `@backstage/plugin-pagerduty` | 官方 | PagerDuty 告警 | GA |
| `@backstage/plugin-sonarqube` | 官方 | 代码质量 | GA |
| `backstage-plugin-ai-assistant` | 社区 | AI 代码助手集成 | Beta |

### Kubernetes 插件增强配置

```yaml
# app-config.yaml Kubernetes 配置
kubernetes:
  serviceLocatorMethod:
    type: 'multiTenant'
  clusterLocatorMethods:
    - type: 'config'
      clusters:
        - url: https://k8s-prod.company.io
          name: production
          authProvider: 'serviceAccount'
          skipTLSVerify: false
          serviceAccountToken: ${K8S_PROD_TOKEN}
          caData: ${K8S_PROD_CA}
          customResources:
            - group: 'argoproj.io'
              apiVersion: 'v1alpha1'
              plural: 'rollouts'
            - group: 'serving.kserve.io'
              apiVersion: 'v1beta1'
              plural: 'inferenceservices'
        - url: https://k8s-staging.company.io
          name: staging
          authProvider: 'oidc'
          oidcTokenProvider: okta
```

### Scaffolder Templates 高级实践

```yaml
# 完整微服务脚手架模板
apiVersion: scaffolder.backstage.io/v1beta3
kind: Template
metadata:
  name: microservice-java-spring
  title: Java Spring Boot 微服务
  description: 生产就绪的 Java 微服务，含 K8s 部署、监控、CI/CD
  tags:
    - java
    - spring-boot
    - kubernetes
    - recommended
spec:
  owner: platform-team
  type: service

  parameters:
    - title: 服务基本信息
      required: [serviceName, owner, description]
      properties:
        serviceName:
          type: string
          title: 服务名称
          pattern: '^[a-z][a-z0-9-]{2,30}$'
          ui:autofocus: true
        owner:
          type: string
          title: 所属团队
          ui:field: OwnerPicker
          ui:options:
            catalogFilter:
              kind: Group
        description:
          type: string
          title: 服务描述
        javaVersion:
          type: string
          title: Java 版本
          default: '21'
          enum: ['17', '21']
          enumNames: ['Java 17 (LTS)', 'Java 21 (Latest LTS)']

    - title: 基础设施配置
      properties:
        database:
          type: boolean
          title: 需要数据库？
          default: true
        databaseType:
          type: string
          title: 数据库类型
          if:
            properties:
              database:
                const: true
          enum: [postgres, mysql, mongodb]
          default: postgres
        minReplicas:
          type: integer
          title: 最小副本数
          default: 2
          minimum: 1
          maximum: 10

  steps:
    - id: fetch-base
      name: 获取基础模板
      action: fetch:template
      input:
        url: ./skeleton
        values:
          serviceName: ${{ parameters.serviceName }}
          owner: ${{ parameters.owner }}
          javaVersion: ${{ parameters.javaVersion }}
          database: ${{ parameters.database }}

    - id: create-repo
      name: 创建 GitHub 仓库
      action: github:repo:create
      input:
        repoUrl: github.com?owner=my-company&repo=${{ parameters.serviceName }}
        description: ${{ parameters.description }}
        repoVisibility: private
        defaultBranch: main
        topics:
          - ${{ parameters.owner }}
          - java
          - microservice

    - id: register-catalog
      name: 注册服务目录
      action: catalog:register
      input:
        repoContentsUrl: ${{ steps['create-repo'].output.repoContentsUrl }}
        catalogInfoPath: /catalog-info.yaml

    - id: create-jira-epic
      name: 创建 Jira Epic
      action: jira:create:epic
      input:
        projectKey: OPS
        summary: "服务初始化：${{ parameters.serviceName }}"
        description: "新微服务 ${{ parameters.serviceName }} 初始化完成，owner: ${{ parameters.owner }}"

  output:
    links:
      - title: GitHub 仓库
        url: ${{ steps['create-repo'].output.remoteUrl }}
      - title: 服务目录
        icon: catalog
        entityRef: ${{ steps['register-catalog'].output.entityRef }}
      - title: CI/CD 流水线
        url: ${{ steps['create-repo'].output.remoteUrl }}/actions
```

### TechDocs 2025 最佳实践

```yaml
# TechDocs 配置（app-config.yaml）
techdocs:
  builder: 'external'           # 推荐外部构建（CI/CD 触发）
  generator:
    runIn: 'docker'
    dockerImage: 'spotify/techdocs:v1.4.0'
  publisher:
    type: 'awsS3'               # 或 googleGcs / azureBlobStorage
    awsS3:
      bucketName: ${TECHDOCS_S3_BUCKET}
      region: us-east-1
      credentials:
        roleArn: ${TECHDOCS_ROLE_ARN}
  cache:
    ttl: 3600000                # 1小时缓存
```

```yaml
# mkdocs.yml 推荐配置
site_name: 'My Service'
site_description: 'API 文档与运维手册'
docs_dir: docs

plugins:
  - techdocs-core              # 必须包含
  - search
  - git-revision-date-localized:
      enable_creation_date: true
  - mermaid2:                  # 支持 Mermaid 图表
      version: 10.4.0

markdown_extensions:
  - admonition
  - pymdownx.details
  - pymdownx.superfences:
      custom_fences:
        - name: mermaid
          class: mermaid
          format: !!python/name:pymdownx.superfences.fence_code_format
  - pymdownx.tabbed
  - tables
  - attr_list
```

---

## 企业 Backstage 落地路径

### 分阶段落地计划（6 个月）

```
Month 1-2：基础建设
├── 部署 Backstage（K8s Helm Chart）
├── 接入企业 SSO（Okta/Azure AD）
├── 导入现有服务到 Catalog（50+ 个）
├── 配置 Kubernetes 插件
└── 建立基础治理（Owner 规则）

Month 3-4：增值功能
├── TechDocs 平台化（存量文档迁移）
├── 第一个 Scaffolder Template（最常用服务类型）
├── GitHub Actions / Argo CD 集成
├── 建立 Champion 网络（每团队 1 人）
└── 首次 eNPS 基线调查

Month 5-6：生态扩展
├── 成本可见性插件（Cost Insights）
├── 安全评分卡（安全合规可视化）
├── 自定义 Actions（内部工具集成）
├── Platform API（外部系统集成）
└── 度量仪表盘（DORA + 平台指标）
```

### Helm 部署配置

```yaml
# backstage/values.yaml（生产配置）
backstage:
  image:
    registry: ghcr.io
    repository: my-company/backstage
    tag: "1.28.0"

  appConfig:
    app:
      title: "My Company Developer Portal"
      baseUrl: https://backstage.company.io
    backend:
      baseUrl: https://backstage.company.io
      cors:
        origin: https://backstage.company.io
      database:
        client: pg
        connection:
          host: ${POSTGRES_HOST}
          port: 5432
          user: ${POSTGRES_USER}
          password: ${POSTGRES_PASSWORD}
          database: backstage
    auth:
      providers:
        github:
          development:
            clientId: ${GITHUB_CLIENT_ID}
            clientSecret: ${GITHUB_CLIENT_SECRET}

  resources:
    requests:
      cpu: "500m"
      memory: "1Gi"
    limits:
      cpu: "2"
      memory: "2Gi"

  autoscaling:
    enabled: true
    minReplicas: 2
    maxReplicas: 5
    targetCPUUtilizationPercentage: 70

postgresql:
  enabled: true
  auth:
    password: ${POSTGRES_PASSWORD}
  primary:
    persistence:
      size: 20Gi
    resources:
      requests:
        cpu: "500m"
        memory: "1Gi"
```

### 常见陷阱

| 陷阱 | 表现 | 解决方案 |
|------|------|---------|
| Catalog 数据腐化 | Entity 信息过时，开发者不信任 | 自动化 Catalog 导入 + 定期验证 |
| 插件爆炸 | 插件过多，性能下降 | 分阶段引入，定期审查使用率 |
| 权限过于宽松 | 任意人可修改任意实体 | 启用 Permission Framework |
| TechDocs 废弃 | 文档与代码脱节 | CI 检查文档变更，与代码 PR 绑定 |
| 无人维护 | 平台团队 turnover 后停滞 | 建立 runbook，至少 3 人 on-call |
| 强制迁移 | 团队抵触，影子工具盛行 | 迁移激励计划，保留旧工具过渡期 |

---

## 参考资源

- [Backstage 官方文档](https://backstage.io/docs/)
- [New Backend System 迁移指南](https://backstage.io/docs/backend-system/building-backends/migrating)
- [Scaffolder Actions 目录](https://backstage.io/docs/features/software-templates/builtin-actions)
- [插件生态市场](https://backstage.io/plugins)
- [Backstage GitHub](https://github.com/backstage/backstage)
