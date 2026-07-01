---
title: 平台工程与 IDP
category: concepts
tags:
  - platform-engineering
  - idp
  - backstage
  - crossplane
  - k8s
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

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

## 相关概念

- developer experience tooling：开发者体验工具链
- GitOps：GitOps 基础设施管理
- [[progressive-delivery-strategies]]：渐进式交付策略
- cloud native security：零信任安全模型

## Related

- [[concepts/gitops-production-operations.md|gitops production operations]] — GitOps 生产运维
- [[concepts/progressive-delivery-strategies.md|progressive delivery strategies]] — 渐进式交付策略
- [[concepts/finops-greenops-practices.md|finops greenops practices]] — FinOps 与绿色运维实践
