---
title: RBAC 治理模型
description: 'Kubernetes RBAC 最小权限原则实施、权限矩阵维护、审查流程与 Break Glass 应急机制'
summary: 'Kubernetes RBAC 最小权限原则实施、权限矩阵维护、审查流程与 Break Glass 应急机制'
category: production-operations
tags:
- governance
- rbac
- security
- least-privilege
- break-glass
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- RBAC 治理模型 是什么
- 如何实施 Kubernetes RBAC 最小权限
trigger_keywords:
- rbac
- least-privilege
- rbac-lookup
- rakkess
- break-glass
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# RBAC 治理模型

## 1. 概述

Role-Based Access Control（RBAC）是 Kubernetes 的核心授权机制。缺乏治理的 RBAC 配置会导致：权限膨胀（Privilege Creep）、过度授权（Over-Permissioning）、审计困难和安全事件响应迟缓。

本文定义 RBAC 的最小权限实施策略、权限矩阵管理、定期审查流程和应急权限机制。

核心原则：
- **最小权限**：只授予完成任务所需的最小权限
- **职责分离**：开发、运维、安全角色明确分离
- **定期审查**：每季度审查权限分配，及时回收
- **应急可控**：Break Glass 机制确保紧急情况下可快速提权

## 2. 角色模型设计

### 2.1 标准角色层级

```
Platform Admin          # 平台管理员（集群级）
  ├── Cluster Viewer    # 集群只读
  ├── Namespace Admin   # 命名空间管理员
  ├── Developer         # 开发人员
  ├── CI/CD             # CI/CD 服务账号
  └── Auditor           # 审计员（只读 + 日志）
```

### 2.2 角色定义

```yaml
# 开发人员角色：命名空间级读写
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: developer
  namespace: ${NAMESPACE}
rules:
  # 工作负载管理
  - apiGroups: ["apps"]
    resources: ["deployments", "replicasets", "statefulsets"]
    verbs: ["get", "list", "watch", "create", "update", "patch"]
  - apiGroups: [""]
    resources: ["pods", "pods/log", "pods/exec"]
    verbs: ["get", "list", "watch", "create"]
  - apiGroups: [""]
    resources: ["pods/exec"]
    verbs: ["create"]
    
  # 配置管理
  - apiGroups: [""]
    resources: ["configmaps", "secrets"]
    verbs: ["get", "list", "watch", "create", "update", "patch"]
  - apiGroups: [""]
    resources: ["services", "endpoints"]
    verbs: ["get", "list", "watch", "create", "update", "patch"]
  - apiGroups: ["networking.k8s.io"]
    resources: ["ingresses"]
    verbs: ["get", "list", "watch", "create", "update", "patch"]
    
  # 自身资源查看
  - apiGroups: [""]
    resources: ["resourcequotas", "limitranges"]
    verbs: ["get", "list", "watch"]
  - apiGroups: ["metrics.k8s.io"]
    resources: ["pods", "nodes"]
    verbs: ["get", "list"]

---
# 只读角色：运维/支持人员
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: viewer
  namespace: ${NAMESPACE}
rules:
  - apiGroups: ["*"]
    resources: ["*"]
    verbs: ["get", "list", "watch"]
  # 排除敏感资源
  # 注意：secrets 不在此角色中

---
# CI/CD 角色：仅部署权限
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: cicd-deployer
  namespace: ${NAMESPACE}
rules:
  - apiGroups: ["apps"]
    resources: ["deployments", "statefulsets"]
    verbs: ["get", "list", "watch", "create", "update", "patch"]
  - apiGroups: [""]
    resources: ["services", "configmaps"]
    verbs: ["get", "list", "watch", "create", "update", "patch"]
  - apiGroups: [""]
    resources: ["secrets"]
    verbs: ["get", "list"]    # 只读 secrets（用于验证部署）
  - apiGroups: [""]
    resources: ["pods"]
    verbs: ["get", "list", "watch"]
  - apiGroups: [""]
    resources: ["pods/log"]
    verbs: ["get"]
```

### 2.3 ClusterRole 与聚合

