---
title: Backstage 与平台目录的整合
description: '- 日志和事件聚合'
category: synthesis
tags:
- backstage
- platform-engineering
- developer-experience
- service-catalog
last_updated: 2026-05
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
created: "2026-05-23"
relationships:
  - target: "[[domain-17-system-foundation/topic-dictionary/networking/service]]"
    type: uses
  - target: "[[entities/backstage]]"
    type: related_to
  - target: "[[domain-17-system-foundation/topic-cheat-sheet/k8s]]"
    type: related_to
---

# [[entities/backstage|Backstage]] 与平台目录的整合

## Backstage 核心插件

```
必备插件:
├── Kubernetes: 集群资源可视化
├── TechDocs: 技术文档中心
├── Catalog: 服务目录与依赖图
├── Scaffolder: 自服务模板
├── Cost Insights: 成本可视化
└── PagerDuty/OpsGenie: On-call 信息
```

## 与 [[domain-17-system-foundation/topic-cheat-sheet/k8s|K8s]] 集成

```
Backstage 读取 K8s:
  - 服务 → Deployment 关联
  - Pod 状态实时显示
  - 日志和事件聚合
```

## 服务目录价值

```
统一视图:
  - 谁拥有这个服务？
  - 服务的依赖关系？
  - 当前的 SLO 状态？
  - 最近的发布历史？
```

## 相关 Domain

- domain-07-platform-engineering/01-idp/01-internal-developer-platform
- domain-20-application-patterns/01-microservices/01-[[domain-17-system-foundation/topic-dictionary/networking/service|service]]-mesh-patterns
