---
title: TKE 身份认证与 CAM 集成
description: 'CAM 角色绑定、ServiceAccount IAM 策略、TKE 审计日志集成、子账号权限管理全面指南'
summary: 'CAM 角色绑定、ServiceAccount IAM 策略、TKE 审计日志集成、子账号权限管理全面指南'
category: cloud-providers
tags:
- cloud
- k8s
- tke
- tencent
- iam
- cam
- rbac
tier: supporting
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
- TKE CAM 集成是什么
- 如何配置 TKE ServiceAccount IAM
- 如何管理 TKE 子账号权限
trigger_keywords:
- CAM
- RBAC
- ServiceAccount
- IAM
- 审计日志
- 子账号
prerequisites:
- kubectl-basics
- cloud-basics
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


# TKE 身份认证与 CAM 集成

## 1. 认证架构

```
TKE 身份认证体系：

腾讯云层面：
  ├── 主账号（Root Account）
  │   └── 完全控制权限，不建议日常使用
  │
  ├── 子账号（Sub-Account）
  │   └── 通过 CAM 策略控制权限
  │
  └── 协作者（Collaborator）
      └── 跨账号访问

TKE 集群层面：
  ├── K8s RBAC
  │   ├── Role / ClusterRole
  │   ├── RoleBinding / ClusterRoleBinding
  │   └── ServiceAccount
  │
  └── CAM 集成
      ├── Pod 绑定 CAM 角色
      └── ServiceAccount 映射 IAM 策略
```

## 2. CAM 角色绑定

### 2.1 创建 CAM 角色

```bash
# 创建信任策略（允许 TKE Pod 使用）
cat > trust-policy.json << 'EOF'
{
  "version": "2.0",
  "statement": [
    {
      "effect": "allow",
      "principal": {
        "qcs": ["qcs::cam::uin/100000000000:uin/100000000000"]
      },
      "action": "name/sts:AssumeRole",
      "resource": "*",
      "condition": {
        "string_equal": {
          "qcs:resource": "tke:cluster:cls-xxxxxxxx:namespace/production"
        }
      }
    }
  ]
}
EOF

# 创建角色
tccli cam CreateRole \
  --RoleName "tke-prod-app-role" \
  --PolicyDocument "$(cat trust-policy.json)" \
  --Description "TKE production app role for COS and CMQ access"
```

### 2.2 绑定角色到 ServiceAccount

```yaml
# ServiceAccount 注解中指定 CAM 角色
apiVersion: v1
kind: ServiceAccount
metadata:
  name: app-sa
  namespace: production
  annotations:
    # 绑定 CAM 角色
    eks.tke.cloud.tencent.com/role-arn: "qcs::cam::uin/100000000000:roleName/tke-prod-app-role"

---
# Deployment 使用该 ServiceAccount
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app-backend
  namespace: production
spec:
  template:
    spec:
      serviceAccountName: app-sa
      containers:
      - name: app
        image: app:v1.0
        env:
        - name: TKE_REGION
          value: "ap-guangzhou"
```

### 2.3 应用代码获取临时凭证

```python
# Python SDK 获取临时凭证
from tencentcloud.common import credential
from tencentcloud.sts.v20180813 import sts_client, models

# 方式 1: 使用环境变量（推荐，TKE 自动注入）
# TKE 会通过 Metadata Service 注入临时凭证
import requests
import json

def get_temp_credentials():
    """通过 TKE Metadata Service 获取临时凭证"""
    url = "http://metadata.tencentyun.com/meta-data/cam/security-credentials"
    resp = requests.get(url, timeout=5)
    return resp.json()

# 方式 2: 使用 SDK
creds = credential.DefaultCredentialProvider().get_credentials()
```

```go
// Go SDK
import (
    "github.com/tencentcloud/tencentcloud-sdk-go/tencentcloud/common"
    "github.com/tencentcloud/tencentcloud-sdk-go/tencentcloud/common/profile"
)

// DefaultCredentialProvider 自动从环境变量/Metadata 获取凭证
provider := common.DefaultCredentialProvider{}
cred, err := provider.GetCredential()
```

