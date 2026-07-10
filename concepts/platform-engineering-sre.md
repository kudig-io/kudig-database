---
title: 平台工程与 SRE 的协作模式
description: '| **工具链** | 选择、集成、维护 | 监控、告警、On-call |'
summary: '| **工具链** | 选择、集成、维护 | 监控、告警、On-call |'
category: synthesis
tags:
- platform-engineering
- sre
- devops
- internal-developer-platform
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 平台工程与 SRE 的协作模式 是什么
- 如何 平台工程与 SRE 的协作模式
trigger_keywords:
- 平台工程与
- SRE
- 的协作模式
prerequisites:
- kubectl-basics
relationships:
- target: '[[系统基础/速查卡/k8s.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 平台工程与 SRE 的协作模式

## 职责边界

| 职责 | 平台工程 | SRE |
|------|---------|-----|
| **基础设施** | 提供标准化的 [[系统基础/速查卡/k8s.md|K8s]] 平台 | 确保平台可靠性 |
| **开发者体验** | 构建 IDP、模板、文档 | 定义 SLO、发布规范 |
| **工具链** | 选择、集成、维护 | 监控、告警、On-call |
| **安全** | 平台级安全基线 | 运行时安全监控 |
| **成本** | 资源配额、计费 | 利用率优化 |

## 协作接口

```
平台工程提供:
  - 标准化的 Namespace/Cluster 模板
  - 预配置的监控和告警
  - 自助式部署流水线

SRE 定义:
  - 新服务的 SLO 要求
  - 发布检查清单
  - On-call 轮换机制
```

## 共同目标

```
开发者体验        平台可靠性
     ↘              ↙
      内部开发者平台 (IDP)
```

## 相关 Domain

- 平台工程/01-idp/01-internal-developer-platform
- [[可靠性/SRE实践/04-toil-reduction-automation.md|04 toil reduction automation]]
## Related

- [[系统基础/知识字典/security/runtime-security.md|运行时安全]]


<!-- risk-assessed -->
