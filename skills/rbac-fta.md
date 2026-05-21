---
title: RBAC 异常故障树分析
description: ROLE_OR --> ROLE3[Role 配置错误]
category: skills
tags:
- k8s
- fta
- troubleshooting
- apiserver
- helm
- rbac
- webhook
- job
- cronjob
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- RBAC 异常故障树分析 是什么
- 如何 RBAC 异常故障树分析
trigger_keywords:
- RBAC
- 异常故障树分析
prerequisites:
- kubectl-basics
- helm-basics
fta_id: FTA-RBAC-001
component: Rbac
severity: medium
---

# RBAC 异常故障树分析

<!-- condition: kubectl get events -A | grep -E 'Forbidden|Denied|Unauthorized' 显示 RBAC 相关拒绝事件 -->

# RBAC 异常 FTA 树

## 适用范围与说明
- **目标**：覆盖权限拒绝、权限过宽与策略漂移的关键成因与路径。
- **范围**：Role/ClusterRole、Binding、ServiceAccount、鉴权链路、审计与回滚。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: RBAC 权限异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> ROLE[Role/ClusterRole 异常]
  OR0 --> BIND[Binding 异常]
  OR0 --> SA[ServiceAccount 异常]
  OR0 --> AUTH[鉴权链路异常]
  OR0 --> AUDIT[审计/回滚缺失]

  %% Role/ClusterRole 异常分支 - 扩展到3-4层
  ROLE_OR{{OR}}
  ROLE --> ROLE_OR
  ROLE_OR --> ROLE1[权限不足]
  ROLE_OR --> ROLE2[权限过宽]
  ROLE_OR --> ROLE3[Role 配置错误]

  ROLE1_OR{{OR}}
  ROLE1 --> ROLE1_OR
  ROLE1_OR --> ROLE1A[缺少必要 verbs]
  ROLE1_OR --> ROLE1B[缺少必要 resources]
  ROLE1_OR --> ROLE1C[apiGroups 配置错误]

  ROLE2_OR{{OR}}
  ROLE2 --> ROLE2_OR
  ROLE2_OR --> ROLE2A[使用 * 通配符]
  ROLE2_OR --> ROLE2B[cluster-admin 滥用]
  ROLE2_OR --> ROLE2C[危险权限未限制]

  ROLE3_OR{{OR}}
  ROLE3 --> ROLE3_OR
  ROLE3_OR --> ROLE3A[语法错误]
  ROLE3_OR --> ROLE3B[resourceNames 配置错误]

  %% Binding 异常分支 - 扩展到3-4层 + AND 门
  BIND_OR{{OR}}
  BIND --> BIND_OR
  BIND_OR --> BIND1[Binding 缺失]
  BIND_OR --> BIND2[Binding 配置错误]
  BIND_OR --> BIND3[跨命名空间问题]

  BIND1_AND{{AND}}
  BIND1 --> BIND1_AND
  BIND1_AND --> BIND1A[Role 存在]
  BIND1_AND --> BIND1B[RoleBinding 未创建]

  BIND2_OR{{OR}}
  BIND2 --> BIND2_OR
  BIND2_OR --> BIND2A[subjects 配置错误]
  BIND2_OR --> BIND2B[roleRef 引用错误]

  BIND3_OR{{OR}}
  BIND3 --> BIND3_OR
  BIND3_OR --> BIND3A[RoleBinding 引用 ClusterRole 失败]
  BIND3_OR --> BIND3B[ClusterRoleBinding 跨 NS 问题]

  %% ServiceAccount 异常分支 - 扩展到3-4层
  SA_OR{{OR}}
  SA --> SA_OR
  SA_OR --> SA1[SA Token 问题]
  SA_OR --> SA2[SA 不存在]
  SA_OR --> SA3[SA 配置问题]

  SA1_OR{{OR}}
  SA1 --> SA1_OR
  SA1_OR --> SA1A[Token 过期]
  SA1_OR --> SA1B[Token 未挂载]
  SA1_OR --> SA1C[Token 签名无效]

  SA2_OR{{OR}}
  SA2 --> SA2_OR
  SA2_OR --> SA2A[SA 被删除]
  SA2_OR --> SA2B[SA 未创建]

  SA3_OR{{OR}}
  SA3 --> SA3_OR
  SA3_OR --> SA3A[automountServiceAccountToken 禁用]
  SA3_OR --> SA3B[imagePullSecrets 缺失]

  %% 鉴权链路异常分支 - 扩展到3-4层 + AND 门
  AUTH_OR{{OR}}
  AUTH --> AUTH_OR
  AUTH_OR --> AUTH1[API Server 鉴权问题]
  AUTH_OR --> AUTH2[Webhook 鉴权问题]
  AUTH_OR --> AUTH3[聚合鉴权问题]

  AUTH1_OR{{OR}}
  AUTH1 --> AUTH1_OR
  AUTH1_OR --> AUTH1A[RBAC 模式未启用]
  AUTH1_OR --> AUTH1B[鉴权顺序问题]

  AUTH2_AND{{AND}}
  AUTH2 --> AUTH2_AND
  AUTH2_AND --> AUTH2A[Webhook 不可用]
  AUTH2_AND --> AUTH2B[failurePolicy 为 Deny]

  AUTH3_OR{{OR}}
  AUTH3 --> AUTH3_OR
  AUTH3_OR --> AUTH3A[聚合 API 权限问题]
  AUTH3_OR --> AUTH3B[extension-apiserver-authentication 问题]

  %% 审计/回滚缺失分支 - 扩展到3-4层
  AUDIT_OR{{OR}}
  AUDIT --> AUDIT_OR
  AUDIT_OR --> AUD1[审计问题]
  AUDIT_OR --> AUD2[回滚问题]
  AUDIT_OR --> AUD3[权限漂移]

  AUD1_OR{{OR}}
  AUD1 --> AUD1_OR
  AUD1_OR --> AUD1A[审计未启用]
  AUD1_OR --> AUD1B[审计策略不完整]

  AUD2_OR{{OR}}
  AUD2 --> AUD2_OR
  AUD2_OR --> AUD2A[无历史版本]
  AUD2_OR --> AUD2B[回滚操作失败]

  AUD3_OR{{OR}}
  AUD3 --> AUD3_OR
  AUD3_OR --> AUD3A

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[skills/ts-security-auth.md|安全认证排查]]

## Related

- [[helm-fta]] — Helm 发布异常故障树分析
- [[skills/skill-reference-diagnostic-workflow.md|skill-reference-diagnostic-workflow]] — Diagnostic Workflow
- [[skills/ts-command-output.md|ts-command-output]] — 命令输出根因解析
- [[skills/ts-resources-scheduling.md|ts-resources-scheduling]] — 资源调度故障排查
- [[skills/Kubernetes Diagnostic Skills Overview.md|Kubernetes Diagnostic Skills Overview]] — Kubernetes Diagnostic Skills Overview

- [[domain-10-troubleshooting-diagnostics/topic-fta/list/rbac-fta.md|RBAC 异常故障树分析]]
- [[skills/skill-23-job-cronjob-failure|Job/CronJob 故障诊断与修复 / Job & CronJob Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
