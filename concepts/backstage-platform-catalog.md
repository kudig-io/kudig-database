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
relationships:
- target: '[[系统基础/知识字典/networking/service.md]]'
  type: uses
- target: '[[entities/backstage.md]]'
  type: related_to
- target: '[[系统基础/速查卡/k8s.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[entities/backstage.md|Backstage]] 与平台目录的整合

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

## 与 [[系统基础/速查卡/k8s.md|K8s]] 集成

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

- 平台工程/01-idp/01-internal-developer-platform
- 应用模式/01-microservices/01-[[系统基础/知识字典/networking/service.md|service]]-mesh-patterns


<!-- risk-assessed -->
