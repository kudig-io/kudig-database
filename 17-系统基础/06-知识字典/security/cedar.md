---
title: Cedar 策略语言
description: Cedar 是 AWS 开源的策略语言，用于定义和执行细粒度授权策略，语法简洁直观，专为应用级权限管理设计，已被 Amazon Verified
  Permiss...
summary: Cedar 是 AWS 开源的策略语言，用于定义和执行细粒度授权策略，语法简洁直观，专为应用级权限管理设计，已被 Amazon Verified
  Permiss...
category: dictionary
tags:
- k8s
- glossary
- security
- policy
- authorization
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cedar 策略语言 是什么
- Cedar 详解
trigger_keywords:
- Cedar 策略语言
- Cedar
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Cedar 策略语言（Cedar）

## 概述

Cedar 是 AWS 开源的策略语言，用于定义和执行细粒度授权策略，语法简洁直观，专为应用级权限管理设计，已被 Amazon Verified Permissions 采用。

## 核心概念/原理

- **策略语言**：专为授权决策设计的声明式语言
- **AWS 背景**：Amazon Verified Permissions 的核心引擎
- **形式化验证**：支持策略的形式化验证
- **应用集成**：嵌入到应用中的授权引擎

## 关键机制或特性

- Entity（用户/资源/动作的定义）
- Policy（when/unless 条件的策略规则）
- 层次化资源模型
- 策略组（Policy Set）管理
- 策略评估（is-authorized API）
- 形式化验证工具
- SDK（Rust/Java/Go）

## 使用场景与最佳实践

- 应用的细粒度授权策略
- 多租户 SaaS 的权限管理
- AWS 资源的 IAM 策略
- 替代 OPA 的轻量策略方案
- 需要形式化验证的安全策略

## 架构深度解析

### Cedar 策略语言与评估模型