```yaml
# 平台管理员 ClusterRole（聚合多个子角色）
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: platform-admin
  labels:
    rbac.authorization.k8s.io/aggregate-to-admin: "true"
aggregationRule:
  clusterRoleSelectors:
    - matchLabels:
        rbac.platform.io/aggregate: "platform-admin"
rules: []

---
# 聚合子角色：存储管理
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: platform-admin-storage
  labels:
    rbac.platform.io/aggregate: "platform-admin"
rules:
  - apiGroups: [""]
    resources: ["persistentvolumes", "persistentvolumeclaims"]
    verbs: ["*"]
  - apiGroups: ["storage.k8s.io"]
    resources: ["storageclasses", "csinodes", "csidrivers"]
    verbs: ["get", "list", "watch"]

---
# 审计员 ClusterRole
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: auditor
rules:
  - apiGroups: ["*"]
    resources: ["*"]
    verbs: ["get", "list", "watch"]
  - apiGroups: ["audit.k8s.io"]
    resources: ["events"]
    verbs: ["get", "list", "watch"]
  # 排除 secrets
  - apiGroups: [""]
    resources: ["secrets"]
    verbs: []    # 显式拒绝
```

## 3. 权限矩阵维护

### 3.1 权限矩阵格式

```yaml
# rbac-matrix.yaml
roles:
  - name: platform-admin
    scope: cluster
    description: "平台管理员，管理集群基础设施"
    permissions:
      - apiGroups: ["*"]
        resources: ["*"]
        verbs: ["*"]
    assignment:
      - subject: "group:platform-team"
        binding: ClusterRoleBinding
        
  - name: namespace-admin
    scope: namespace
    description: "命名空间管理员，管理团队资源"
    permissions:
      - apiGroups: ["apps", ""]
        resources: ["deployments", "services", "configmaps", "secrets"]
        verbs: ["*"]
      - apiGroups: [""]
        resources: ["pods", "pods/log", "pods/exec"]
        verbs: ["get", "list", "watch", "create"]
    assignment:
      - subject: "group:${TEAM}-leads"
        binding: RoleBinding
        
  - name: developer
    scope: namespace
    description: "开发人员，部署和调试应用"
    permissions:
      - apiGroups: ["apps"]
        resources: ["deployments", "replicasets"]
        verbs: ["get", "list", "watch", "create", "update", "patch"]
      - apiGroups: [""]
        resources: ["pods/exec"]
        verbs: ["create"]
    assignment:
      - subject: "group:${TEAM}-developers"
        binding: RoleBinding
```

### 3.2 权限矩阵生成脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# generate-rbac-matrix.sh

echo "=== RBAC 权限矩阵报告 ==="
echo "生成时间: $(date)"
echo ""

# 遍历所有 RoleBinding
kubectl get rolebindings -A -o json | jq -r '
  .items[] |
  .metadata.namespace as $ns |
  .roleRef.name as $role |
  .subjects[]? |
  [$ns, $role, .kind, .name] | @tsv
' | sort | while IFS=$'\t' read -r ns role kind name; do
  echo "Namespace: ${ns} | Role: ${role} | Subject: ${kind}/${name}"
done

echo ""
echo "=== ClusterRole 绑定 ==="
kubectl get clusterrolebindings -o json | jq -r '
  .items[] |
  .roleRef.name as $role |
  .subjects[]? |
  [$role, .kind, .name] | @tsv
' | sort | while IFS=$'\t' read -r role kind name; do
  echo "ClusterRole: ${role} | Subject: ${kind}/${name}"
done
```
## 4. 定期 RBAC 审查流程

### 4.1 审查周期

```yaml
rbac-review-schedule:
  quarterly:
    - name: 权限全面审查
      scope: all-namespaces
      owner: security-team
      checklist:
        - "检查所有 ClusterRoleBinding 是否符合最小权限"
        - "审查 serviceaccount 权限是否过期"
        - "回收 90 天未使用的权限"
        - "验证 Break Glass 记录"
        
  monthly:
    - name: 新增权限审查
      scope: audit-log
      owner: platform-team
      checklist:
        - "检查上月新增的 RoleBinding"
        - "验证新增权限是否经过审批"
        - "检查权限申请单据"
        
  weekly:
    - name: 异常权限检测
      scope: automated
      owner: sre-oncall
      checklist:
        - "检测异常的权限提升"
        - "检查 serviceaccount 的异常调用"
