---
title: Cedar (entities)
description: '## 概述'
summary: 'Cedar 是一个由 AWS 开发的开源策略语言和评估引擎，用于定义和执行细粒度的访问控制策略。它专为应用程序的授权决策设计，提供人类可读的策略语法、形式化验证工具和高性能的策略评估引擎。'
category: entities
tags:
- k8s
- cncf
- orchestration
- cedar
- argocd
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cedar 是什么
- 如何 Cedar
trigger_keywords:
- Cedar
prerequisites:
- kubectl-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Cedar

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Rust

## 概述

Cedar 是由 AWS 开发并捐赠给 CNCF 沙箱的策略语言和评估引擎，专为云原生授权和访问控制设计。它提供了一种声明式的、可分析的策略语言，让开发者能够以简洁的语法定义细粒度的访问控制策略。Cedar 的设计灵感来源于 Amazon Verified Permissions 和 IAM 策略语言，但提供了更强的表达能力和形式化验证能力。Cedar 已在 AWS Verified Permissions、Okta 等产品中获得商业应用。

## Key Features（核心能力）

- **声明式策略语言**：使用类自然语言语法定义权限策略，可读性高
- **高性能评估引擎**：Rust 实现的策略评估引擎，支持亚毫秒级决策
- **细粒度访问控制**：支持基于属性（ABAC）和基于角色（RBAC）的混合模型
- **形式化验证**：策略可进行形式化分析和自动推理验证
- **Schema 驱动**：通过 Schema 定义实体和操作，实现策略类型安全
- **可审计性**：策略评估日志支持合规审计和调试

## 架构与工作原理

Cedar 架构由三部分组成：Schema 定义实体类型（Principal, Resource, Action）和层级关系；Policy 以 permit/forbid 子句定义授权规则；Evaluator 引擎接收请求（Principal, Action, Resource, Context）并返回 Allow/Deny 决策。策略编译为高效的中间表示（IR），评估时通过图遍历检查权限传播路径。

## K8s 集成

Cedar 可集成到 Kubernetes Admission Webhook 中作为授权策略引擎，替代或增强 RBAC。通过 ValidatingWebhook 拦截 API 请求，使用 Cedar 策略进行细粒度授权决策。相比 K8s 原生 RBAC，Cedar 能表达更复杂的条件策略（如基于资源标签、时间、地理区域的访问控制）。

## 生产用例

- **细粒度 API 授权**：超越 RBAC，实现基于资源属性和环境条件的访问控制
- **多租户权限隔离**：在共享集群中为不同租户定义定制化授权策略
- **应用层授权**：微服务间调用的细粒度权限控制
- **合规审计**：策略可分析和可审计性满足合规要求

## 安装与配置

```bash
# 🟢 安装 Cedar CLI
cargo install cedar-policy-cli

# 🟢 或使用预编译二进制
curl -L https://github.com/cedar-policy/cedar/releases/latest/download/cedar-cli-linux-x86_64 -o cedar
chmod +x cedar && mv cedar /usr/local/bin/

# 🟢 验证安装
cedar --version

# 🟢 Rust SDK 集成
# Cargo.toml: cedar-policy = "4.0"
```

### 策略示例

```cedar
// Schema 定义
namespace MyApp {
  entity User in [UserGroup];
  entity UserGroup;
  entity Document in [Folder];
  entity Folder;
  
  action read appliesTo {
    principal: [User],
    resource: [Document]
  };
  action write appliesTo {
    principal: [User],
    resource: [Document]
  };
  action delete appliesTo {
    principal: [User],
    resource: [Document]
  };
}

// 策略: 允许 admin 组用户读取所有文档
permit (
  principal in MyApp::UserGroup::"admins",
  action == MyApp::Action::"read",
  resource
);

// 策略: 允许文档所有者读写自己的文档
permit (
  principal,
  action in [MyApp::Action::"read", MyApp::Action::"write"],
  resource
) when {
  resource.owner == principal
};

// 策略: 禁止删除标记为“受保护”的文档
forbid (
  principal,
  action == MyApp::Action::"delete",
  resource
) when {
  resource.protected == true
};

// 策略: 仅允许工作时间访问
permit (
  principal,
  action,
  resource
) when {
  context.hour >= 9 && context.hour <= 18
};
```

