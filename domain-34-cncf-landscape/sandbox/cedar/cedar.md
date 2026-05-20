---
title: Cedar
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- rbac
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Cedar 是什么
- 如何 Cedar
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Cedar
- cncf
- landscape
---

# Cedar

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://www.cedarpolicy.com/ |
| **GitHub** | https://github.com/cedar-policy/cedar |
| **许可证** | Apache-2.0 |
| **开发语言** | Rust |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Cedar 是一个由 AWS 开发的开源策略语言和评估引擎，用于定义和执行细粒度的访问控制策略。它专为应用程序的授权决策设计，提供人类可读的策略语法、形式化验证工具和高性能的策略评估引擎。

### 核心特性

- **策略语言**: 人类可读的策略定义语法，易于编写和审查
- **形式化验证**: 提供数学证明级别的策略分析工具
- **高性能引擎**: Rust 实现的策略评估引擎，亚毫秒级决策
- **ABAC/RBAC**: 支持基于属性和角色的访问控制
- **模板策略**: 策略模板实现策略复用
- **Schema 验证**: 对策略进行静态类型检查
- **多语言 SDK**: Rust, Go, Java, Python, JavaScript

---

## 快速开始

### 安装

```bash
# Rust CLI
cargo install cedar-policy-cli

# 或下载预编译二进制
```

### 策略定义

```cedar
// 允许照片所有者查看和编辑自己的照片
permit(
  principal,
  action in [Action::"viewPhoto", Action::"editPhoto"],
  resource
)
when {
  principal == resource.owner
};

// 允许管理员组执行所有操作
permit(
  principal in Group::"admins",
  action,
  resource
);

// 禁止已封禁用户访问
forbid(
  principal,
  action,
  resource
)
when {
  principal.banned == true
};

// 仅在工作时间允许访问
permit(
  principal in Group::"employees",
  action == Action::"accessSystem",
  resource
)
when {
  context.time.hour >= 9 && context.time.hour < 18
};
```

### 评估请求

```json
{
  "principal": "User::\"alice\"",
  "action": "Action::\"viewPhoto\"",
  "resource": "Photo::\"vacation.jpg\"",
  "context": {},
  "entities": [
    {
      "uid": "User::\"alice\"",
      "attrs": { "banned": false },
      "parents": [{ "type": "Group", "id": "employees" }]
    },
    {
      "uid": "Photo::\"vacation.jpg\"",
      "attrs": { "owner": "User::\"alice\"" },
      "parents": []
    }
  ]
}
```

```bash
cedar evaluate --policies policies.cedar --entities entities.json --request request.json
# 输出: ALLOW
```

### Go SDK 集成

```go
import "github.com/cedar-policy/cedar-go"

func checkAccess(principal, action, resource string) bool {
    ps, _ := cedar.NewPolicySet("policy-id", []byte(`
        permit(principal, action == Action::"view", resource)
        when { principal == resource.owner };
    `))
    
    req := cedar.Request{
        Principal: cedar.EntityUID{Type: "User", ID: principal},
        Action:    cedar.EntityUID{Type: "Action", ID: action},
        Resource:  cedar.EntityUID{Type: "Photo", ID: resource},
    }
    
    decision, _ := ps.IsAuthorized(entities, req)
    return decision == cedar.Allow
}
```

---

## 最佳实践

1. **Schema 定义**: 先定义 Entity Schema，确保策略类型安全
2. **形式化验证**: 使用 Cedar 分析工具验证策略正确性和完整性
3. **forbid 优先**: 先定义禁止策略再定义允许策略，确保安全默认
4. **策略模板**: 使用模板减少重复策略定义
5. **外部化策略**: 将策略从应用代码中分离，独立管理和部署

---

## 参考资源

- [Cedar 官方文档](https://www.cedarpolicy.com/docs/)
- [Cedar GitHub](https://github.com/cedar-policy/cedar)
- [Cedar Playground](https://www.cedarpolicy.com/en/playground)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
