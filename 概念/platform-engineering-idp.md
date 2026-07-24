---
title: 平台工程与 IDP
summary: 平台工程与 IDP：平台工程已从基础设施运维演进为产品化思维驱动的内部开发者平台建设。核心理念：
category: concepts
tags:
- platform-engineering
- idp
- backstage
- crossplane
- k8s
tier: core
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
status: stable
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 平台工程与内部开发者平台（IDP）

## IDP 演进

平台工程已从基础设施运维演进为**产品化思维**驱动的内部开发者平台建设。核心理念：

- **Platform as a Product**：平台团队以产品管理方式运营内部平台，开发者是客户，需持续收集反馈、衡量满意度（DORA 指标、SPACE 框架）
- **开发者自助 Golden Paths**：为常见工作流（创建服务、配置 CI/CD、接入可观测性）提供预设模板和自助门户，减少认知负担
- **自助服务抽象**：开发者通过声明式接口请求资源（数据库、队列、域名），无需理解底层基础设施细节

平台成熟度模型：
1. **Level 0**：手动运维，脚本化
2. **Level 1**：标准化 CI/CD，基础自助
3. **Level 2**：完整 IDP，Golden Paths，自助资源调配
4. **Level 3**：AI 增强平台，智能推荐，自动优化

## Backstage v1.51+

