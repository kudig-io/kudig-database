---
title: 基于角色的访问控制
description: RBAC（Role-Based Access Control）是 Kubernetes 的权限管理机制，通过角色（Role/ClusterRole）和绑定（Ro...
summary: RBAC（Role-Based Access Control）是 Kubernetes 的权限管理机制，通过角色（Role/ClusterRole）和绑定（Ro...
category: dictionary
tags:
- k8s
- glossary
- rbac
- security
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 基于角色的访问控制 是什么
- RBAC (Role-Based Access Control) 详解
trigger_keywords:
- 基于角色的访问控制
- RBAC (Role-Based Access Control)
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 基于角色的访问控制

> **英文名**: RBAC (Role-Based Access Control)

## 概述

RBAC（Role-Based Access Control）是 Kubernetes 的权限管理机制，通过角色（Role/ClusterRole）和绑定（RoleBinding/ClusterRoleBinding）来控制用户、组和 ServiceAccount 对集群资源的访问权限。

## 核心概念/原理

### RBAC 四大资源

| 资源 | 范围 | 作用 |
|------|------|------|
| Role | 命名空间 | 定义权限规则 |
| ClusterRole | 集群 | 定义权限规则 |
| RoleBinding | 命名空间 | 将权限授予主体 |
| ClusterRoleBinding | 集群 | 将权限授予主体 |

### 授权流程

```
用户请求 → API Server → RBAC Authorizer → 匹配 Role/ClusterRole 规则 → 允许/拒绝
```

### RBAC 决策规则

- **默认拒绝**：没有明确允许的操作都会被拒绝。
- **权限叠加**：多个 RoleBinding 的权限取并集。
- **不可拒绝**：RBAC 只支持"允许"，不支持显式"拒绝"。

## 关键机制或特性

- RBAC 从 K8s v1.8 起成为稳定特性。
- 支持 `*` 通配符匹配所有 verbs/resources/apiGroups。
- 支持自定义动词（如 `bind`、`escalate`）。

## 使用场景与最佳实践

- 始终启用 RBAC（禁用 `--authorization-mode=AlwaysAllow`）。
- 遵循最小权限原则，避免过度授权。
- 使用 `kubectl auth can-i` 验证权限配置。
- 定期运行 RBAC 审计工具（如 rakkess、rbac-lookup）。
- 为每个应用创建独立的 ServiceAccount 并绑定最小权限。

## 架构深度解析

### RBAC 授权决策全链路

