---
title: Open Policy Agent
description: Open Policy Agent（OPA）是 CNCF 毕业项目，提供通用的策略引擎，可在 Kubernetes 准入控制、API 网关、SSH、Terraf...
summary: Open Policy Agent（OPA）是 CNCF 毕业项目，提供通用的策略引擎，可在 Kubernetes 准入控制、API 网关、SSH、Terraf...
category: dictionary
tags:
- k8s
- glossary
- opa
- policy
- security
- cncf
tier: core
created: 2026-05
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Open Policy Agent 是什么
- OPA (Open Policy Agent) 详解
trigger_keywords:
- Open Policy Agent
- OPA (Open Policy Agent)
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Open Policy Agent

> **英文名**: OPA (Open Policy Agent)

## 概述

Open Policy Agent（OPA）是 CNCF 毕业项目，提供通用的策略引擎，可在 Kubernetes 准入控制、API 网关、SSH、Terraform 等场景中执行统一的策略决策。

## 核心概念/原理

### 核心概念

- **Rego**：OPA 的策略编写语言，声明式、逻辑编程风格。
- **Policy**：定义允许/拒绝条件的规则集合。
- **Input**：请求上下文（JSON 格式）。
- **Decision**：OPA 返回的 allow/deny 结果。

```rego
package kubernetes.admission

deny[msg] {
  input.request.kind.kind == "Pod"
  not input.request.object.spec.containers[_].securityContext.runAsNonRoot
  msg := "Pod must set runAsNonRoot=true"
}
```

## 关键机制或特性

- **Gatekeeper**：OPA 的 Kubernetes 原生实现，通过 CRD 管理策略。
- **ConstraintTemplate**：参数化的策略模板。
- **Audit**：定期审计已有资源是否违反策略。
- **Mutation**：自动修正不符合策略的资源。
- **外部数据**：引用 ConfigMap 等外部数据辅助决策。

## 使用场景与最佳实践

- 使用 OPA Gatekeeper 替代 PSP 实现 Pod 安全策略。
- 定义约束：禁止 latest 标签镜像、要求 resource limits 等。
- 使用 ConstraintTemplate 构建团队可复用的策略库。
- 配合 CI/CD 在部署前进行策略检查（dry-run）。
- 启用 Audit 功能定期扫描集群中的违规资源。

## 参考链接

- [OPA Official](https://www.openpolicyagent.org/)

## Related

- [[domain-17-system-foundation/topic-dictionary/security/kyverno.md|Kyverno]]
- [[domain-17-system-foundation/topic-dictionary/security/admission-controller.md|Admission Controller]]
- [[domain-17-system-foundation/topic-dictionary/security/pod-security-policy.md|Pod Security Policy]]
- [[domain-17-system-foundation/topic-dictionary/security/rbac.md|RBAC]]
- [[domain-17-system-foundation/topic-dictionary/security/webhook.md|Webhook]]


<!-- risk-assessed -->