## 3. ServiceAccount IAM 策略

### 3.1 细粒度权限控制

```json
// CAM 策略：只允许访问特定 COS Bucket
{
  "version": "2.0",
  "statement": [
    {
      "effect": "allow",
      "action": [
        "cos:GetObject",
        "cos:PutObject",
        "cos:ListBucket"
      ],
      "resource": [
        "qcs::cos:ap-guangzhou:uid/100000000000:bucket-prod-data/*",
        "qcs::cos:ap-guangzhou:uid/100000000000:bucket-prod-data"
      ]
    },
    {
      "effect": "deny",
      "action": [
        "cos:DeleteBucket",
        "cos:PutBucketPolicy"
      ],
      "resource": "*"
    }
  ]
}
```

### 3.2 多服务权限隔离

```yaml
# 订单服务 - 只能访问订单相关的 COS 和数据库
apiVersion: v1
kind: ServiceAccount
metadata:
  name: order-sa
  namespace: production
  annotations:
    eks.tke.cloud.tencent.com/role-arn: "qcs::cam::uin/100000000000:roleName/order-service-role"

---
# 支付服务 - 只能访问支付网关和加密密钥
apiVersion: v1
kind: ServiceAccount
metadata:
  name: payment-sa
  namespace: production
  annotations:
    eks.tke.cloud.tencent.com/role-arn: "qcs::cam::uin/100000000000:roleName/payment-service-role"

---
# 日志服务 - 只能写入 CLS
apiVersion: v1
kind: ServiceAccount
metadata:
  name: logger-sa
  namespace: logging
  annotations:
    eks.tke.cloud.tencent.com/role-arn: "qcs::cam::uin/100000000000:roleName/logger-role"
```

## 4. K8s RBAC 配置

### 4.1 命名空间级权限

```yaml
# 开发团队角色：只能读取和部署
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: developer
  namespace: production
rules:
- apiGroups: ["apps", ""]
  resources: ["deployments", "pods", "services"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["apps"]
  resources: ["deployments"]
  verbs: ["create", "update", "patch"]
- apiGroups: [""]
  resources: ["pods/log"]
  verbs: ["get"]

---
# 绑定到子账号组
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: developer-binding
  namespace: production
subjects:
- kind: Group
  name: "developer-team"
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: developer
  apiGroup: rbac.authorization.k8s.io
```

### 4.2 集群级权限

```yaml
# SRE 角色：集群管理权限
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: sre-admin
rules:
- apiGroups: [""]
  resources: ["*"]
  verbs: ["*"]
- apiGroups: ["apps"]
  resources: ["*"]
  verbs: ["*"]
- apiGroups: ["rbac.authorization.k8s.io"]
  resources: ["roles", "rolebindings"]
  verbs: ["get", "list", "watch"]
# 不允许修改 RBAC
- apiGroups: ["rbac.authorization.k8s.io"]
  resources: ["clusterroles", "clusterrolebindings"]
  verbs: ["get", "list", "watch"]

---
# 只读监控角色
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: monitor-readonly
rules:
- apiGroups: [""]
  resources: ["pods", "nodes", "services", "endpoints"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["apps"]
  resources: ["deployments", "statefulsets"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["metrics.k8s.io"]
  resources: ["pods", "nodes"]
  verbs: ["get", "list"]
```

### 4.3 TKE 集成 CAM 的 RBAC 映射

```yaml
# TKE 支持将 CAM 用户/组映射到 K8s RBAC
# 在 TKE 控制台配置，或通过 API：

# 映射 CAM 子账号到 K8s ClusterRole
# CAM 子账号组 "sre-team" → K8s ClusterRole "sre-admin"
# CAM 子账号 "dev-user-01" → K8s Role "developer" (namespace: production)
```

## 5. TKE 审计日志

### 5.1 启用审计日志

```bash
# 启用集群审计日志
tccli tke EnableClusterAudit \
  --ClusterId "cls-xxxxxxxx" \
  --LogsetId "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" \
  --TopicId "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
```