```

### 4.2 自动化审查脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# rbac-review.sh - 自动化 RBAC 审查

REPORT_FILE="rbac-review-$(date +%Y%m%d).md"

cat > ${REPORT_FILE} << 'HEADER'
# RBAC 审查报告

## 1. 高风险权限（Cluster Admin 级别）
HEADER

# 查找拥有 cluster-admin 权限的非管理员
kubectl get clusterrolebindings -o json | jq -r '
  .items[] |
  select(.roleRef.name == "cluster-admin") |
  .subjects[]? |
  select(.name != "system:masters") |
  "  - \( .kind)/\( .name ) (ClusterRoleBinding)"
' >> ${REPORT_FILE}

cat >> ${REPORT_FILE} << 'SECTION2'

## 2. 过期 ServiceAccount 权限
SECTION2

# 查找 90 天未使用的 ServiceAccount
kubectl get serviceaccounts -A -o json | jq -r '
  .items[] |
  select(.metadata.creationTimestamp | fromdate < (now - 7776000)) |
  .metadata.namespace as $ns |
  .metadata.name as $sa |
  "\($ns)/\($sa)"
' | while read sa; do
  echo "  - ${sa} (创建超过 90 天)" >> ${REPORT_FILE}
done

cat >> ${REPORT_FILE} << 'SECTION3'

## 3. 非标准命名空间绑定
SECTION3

# 查找绑定到 kube-system 等系统命名空间的角色
kubectl get rolebindings -n kube-system -o json | jq -r '
  .items[] |
  select(.metadata.namespace == "kube-system") |
  .subjects[]? |
  select(.name != "system:serviceaccounts:kube-system") |
  "  - \( .metadata.name ): \( .kind)/\( .name )"
' >> ${REPORT_FILE}

echo "审查报告已生成: ${REPORT_FILE}"
```
### 4.3 审查工具

```bash
# rbac-lookup: 快速查看谁有什么权限
# 安装
brew install rbac-lookup    # macOS
go install github.com/reactiveops/rbac-lookup@latest

# 使用
rbac-lookup                         # 列出所有绑定
rbac-lookup developer               # 查找 developer 角色的绑定
rbac-lookup --kind Group            # 按类型过滤
rbac-lookup --output wide           # 详细输出

# rakkess: 查看当前用户能做什么
# 安装
go install github.com/corneliusweig/rakkess@latest

# 使用
rakkess                             # 查看当前用户所有权限
rakkess --as system:serviceaccount:default:my-sa  # 模拟 ServiceAccount
rakkess --namespace production      # 限定命名空间
rakkess --verbs get,list,watch      # 过滤动词
```

## 5. Break Glass 应急机制

### 5.1 设计原则

Break Glass 机制允许紧急情况下临时提升权限，同时确保：
- 所有提权操作有审计记录
- 权限自动过期（最长 4 小时）
- 需要二次确认和审批
- 触发安全告警

### 5.2 Break Glass CRD

```yaml
# BreakGlassRequest CRD
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: breakglassrequests.security.io
spec:
  group: security.io
  versions:
    - name: v1alpha1
      served: true
      storage: true
      schema:
        openAPIV3Schema:
          type: object
          properties:
            spec:
              type: object
              properties:
                subject:
                  type: object
                  properties:
                    kind:
                      type: string
                      enum: [User, Group, ServiceAccount]
                    name:
                      type: string
                targetRole:
                  type: string
                targetNamespace:
                  type: string
                  default: "*"
                reason:
                  type: string
                  minLength: 20
                duration:
                  type: string
                  default: "4h"
                  pattern: "^([1-4]h)$"
                approver:
                  type: string
              required: [subject, targetRole, reason, approver]
            status:
              type: object
              properties:
                phase:
                  type: string
                  enum: [Pending, Approved, Active, Expired, Revoked]
                expiresAt:
                  type: string
                bindingName:
                  type: string
  scope: Cluster
  names:
    plural: breakglassrequests
    singular: breakglassrequest
    kind: BreakGlassRequest
    shortNames: [bgr]
```

### 5.3 Break Glass 流程

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 申请提权
kubectl apply -f - <<EOF
apiVersion: security.io/v1alpha1
kind: BreakGlassRequest
metadata:
  name: bgr-20260702-001
spec:
  subject:
    kind: User
    name: zhangsan@example.com
  targetRole: namespace-admin
  targetNamespace: team-payment
  reason: "生产支付服务 P0 故障，需要紧急查看 Secret 排查证书过期问题"
  duration: "2h"
  approver: "lisi@example.com"
