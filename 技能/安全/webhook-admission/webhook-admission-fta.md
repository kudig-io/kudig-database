---
title: Admission Webhook 异常 FTA 树 (skills)
description: 'description: ''- **范围**：Webhook 服务可用性、规则配置、证书与 TLS、回退策略、审计。'''
summary: 'description: ''- **范围**：Webhook 服务可用性、规则配置、证书与 TLS、回退策略、审计。'''
category: skills
tags:
- k8s
- fta
- troubleshooting
- etcd
- apiserver
- coredns
- helm
- argocd
- webhook
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Admission Webhook 异常 FTA 树 是什么
- 如何 Admission Webhook 异常 FTA 树
trigger_keywords:
- Admission
- Webhook
- 异常
- FTA
prerequisites:
- kubectl-basics
- helm-basics
- gitops-basics
- etcd-basics
fta_id: FTA-WEBHOOK_ADMISSION-001
component: Webhook Admission
severity: critical
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Admission Webhook 异常 FTA 树

---
title: Admission Webhook 异常故障树分析
description: '- **范围**：Webhook 服务可用性、规则配置、证书与 TLS、回退策略、审计。'
category: fta
tags:
- fta
- troubleshooting
- webhook
- admission
- mutating
- validating
- timeout
- apiserver
- coredns
- helm
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- Admission Webhook 异常故障树分析 是什么
- 如何 Admission Webhook 异常故障树分析
- Admission Webhook 异常故障树分析 根因分析
- Admission Webhook 异常故障树分析 故障树
trigger_keywords:
- Admission
- Webhook
- 异常故障树分析
- fta
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: structural
  path: ../故障诊断/高级排障/structural-01-control-plane/05-webhook-admission-troubleshooting.md
  label: '结构化排障: 05-webhook-admission-troubleshooting'
fta_metadata:
  fta_id: FTA-WEBHOOK-001
  top_event: Admission Webhook 异常 (拒绝/超时/策略冲突)
  top_event_id: TE-WEBHOOK-001
  bottom_events_count: 16
  gate_types: [OR, AND]
  entry_conditions:
    - "kubectl get events -A | grep -E 'Webhook|MutatingWebhook|ValidatingWebhook' 显示拒绝"
    - "kubectl describe pod <name> -n <ns> | grep -E 'admission webhook|denied' 显示 webhook 拒绝"
    - "kubectl run 测试 --image=nginx 失败显示 webhook 错误"
agent_notes:
  decision_tree_entry: "kubectl get mutatingwebhookconfiguration,validatingwebhookconfiguration -A 检查 webhook 配置"
  critical_commands:
    - "kubectl get mutatingwebhookconfiguration,validatingwebhookconfiguration -A"
    - "kubectl describe mutatingwebhookconfiguration <name>"
    - "kubectl get events -A | grep -E 'Webhook|admission'"
    - "kubectl logs -n <ns> -l app=<webhook-name> --tail=50"
  danger_operations:
    - action: "kubectl delete mutatingwebhookconfiguration <name>"
      risk: "删除 MutatingWebhook 会关闭变异钩子功能，可能影响 Pod 注入和修改"
      requires_confirmation: true
    - action: "kubectl delete validatingwebhookconfiguration <name>"
      risk: "删除 ValidatingWebhook 会关闭验证钩子功能，可能允许非法配置通过"
      requires_confirmation: true
---

<!-- condition: kubectl get events -A | grep -E 'Webhook.*denied|admission.*rejected' 显示 Webhook 拒绝事件 -->

# Admission Webhook 异常 FTA 树

