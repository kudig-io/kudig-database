---
title: PSP/SCC 异常故障树分析 (skills)
description: '<!-- condition: kubectl get events -A | grep -E ''Forbidden|violates
  PodSecurity'' 显示安全策略拒绝 -->'
summary: '<!-- condition: kubectl get events -A | grep -E ''Forbidden|violates PodSecurity''
  显示安全策略拒绝 -->'
category: skills
tags:
- k8s
- fta
- troubleshooting
- opa
- pdb
- webhook
- gpu
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- PSP/SCC 异常故障树分析 是什么
- 如何 PSP/SCC 异常故障树分析
trigger_keywords:
- PSP
- SCC
- 异常故障树分析
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
- policy-basics
fta_id: FTA-PSP_SCC-001
component: Psp Scc
severity: medium
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# PSP/SCC 异常故障树分析

<!-- condition: kubectl get events -A | grep -E 'Forbidden|violates PodSecurity' 显示安全策略拒绝 -->

# PSP/SCC 异常 FTA 树

## 适用范围与说明
- **目标**：覆盖 Pod Security 策略阻断、误放行与迁移冲突的关键成因与路径。
- **范围**：PSP/SCC/PSA 策略、准入链路、策略审计、回滚与合规。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: Pod Security 策略异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> POL[策略配置异常]
  OR0 --> MIG[迁移与兼容异常]
  OR0 --> AUTH[准入链路异常]
  OR0 --> BIND[绑定与授权异常]
  OR0 --> AUDIT[审计与回滚缺失]

  %% ========== 1. 策略配置异常 ==========
  POL_OR{{OR}}
  POL --> POL_OR
  POL_OR --> POL_STRICT[策略过严]
  POL_OR --> POL_LOOSE[策略过宽]
  POL_OR --> POL_CONFLICT[策略冲突]

  %% 1.1 策略过严
  POL_STRICT_OR{{OR}}
  POL_STRICT --> POL_STRICT_OR
  POL_STRICT_OR --> POL_STRICT1[runAsNonRoot 阻断需 root 的应用]
  POL_STRICT_OR --> POL_STRICT2[禁止 privileged 容器]
  POL_STRICT_OR --> POL_STRICT3[禁止 hostPath/hostNetwork]
  POL_STRICT_OR --> POL_STRICT4[capabilities 限制过严]

  %% 1.2 策略过宽
  POL_LOOSE_OR{{OR}}
  POL_LOOSE --> POL_LOOSE_OR
  POL_LOOSE_OR --> POL_LOOSE1[允许 privileged 容器]
  POL_LOOSE_OR --> POL_LOOSE2[允许 hostPID/hostIPC]
  POL_LOOSE_OR --> POL_LOOSE3[未限制 capabilities]
  POL_LOOSE_OR --> POL_LOOSE4[allowPrivilegeEscalation: true]

  %% 1.3 策略冲突
  POL_CONFLICT_OR{{OR}}
  POL_CONFLICT --> POL_CONFLICT_OR
  POL_CONFLICT_OR --> POL_CONFLICT1[多 PSP 优先级冲突]
  POL_CONFLICT_OR --> POL_CONFLICT2[PSA 与 OPA/Gatekeeper 冲突]
  POL_CONFLICT_OR --> POL_CONFLICT3[命名空间标签与策略不一致]

  %% ========== 2. 迁移与兼容异常 ==========
  MIG_OR{{OR}}
  MIG --> MIG_OR
  MIG_OR --> MIG_PSA[PSP → PSA 迁移]
  MIG_OR --> MIG_OPA[OPA/Gatekeeper 迁移]
  MIG_OR --> MIG_SCC[OpenShift SCC 迁移]

  %% 2.1 PSP → PSA 迁移
  MIG_PSA_OR{{OR}}
  MIG_PSA --> MIG_PSA_OR
  MIG_PSA_OR --> MIG_PSA1[PSA 标签未配置]
  MIG_PSA_OR --> MIG_PSA2[PSA 级别选择不当]
  MIG_PSA_OR --> MIG_PSA3[迁移期间双重校验]

  %% AND 门：PSP 已移除 + PSA 未配置
  AND_PSA{{"AND: PSP 移除 + PSA 未配置"}}
  MIG_PSA --> AND_PSA
  AND_PSA --> AND_PSA1[K8s >= 1.25 已移除 PSP]
  AND_PSA --> AND_PSA2[命名空间未配置 PSA 标签]

  %% 2.2 OPA/Gatekeeper 迁移
  MIG_OPA_OR{{OR}}
  MIG_OPA --> MIG_OPA_OR
  MIG_OPA_OR --> MIG_OPA1[ConstraintTemplate 缺失]
  MIG_OPA_OR --> MIG_OPA2[Constraint 配置错误]
  MIG_OPA_OR --> MIG_OPA3[Gatekeeper 与 PSA 重复校验]

  %% 2.3 OpenShift SCC 迁移
  MIG_SCC_OR{{OR}}
  MIG_SCC --> MIG_SCC_OR
  MIG_SCC_OR --> MIG_SCC1[SCC 优先级配置错误]
  MIG_SCC_OR --> MIG_SCC2[ServiceAccount 绑定 SCC 错误]
  MIG_SCC_OR --> MIG_SCC3[SCC 与 PSA 冲突]

  %% ========== 3. 准入链路异常 ==========
  AUTH_OR{{OR}}
  AUTH --> AUTH_OR
  AUTH_OR --> AUTH_WEBHOOK[Webhook 准入异常]
  AUTH_OR --> AUTH_API[API Server 异常]
  AUTH_OR --> AUTH_ORDER[准入顺序异常]

  %% 3.1 Webhook 准入异常
  AUTH_WEBHOOK_OR{{OR}}
  AUTH_WEBHOOK --> AUTH_WEBHOOK_OR
  AUTH_WEBHOOK_OR --> AUTH_WEBHOOK1[Gatekeeper Webhook 超时]
  AUTH_WEBHOOK_OR --> AUTH_WEBHOOK2[OPA Webhook 不可用]
  AUTH_WEBHOOK_OR --> AUTH_WEBHOOK3[Kyverno Webhook 异常]

  %% 3.2 API Server 异常
  AUTH_API_OR{{OR}}
  AUTH_API --> AUTH_API_OR
  AUTH_API_OR --> AUTH_API1[准入控制器未启用]
  AUTH_API_OR --> AUTH_API2[PodSecurity 准入控制器配置错误]

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## Related

- [[gpu-fta]] — GPU 异常故障树分析
- [[skills/ts-workloads.md|ts-workloads]] — 工作负载故障排查
- [[pdb-fta]] — PDB 异常故障树分析
- [[skills/assessment-daily-check-quiz.md|assessment-daily-check-quiz]] — Daily Check Quiz
- [[opa]] — OPA (Open Policy Agent)

- [[domain-10-troubleshooting-diagnostics/FTA故障树/list/psp-scc-fta.md|PSP/SCC 异常故障树分析]]
- [[skills/skill-reference-remediation-playbook.md|Remediation Playbook]] — Cross-reference
- [[domain-19-landscape-references/领域索引/security-index.md|Security 安全知识图谱索引]]


<!-- risk-assessed -->