EOF

# 2. 审批（需要 approver 确认）
kubectl patch breakglassrequest bgr-20260702-001 \
  --type='merge' -p='{"status":{"phase":"Approved"}}'

# 3. 控制器自动创建 RoleBinding（带 TTL）
# 控制器代码逻辑：
# - 验证审批人是否为指定 approver
# - 创建 RoleBinding，设置 annotation 记录过期时间
# - 创建 CronJob 在过期时自动删除

# 4. 查看当前活跃的 Break Glass 请求
kubectl get breakglassrequests -o json | jq '
  .items[] | select(.status.phase == "Active") |
  {
    name: .metadata.name,
    subject: .spec.subject.name,
    role: .spec.targetRole,
    namespace: .spec.targetNamespace,
    expiresAt: .status.expiresAt
  }
'

# 5. 紧急撤销
kubectl patch breakglassrequest bgr-20260702-001 \
  --type='merge' -p='{"status":{"phase":"Revoked"}}'
```
### 5.4 Break Glass 告警

```yaml
# Prometheus 告警规则
groups:
  - name: break-glass
    rules:
      - alert: BreakGlassActivated
        expr: increase(breakglass_requests_total{phase="Active"}[5m]) > 0
        for: 0m
        labels:
          severity: critical
        annotations:
          summary: "Break Glass 权限已激活: {{ $labels.subject }} -> {{ $labels.role }}"
          description: "请求原因: {{ $labels.reason }}"
          
      - alert: BreakGlassExpired
        expr: increase(breakglass_requests_total{phase="Expired"}[5m]) > 0
        for: 0m
        labels:
          severity: info
        annotations:
          summary: "Break Glass 权限已过期: {{ $labels.subject }}"
          
      - alert: BreakGlassAbuse
        expr: |
          count by (subject) (
            breakglass_requests_total{phase="Active"}
          ) > 3
        for: 1h
        labels:
          severity: warning
        annotations:
          summary: "用户 {{ $labels.subject }} 有超过 3 个活跃 Break Glass 请求"
```

## 6. RBAC 安全最佳实践

### 6.1 禁止项

```yaml
# 绝对不允许的配置
forbidden-practices:
  - description: "匿名用户绑定高权限角色"
    check: |
      kubectl get clusterrolebindings -o json | jq '
        .items[] | select(.subjects[]?.name == "system:anonymous")
      '
      
  - description: "默认 ServiceAccount 绑定自定义角色"
    check: |
      kubectl get rolebindings -A -o json | jq '
        .items[] | select(.subjects[]?.name == "default")
      '
      
  - description: "Wildcard 动词或资源"
    check: |
      kubectl get clusterroles -o json | jq '
        .items[] | select(
          .rules[]? | .verbs[]? == "*" or .resources[]? == "*"
        ) | .metadata.name
      '
```

### 6.2 推荐配置

```yaml
best-practices:
  - name: ServiceAccount 自动挂载禁用
    config: |
      automountServiceAccountToken: false
      
  - name: 使用 Group 绑定而非 User
    reason: "Group 变更不需要修改 ClusterRoleBinding"
    
  - name: 命名空间级 Role 优先于 ClusterRole
    reason: "限制权限爆炸半径"
    
  - name: 定期轮换 ServiceAccount Token
    config: |
      apiVersion: v1
      kind: ServiceAccount
      metadata:
        name: my-sa
      automountServiceAccountToken: false
```

## 7. 监控看板

```promql
# RBAC 权限统计
count by (roleRef_kind, roleRef_name) (
  clusterrolebinding_info
)

# 活跃 Break Glass 请求数
count(breakglass_requests{phase="Active"})

# 按命名空间统计 RoleBinding 数量
count by (metadata_namespace) (
  rolebinding_info
)
```

## Related

- [[01-namespace-strategy-lifecycle|命名空间规划策略]]
- [[02-label-convention-governance|标签/注解规范治理]]
- [[03-admission-policy-governance|准入策略治理]]

## See Also

- [Kubernetes RBAC 文档](https://kubernetes.io/docs/reference/access-authn-authz/rbac/)
- [rbac-lookup](https://github.com/FairwindsOps/rbac-lookup)
- [rakkess](https://github.com/corneliusweig/rakkess)
- [audit2rbac](https://github.com/liggitt/audit2rbac)


<!-- risk-assessed -->
