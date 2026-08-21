---
title: 角色
description: Role 是 Kubernetes RBAC 中命名空间级别的权限定义资源。它定义了一组允许的操作（verbs）和可操作的资源（resources）。...
summary: Role 是 Kubernetes RBAC 中命名空间级别的权限定义资源。它定义了一组允许的操作（verbs）和可操作的资源（resources）。...
category: dictionary
tags:
- k8s
- glossary
- role
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
- 角色 是什么
- Role 详解
trigger_keywords:
- 角色
- Role
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 角色

> **英文名**: Role

## 概述

Role 是 Kubernetes RBAC 中命名空间级别的权限定义资源。它定义了一组允许的操作（verbs）和可操作的资源（resources）。

## 核心概念/原理

### 核心概念

- **verbs**：允许的操作（`get`, `list`, `watch`, `create`, `update`, `patch`, `delete`）。
- **resources**：可操作的资源类型（`pods`, `services`, `deployments` 等）。
- **resourceNames**：限定特定资源实例名称。
- **apiGroups**：API 组（`""` 表示核心组，`apps` 表示 apps 组等）。

### 示例

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: pod-reader
  namespace: default
rules:
- apiGroups: [""]
  resources: ["pods"]
  verbs: ["get", "list", "watch"]
```

## 关键机制或特性

- Role 仅在命名空间内生效。
- ClusterRole 在集群范围生效。
- Role 和 ClusterRole 都通过 Binding 关联到用户/组/ServiceAccount。

## 使用场景与最佳实践

- 遵循最小权限原则。
- 优先使用 ClusterRole + RoleBinding 的模式减少重复定义。
- 定期审计 RBAC 配置，清理不再使用的 Role。

## 架构深度解析

### Role 授权链路与资源模型

```
┌──────────────────────────────────────────────────────────────┐
│  Role（命名空间级权限声明）                                     │
│  apiVersion: rbac.authorization.k8s.io/v1                     │
│  kind: Role                                                    │
│  metadata: { namespace: app }                                  │
│  rules:                                                        │
│  ├─ apiGroups: [""]                                            │
│  │   resources: [pods, pods/log]                               │
│  │   verbs: [get, list, watch]                                 │
│  ├─ apiGroups: ["apps"]                                        │
│  │   resources: [deployments]                                  │
│  │   verbs: [get, patch]                                       │
│  │   resourceNames: ["order-api"]   ← 单实例级授权             │
│  └─ nonResourceURLs: []（Role 不支持，仅 ClusterRole）          │
│                                                                 │
│  绑定：RoleBinding（subject → roleRef）                         │
│  ├─ subjects: user/group/serviceaccount                         │
│  └─ roleRef: kind: Role（仅限同命名空间）                       │
│                                                                 │
│  授权决策：                                                      │
│  Request(verb=get, resource=pods, ns=app)                       │
│   → 匹配 rules → Allow                                          │
│   → 不匹配 → NoOpinion → 下一 authorizer                        │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 文件路径 | 职责 |
| --- | --- | --- |
| RBAC 授权 | `plugin/pkg/auth/authorizer/rbac/rbac.go` | 决策主逻辑 |
| 规则校验 | `pkg/registry/rbac/validation/` | Role 规则合法性校验 |
| 绑定解析 | `pkg/registry/rbac/rolebinding/` | subject→roleRef 关联 |
| 权限检查 | `pkg/registry/rbac/helpers.go` | Verb/Resource 匹配工具 |

### 流程步骤

1. 管理员创建 Role（rules 声明权限）与 RoleBinding（subject 绑定）。
2. 请求进入 RBAC authorizer，解析请求属性（user/verb/resource/ns）。
3. 查找该命名空间内 subject 匹配的 RoleBinding，展开 roleRef 规则。
4. 规则匹配：apiGroups + resources + verbs + resourceNames 精确/通配匹配。
5. 命中返回 Allow；未命中返回 NoOpinion，由后续授权器决定。

## 生产案例

### 案例 1：Role 与 ClusterRole 同名冲突导致的权限漂移

| 时间 | 事件 |
| --- | --- |
| T+0 | 团队在 `prod` 命名空间创建 `Role: view`，授权删除权限 |
| T+1d | 该命名空间用户突然获得删除权限 |
| T+2d | 定位：RoleBinding 引用了同名 ClusterRole `view`（预定义只读），
|     | 而非新建的 Role（同名资源在不同类型间不冲突） |
| T+3d | 重命名 Role 为 `prod-view-ext`，绑定关系明确 |

- **根因分析**：Role 与 ClusterRole 是不同资源类型，同名不冲突；roleRef 中 `kind` 字段决定了引用的是哪个。混淆 kind 导致预期外的权限面。
- **修复命令**：
```bash
# 1. 查看绑定实际引用的角色类型（只读）
kubectl get rolebinding -n prod -o yaml | grep -B2 -A4 roleRef
# 2. 修正引用（🟡 中风险）
kubectl edit rolebinding view-bind -n prod
#   把 roleRef.kind 改为 Role / 或把 name 改为正确角色
# 3. 验证权限面
kubectl auth can-i delete pods -n prod --as=system:serviceaccount:prod:ci-sa
```

### 案例 2：resourceNames 误用导致越权删除

| 时间 | 事件 |
| --- | --- |
| T+0 | 运维创建 Role 允许删除 `resourceNames: ["order-api-v1"]` 的 Deployment |
| T+30min | 发布新版本后 Deployment 重命名为 `order-api-v2` |
| T+1h | 运维删除失败（权限不足），误以为 Role 失效，直接授予 `delete deployments`（无 resourceNames） |
| T+2d | 该 SA 获得删除全部 Deployment 的权限 |
| T+1w | 事故：CI 误删生产全部 Deployment |
| T+2w | 恢复 + 引入 resourceNames 白名单 + 权限变更评审 |

