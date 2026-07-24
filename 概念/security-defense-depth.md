---
title: Defense-in-Depth Security
description: '- [[概念/eBPF × 运行时安全.md|eBPF x 运行时安全]] — synthesis'
summary: '- [[概念/eBPF × 运行时安全.md|eBPF x 运行时安全]] — synthesis'
category: concepts
tags:
- k8s
- security
- rbac
- networkpolicy
- pod-security
- defense-in-depth
- etcd
- kubelet
- istio
- falco
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Defense-in-Depth Security 是什么
- 如何 Defense-in-Depth Security
trigger_keywords:
- Defense-in-Depth
- Security
prerequisites:
- kubectl-basics
- service-mesh-basics
- ebpf-basics
- etcd-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Defense-in-Depth Security

## Security Layers

[[Kubernetes|Kubernetes]] security follows a defense-in-depth model across four layers:

### Layer 1: Cluster Access Control

**Authentication** verifies identity via:
- X.509 client certificates
- Bearer tokens (static or ServiceAccount)
- OpenID Connect (OIDC) integration
- Webhook token authentication

**Authorization** controls access via:
- **RBAC** (Role-Based Access Control): ClusterRole/Role + ClusterRoleBinding/RoleBinding
- **ABAC** (Attribute-Based): Legacy, rarely used
- **Node Authorization**: Restricted [[kubelet|kubelet]] permissions
- **Webhook Authorization**: External authorization [[Service|service]]

**Admission Control** intercepts requests before persistence:
- **Mutating**: Modify requests (e.g., inject sidecar, set defaults)
- **Validating**: Reject non-compliant requests (e.g., resource quotas, policy engines)

### Layer 2: Network Isolation

- **NetworkPolicy**: Pod-level firewall controlling ingress/egress traffic
- **Namespace isolation**: Logical network boundaries
- **Service Mesh mTLS**: Encrypted service-to-service communication (Istio/Linkerd)

### Layer 3: Container/Runtime Security

- **Pod Security Standards**: Three levels -- Privileged, Baseline, Restricted
- **Seccomp/AppArmor/SELinux**: System call and MAC profiles
- **Capabilities**: Drop ALL, add only needed capabilities
- **Image security**: Scanning (Trivy/Clair), signing (Cosign/Notary)
- **Secure containers**: gVisor, Kata Containers for strong isolation

### Layer 4: Data Security

- **Secrets Management**: etcd encryption at rest, External Secrets Operator, Vault integration
- **Audit Logging**: Record all API operations for compliance and forensics

## Zero Trust Architecture

In zero trust, no component is inherently trusted:
- Every API request requires authentication
- Every access requires authorization (least privilege RBAC)
- All network traffic is subject to NetworkPolicy
- Runtime behavior is monitored by Falco or similar tools

## RBAC Best Practices

- Use **Role** (namespace-scoped) over **ClusterRole** when possible
- Bind to **ServiceAccounts**, not Users, for in-cluster workloads
- Apply **least privilege**: grant only required verbs on required resources
- Regularly audit RBAC with `kubectl auth can-i` checks

## 源码实现分析

### kube-apiserver 认证-授权-准入链

```go
// k8s.io/apiserver/pkg/server/genericapiserver.go
func (s *GenericAPIServer) BuildHandlerChain(apiHandler http.Handler) http.Handler {
    handler := apiHandler
    // 请求处理链（从外到内）：
    handler = genericapifilters.WithAudit(handler, ...)        // 7. 审计日志
    handler = genericapifilters.WithAuthorization(handler, ...) // 6. 授权（RBAC/Node/Webhook）
    handler = genericapifilters.WithAuthentication(handler, ...)// 5. 认证（X509/Token/OIDC）
    handler = genericfilters.WithTimeoutForNonLongRunningRequests(handler, ...) // 4. 超时
    handler = genericapifilters.WithRequestInfo(handler, ...)  // 3. 解析请求信息
    handler = genericapifilters.WithCORS(handler, ...)         // 2. CORS
    handler = genericapifilters.WithPanicRecovery(handler, ...) // 1. Panic 恢复
    return handler
}
// RBAC 授权核心逻辑
func (r *RBACAuthorizer) Authorize(ctx context.Context, a authorizer.Attributes) (authorizer.Decision, error) {
    // 1. 获取用户绑定的所有 Role/ClusterRole
    roles := r.getRoles(a.GetUser())
    // 2. 遍历规则匹配 verb + resource + namespace
    for _, rule := range roles.Rules {
        if ruleMatches(rule, a.GetVerb(), a.GetResource(), a.GetNamespace()) {
            return authorizer.DecisionAllow, nil
        }
    }
    return authorizer.DecisionNoOpinion, nil // 默认拒绝
}
```

### 纵深防御架构

```
┌──────────────────────────────────────────────────────────┐
│              Kubernetes 纵深防御四层模型                  │
├──────────────────────────────────────────────────────────┤
│  Layer 4: 数据安全                                       │
│  │ etcd 加密 / External Secrets / Vault / 审计日志    │
│  Layer 3: 容器/运行时安全                               │
│  │ Pod Security Standards / Seccomp / AppArmor       │
│  │ Capabilities / 镜像签名 / gVisor / Kata           │
│  Layer 2: 网络隔离                                       │
│  │ NetworkPolicy / Namespace / mTLS (Istio/Linkerd)  │
│  Layer 1: 集群访问控制                                   │
│  │ Authentication / RBAC / Admission Control         │
└──────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：最小权限 RBAC 配置

```yaml
# 🟡 中风险：创建 RBAC 规则影响权限分配
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: production
  name: app-deployer
