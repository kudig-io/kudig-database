---
title: Cluster Governance
description: 集群治理知识域 — Namespace 策略、标签规范、准入策略、RBAC 治理、资源配额管理
category: subdomain
tags:
- governance
- namespace
- rbac
- resource-quota
- admission-policy
tier: core
created: '2026-07-02'
last_updated: '2026-07-21'
---
# 集群治理 Cluster Governance

> 多团队共享集群的治理体系，确保资源公平、安全合规、成本可控。

## 治理维度

| 维度 | 机制 | 工具 |
|------|------|------|
| 命名空间 | Namespace 分层策略 | HNC/Kyverno |
| 标签规范 | 强制标签策略 | OPA/Kyverno |
| 准入控制 | 策略引擎 | Gatekeeper/Kyverno |
| 权限 | RBAC 最小权限 | rbac-lookup/kubectl-who-can |
| 资源 | Quota/LimitRange | K8s 原生 |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[13-生产运维/02-集群治理/01-namespace-strategy-lifecycle.md\|Namespace 策略]] | 命名空间分层与生命周期 | intermediate |
| [[13-生产运维/02-集群治理/02-label-convention-governance.md\|标签规范]] | 强制标签与注解规范 | intermediate |
| [[13-生产运维/02-集群治理/03-admission-policy-governance.md\|准入策略]] | Admission Webhook 治理 | advanced |
| [[13-生产运维/02-集群治理/04-rbac-governance-model.md\|RBAC 治理]] | 权限模型与审计 | advanced |
| [[13-生产运维/02-集群治理/14-resource-quota-management.md\|资源配额]] | Quota/LimitRange 管理 | intermediate |

## Related

- [[08-安全/04-策略治理/index.md|策略治理]]
- [[13-生产运维/01-成本治理/index.md|成本治理]]
- [[10-平台工程/index.md|平台工程]]