```
┌──────────────────────────────────────────────────────────────┐
│  策略编写                                                      │
│  ├─ 实体（Entity）：用户/资源/动作（层级化，如 org→app→doc）  │
│  ├─ 策略（Policy）：permit/forbid + when/unless 条件           │
│  └─ 策略集（Policy Set）：多条策略的集合管理                   │
│   │                                                           │
│   ▼  is_authorized(principal, action, resource, context)      │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ Cedar 评估引擎                                            │  │
│  │ ├─ 精确匹配 → 首次匹配策略（无冲突裁决）                 │  │
│  │ ├─ 作用域与层级继承（ancestors 隐式授权）                │  │
│  │ ├─ 上下文（context）：请求环境属性                        │  │
│  │ └─ 决策：Allow / Deny（forbid 优先）                     │  │
│  └──────────────────────────┬──────────────────────────────┘  │
│                             │ 结果                          │
│                             ▼                                │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ 消费方                                                    │  │
│  │ ├─ 应用内嵌 SDK（Rust/Java/Go/Python）                   │  │
│  │ ├─ AWS 服务（Verified Permissions）                      │  │
│  │ └─ 形式化验证工具（Dafny 证明）                          │  │
│  └─────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（cedar-policy/cedar）

| 模块 | 路径 | 关键职责 |
|---|---|---|
| 语言解析 | cedar-policy/ | 语法解析与 AST |
| 评估引擎 | cedar-policy/src/ | 策略评估与决策 |
| 实体模型 | cedar-policy/entities/ | 实体图与层级处理 |
| 验证工具 | cedar-policy-validator/ | 策略验证与类型检查 |
| SDK | cedar-policy-binding-*/ | Rust/Java/Go/Python 绑定 |

### 流程步骤

1. 用 Cedar 语法编写策略（permit/forbid + 条件），如 `permit(principal, action == Action::"read", resource) when { resource.owner == principal }`。
2. 构造实体与上下文：用户、资源、层级关系、请求环境属性。
3. 调用 `is_authorized` 评估：引擎按策略集匹配请求三元组（principal/action/resource）。
4. 决策规则：任一 forbid 匹配即 Deny；有 permit 匹配则 Allow；否则默认 Deny。
5. 策略经形式化验证（类型检查/证明）后发布，应用 SDK 内嵌或调用授权服务。

## 生产案例

### 案例 1：层级授权误配导致越权访问（2024 年 SaaS 权限事故）

| 时间 | 事件 |
|---|---|
| T+0 | 平台新增"组织-应用-文档"层级实体模型 |
| T+1h | 测试发现普通成员可访问其他组织文档 |
| T+2h | 定位为策略中 `resource in app` 未校验组织边界，层级继承放大了权限 |
| T+4h | 策略增加组织边界条件（`resource.organization == principal.organization`），回归通过 |

- **根因**：层级模型 + 隐式继承未加边界校验；策略验证覆盖不足。
- **修复命令**（策略修复 + 验证）：
```bash
# 🟢 用验证工具检查策略类型与作用域
cedar-policy-validator validate --schema schema.cedarschema --policies policy.cedar
# 🔴 增加组织边界条件后重新评估
# permit(principal, action, resource) when { resource.organization == principal.organization }
```

### 案例 2：策略集混乱导致意外 Deny

- **现象**：策略更新后大量合法请求被拒（误伤线上）。
- **诊断**：新旧策略叠加，forbid 规则作用域过宽；无策略版本管理与灰度。
- **修复**：策略集版本化（语义化版本）+ 影子评估（新旧策略并行对比决策差异）；变更走审批与灰度，差异超阈值自动回滚。

## 对比评测

| 维度 | Cedar | OPA/Rego | OpenFGA |
|---|---|---|---|
| 语言风格 | 声明式权限策略 | 通用策略（数据查询） | 关系元组+DSL |
| 授权模型 | 实体+动作+条件 | 任意（通用） | 关系图 |
| 决策语义 | 首次匹配+forbid 优先 | 规则求值 | 图遍历 |
| 验证能力 | 形式化验证（Dafny） | 测试用例 | 模型校验 |
| 生态 | AWS 支持 | 大（CNCF） | CNCF 项目 |

- **选型建议**：AWS 生态/需形式化验证选 Cedar；通用策略决策选 OPA；大规模关系授权选 OpenFGA。

## 故障排查速查

| 症状 | 可能原因 | 排查命令 |
|---|---|---|
| 意外 Deny | 策略条件/层级错误 | 本地 `cedar` CLI 复现评估 |
| 越权 Allow | 层级继承未加边界 | 审查实体层级与 in 表达式 |
| 语法错误 | 类型/作用域不符 | `cedar-policy-validator validate` |
| 决策不一致 | 策略版本混乱 | 核对策略集版本与发布记录 |
| 性能下降 | 策略过多/实体图大 | 精简策略、缓存评估结果 |

## 生产部署清单

- [ ] 策略纳入 GitOps + 语义化版本管理，变更走审批
- [ ] 影子评估（新旧策略对比）纳入发布流程，差异超阈值回滚
- [ ] 实体模型 schema 化（.cedarschema），CI 执行类型检查与验证
- [ ] 授权服务/内嵌 SDK 版本锁定，升级先测试集群
- [ ] 监控决策 QPS、Deny 率、评估延迟与策略版本漂移

## 升级决策点

| 级别 | 条件 | 动作 |
|---|---|---|
| P0 | 策略错误导致大面积误拒/越权 | 立即回滚策略版本，评估影响面后修复 |
| P1 | 实体模型/层级变更 | 影子评估验证差异，灰度业务接入后切换 |
| P2 | Cedar SDK/引擎升级 | 测试环境验证评估语义兼容性后滚动 |

## 面试要点

> 以下 Q&A 覆盖 Cedar 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Cedar 与 Rego（OPA）在设计哲学上有何不同？**
   A：Cedar 专为权限决策设计：语法受限（permit/forbid + when/unless），决策语义确定（首次匹配、forbid 优先、默认 Deny），无副作用，天然可做形式化验证（Dafny 证明）；Rego 是通用查询语言，表达力强但语义灵活，需要测试保障正确性。Cedar 的取舍是"可证明的正确性优先"，Rego 是"表达力优先"。

2. **Q：Cedar 如何处理层级化资源授权？**
   A：实体可以组织成层级（如组织→应用→文档），策略中可用 `resource in <实体>` 判断资源是否属于某层级节点，子资源自动继承父节点的授权关系。但层级继承会放大权限，必须显式添加边界条件（如组织一致性校验），并通过验证工具与影子评估防止越权。

3. **Q：Cedar 生产落地的关键实践？**
   A：① 实体模型 schema 化 + CI 类型检查/形式化验证；② 策略版本化 + 影子评估灰度（对比新旧决策差异）；③ 决策服务高可用与缓存（评估结果 TTL）；④ 全量审计日志（决策输入/输出/策略版本）；⑤ 监控 Deny 率与误拒告警，异常一键回滚策略版本。

## 运维要点

- 策略治理：GitOps + 版本化 + 审批，影子评估默认流程。
- 实体管理：schema 变更走迁移流程，与策略版本联动发布。
- 性能：评估结果缓存（TTL），实体图大小监控，超阈值拆分。
- 审计：决策记录（主体/动作/资源/结果/策略版本）归档对接 SIEM。
- 告警：Deny 率突增/突降、评估延迟、策略版本漂移。

## 参考链接

- https://www.cedarpolicy.com/
- https://github.com/cedar-policy/cedar

## Related

- [[17-系统基础/06-知识字典/security/opa.md|OPA]]
- [[17-系统基础/06-知识字典/security/openfga.md|OpenFGA]]
- [[17-系统基础/06-知识字典/security/kyverno.md|Kyverno]]


<!-- risk-assessed -->