rules:
- apiGroups: ["apps"]
  resources: ["deployments"]
  verbs: ["get", "list", "update", "patch"]  # 仅允许更新，不允许删除
- apiGroups: [""]
  resources: ["configmaps"]
  verbs: ["get", "list", "create", "update"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  namespace: production
  name: ci-deployer-binding
subjects:
- kind: ServiceAccount
  name: ci-deployer
  namespace: ci-system
roleRef:
  kind: Role
  name: app-deployer
  apiGroup: rbac.authorization.k8s.io
```

### 场景二：Pod Security Standards 命名空间强制

```yaml
# 🟡 中风险：修改命名空间标签影响所有新 Pod 创建
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    pod-security.kubernetes.io/enforce: restricted    # 强制执行 restricted
    pod-security.kubernetes.io/enforce-version: latest
    pod-security.kubernetes.io/audit: restricted       # 审计记录
    pod-security.kubernetes.io/warn: restricted        # 警告提示
```

### 场景三：审计 RBAC 权限

```bash
# 🟢 低风险：只读审计
kubectl auth can-i delete pods -n production --as=system:serviceaccount:ci:ci-deployer
kubectl auth can-i '*' '*' --all-namespaces --as=system:serviceaccount:default:default
# 检查集群中所有 ClusterRoleBinding
kubectl get clusterrolebindings -o json | jq '.items[] | select(.roleRef.name=="cluster-admin") | .subjects'
# 检查谁有 secrets 读取权限
kubectl get rolebindings,clusterrolebindings -A -o json | \
  jq '.items[] | select(.roleRef.name | test("secret"; "i"))'
```

## 常见误区

| # | 误区 | 正确理解 |
|---|------|----------|
| 1 | 有 RBAC 就安全了 | RBAC 只是一层；还需 NetworkPolicy + PodSecurity + 镜像扫描 + 运行时监控 |
| 2 | 默认 ServiceAccount 无权限 | 默认 SA 自动挂载 token，可能被滥用；应设置 automountServiceAccountToken: false |
| 3 | NetworkPolicy 默认拒绝所有 | 默认无任何 NetworkPolicy 时全放行；必须显式创建默认拒绝策略 |
| 4 | Privileged 容器只是“权限多一点” | Privileged = 完全控制宿主机（所有 capabilities + 所有设备 + 无 seccomp） |
| 5 | 内网不需要 mTLS | 内网横向移动是主要攻击路径；零信任要求所有通信加密+认证 |
| 6 | 审计日志开了就安全了 | 审计只是记录；必须配合告警规则（Falco/ELK）实时检测异常 |

## 面试要点

1. **Q: Kubernetes 纵深防御的四层分别是什么？各层的关键技术？**
   A: ① 集群访问控制：Authentication(X509/OIDC) + RBAC + Admission Webhook；② 网络隔离：NetworkPolicy + Namespace + Service Mesh mTLS；③ 容器/运行时安全：Pod Security Standards + Seccomp/AppArmor + 镜像签名扫描 + gVisor/Kata；④ 数据安全：etcd 加密 + External Secrets + 审计日志。每层独立生效，任一层被突破其他层仍提供保护。

2. **Q: 如何实现零信任架构？**
   A: 核心原则“永不信任，始终验证”：① 每个 API 请求必须认证（无匿名访问）；② 每次访问必须授权（最小权限 RBAC）；③ 所有网络流量受 NetworkPolicy 控制（默认拒绝）；④ 服务间通信 mTLS 加密+双向认证；⑤ 运行时行为监控（Falco 检测异常 syscall）。

3. **Q: Pod Security Standards 三个级别的区别？**
   A: Privileged：无限制（系统组件用）；Baseline：禁止已知提权操作（hostNetwork/PID、privileged、hostPath）；Restricted：最严格（必须 runAsNonRoot、drop ALL capabilities、readOnlyRootFilesystem、seccompProfile=RuntimeDefault）。生产命名空间应使用 Restricted。

4. **Q: 发现集群被入侵后的应急响应流程？**
   A: ① 過制：隔离受影响节点（cordon+网络策略）、撤销可疑 token/证书；② 评估：审计日志回溯攻击路径、检查 RBAC 变更、扫描后门镜像；③ 清除：删除恶意资源、轮换所有凭证、重建受影响节点；④ 加固：修复漏洞入口、加强准入策略、启用更严格审计；⑤ 复盘：无责事后分析、更新检测规则。

## Related

- [[falco]] — Falco
- [[实体/trivy.md|trivy]] — Trivy
- [[实体/vault.md|vault]] — HashiCorp Vault
- [[概念/secrets-management.md|secrets-management]] — Secrets Management
- [[概念/multi-tenancy-isolation.md|multi-tenancy-isolation]] — Multi-Tenancy Isolation
- [[pod-lifecycle|Pod Lifecycle]]
- [[实体/networkpolicy.md|NetworkPolicy]]
- [[技能/安全/rbac/audit-rbac-configurations.md|Audit RBAC Configurations]]
- [[概念/multi-tenancy-isolation.md|Multi-Tenancy Isolation]]
- [[概念/eBPF × 运行时安全.md|eBPF x 运行时安全]] — synthesis
- [[概念/纵深防御 × 供应链安全.md|纵深防御 x 供应链安全]] — synthesis
- [[概念/服务网格 × 零信任安全.md|服务网格 x 零信任安全]] — synthesis

- [[概念/Deployment × Secret 管理.md|Deployment × Secret 管理]]

<!-- risk-assessed -->