```
┌──────────────────────────────────────────────────────────────┐
│  API 请求                                                     │
│  ├─ 认证（Authentication）：x509/OIDC/SA Token/Bearer         │
│  │   → 提取 user + groups                                      │
│  ▼                                                           │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ 授权（Authorization）链：                               │  │
│  │ ① Node authorizer（kubelet 专用，先于 RBAC）             │  │
│  │ ② RBAC authorizer（核心决策）                           │  │
│  │ ③ Webhook authorizer（外部策略，可选）                   │  │
│  │ ④ ABAC（已废弃，默认关闭）                              │  │
│  │ 决策语义：Allow 短路；NoOpinion 继续；Deny 直接拒绝      │  │
│  └─────────────────────────────────────────────────────────┘  │
│   │                                                           │
│   ▼ RBAC 内部决策                                             │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ 1. 提取请求属性（user/groups/verb/resource/resourceName/│  │
│  │    ns/apiGroup）                                         │  │
│  │ 2. 查找匹配的 RoleBinding/ClusterRoleBinding            │  │
│  │ 3. 展开 roleRef（Role/ClusterRole + 聚合规则）           │  │
│  │ 4. 规则匹配（精确 + 通配 `*`）                           │  │
│  │ 5. 命中 → Allow；未命中 → NoOpinion                     │  │
│  └─────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 文件路径 | 职责 |
| --- | --- | --- |
| 授权链 | `plugin/pkg/auth/authorizer/` | Node/RBAC/Webhook/ABAC 编排 |
| RBAC 核心 | `pkg/registry/rbac/` | 绑定与规则解析 |
| 缓存 | `plugin/pkg/auth/authorizer/rbac/rbac.go` | 决策缓存（~30s） |
| 聚合 | `pkg/controller/clusterroleaggregation/` | 聚合角色合并 |

### 流程步骤

1. 请求完成认证后进入授权链，依次调用各 authorizer。
2. RBAC 解析请求属性，遍历全部 RoleBinding/ClusterRoleBinding 找 subject 匹配。
3. 展开绑定引用的角色规则（含聚合展开、通配符展开）。
4. 规则与请求属性精确匹配，命中即返回 Allow。
5. 无匹配返回 NoOpinion 交下一授权器；缓存决策结果约 30 秒。

## 生产案例

### 案例 1：误用通配符导致 RBAC 全面失控

| 时间 | 事件 |
| --- | --- |
| T+0 | 为"快速上线"创建 `verbs: ["*"] resources: ["*"]` 的自定义 ClusterRole |
| T+1d | 绑定到全体 SA 后，任意 Pod 可读写全部 Secret |
| T+2d | 监控发现异常：某 Pod 频繁读取 kube-system Secret |
| T+4h | 定位：通配符角色被业务 SA 引用，权限面失控 |
| T+1w | 清理通配符角色，建立权限评审，引入 Kubescape 扫描 |

- **根因分析**：`*` 通配符角色是权限爆炸的典型来源；RBAC 无内置"危险权限"警告，审计滞后是常态。
- **修复命令**：
```bash
# 1. 扫描通配符权限（只读）
kubectl get clusterrole -A -o json | jq -r '.items[] | select(any(.rules[]; (.verbs|index("*")) and (.resources|index("*")))) | .metadata.name'
# 2. 删除宽泛绑定（🔴 高风险：先确认依赖方）
kubectl get clusterrolebinding -o json | jq -r '.items[] | select(.roleRef.name=="wildcard-role") | .metadata.name'
kubectl delete clusterrolebinding <binding> --dry-run=client  # 先演练
# 3. 收敛为最小权限角色
kubectl create clusterrole app-reader --verb=get,list,watch --resource=pods,services,configmaps
```

### 案例 2：subject 匹配疏忽导致跨团队越权

| 时间 | 事件 |
| --- | --- |
| T+0 | A 团队将本团队 SA 绑定到 `edit` 角色（命名空间 prod-a） |
| T+1w | 审计发现 A 团队 SA 可操作 prod-b 的资源 |
| T+2d | 定位：ClusterRoleBinding 中 subjects 误写了 `namespace: ""` 的全局绑定 |
| T+4h | 修正：改为命名空间内 RoleBinding，越权消失 |
| T+1w | 全量审计 ClusterRoleBinding 的 subjects 范围 |

- **根因分析**：ClusterRoleBinding 是集群级绑定，subjects 不区分命名空间；将"本团队"理解成"本命名空间"是常见错误。
- **修复命令**：
```bash
# 1. 审计全局绑定（只读）
kubectl get clusterrolebinding -o json | jq -r '.items[].subjects[]? | select(.namespace != null) | [.namespace, .name, .kind] | @tsv' | sort | uniq -c
# 2. 将命名空间级需求改为 RoleBinding（🟡 中风险）
kubectl create rolebinding app-edit -n prod-a --clusterrole=edit --serviceaccount=prod-a:app-sa
kubectl delete clusterrolebinding app-edit-global --ignore-not-found
```

## 对比评测

| 维度 | RBAC（内置） | OPA/Gatekeeper | OpenFGA | Cedar |
| --- | --- | --- | --- | --- |
| 定位 | 集群基础授权 | 准入策略 | 应用细粒度授权 | 云原生授权语言 |
| 粒度 | API 资源级 | 对象属性级 | 对象实例级 | 对象实例级 |
| 学习成本 | 低 | 高（Rego） | 中 | 低 |
| 变更生效 | ~30s 缓存 | 即时 | 即时 | 即时 |
| 组合使用 | 底座 | 叠加 | 叠加 | 叠加 |

**选型建议**：RBAC 是必选底座；准入/组织策略叠加 Gatekeeper；应用层细粒度权限用 OpenFGA/Cedar。

## 故障排查速查

| 现象 | 可能原因 | 处理命令 |
| --- | --- | --- |
| 403 但理论上应放行 | 缓存（~30s）/多个 authorizer 拒绝 | 等待重试；`--authorization-mode` 检查 |
| 权限比预期大 | 通配符角色/聚合角色 | `kubectl auth can-i --list --as=<user>` 全量审计 |
| SA 跨 ns 有权限 | 误用 ClusterRoleBinding | 检查绑定 kind 与 subjects 范围 |
| 新角色不生效 | 绑定缺失/名称拼写错误 | `kubectl get rolebinding -A \| grep <sa>` |
| 组件 401/403 | 组件证书身份无权限 | 检查 system: 前缀角色绑定 |

## 生产部署清单

- [ ] 审计并清理 `verbs: ["*"]` / `resources: ["*"]` 通配符角色
- [ ] 命名空间级权限一律 RoleBinding，ClusterRoleBinding 需审批
- [ ] `kubectl auth can-i --list` 权限基线纳入季度审计
- [ ] 授权链配置确认：Node 在前、RBAC 在后、Webhook 可选
- [ ] RBAC 变更双人复核 + 审计日志对接 SIEM

## 升级决策点

| 级别 | 条件 | 动作 |
| --- | --- | --- |
| P0 | 发现通配符角色被业务 SA 引用 | 立即收敛并轮换相关凭证 |
| P1 | ClusterRoleBinding 存在大量命名空间级 subjects | 迁移为 RoleBinding |
| P2 | 授权链顺序/模式未确认 | 核对 `--authorization-mode` 配置 |

## 面试要点

1. **Q：RBAC 授权器与其他授权器（Node/Webhook/ABAC）的关系？**
   A：按 `--authorization-mode` 配置的顺序串联：Node authorizer 先处理 kubelet 请求（提高效率），RBAC 处理常规用户/SA，Webhook 可对接外部策略系统，ABAC 已废弃。决策语义：Allow 短路返回；Deny 直接拒绝；NoOpinion 交给下一个授权器。全部 NoOpinion 则拒绝。
2. **Q：为什么 RBAC 变更后权限不是立即生效？**
   A：RBAC authorizer 对授权结果做缓存（默认约 30 秒），避免每次请求都遍历全部绑定；因此变更后存在短暂生效延迟。生产验证权限时应等待缓存过期，而非立即断言。
3. **Q：最小权限原则在 K8s 中如何落地？**
   A：四步：一是角色收敛（按需 verbs/resources，禁止通配符）；二是作用域收敛（默认 RoleBinding 而非 ClusterRoleBinding）；三是实例收敛（resourceNames 白名单）；四是持续审计（auth can-i 基线 + 扫描工具 + 季度复核）。同时区分"平台角色"与"业务角色"分开管理。

## 运维要点

- 基线审计：季度导出全部绑定关系比对基线，新增即告警。
- 缓存意识：脚本断言权限前等待 30s+。
- 审计日志：`rbac.authorization.k8s.io` 写操作与异常 403 模式告警。
- 与 IaC 集成：RBAC 对象纳入 GitOps，杜绝手工 kubectl 直接改。
- 排障入口：403 先 `auth can-i` 验证实际决策，再查绑定链与授权模式。

## 参考链接

- [RBAC (Role-Based Access Control) - Official Documentation](https://kubernetes.io/docs/reference/access-authn-authz/rbac/)

## Related

- [[17-系统基础/06-知识字典/security/role.md|Role]]
- [[17-系统基础/06-知识字典/security/clusterrole.md|Clusterrole]]
- [[17-系统基础/06-知识字典/security/rolebinding.md|Rolebinding]]
- [[17-系统基础/06-知识字典/security/clusterrolebinding.md|Clusterrolebinding]]
- [[17-系统基础/06-知识字典/security/service-account.md|Service Account]]


<!-- risk-assessed -->
