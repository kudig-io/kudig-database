---
title: "RBAC & Quota Remediation Playbook"
category: remediation
skill_set: "k8s-rbac-quota"
created: "2026-05-22"
updated: "2026-05-22"
last_updated: 2026-05-22
tags: ["reference", "remediation", "playbook", "visibility/public"]
---

# 修复操作手册 / Remediation Playbook

> **来源**: SKILL-RBAC-001 v1.0 — RBAC & Quota Failure 诊断与修复

## 目录

- [风险级别说明](#风险级别说明)
- [修复操作](#修复操作)
  - [🟢 低风险](#-低风险)
    - [REM-001 更新 Role/ClusterRole](#rem-001)
    - [REM-002 调整 ResourceQuota](#rem-002)
    - [REM-003 创建 ServiceAccount](#rem-003)
    - [REM-004 创建 RoleBinding/ClusterRoleBinding](#rem-004)
  - [🟡 中风险](#-中风险)
    - [REM-005 调整 [[NetworkPolicy|NetworkPolicy]]](#rem-005)
  - [🔴 高风险](#-高风险)
    - [REM-006 调整 PodSecurity/Admission](#rem-006)
- [验证确认](#验证确认)
- [升级协议](#升级协议)

## 风险级别说明

| 风险级别 | 标识 | 含义 | Agent 行为 |
|---------|------|------|-----------|
| 低风险 | 🟢 | 权限和配额调整 | 可建议自动执行 |
| 中风险 | 🟡 | 网络策略变更可能影响流量 | 建议操作并等待人工审批 |
| 高风险 | 🔴 | 安全策略变更 | 仅提供操作指导，由人工执行 |

## 修复操作

### 🟢 低风险

#### REM-001: 更新 Role/ClusterRole

- **适用根因**: RC-001
- **前置检查**:
  ```bash
  kubectl auth can-i <verb> <resource> --as=system:serviceaccount:<ns>:<sa> -n <ns>
  # 确认具体缺少哪个权限
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 方案 A: 编辑现有 Role 添加权限
  kubectl patch role <role-name> -n <namespace> --type='json' -p='
  [{"op": "add", "path": "/rules/-", "value":
    {"apiGroups":[""],"resources":["configmaps"],"verbs":["get","list","create","update"]}}]'

  # 方案 B: 应用新的 Role YAML
  cat <<EOF | kubectl apply -f -
  apiVersion: rbac.authorization.k8s.io/v1
  kind: Role
  metadata:
    namespace: <namespace>
    name: <role-name>
  rules:
  - apiGroups: [""]
    resources: ["pods", "services"]
    verbs: ["get", "list", "watch", "create", "update", "patch"]
  EOF
  ```
- **后置验证**:
  ```bash
  kubectl auth can-i <verb> <resource> --as=system:serviceaccount:<ns>:<sa> -n <ns>
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  kubectl rollout undo role <role-name> -n <namespace>
  # 或手动恢复原始 Role YAML
  ```

#### REM-002: 调整 ResourceQuota

- **适用根因**: RC-002
- **前置检查**:
  ```bash
  kubectl describe resourcequota <quota-name> -n <namespace>
  # 确认哪个资源已耗尽
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 方案 A: 增加配额
  kubectl patch resourcequota <quota-name> -n <namespace> --type='json' -p='
  [{"op": "replace", "path": "/spec/hard/pods", "value": "30"},
   {"op": "replace", "path": "/spec/hard/requests.cpu", "value": "20"},
   {"op": "replace", "path": "/spec/hard/requests.memory", "value": "40Gi"}]'

  # 方案 B: 删除配额限制（不推荐长期使用）
  kubectl delete resourcequota <quota-name> -n <namespace>
  ```
- **后置验证**:
  ```bash
  kubectl describe resourcequota <quota-name> -n <namespace>
  # 预期: used < hard for all resources
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 恢复原始配额值
  kubectl patch resourcequota <quota-name> -n <namespace> --type='json' -p='
  [{"op": "replace", "path": "/spec/hard/pods", "value": "<original>"}]'
  ```

#### REM-003: 创建 ServiceAccount

- **适用根因**: RC-003
- **前置检查**:
  ```bash
  kubectl get sa <sa-name> -n <namespace>
  # 预期: NotFound
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  kubectl create serviceaccount <sa-name> -n <namespace>
  ```
- **后置验证**:
  ```bash
  kubectl get sa <sa-name> -n <namespace>
  # 预期: ServiceAccount 存在且已分配 token secret
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  kubectl delete serviceaccount <sa-name> -n <namespace>
  ```

#### REM-004: 创建 RoleBinding/ClusterRoleBinding

- **适用根因**: RC-004
- **前置检查**:
  ```bash
  kubectl get rolebinding -n <namespace>
  kubectl get clusterrolebinding | grep <sa-name>
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # RoleBinding
  kubectl create rolebinding <binding-name> \
    --role=<role-name> \
    --serviceaccount=<namespace>:<sa-name> \
    -n <namespace>

  # ClusterRoleBinding
  kubectl create clusterrolebinding <binding-name> \
    --clusterrole=<clusterrole-name> \
    --serviceaccount=<namespace>:<sa-name>
  ```
- **后置验证**:
  ```bash
  kubectl auth can-i <verb> <resource> --as=system:serviceaccount:<ns>:<sa> -n <ns>
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  kubectl delete rolebinding <binding-name> -n <namespace>
  kubectl delete clusterrolebinding <binding-name>
  ```

### 🟡 中风险

#### REM-005: 调整 NetworkPolicy

- **适用根因**: RC-005
- **前置检查**:
  ```bash
  kubectl get networkpolicy -n <namespace>
  kubectl describe networkpolicy <name> -n <namespace>
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 允许 Pod 访问 Kubernetes API (tcp/443)
  cat <<EOF | kubectl apply -f -
  apiVersion: networking.k8s.io/v1
  kind: NetworkPolicy
  metadata:
    name: allow-api-access
    namespace: <namespace>
  spec:
    podSelector: {}
    policyTypes:
    - Egress
    egress:
    - to:
      - namespaceSelector:
          matchLabels:
            kubernetes.io/metadata.name: kube-system
      ports:
      - protocol: TCP
        port: 443
  EOF
  ```
- **后置验证**:
  ```bash
  kubectl run test-pod --rm -i --restart=Never --image=curlimages/curl \
    -n <namespace> -- https://kubernetes.default.svc.cluster.local/healthz -k
  ```

### 🔴 高风险

#### REM-006: 调整 PodSecurity/Admission

- **适用根因**: RC-006
- **影响说明**: 修改 PodSecurity 标准或 Admission Webhook 配置可能降低集群安全基线。
- **操作步骤**:
  1. **确认拒绝原因**:
     ```bash
     kubectl get events -n <namespace> | grep -i "violated\|admission\|denied"
     ```
  2. **评估安全影响后调整**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

     ```bash
     # 为特定 namespace 放宽 PodSecurity（示例：从 restricted 到 baseline）
     kubectl label namespace <namespace> pod-security.kubernetes.io/enforce=baseline --overwrite
     ```
  3. **或修改 Pod spec 以符合策略**
- **安全检查**:
  - 确认放宽范围仅限于目标 namespace
  - 记录安全策略变更用于审计
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

  ```bash
  kubectl label namespace <namespace> pod-security.kubernetes.io/enforce=restricted --overwrite
  ```

## 验证确认

### 即时验证

```bash
# V1: ServiceAccount 存在
kubectl get sa <sa-name> -n <namespace>

# V2: 权限检查通过
kubectl auth can-i <verb> <resource> --as=system:serviceaccount:<ns>:<sa> -n <ns>

# V3: ResourceQuota 未超限
kubectl describe resourcequota -n <namespace>

# V4: Pod 可以正常创建
kubectl get pods -n <namespace> -l app=<label>
```

### 解决确认标准

- [ ] ServiceAccount 存在且正确关联
- [ ] Role/ClusterRole 包含所需权限
- [ ] RoleBinding/ClusterRoleBinding 正确关联 SA 和 Role
- [ ] ResourceQuota used < hard（或不存在配额限制）
- [ ] 目标 Pod/Deployment 可以正常创建和运行
- [ ] 无 forbidden/unauthorized 事件

## 升级协议

### 自动升级条件

| 条件 | 说明 |
|------|------|
| 权限修复后仍失败 | 可能存在外部身份认证问题（OIDC/LDAP） |
| 涉及 ClusterAdmin 权限 | 需要安全团队审批 |
| 多个 namespace 同时受影响 | 可能是集群级 RBAC 配置被篡改 |

### 升级消息模板

```
【{severity}】RBAC & Quota Failure - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: {namespace} 中 {resource} 因 {reason} 失败
- 影响范围: 
  - 受影响服务: {affected_services}
  - 受影响操作: {affected_operations}
- 可能根因: {suspected_root_cause}
- 已尝试修复: {attempted_remediation}
- Skill 版本: SKILL-RBAC-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```