## 适用范围与说明
- **目标**：覆盖准入 Webhook 拒绝、超时与策略冲突的关键成因与路径。
- **范围**：Webhook 服务可用性、规则配置、证书与 TLS、回退策略、审计。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: Admission Webhook 异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> SVC[Webhook 服务异常]
  OR0 --> RULE[规则配置错误]
  OR0 --> TLS[TLS 证书异常]
  OR0 --> FAIL[回退策略异常]
  OR0 --> PERF[性能与超时异常]
  OR0 --> AUDIT[审计与回滚缺失]

  %% ========== 1. Webhook 服务异常 ==========
  SVC_OR{{OR}}
  SVC --> SVC_OR
  SVC_OR --> SVC_POD[Webhook Pod 异常]
  SVC_OR --> SVC_NET[网络连通异常]
  SVC_OR --> SVC_SVC[Service 配置异常]

  %% 1.1 Webhook Pod 异常
  SVC_POD_OR{{OR}}
  SVC_POD --> SVC_POD_OR
  SVC_POD_OR --> SVC_POD1[Pod 未就绪/CrashLoop]
  SVC_POD_OR --> SVC_POD2[资源不足导致 OOM]
  SVC_POD_OR --> SVC_POD3[镜像拉取失败]

  %% 1.2 网络连通异常
  SVC_NET_OR{{OR}}
  SVC_NET --> SV

## 生产案例

### 案例 1: Admission Webhook 超时导致所有资源创建失败

| 时间 | 事件 |
|------|------|
| 17:00 | 所有 kubectl apply 报错: "connection refused" 或 "context deadline exceeded" |
| 17:05 | `kubectl get validatingwebhookconfigurations` 发现 webhook 服务不可达 |
| 17:08 | Webhook Pod CrashLoopBackOff |
| 17:12 | 🟡 设置 failurePolicy: Ignore 或修复 Webhook Pod |
| 17:15 | 资源创建恢复 |

**根因**: Webhook 服务 OOMKilled，failurePolicy=Fail 导致所有请求被拒绝。

### 案例 2: Webhook 规则过广导致循环调用

**现象**: Webhook Pod 反复重启，日志显示 "too many redirects"。

**诊断**: Webhook 拦截了自身 namespace 的资源创建，形成循环

**修复**: 🟢 添加 namespaceSelector 排除 webhook 自身 namespace

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | Webhook 导致集群不可用 | 删除/禁用问题 Webhook |
| P1 | 部分资源创建失败 | 检查 failurePolicy 和 Webhook 状态 |
| P2 | Webhook 延迟偏高 | 优化 timeoutSeconds 和规则范围 |

## 面试要点

1. **Q: Mutating 与 Validating Admission Webhook 的执行顺序？**
   A: 请求先经过所有 Mutating Webhook(可修改对象)，再经过所有 Validating Webhook(只能拒绝)。两者内部按 name 字母序执行，失败则拒绝请求。

2. **Q: failurePolicy 的 Fail 与 Ignore 如何选择？**
   A: Fail: Webhook 不可用时拒绝请求，保证策略强制执行(安全类)；Ignore: Webhook 不可用时放行，保证可用性(非关键策略)。生产安全类用 Fail，其他用 Ignore。

3. **Q: 如何避免 Webhook 影响自身组件？**
   A: ① namespaceSelector 排除 kube-system 和 webhook 自身 ns ② objectSelector 排除特定 label ③ 设置合理的 timeoutSeconds(默认 10s) ④ 使用 reinvocationPolicy 避免循环。

## 相关链接

- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/fta-方法论/execution-engine/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## Related

- [[etcd-fta]] — etcd 异常故障树分析
- [[gitops-argocd-fta]] — GitOps(ArgoCD) 异常故障树分析
- [[技能/skill-MOC.md|skill-MOC]] — topic-skills MOC
- [[helm]] — Helm
- [[coredns]] — CoreDNS

- [[故障诊断/FTA故障树/list/webhook-admission-fta.md|Admission Webhook 异常 FTA 树]]
- [[技能/Agent Orchestration Patterns.md|Agent Orchestration Patterns for FTA]] — Cross-reference


<!-- risk-assessed -->