### 策略评估示例 (Go)

```go
package main

import (
    "fmt"
    "github.com/cedar-policy/cedar-go"
)

func main() {
    // 加载策略
    policy := `
    permit (
        principal == User::"alice",
        action == Action::"read",
        resource == Document::"report.pdf"
    );`
    
    // 创建评估器
    authorizer := cedar.NewAuthorizer()
    authorizer.AddPolicy(cedar.MustParsePolicy(policy))
    
    // 评估请求
    request := cedar.Request{
        Principal: cedar.Entity("User::\"alice\""),
        Action:    cedar.Entity("Action::\"read\""),
        Resource:  cedar.Entity("Document::\"report.pdf\""),
    }
    
    decision := authorizer.IsAuthorized(request)
    fmt.Printf("Decision: %s\n", decision.Decision) // Allow
}
```

## 运维操作

### 常用命令

```bash
# 🟢 验证策略语法
cedar check-parse --policies policy.cedar

# 🟢 验证 Schema
cedar check-schema --schema schema.json

# 🟢 评估策略 (测试)
cedar evaluate \
  --policies policy.cedar \
  --entities entities.json \
  --request '{"principal": "User::\"alice\"", "action": "Action::\"read\"", "resource": "Document::\"doc1\""}'

# 🟢 策略分析 (冲突检测)
cedar analyze --policies policy.cedar --schema schema.json

# 🟢 格式化策略
cedar format --policies policy.cedar
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 策略解析失败 | 语法错误 | `cedar check-parse` | 修正策略语法 |
| 评估结果错误 | 实体层级不匹配 | `cedar evaluate --verbose` | 检查实体关系和 Schema |
| 性能问题 | 策略过多/实体层级深 | 分析策略复杂度 | 优化策略结构 |
| Schema 不匹配 | 实体类型未定义 | `cedar check-schema` | 更新 Schema 定义 |

### 排查流程

```
1. cedar check-parse → 验证策略语法
2. cedar check-schema → 验证 Schema 一致性
3. cedar evaluate --verbose → 详细评估日志
4. cedar analyze → 策略冲突和冗余分析
5. 检查实体层级关系是否正确
```

## 生产案例

### 案例1: 多租户 API 授权
- **场景**: SaaS 平台需要细粒度的租户级 API 授权
- **方案**: Cedar 定义租户隔离策略，集成到 API Gateway
- **效果**: 替代硬编码权限检查，策略变更无需重新部署

### 案例2: K8s Admission 增强
- **场景**: 原生 RBAC 无法满足基于标签的条件授权
- **方案**: Cedar 集成到 Admission Webhook，基于资源标签授权
- **效果**: 实现“仅允许删除非生产环境资源”等复杂策略

## 对比替代方案

| 维度 | Cedar | OPA/Rego | K8s RBAC | AWS IAM |
|------|-------|----------|----------|--------|
| 语言类型 | 声明式 | 逻辑式 | YAML | JSON |
| 类型安全 | Schema 驱动 | 无 | 无 | 无 |
| 形式化验证 | 支持 | 不支持 | 不支持 | 不支持 |
| 性能 | 亚毫秒 | 毫秒级 | 快 | 快 |
| 学习曲线 | 中 | 陡峭 | 低 | 中 |
| 开源 | 是 | 是 | 是 | 否 |

## 检查清单

- [ ] Schema 定义了完整的实体类型和层级
- [ ] 策略经过 cedar analyze 分析无冲突
- [ ] 评估性能满足要求 (亚毫秒级)
- [ ] 策略变更有版本控制和审计
- [ ] 测试覆盖了 permit 和 forbid 场景
- [ ] 与现有 RBAC 策略无冲突

## Related

- [[clusternet]] — Clusternet
- [[kubeslice]] — KubeSlice
- [[hyperlight]] — Hyperlight
- [[kubescape]] — Kubescape
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cedar
- [[实体/cncf-security.md|[[CNCF 安全与合规项目全景|CNCF 安全与合规项目全景]]]] — Cross-reference


<!-- risk-assessed -->