- **根因分析**：resourceNames 是"白名单而非前缀匹配"；放宽权限时的"简单粗暴"升级（去掉 resourceNames）是权限爆炸的常见路径。
- **修复命令**：
```bash
# 1. 审计当前宽权限（只读）
kubectl get role -A -o yaml | grep -B3 -A2 "delete deployments"
# 2. 收紧：resourceNames 显式列白名单（🟡 中风险）
kubectl apply -f - <<'EOF'
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata: { name: deploy-operator, namespace: prod }
rules:
- apiGroups: ["apps"]
  resources: ["deployments"]
  verbs: ["get", "delete"]
  resourceNames: ["order-api-v1", "order-api-v2", "payment-api"]
EOF
# 3. 验证
kubectl auth can-i delete deployments/other -n prod --as=system:serviceaccount:prod:ci  # 🟢 应返回 no
```

## 对比评测

| 维度 | Role | ClusterRole | RoleBinding | ClusterRoleBinding |
| --- | --- | --- | --- | --- |
| 作用域 | 单命名空间 | 集群级 | 单命名空间 | 集群级 |
| 可绑定角色 | 本 ns Role | 任意 ClusterRole | 仅本 ns Role | 任意 ClusterRole |
| 常见场景 | 应用权限 | 跨 ns 复用/集群管理 | 团队授权 | 全局角色 |
| 泄漏风险 | 低 | 高 | 低 | 高 |

**选型建议**：团队/应用权限用 Role + RoleBinding；需要跨命名空间共享时创建 ClusterRole 但用 RoleBinding 引用（限定作用域）。

## 故障排查速查

| 现象 | 可能原因 | 处理命令 |
| --- | --- | --- |
| 403：未授权 | Role 缺 verb/资源 | `kubectl auth can-i get pods -n <ns> --as=<user>` |
| 权限超出预期 | roleRef.kind 误用 ClusterRole | 检查 `kubectl get rolebinding -o yaml` 的 kind |
| 指定实例仍不可操作 | resourceNames 大小写/不存在 | 精确核对资源名称（含版本后缀） |
| 改了 Role 不生效 | RBAC 缓存 ~30s | 等待后重试或重启 kube-apiserver（不推荐） |
| 聚合规则混乱 | 多个 ClusterRole 聚合 | 用 `kubectl auth can-i --list` 全量确认 |

## 生产部署清单

- [ ] 应用权限一律 Role + RoleBinding（单命名空间）
- [ ] resourceNames 白名单化管理，删除/修改走变更审批
- [ ] 角色命名规范：`<app>-<role>`，避免与内置角色同名
- [ ] 权限变更评审：roleRef.kind 变更必须双人复核
- [ ] 季度审计：`kubectl get rolebinding -A` 全量导出比对

## 升级决策点

| 级别 | 条件 | 动作 |
| --- | --- | --- |
| P0 | 存在无 resourceNames 的 delete/update 宽权限 | 立即收敛为白名单或拆分角色 |
| P1 | roleRef.kind 混淆（引用错误角色类型） | 修正绑定并审计影响面 |
| P2 | 角色命名无规范 | 建立规范并逐步迁移 |

## 面试要点

1. **Q：Role 和 ClusterRole 的区别？ClusterRole 能被 RoleBinding 引用吗？**
   A：Role 限定单个命名空间；ClusterRole 是集群级定义，可被 ClusterRoleBinding 引用（集群范围）也可被 RoleBinding 引用（借用其规则但在命名空间内生效）。反之 Role 不能绑定到 ClusterRoleBinding。这是实现"一处定义、按命名空间复用"的标准方式。
2. **Q：resourceNames 有什么限制？**
   A：resourceNames 只能限定"按名称访问"的操作（get/update/patch/delete），对 list/watch 无效（list 是集合操作，无法用名称限定）；且必须与具体资源名精确匹配，不支持前缀/通配。白名单是"越精确越安全"，但注意它不适用于 create（创建时名称未知）。
3. **Q：如何验证一条权限是否真的授予了某用户？**
   A：三种方式：`kubectl auth can-i <verb> <resource> -n <ns> --as=<user>`（实际决策验证）、`kubectl auth can-i --list -n <ns> --as=<user>`（全量权限清单）、以及从 RoleBinding/ClusterRoleBinding 反查绑定链。前两者基于真实授权器，结果最可靠。

## 运维要点

- 命名规范：Role 命名含应用前缀，避免与内置角色（view/edit/admin）混淆。
- 变更评审：roleRef.kind 变更、删除权限授予必须双人复核。
- 缓存感知：RBAC 变更约 30s 生效，脚本断言需等待。
- 审计联动：Role/RoleBinding 写操作审计日志对接 SIEM，异常绑定告警。
- 排障入口：403 先 `auth can-i` 定位缺口，再反向查 roleRef 链。

## 参考链接

- [Role - Official Documentation](https://kubernetes.io/docs/reference/access-authn-authz/rbac/)

## Related

- [[17-系统基础/06-知识字典/security/rbac.md|Rbac]]
- [[17-系统基础/06-知识字典/security/clusterrole.md|Clusterrole]]
- [[17-系统基础/06-知识字典/security/rolebinding.md|Rolebinding]]
- [[17-系统基础/06-知识字典/security/clusterrolebinding.md|Clusterrolebinding]]
- [[17-系统基础/06-知识字典/security/service-account.md|Service Account]]


<!-- risk-assessed -->
