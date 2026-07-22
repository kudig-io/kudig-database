---
title: RBAC 授权模型
summary: RBAC 授权模型：Role-Based Access Control（RBAC）是 Kubernetes 的核心授权机制。它通过将权限（Rule）打包成角色（Role/ClusterRole），再将角色绑定到主体（Subject），实现对集群资源的精细化访问控制。RBAC
  是 Kubernetes 多租户与安全隔离的基石。
category: concepts
tags:
- core-concept
- domain-05
- visibility/public
tier: core
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# RBAC 授权模型

Role-Based Access Control（RBAC）是 Kubernetes 的核心授权机制。它通过将权限（Rule）打包成角色（Role/ClusterRole），再将角色绑定到主体（Subject），实现对集群资源的精细化访问控制。RBAC 是 Kubernetes 多租户与安全隔离的基石。

## RBAC 四要素

- **Role**：命名空间级别的角色，定义一组权限规则（verbs + resources），仅在所在命名空间生效。适用于命名空间内部的权限分配。
- **ClusterRole**：集群级别的角色，权限范围覆盖整个集群，或作用于集群作用域资源（如 Node、PersistentVolume、Namespace）。ClusterRole 也可以被 RoleBinding 引用，从而在特定命名空间内生效。
- **RoleBinding**：将 Role 或 ClusterRole 绑定到一个或多个 Subject，限制在特定命名空间内生效。一个 RoleBinding 只能在一个命名空间中起作用。
- **ClusterRoleBinding**：将 ClusterRole 绑定到 Subject，在整个集群范围内生效。注意：ClusterRoleBinding 不能绑定 Role，只能绑定 ClusterRole。

## Subjects 类型

RBAC 中的 `subjects` 字段支持三种主体：

- **User**：外部身份系统中的用户，如 X.509 客户端证书中的 CN、OIDC 身份提供者中的用户名。Kubernetes 本身不管理 User 对象，仅在外部认证通过后将其用户名传递给授权模块。
- **Group**：用户所属的组织或团队，如证书中的 O 字段、OIDC 的 groups claim。便于批量授权，避免为每个用户单独创建绑定。
- **ServiceAccount**：集群内部 Pod 使用的身份，由 Kubernetes 管理。每个 Pod 默认挂载其命名空间中的 `default` ServiceAccount。生产环境建议为每个应用创建独立 ServiceAccount，遵循最小权限原则。

## 聚合 ClusterRole

Kubernetes 1.9+ 支持通过 `aggregationRule` 动态聚合多个 ClusterRole。控制器会自动将匹配标签选择器的 ClusterRole 的规则合并到目标角色中。此机制广泛用于动态扩展权限，例如 Operators 自动注册其 CRD 的权限到 `admin`、`edit` 或 `view` 角色中。聚合角色本身不直接包含规则，其权限来自被聚合的子角色。

典型聚合角色示例：

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: monitoring-aggregate
aggregationRule:
  clusterRoleSelectors:
  - matchLabels:
      rbac.example.com/aggregate-to-monitoring: "true"
rules: []
```

## 默认内置角色

Kubernetes 预定义了四个高频使用的 ClusterRole，平台团队可直接复用：

| 角色 | 权限范围 | 适用场景 |
|---|---|---|
| view | 只读，排除 Secret | 运维监控、只读审计 |
| edit | 读写，排除 RBAC 与配额 | 开发人员日常操作 |
| admin | 命名空间完全控制，包括 RBAC | 命名空间管理员 |
| cluster-admin | 集群超级管理员 | 集群运维、平台团队 |

内置角色经过长期验证，建议优先使用内置角色而非从零构建自定义角色，以减少权限过度授予的风险。

## 阿里云 RAM 与 K8s RBAC 的映射

在阿里云 ACK 中，用户通过 RAM（Resource Access Management）进行身份认证，再通过 ACK 的授权体系映射到 Kubernetes RBAC：

- **RAM 用户/角色** → 对应 Kubernetes 的 User/Group。
- ACK 控制台支持为 RAM 用户绑定预置的 ClusterRole（如 `admin`、`view`），或自定义 RBAC 策略。
- 对于企业级场景，建议通过 RAM SSO 或 OIDC 对接企业身份源（如 AD、LDAP），再在 ACK 中使用 RBAC 做细粒度授权，实现双层管控：RAM 负责"谁能进"，RBAC 负责"进来能做什么"。

## 远程顾问诊断要点

权限不足是远程顾问场景中高频出现的问题。排查 RBAC 问题的核心思路是确认"谁"对"什么"执行"什么操作"被拒绝：

1. **使用 kubectl auth can-i**：指导用户运行 `kubectl auth can-i <verb> <resource> --as=<user> --as-group=<group> -n <namespace>`，直接验证特定主体是否具备某权限。这是最高效的远程排查手段，无需猜测即可定位缺失的权限。
2. **Impersonate 模拟**：若用户无法确认 ServiceAccount 的实际权限，可让其使用具备 `impersonate` 权限的管理员账号执行 `kubectl auth can-i --as=system:serviceaccount:<ns>:<sa>`，模拟目标身份进行验证。注意 impersonate 本身需要授权。
3. **检查 RoleBinding 的命名空间范围**：常见错误是将 RoleBinding 创建在错误的命名空间，导致用户在该命名空间无权访问，而在其他命名空间正常。注意 RoleBinding 的 `metadata.namespace` 决定了生效范围，而 `subjects` 中引用的 ServiceAccount 的命名空间需单独指定。
4. **审计日志定位**：若用户具备查看审计日志的权限，通过搜索 `responseStatus.code=403` 并关联 `user.username` 与 `requestURI`，可精确定位是哪条规则缺失或 verbs 不匹配。审计日志是追溯权限问题的最终手段。
5. **CRD 权限遗漏**：若用户报告无法操作自定义资源（CRD），检查是否为其角色添加了相应 CRD 的 `apiGroups`、`resources` 和 `verbs`。聚合角色或 Operator 可能未自动扩展新版本的 CRD 权限。

更多排查细节可参考 [[故障诊断/高级排障/06-security-auth/01-rbac-troubleshooting.md|rbac-troubleshooting]] 与技能页面 [[故障诊断/技能体系/skill-set/k8s-rbac-quota/SKILL.md|k8s-rbac-quota]]。

## 源码实现分析

### RBAC 授权决策流程

```go
// kubernetes/plugin/pkg/auth/authorizer/rbac/rbac.go
func (r *RBACAuthorizer) Authorize(ctx context.Context,
    requestAttributes authorizer.Attributes) (authorizer.Decision, error) {
    // 1. 获取请求主体（User/Group/ServiceAccount）
    subject := requestAttributes.GetUser()
    
    // 2. 查找所有匹配的 RoleBinding/ClusterRoleBinding
    bindings := r.getBindings(subject)  // 按 subject 索引
    
    // 3. 遍历每个 binding 的 rules 检查是否允许
    for _, binding := range bindings {
        for _, rule := range binding.Role.Rules {
            if ruleMatches(rule, requestAttributes) {
                // verb + resource + apiGroup + namespace 全部匹配
                return authorizer.DecisionAllow, nil
            }
        }
    }
    // 4. 无匹配规则 → 拒绝（默认拒绝）
    return authorizer.DecisionNoOpinion, nil
}
```

### 权限检查命令链路

```
kubectl auth can-i create pods --as=system:serviceaccount:dev:deployer
    │
    ▼
