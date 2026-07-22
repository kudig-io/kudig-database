---
title: Backstage 与平台目录的整合
description: '- 日志和事件聚合'
summary: '- 日志和事件聚合'
category: synthesis
tags:
- backstage
- platform-engineering
- developer-experience
- service-catalog
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Backstage 与平台目录的整合 是什么
- 如何 Backstage 与平台目录的整合
trigger_keywords:
- Backstage
- 与平台目录的整合
prerequisites:
- kubectl-basics
relationships:
- target: '[[系统基础/知识字典/networking/service.md]]'
  type: uses
- target: '[[实体/backstage.md]]'
  type: related_to
- target: '[[系统基础/速查卡/k8s.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[实体/backstage.md|Backstage]] 与平台目录的整合

## 概述

Backstage 是 Spotify 开源的开发者门户框架，已成为云原生平台工程领域的事实标准。其核心价值在于**统一服务目录**（Software Catalog）——为组织内所有服务、组件、资源提供单一可信来源（SSOT）。通过集成 Kubernetes、CI/CD、监控、文档等系统，Backstage 将分散的开发者工具链整合为一致的体验，大幅降低认知负荷。

## Backstage 架构深度解析

### 三层架构

```
┌─────────────────────────────────────────┐
│         前端层 (React App)               │
│   - 服务目录浏览、搜索、Scaffolder       │
│   - 插件化 UI 组件                       │
├─────────────────────────────────────────┤
│         后端层 (Node.js)                 │
│   - Catalog 摄入与实体解析               │
│   - 权限集成、任务调度                   │
│   - 插件后端 API                         │
├─────────────────────────────────────────┤
│         数据层                           │
│   - Catalog 实体存储 (PostgreSQL)        │
│   - 搜索索引                             │
└─────────────────────────────────────────┘
```

### 实体模型（Entity Model）

Backstage Catalog 的核心是统一的实体描述格式 `catalog-info.yaml`：

```yaml
# catalog-info.yaml
apiVersion: backstage.io/v1alpha1
kind: Component
metadata:
  name: order-service
  description: 订单处理微服务
  tags:
    - java
    - spring-boot
  links:
    - url: https://github.com/org/order-service
      title: Repository
      icon: github
spec:
  type: service                        # service | website | library | ...
  lifecycle: production                # experimental | production | deprecated
  owner: team-payments                 # 引用 Group 实体
  system: e-commerce-platform          # 引用 System 实体
  dependsOn:
    - "resource:default/payment-db"    # 依赖关系
  providesApis:
    - "api:default/order-api"          # 提供 API
```

实体之间的**依赖图谱**是 Backstage 最强大的能力之一——可以可视化分析服务间依赖、识别单点故障、评估变更影响范围。

## 核心插件

```
必备插件:
├── Kubernetes: 集群资源可视化
│   → 关联 catalog 实体与 K8s Deployment
│   → 实时显示 Pod 状态、日志、事件
├── TechDocs: 技术文档中心
│   → Markdown 文档自动构建发布
│   → 类似 Docs-as-Code
├── Catalog: 服务目录与依赖图
│   → 实体摄入、搜索、关系图谱
├── Scaffolder: 自服务模板
│   → 脚手架创建新服务
│   → 标准化项目结构
├── Cost Insights: 成本可视化
│   → 集成 Kubecost/OpenCost
│   → 团队维度成本分摊
└── PagerDuty/OpsGenie: On-call 信息
    → 服务与值班关联
    → 事件时间线展示
```

## 与 [[系统基础/速查卡/k8s.md|K8s]] 集成

### 注解关联机制

Backstage 通过 Kubernetes 注解将 catalog 实体与集群资源关联：

```yaml
# K8s Deployment 中的注解
metadata:
  annotations:
    backstage.io/managed-by-location: url:https://github.com/org/order-service/catalog-info.yaml
    backstage.io/managed-by-origin-location: url:https://github.com/org/order-service/catalog-info.yaml
```

### Backstage 读取 K8s 的能力

```
Backstage Kubernetes 插件读取:
  - 服务 → Deployment 关联（通过注解匹配）
  - Pod 状态实时显示（Running/Pending/Error）
  - 容器日志流式查看
  - K8s Events 事件聚合
  - 跨集群资源统一视图
```

## 服务目录价值

```
统一视图: "谁拥有这个服务？"
  → Catalog 中的 owner 字段关联到 Group 实体

依赖关系: "这个服务的上下游是什么？"
  → dependsOn / dependencyOf 构建完整依赖图

运行状态: "当前的 SLO 状态？"
  → 集成 Prometheus/Grafana，展示 SLO 达标率

发布历史: "最近的发布历史？"
  → 集成 ArgoCD/CI-CD，展示部署时间线
```

## 最佳实践

