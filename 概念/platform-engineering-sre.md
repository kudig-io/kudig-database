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
last_updated: 2026-07
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

## 概述

平台工程与 SRE（Site Reliability Engineering）的协作是云原生组织成熟度的重要标志。平台工程专注于构建内部开发者平台（IDP），降低开发者的认知负荷和运维门槛；SRE 专注于保障系统的可靠性、性能和可观测性。两者在工具链、流程和目标上有大量交叉，明确职责边界和协作接口是实现高效组织运转的关键。

## 职责边界

| 职责 | 平台工程 | SRE |
|------|---------|-----|
| **基础设施** | 提供标准化的 [[系统基础/速查卡/k8s.md|K8s]] 平台、集群模板、节点池配置 | 确保平台可靠性，定义 SLA/SLO，容量规划 |
| **开发者体验** | 构建 IDP、应用模板、文档门户 | 定义发布规范、错误预算策略、On-call 标准 |
| **工具链** | 选择、集成、维护 CI/CD、GitOps、监控栈 | 配置告警规则、On-call 轮值、事后复盘 |
| **安全** | 平台级安全基线（RBAC、NetworkPolicy 默认值） | 运行时安全监控、漏洞响应、合规审计 |
| **成本** | 资源配额、计费模型、存储治理 | 利用率优化、容量预测、资源碎片整理 |
| **可靠性** | 平台 HA 架构、多集群管理 | SLO 定义、错误预算、故障注入验证 |

## 协作接口

### 平台工程交付物

```
平台工程提供:
  - 标准化的 Namespace/Cluster 模板
    → 预配置 ResourceQuota、LimitRange、NetworkPolicy
    → 预安装监控 Agent、日志采集器
  - 预配置的监控和告警
    → 黄金信号看板（USE/RED 方法）
    → 默认告警规则（PodCrashLoopBackOff、NodeNotReady）
  - 自助式部署流水线
    → GitOps 模板（ArgoCD Application 模板）
    → CI/CD 流水线（Tekton / GitHub Actions）
  - 开发者门户（Backstage）
    → 服务目录、文档中心、Scaffolder
  - 安全基线
    → 默认 Pod Security Standards（restricted）
    → 镜像扫描（Trivy）准入策略
```

### SRE 交付物

```
SRE 定义:
  - 新服务的 SLO 要求
    → 可用性目标（99.9% / 99.95% / 99.99%）
    → 延迟目标（P99 < 200ms）
    → 吞吐量目标
  - 发布检查清单
    → 上线前 SLO 达标验证
    → 容量规划确认
    → DR 演练通过
  - On-call 轮换机制
    → 7x24 响应体系
    → 事后复盘（Postmortem）流程
    → 错误预算消耗追踪
  - 可靠性工程
    → 混沌工程实验设计
    → 容量规划和预测
    → 事故分析和改进跟踪
```

## 共同目标

```
开发者体验        平台可靠性
     ↘              ↙
      内部开发者平台 (IDP)
           ↓
   "开发者可以自助式地、可靠地
    部署和管理他们的服务"

度量指标:
  - 部署频率（平台工程关注）
  - 变更失败率（SRE 关注）
  - 平均恢复时间（SRE 关注）
  - 开发者满意度（双方关注）
```

## 最佳实践

- **建立清晰的上游/下游关系**：平台工程是 SRE 的上游——平台不稳定，SRE 做再多也无法保障可靠性。平台工程应将 SLO 作为平台的核心交付指标
- **共用 Backstage 作为协作平台**：平台工程在 Backstage 中维护服务目录和文档，SRE 在 Backstage 中展示 SLO 状态和 On-call 信息——统一信息来源
- **平台工程嵌入 SRE 值班**：让平台工程师参与 SRE On-call 轮值，亲身体验平台问题对业务的影响——这比任何文档都有效
- **定期联合复盘**：重大事故复盘应包含平台工程和 SRE 双方视角——根因可能在平台设计而非运维操作
- **自动化减少 Toil**：SRE 识别的重复性运维工作（Toil）应反馈给平台工程，通过平台能力自动化消除

## 常见陷阱

- **职责模糊导致推诿**：当监控告警出现时，"这是平台问题还是应用问题"的争论浪费时间——应在服务上线时就定义清晰的告警 owner
- **平台过度抽象增加复杂度**：平台工程追求"对开发者透明"，但过度抽象会导致问题排查困难——需要在抽象和透明之间找到平衡
- **SRE 变成高级运维**：如果 SRE 花大量时间处理工单而非工程改进，说明组织没有真正实施 SRE——SRE 应该有至少 50% 的时间用于工程改进

## 相关 Domain

- 平台工程/01-idp/01-internal-developer-platform
- [[可靠性/SRE实践/04-toil-reduction-automation.md|04 toil reduction automation]]

## 相关页面

- [[概念/backstage-platform-catalog.md|Backstage 平台目录]] — IDP 核心组件
- [[概念/slo-monitoring-integration.md|SLO 与监控集成]] — SLO 工程实践
- [[概念/observability-finops.md|可观测性与 FinOps]] — 成本治理协作

## Related

- [[系统基础/知识字典/security/runtime-security.md|运行时安全]]


<!-- risk-assessed -->