### 5.2 审计日志查询

```sql
-- 查询所有敏感操作
SELECT *
FROM tke_audit_log
WHERE verb IN ('create', 'update', 'patch', 'delete')
  AND objectRef.resource IN ('secrets', 'roles', 'rolebindings', 'serviceaccounts')
  AND requestTimestamp > NOW() - INTERVAL 24 HOUR
ORDER BY requestTimestamp DESC

-- 查询特定用户的操作
SELECT *
FROM tke_audit_log
WHERE user.username = 'dev-user-01'
  AND requestTimestamp > NOW() - INTERVAL 7 DAY

-- 查询 RBAC 拒绝事件
SELECT *
FROM tke_audit_log
WHERE responseStatus.code = 403
  AND requestTimestamp > NOW() - INTERVAL 1 HOUR
```

### 5.3 审计告警

```yaml
# 敏感资源删除告警
# 在 CLS 中配置告警规则
# 条件：verb="delete" AND objectRef.resource="secrets"

# RBAC 提权告警
# 条件：verb="create" AND objectRef.resource IN ("clusterroles", "clusterrolebindings")
```

## 6. 子账号权限管理

### 6.1 权限分层模型

```
推荐的权限分层：

Tier 1 - 集群管理员（SRE Lead）
  ├── 完全集群管理权限
  ├── RBAC 管理权限
  └── 节点池管理权限

Tier 2 - 运维工程师（SRE）
  ├── 工作负载管理权限
  ├── 日志/监控查看权限
  └── 存储/网络管理权限

Tier 3 - 开发工程师
  ├── 命名空间级部署权限
  ├── Pod 日志查看权限
  └── ConfigMap/Secret 只读权限

Tier 4 - 只读用户
  ├── 资源查看权限
  └── 监控面板查看权限
```

### 6.2 命名空间隔离

```yaml
# 为每个团队创建独立命名空间
apiVersion: v1
kind: Namespace
metadata:
  name: team-frontend
  labels:
    team: frontend

---
apiVersion: v1
kind: Namespace
metadata:
  name: team-backend
  labels:
    team: backend

---
# ResourceQuota 限制
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-quota
  namespace: team-frontend
spec:
  hard:
    requests.cpu: "32"
    requests.memory: 64Gi
    limits.cpu: "64"
    limits.memory: 128Gi
    pods: "50"
    services: "10"
    persistentvolumeclaims: "20"
```

### 6.3 安全最佳实践

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
安全检查清单：

□ 主账号不用于日常操作
□ 所有子账号启用 MFA
□ 遵循最小权限原则
□ 定期审查权限（季度）
□ 启用审计日志并配置告警
□ ServiceAccount 绑定最小权限 CAM 角色
□ 禁止使用 default ServiceAccount
□ 生产环境禁止 kubectl edit（通过 GitOps）
□ Secret 使用 KMS 加密
□ 网络策略默认拒绝所有
```
## 7. 故障排查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 ServiceAccount 配置
kubectl get sa app-sa -n production -o yaml

# 检查 RBAC 权限
kubectl auth can-i create deployments --namespace production --as=system:serviceaccount:production:app-sa
kubectl auth can-i get secrets --namespace production --as=system:serviceaccount:production:app-sa

# 检查 Role/RoleBinding
kubectl get role,rolebinding -n production
kubectl describe rolebinding <name> -n production

# 检查 ClusterRole/ClusterRoleBinding
kubectl get clusterrolebinding | grep <user-or-group>

# CAM 角色验证
tccli cam GetRole --RoleName "tke-prod-app-role"
tccli cam ListAttachedRolePolicies --RoleName "tke-prod-app-role"

# 审计日志检查
# 在 CLS 控制台查询最近的审计事件
```
## Related

- [[05-tke-troubleshooting-playbook|TKE 故障排查手册]]
- [[02-tke-networking-vpc-cni|TKE 网络与 VPC-CNI]]

## See Also

- TKE CAM 集成文档
- K8s RBAC 官方文档
- 腾讯云 CAM 最佳实践


<!-- risk-assessed -->