- **将 catalog-info.yaml 纳入应用仓库**：每个服务仓库包含自己的 `catalog-info.yaml`，通过 Backstage 的 Git 摄入器自动发现——这保证了目录与代码的同步
- **建立实体命名规范**：统一的命名约定（`team-service`）是可搜索性和依赖图可读性的基础
- **优先集成 Kubernetes 插件**：K8s 实时状态是开发者最频繁查看的信息，早期集成价值最高
- **利用 Scaffolder 标准化新服务创建**：定义黄金路径模板，确保新服务自带 CI/CD、监控、catalog-info 等标准配置
- **定期清理过期实体**：配置 catalog 规则自动标记 `lifecycle: deprecated` 的实体，保持目录清洁

## 常见陷阱

- **实体摄入性能问题**：大型组织可能有数千个仓库，Backstage 的 Git 摄入器扫描全量仓库会很慢——应使用增量扫描和 Webhook 触发
- **Catalog 数据与实际不一致**：catalog-info.yaml 在仓库中但服务实际已下线，需配置定期清理策略（如 entity 过期自动删除）
- **插件选型过多导致维护负担**：Backstage 生态有大量社区插件，但每个插件都需要维护和升级——优先选择核心插件和高活跃度社区插件

## 源码实现分析

### Backstage Catalog 实体模型

```typescript
// packages/catalog-model/src/entity.ts
// Backstage 核心实体模型
export interface Entity {
  apiVersion: string;  // 'backstage.io/v1alpha1'
  kind: string;        // Component | API | Resource | System | Domain
  metadata: {
    name: string;
    namespace?: string;
    annotations: Record<string, string>;
    // 关键注解:
    // 'backstage.io/kubernetes-id': 关联 K8s 资源
    // 'github.com/project-slug': 关联 GitHub 仓库
  };
  spec: {
    type: string;      // service | website | library
    owner: string;     // team:platform-engineering
    lifecycle: string; // production | experimental
    system?: string;   // 所属系统
  };
}
// Catalog 通过 Processor 链解析 catalog-info.yaml
// KubernetesDiscoveryProcessor: 自动发现集群中的 K8s 资源
```

```
┌─────────────────────────────────────────────────────────┐
│     Backstage 平台架构                                │
├─────────────────────────────────────────────────────────┤
│  Developer Portal (React Frontend)                      │
│       │                                                 │
│       ▼                                                 │
│  ┌────────────────────────────────────────┐  │
│  │  Backend (Node.js)                          │  │
│  │  Catalog │ Scaffolder │ TechDocs │ Search │  │
│  └────────────────────────────────────────┘  │
│       │              │              │         │
│       ▼              ▼              ▼         │
│  ┌────────┐  ┌──────────┐  ┌─────────┐  │
│  │Catalog DB│  │K8s Cluster│  │Git Repos│  │
│  │(SQLite/PG)│  │(discovery)│  │(TechDocs)│  │
│  └────────┘  └──────────┘  └─────────┘  │
└─────────────────────────────────────────────────────────┘
```

### 生产运维：Backstage 部署与配置

```bash
# 🟢 检查 Backstage 健康状态
kubectl get pods -n backstage
kubectl logs -n backstage -l app=backstage --tail=30

# 🟢 验证 Catalog 实体注册
curl -s http://backstage.internal/api/catalog/entities | jq '.[].metadata.name'

# 🟡 触发 Catalog 重新处理
curl -X POST http://backstage.internal/api/catalog/refresh \
  -H 'Content-Type: application/json' \
  -d '{"entityRef": "component:default/my-service"}'
```

## 面试要点

1. **Backstage 的 Software Catalog 解决什么问题？**
   - 解决微服务架构下“谁拥有什么服务”的可见性问题
   - 通过 catalog-info.yaml 声明式描述服务元数据
   - 自动发现 K8s 资源、GitHub 仓库、CI/CD 状态
   - 提供统一的服务目录和所有权视图

2. **Backstage 与 K8s 的集成方式？**
   - Kubernetes 插件：通过 ServiceAccount 连接集群 API
   - 自动发现 Deployment/Service/Ingress 等资源
   - 通过注解 `backstage.io/kubernetes-id` 关联实体与 K8s 资源
   - 支持多集群、多 namespace 发现

3. **如何设计 Backstage 的 Software Templates？**
   - 使用 Scaffolder 插件提供自助服务模板
   - 模板步骤：获取输入 → 渲染模板 → 发布 Git → 注册 Catalog → 触发 CI/CD
   - 降低开发者认知负担，标准化服务创建流程

4. **Backstage 的插件架构如何工作？**
   - 前端：React 插件，通过 Extension Point 注入页面/卡片
   - 后端：Node.js 插件，提供 REST API
   - 核心插件：Catalog / Scaffolder / TechDocs / Search / Kubernetes
   - 社区插件生态丰富（200+ 插件）

## 相关 Domain

- 平台工程/01-idp/01-internal-developer-platform
- 应用模式/01-microservices/01-[[系统基础/知识字典/networking/service.md|service]]-mesh-patterns

## 相关页面

- [[概念/platform-engineering-sre.md|平台工程与 SRE 协作]] — Backstage 作为 IDP 核心
- [[概念/observability-finops.md|可观测性与 FinOps]] — Cost Insights 集成


<!-- risk-assessed -->
