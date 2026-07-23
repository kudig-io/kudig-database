---
title: Audit RBAC Configurations
description: '- [[生态参考/98-merged-indexes/index.md|release-notes-security]]
  — 发布说明索引 — 安全'
summary: '- [[生态参考/98-merged-indexes/index.md|release-notes-security]]
  — 发布说明索引 — 安全'
category: skills
tags:
- k8s
- rbac
- security
- audit
- access-control
- least-privilege
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Audit RBAC Configurations 是什么
- 如何 Audit RBAC Configurations
trigger_keywords:
- Audit
- RBAC
- Configurations
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Audit RBAC Configurations

## Audit Process

### Step 1: Inventory All Roles and Bindings

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get clusterroles,roles --all-namespaces -o wide
kubectl get clusterrolebindings,rolebindings --all-namespaces -o wide
```
### Step 2: Check for Dangerous Permissions

Look for:
- **Wildcard verbs** (`verbs: ["*"]`): Grants all operations on resources
- **Wildcard resources** (`resources: ["*"]`): Grants access to all resource types
- **ClusterRoleBinding to default ServiceAccount**: Gives cluster-wide access to all [[Pods|Pods]] in namespace
- **`[[Secrets|secrets]]` access**: Ability to read secrets is equivalent to full cluster access (via kubeconfig in secrets)

### Step 3: Verify ServiceAccount Permissions

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Check what a ServiceAccount can do
kubectl auth can-i --list --as=system:serviceaccount:default:my-sa
```
### Step 4: Principle of Least Privilege

For each binding, verify:
- Role is namespace-scoped (Role, not ClusterRole) unless cluster-wide access is required
- Verbs are specific (get, list, watch) not wildcard
- Resources are specific (pods, not "*")
- Subjects are specific ServiceAccounts, not groups like `system:authenticated`

### Step 5: Remove Unused Bindings

Identify and remove:
- Bindings to deleted subjects (ServiceAccounts, Users, Groups)
- Roles that are not referenced by any binding
- Overly broad roles where a narrower role would suffice

## Common Anti-Patterns

| Anti-Pattern | Risk | Fix |
|-------------|------|-----|
| `cluster-admin` bound to app ServiceAccount | Full cluster control | Create namespace-scoped Role |
| `verbs: ["*"]` on any resource | Unrestricted operations | Specify exact verbs needed |
| Binding to `system:serviceaccounts` group | All SA access | Bind to specific ServiceAccount |
| No RBAC at all (default permissions) | Uncontrolled access | Enable RBAC, define explicit roles |

## Automation

Use tools like `rbac-lookup` or `kubectl-view-allocations` to audit RBAC at scale. Integrate RBAC review into CI/CD pipelines.

## 生产案例

### 案例 1: 过度授权的 ClusterRoleBinding 导致安全风险

| 时间 | 事件 |
|------|------|
| - | 安全审计发现开发 ServiceAccount 绑定了 cluster-admin |
| - | 该 SA 可删除任何 namespace 和修改 RBAC |
| - | 🟡 替换为最小权限 Role，限制在特定 namespace |

**根因**: 初始配置为调试方便使用了 cluster-admin，未恢复。

### 案例 2: 审计日志未启用导致无法追溯误操作

**现象**: 生产资源被误删，无法确定操作者和时间。

**诊断**: API Server 未配置 --audit-log-path

**修复**: 🟡 启用审计日志 + 配置审计策略(Metadata/Request/RequestResponse)

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 发现未授权访问 | 立即撤销权限 |
| P1 | 权限配置不当 | 审查并修正 RBAC |
| P2 | 审计完善 | 启用审计日志 |

## 面试要点

1. **Q: RBAC 的四大资源对象？**
   A: Role/ClusterRole(定义权限: verbs+resources+apiGroups) + RoleBinding/ClusterRoleBinding(绑定权限到主体: User/Group/ServiceAccount)。ClusterRole 可跨 namespace。

2. **Q: 审计日志的级别？**
   A: None(不记录)、Metadata(记录请求元数据)、Request(记录请求体)、RequestResponse(记录请求和响应)。生产推荐: 默认 Metadata，敏感操作 RequestResponse。

3. **Q: RBAC 最佳实践？**
   A: ① 最小权限原则 ② 避免 cluster-admin ③ 使用 Group 而非 User ④ 定期审查(kubectl auth can-i --list) ⑤ 启用审计日志 ⑥ 使用 OPA/Gatekeeper 策略约束。

## Related

- [[生态参考/98-merged-indexes/index.md|release-notes-security]] — 发布说明索引 — 安全
- [[技能/k8s-pod-security-guide.md|k8s-pod-security-guide]] — Kubernetes Pod 安全最佳实践
- [[技能/configure-health-probes.md|configure-health-probes]] — Configure Health Probes
- [[概念/multi-tenancy-isolation.md|multi-tenancy-isolation]] — Multi-Tenancy Isolation
- [[概念/security-defense-depth.md|security-defense-depth]] — Defense-in-Depth Security
- [[概念/security-defense-depth.md|Defense-in-Depth Security]]
- [[概念/multi-tenancy-isolation.md|Multi-Tenancy Isolation]]
- [[技能/configure-health-probes.md|Configure Health Probes]]


<!-- risk-assessed -->