[Backstage](https://backstage.io/) 是 CNCF **Incubating** 项目，已成为 IDP 门户的事实标准。

关键特性：
- **软件目录（Software Catalog）**：集中管理所有服务、API、资源的元数据
- **软件模板（Software Templates）**：脚手架化创建新服务，内置最佳实践
- **TechDocs**：Docs-as-Code，Markdown/MkDocs 集成
- **210+ 插件生态**：覆盖 Kubernetes、ArgoCD、Grafana、PagerDuty、Snyk 等
- **AI-Native 能力**：内置 AI 助手插件，支持自然语言查询目录、生成文档
- **MCP（Model Context Protocol）支持**：AI Agent 可通过 MCP 协议直接消费 Backstage 目录和模板

相关：developer experience tooling

## Crossplane v2

[Crossplane](https://www.crossplane.io/) 是 CNCF Incubating 项目，将 Kubernetes 扩展为通用控制平面。

v2 关键演进：
- **应用级控制平面**：不再仅限基础设施资源，支持定义应用级抽象（Environment、DatabaseClaim）
- **Composite Resources（XRs）**：组合多个托管资源为单一声明式接口
- **Functions**：用任何语言编写资源编排逻辑（Go、Python、Starlark）
- **AI Agent 消费**：Crossplane 的声明式 API 天然适合 AI Agent 发现和消费，Agent 可通过 kubectl/REST 创建资源声明
- **环境晋升（Environment Promotion）**：原生支持跨环境（dev → staging → prod）的资源配置晋升

与 GitOps 深度集成：Crossplane Claims 存储在 Git 中，由 ArgoCD/Flux 同步。

## Humanitec Platform Orchestrator

Humanitec 提供**编排层**，连接开发者抽象与基础设施实际配置：

- **Score 接口**：开放规范，开发者用 Score 文件声明工作负载需求（资源、变量、端口）
- **Graph 后端**：平台图谱，建模组织内所有资源关系和依赖
- **MCP 支持**：AI Agent 可通过 MCP 协议查询平台状态、创建部署
- **动态配置注入**：根据环境自动注入数据库连接串、密钥等配置

Score 规范示例：
```yaml
apiVersion: score.dev/v1b1
metadata:
  name: my-service
containers:
  main:
    image: registry.example.com/my-service:1.0
    variables:
      DB_HOST: ${resources.db.host}
service:
  ports:
    http: 8080
resources:
  db:
    type: postgres
```

## Kratix

[Kratix](https://kratix.io/) 是一个框架，帮助平台团队构建**基于 Promises 的自助平台**：

- **Promises 抽象**：将平台能力封装为 Promise，开发者请求后自动编排
- **工作流编排**：Promise 内定义多阶段工作流（创建、更新、删除）
- **GitOps 原生**：状态存储在 Git 中，与 GitOps 实践对齐
- **渐进式采用**：可逐步添加 Promises，无需一次性重构

## 平台工程团队结构

成熟的平台组织通常采用双轨团队结构：

| 角色 | 聚焦点 | 产出 |
|------|--------|------|
| **Infrastructure PE** | 集群、网络、存储、安全基线 | Terraform 模块、Crossplane Compositions、安全策略 |
| **DevEx PE** | 开发者体验、门户、CI/CD、模板 | Backstage 配置、Golden Path 模板、SDK |

关键原则：
- 平台团队不超过 8-12 人（两个披萨原则）
- 需有 Product Manager 角色（即使是兼职）
- 建立内部 SLO：自助操作 < 5 分钟完成、文档覆盖率 > 90%
- 定期开发者满意度调查（季度 NPS）

## 企业案例

### RBC Capital Markets
- 运营 **50+ Kubernetes 集群**，覆盖多区域和合规要求
- 使用 IDP 统一管理多集群部署和合规策略
- 通过 Golden Paths 将新服务上线时间从数周缩短至数小时

### Kairos / k0rdent / bindy
- **Kairos**：不可变 Linux 发行版，专为 Kubernetes 边缘节点设计
- **k0rdent**：多集群生命周期管理平台，统一管理分布式 K8s 集群
- **bindy**：Kubernetes 原生的环境管理和资源调配工具

## 工具链关系图

```
开发者
  │
  ▼
Backstage (门户) ──→ Score (Humanitec) ──→ Crossplane (控制平面)
  │                                            │
  ▼                                            ▼
Kratix (Promises)                        云资源 / K8s 集群
  │
  ▼
GitOps (ArgoCD / Flux)
```

## 源码实现分析

### Backstage 插件架构

```typescript
// backstage/packages/core-plugin-api/src/extension.ts
// Backstage 插件系统：每个功能是独立插件，通过 ExtensionPoint 组合
export function createPlugin<T extends RouteRef>(options: {
  id: string;
  routes: { [key: string]: T };
  apis: AnyApiRef[];
}): BackstagePlugin {
  return {
    id: options.id,
    // 插件注册 Extension（页面、卡片、导航项）
    provide: (extension) => registerExtension(options.id, extension),
    // 插件消费其他插件的 API
    consume: (apiRef) => getApi(apiRef),
  };
}

// 自定义插件示例：服务目录集成
export const serviceCatalogPlugin = createPlugin({
  id: 'service-catalog',
  routes: { root: rootRouteRef },
  apis: [discoveryApiRef, identityApiRef],
});
```

### Crossplane Composition 引擎

```go
// github.com/crossplane/crossplane/internal/controller/apiextensions/composite.go
// Crossplane 将 XR (Composite Resource) 调谐为底层云资源
func (r *Reconciler) Reconcile(ctx context.Context, req reconcile.Request) {
    // 1. 获取 Composite Resource (XR)
    xr := r.getCompositeResource(req)
    // 2. 查找对应的 Composition
    comp := r.selectComposition(xr)
    // 3. 渲染 Patch 和 Transform，生成底层资源
    for _, resource := range comp.Spec.Resources {
        composed := r.renderComposedResource(xr, resource)
        // 4. 创建/更新底层资源（RDS/S3/VPC...）
        r.apply(ctx, composed)
    }
    // 5. 回写 XR Status
    r.updateStatus(xr)
}
```

### IDP 架构全景

```
┌───────────────────────────────────────────────────────────┐
│              IDP 平台架构全景                          │
├───────────────────────────────────────────────────────────┤
│                                                           │
│  开发者层                                                │
│  ─────────                                              │
│  Backstage 门户 → 自助服务目录 / Golden Path 模板    │
│       │                                                  │
│  抽象层                                                  │
│  ─────────                                              │
│  Score/Kratix → 工作负载抽象 → 环境无关描述       │
│       │                                                  │
│  控制平面层                                              │
│  ─────────                                              │
│  Crossplane → 云资源编排 / Compositions / XRs      │
│       │                                                  │
│  交付层                                                  │
│  ─────────                                              │
│  ArgoCD/Flux → GitOps 同步 → 多集群部署            │
│       │                                                  │
│  基础设施层                                              │
│  ─────────                                              │
│  K8s 集群 / 云资源 / 网络 / 存储 / 安全基线       │
└───────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：Backstage 服务模板（🟢 自助服务）

```yaml
# Backstage Software Template: 创建新微服务
apiVersion: scaffolder.backstage.io/v1beta3
kind: Template
metadata:
  name: create-microservice
spec:
  parameters:
  - title: 服务信息
    properties:
      serviceName:
        type: string
        title: 服务名称
      team:
        type: string
        title: 负责团队
  steps:
  - id: fetch-template
    action: fetch:template
    input:
      url: ./skeleton
      values:
        serviceName: ${{ parameters.serviceName }}
  - id: publish
    action: publish:github
    input:
      repoUrl: github.com/org/${{ parameters.serviceName }}
  - id: register
    action: catalog:register
    input:
      repoContentsUrl: ${{ steps.publish.output.repoContentsUrl }}
```

### 场景二：Crossplane 数据库自助（🟡 创建云资源）

```yaml
# 开发者只需提交 XR，平台自动编排底层资源
apiVersion: platform.example.org/v1alpha1
kind: PostgreSQLInstance
metadata:
  name: my-app-db
spec:
  version: "15"
  size: small  # 抽象层：开发者无需知道具体实例类型
  region: us-east-1
---
# Composition 自动创建：RDS + SecurityGroup + ParameterGroup + Backup
# 开发者无需接触 AWS 细节
```

### 场景三：平台 SLO 监控（🟢 只读）

```bash
# 平台工程团队内部 SLO 检查
# 自助操作完成率
curl -s https://backstage.internal/api/metrics | \
  jq '.scaffolder_completed_total / .scaffolder_started_total'

# Golden Path 覆盖率
kubectl get services -A -l 'backstage.io/managed=true' --no-headers | wc -l
kubectl get services -A --no-headers | wc -l
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| IDP 就是 Backstage | Backstage 只是门户层，IDP 包含完整工具链 |
| 平台工程 = DevOps 重命名 | 平台工程强调产品化思维、内部客户、SLO |
| 必须一次性全部建设 | 应渐进式采用，先解决最痛的问题 |
| 平台团队不需要 PM | 必须有产品经理角色，否则变成工单团队 |
| 强制所有团队使用 | 平台应通过体验吸引采用，而非强制 |
| 平台不需要 SLO | 平台本身需要 SLO：自助操作 <5min、可用性 >99.9% |

## 面试要点

1. **IDP 与 DevOps 的核心区别？**
   - DevOps：文化 + 实践，开发者自己运维
   - IDP：产品化平台，抽象复杂性，开发者自助服务
   - 核心：降低认知负荷、Golden Path、内部客户思维

2. **平台工程团队如何组织？**
   - 双轨：Infrastructure PE + DevEx PE
   - 不超过 8-12 人，必须有 PM 角色
   - 内部 SLO + 季度 NPS 调查

3. **Backstage 在 IDP 中的角色？**
   - 统一门户：服务目录 + 模板 + 文档 + 插件
   - 不是 IDP 全部，而是开发者交互层
   - 插件架构允许扩展任意功能

4. **如何衡量平台工程的成功？**
   - 开发者满意度（NPS）
   - 自助操作完成率（>80%）
   - 新服务上线时间（周→小时）
   - 平台可用性 SLO（>99.9%）

## 相关概念

- developer experience tooling：开发者体验工具链
- GitOps：GitOps 基础设施管理
- [[progressive-delivery-strategies]]：渐进式交付策略
- cloud native security：零信任安全模型

## Related

- [[概念/platform-engineering-sre|平台工程 × SRE(协作视角)]]
- [[概念/gitops-production-operations.md|gitops production operations]] — GitOps 生产运维
- [[概念/progressive-delivery-strategies.md|progressive delivery strategies]] — 渐进式交付策略
- [[概念/finops-greenops-practices.md|finops greenops practices]] — FinOps 与绿色运维实践


<!-- risk-assessed -->