API Server → SubjectAccessReview API
    │
    ▼
RBACAuthorizer.Authorize()
    │
    ▼
查找 RoleBinding (namespace=default, subject=deployer)
    │
    ▼
匹配 Role rules: verbs=[create], resources=[pods]
    │
    ▼
返回: allowed=true/false
```

## 使用场景

### 场景一：开发人员命名空间权限

```yaml
# 开发团队只能操作 dev 命名空间的工作负载
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: dev-team-edit
  namespace: dev
subjects:
- kind: Group
  name: dev-team@example.com    # OIDC group
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: ClusterRole
  name: edit                    # 内置角色：读写工作负载
  apiGroup: rbac.authorization.k8s.io
```

### 场景二：CI/CD ServiceAccount 最小权限

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: deployer-role
  namespace: production
rules:
- apiGroups: ["apps"]
  resources: ["deployments"]
  verbs: ["get", "list", "patch"]   # 仅允许更新镜像
- apiGroups: [""]
  resources: ["pods"]
  verbs: ["get", "list"]            # 只读检查状态
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: ci-deployer
  namespace: production
subjects:
- kind: ServiceAccount
  name: ci-deployer
  namespace: cicd
roleRef:
  kind: Role
  name: deployer-role
  apiGroup: rbac.authorization.k8s.io
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| 没有规则就是允许 | RBAC 默认拒绝，必须显式授权（白名单模式） |
| Role 可以跨命名空间 | Role 只在定义它的 namespace 生效，跨 ns 需 ClusterRole |
| ClusterRoleBinding 可绑定 Role | 只能绑定 ClusterRole，RoleBinding 才能绑定 Role |
| ServiceAccount 自动有权限 | SA 创建时无任何权限，必须显式绑定 Role |
| 删除 RoleBinding 立即生效 | RBAC 变更即时生效（无缓存），但已建立的连接不受影响 |
| view 角色可以看 Secret | 内置 view 角色明确排除了 Secret 读取权限 |

## 面试要点

1. **RBAC 与 ABAC 的区别？** — RBAC 基于角色（Role→Subject 绑定），管理简单、易审计；ABAC 基于属性（用户属性+资源属性+环境条件），灵活但复杂。K8s 默认用 RBAC，ABAC 已不推荐。

2. **如何实现多租户权限隔离？** — 每个租户一个 Namespace + RoleBinding（绑定 edit/view）；NetworkPolicy 网络隔离；ResourceQuota 资源配额；LimitRange 默认资源约束。ClusterRole 通过 RoleBinding 引用可限定在特定 ns。

3. **ServiceAccount 安全最佳实践？** — 每个应用独立 SA（不用 default）；设置 `automountServiceAccountToken: false`（不需要时）；最小权限原则；定期审计 ClusterRoleBinding（避免 cluster-admin 泛滥）。

4. **如何审计集群权限？** — `kubectl auth can-i --list --as=<user>`；审计日志搜索 403 响应；`kubectl get clusterrolebindings -o wide` 检查 cluster-admin 绑定；工具：rbac-lookup、kubectl-who-can、Paranoid。

## 相关概念

- [[kubernetes-pki-certificate-system]] — Kubernetes PKI 与证书体系
- [[multi-tenancy-isolation]] — 多租户隔离机制
- [[secrets-management]] — Secret 管理机制

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
