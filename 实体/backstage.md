---
title: Backstage [entities]
description: '## 概述'
summary: 'Backstage 是由 Spotify 开发的开源开发者门户（Internal Developer Portal, IDP）框架，于 2020 年捐赠给 CNCF，目前处于 Incubating 阶段。它通过统一的插件化界面整合微服务目录、文档、CI/CD 管道和模板，帮助平台工程团队构建自助式开发体验。'
category: entities
tags:
- k8s
- cncf
- platform
- backstage
- prometheus
- grafana
- argocd
- containerd
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Backstage 是什么
- 如何 Backstage
trigger_keywords:
- Backstage
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只风险/只读（信息收集，无副作用）。



# Backstage

> **CNCF 状态**: Incubating | **类别**: Platform | **主要语言**: TypeScript

## 概述

Backstage 是由 Spotify 于 2016 年内部开发的开发者门户框架，2020 年捐赠至 CNCF 并进入 Incubating 阶段。它是一个用于构建内部开发者平台（IDP）的开源框架，将微服务目录（Software Catalog）、技术文档（TechDocs）、模板脚手架（Scaffolder）和 CI/CD 可视化整合到统一的 Web 界面中。Backstage 的核心理念是"服务即一等公民"——每个微服务、库、数据和 AI 模型在目录中都有唯一条目，关联其 owner、API 定义、依赖关系和运行状态。

Backstage 采用前端 React + 后端 Node.js 的架构，通过插件系统实现高度可扩展。社区已提供 200+ 插件，覆盖 ArgoCD、Prometheus、GitHub、Jira、PagerDuty 等主流工具集成。企业也可以编写自定义插件来对接内部系统。

## Key Features

- **Software Catalog**：基于 `catalog-info.yaml` 描述服务元数据，自动聚合到统一目录，支持按 team、lifecycle、type 多维过滤
- **TechDocs**：将 Markdown 文档构建为可搜索的静态站点，与 Catalog 条目自动关联，类似"服务维基百科"
- **Scaffolder 模板**：通过自定义模板（Template）引导开发者从 CookieCutter 骨架创建新项目，自动注册到 Catalog 并配置 CI/CD
- **插件生态**：提供 ArgoCD、Prometheus、Kubernetes、GitHub、SonarQube 等丰富的官方和社区插件，支持快速扩展
- **Search**：统一的全文搜索能力，支持跨 Catalog 条目、TechDocs 和自定义数据源检索
- **权限系统**（v1.26+）：提供 RBAC 权限框架，控制不同角色对资源和操作的访问权限

## Architecture

Backstage 由三个核心层次构成：**前端 App**（React 单页应用，负责 UI 渲染和插件管理）、**后端**（Node.js 服务，提供 API 网关、数据聚合和认证代理）和**数据库**（默认使用 SQLite，生产环境推荐 PostgreSQL）。Catalog 后端通过 `EntityProvider` 从多个源（Git 仓库、Kubernetes API、外部系统）拉取 `catalog-info.yaml`，经处理后持久化到数据库。TechDocs 使用 MkDocs 将 Markdown 编译为 HTML，并存储到对象存储（S3/GCS/本地）。

## K8s 集成

Backstage 通过 Kubernetes 插件与集群深度集成。插件读取 Kubernetes API Server 的资源信息（Deployment、Pod、Service），在 Catalog 页面实时展示服务的运行状态、日志和事件。配置上需要为 Backstage 提供 ServiceAccount 和 kubeconfig（或 InClusterConfig），并映射集群到 `kubernetes.clusterLocatorMethods`。多个集群可通过 `clusters` 配置项注册。

## 生产部署要点

- **PostgreSQL**：生产环境使用 PostgreSQL 替代默认 SQLite，配置连接池和自动备份
- **认证集成**：集成 GitHub/GitLab/Google/OIDC 作为 IdP，启用 SSO
- **插件治理**：插件升级前在测试环境验证，避免破坏性变更
- **Catalog 自动发现**：使用 `Location` 和 `EntityProvider` 实现从 Git 仓库自动发现 catalog-info.yaml
- **TechDocs 构建器**：将 TechDocs 构建任务分离到独立 Pod 或 Job，避免阻塞主后端

## 生产场景

1. **微服务目录**：数百个微服务的统一视图，开发者可快速查找服务 owner、API 文档和运行状态
2. **新项目脚手架**：开发者通过 Scaffolder 创建新服务，自动生成代码仓库、CI/CD 管道和 Catalog 注册
3. **合规性追踪**：通过自定义插件展示服务的 license、安全扫描结果和合规状态
4. **知识管理**：TechDocs 作为团队的技术知识库，与代码仓库同步更新

## 安装与配置

```bash
# 初始化 Backstage 项目
npx @backstage/create-app@latest
cd my-backstage-app
yarn install
yarn dev  # 本地开发模式 (http://localhost:3000)
```

```yaml
# app-config.yaml 生产配置示例
backend:
  baseUrl: https://backstage.example.com
  listen:
    port: 7007
  database:
    client: pg
    connection:
      host: postgres.backstage.svc
      port: 5432
      user: backstage
      password: ${POSTGRES_PASSWORD}
  auth:
    keys:
      - secret: ${BACKEND_SECRET}

catalog:
  locations:
    - type: url
      target: https://github.com/org/repo/blob/main/catalog-info.yaml
    - type: url
      target: https://github.com/org/templates/blob/main/all-templates.yaml

kubernetes:
  clusterLocatorMethods:
    - type: config
      clusters:
        - url: https://k8s-api.example.com:6443
          name: production
          authProvider: serviceAccount
          serviceAccountToken: ${K8S_SA_TOKEN}
```

```bash
# Kubernetes 部署
kubectl create namespace backstage
kubectl create secret generic backstage-secrets \
  --from-literal=POSTGRES_PASSWORD=xxx \
  --from-literal=BACKEND_SECRET=xxx \
  -n backstage
helm repo add backstage https://backstage.github.io/charts
helm install backstage backstage/backstage -n backstage
```

## 运维操作

```bash
# 🟢 查看 Backstage 状态
kubectl get pods -n backstage
kubectl logs -n backstage -l app.kubernetes.io/name=backstage --tail=50

# 🟢 检查 Catalog 同步状态
curl -s http://backstage:7007/api/catalog/entities | jq length
curl -s http://backstage:7007/api/catalog/locations | jq .

# 🟢 查看插件健康
curl -s http://backstage:7007/api/health

# 🟡 重启 Backstage
kubectl rollout restart deployment/backstage -n backstage

# 🟡 刷新 Catalog 实体
curl -X POST http://backstage:7007/api/catalog/locations/<id>/refresh

# 🔴 重建数据库（丢失所有 Catalog 数据）
kubectl exec -n backstage postgres-0 -- dropdb backstage
kubectl rollout restart deployment/backstage -n backstage
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Catalog 实体缺失 | Git 仓库不可达/格式错误 | `kubectl logs -n backstage -l app=backstage \| grep catalog` | 检查 catalog-info.yaml 格式和仓库权限 |
| TechDocs 构建失败 | MkDocs 依赖缺失/内存不足 | `kubectl logs -n backstage -l app=backstage \| grep techdocs` | 增加内存限制或检查 mkdocs.yml |
| 插件加载失败 | 版本不兼容/依赖缺失 | 检查浏览器 Console 错误 | 升级插件或检查 peerDependencies |
| 数据库连接失败 | PostgreSQL 不可达/密码错误 | `kubectl exec -n backstage backstage-0 -- env \| grep PG` | 检查 Secret 和网络策略 |
| SSO 登录失败 | OIDC 配置错误/回调 URL 不匹配 | 检查 app-config.yaml auth 配置 | 核对 clientId/secret 和回调地址 |

```
排查流程：
├─ 服务不可用
│  ├─ kubectl get pods -n backstage 检查 Pod 状态
│  ├─ 检查 PostgreSQL 连接
│  └─ 检查 Ingress/Service 配置
├─ Catalog 问题
│  ├─ 检查 Location 配置和 Git 访问权限
│  ├─ 验证 catalog-info.yaml 格式
│  └─ 手动触发 refresh API
└─ 插件问题
   ├─ 检查插件版本兼容性
   └─ 查看后端日志中的插件错误
```

## 生产案例

### 案例 1：500+ 微服务统一门户

- **场景**: 大型互联网公司 500+ 微服务分散在多个团队，服务发现和文档管理混乱
- **排查**: 开发者平均花费 30 分钟查找服务 owner 和 API 文档
- **方案**: 部署 Backstage，强制所有服务注册 catalog-info.yaml，集成 ArgoCD/Prometheus 插件
- **效果**: 服务发现时间从 30min 降至 10s，新人 onboarding 时间缩短 60%

### 案例 2：自助式服务创建平台

- **场景**: 新服务创建需要多个团队协调（仓库、CI/CD、监控、文档）
- **排查**: 新服务从申请到上线平均需要 3 天
- **方案**: Backstage Scaffolder 模板一键创建服务（代码仓库+CI/CD+Catalog+监控）
- **效果**: 新服务创建时间从 3 天缩短至 5 分钟

## 对比

| 维度 | Backstage | Port | OpsLevel | Cortex |
|------|-----------|------|---------|--------|
| 开源 | ✅ Apache 2.0 | ❌ SaaS | ❌ SaaS | ❌ SaaS |
| 插件生态 | 200+ 社区插件 | 内置集成 | 内置集成 | 内置集成 |
| 自托管 | ✅ | ❌ | ❌ | ❌ |
| 定制化 | 高（代码级） | 中 | 低 | 低 |
| 适用场景 | 平台工程团队 | 快速上手 | 服务目录 | 服务健康 |

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[实体/argocd.md|[[ArgoCD|argocd]]]]
- [[概念/microservice-resilience-patterns.md|microservice-resilience-patterns]]
- [[概念/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[wasmedge]] — WasmEdge
- [[实体/cncf-runtime.md|cncf-runtime]] — CNCF 容器运行时与工具链项目全景
- [[04-containerd-upgrade-migration]] — [[containerd|containerd]]rd 升级迁移|containerd 升级迁移]]
- [[spin]] — Spin
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 03-backstage-deployment
- 04-backstage-catalog-techdocs
- 99-backstage-idp-guide
- 05-backstage-scaffolder-templates
- backstage
- [[概念/GitOps x 平台工程.md|GitOps x 平台工程]] — Cross-reference
- [[概念/platform-engineering-idp.md|Platform Engineering and Internal Developer Platforms]] — Cross-reference
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
