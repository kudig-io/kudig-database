---
title: CCE身份认证与IAM细粒度授权
description: 'CCE身份认证体系：IAM策略授权、企业项目隔离、统一身份认证(AAD)、API密钥与临时凭证管理'
summary: 'CCE身份认证体系：IAM策略授权、企业项目隔离、统一身份认证(AAD)、API密钥与临时凭证管理'
category: cloud-providers
tags:
- cloud
- k8s
- huawei-cce
- iam
- rbac
- security
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
- CCE IAM认证 是什么
- 如何配置CCE企业项目隔离
- CCE RBAC与IAM如何集成
trigger_keywords:
- CCE
- IAM
- RBAC
- 企业项目
- AAD
- API密钥
- 安全凭证
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

# CCE身份认证与IAM细粒度授权

## 1. 认证体系架构

CCE 身份认证基于华为云 IAM (Identity and Access Management)，并结合 Kubernetes 原生 RBAC 实现双重授权：

```
┌─────────────────────────────────────────────────────────┐
│                     用户/应用                            │
│  (IAM用户 / IAM用户组 / 联邦用户 / 委托账号)             │
└───────────────────────┬─────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────┐
│              华为云 IAM 认证层                            │
│  - AK/SK 签名认证                                        │
│  - Token 认证 (临时安全凭证)                             │
│  - 委托 (Agency) 认证                                    │
└───────────────────────┬─────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────┐
│              CCE API Gateway                              │
│  - 校验 IAM 权限 (CCE 全局操作)                          │
│  - 企业项目过滤                                          │
└───────────────────────┬─────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────┐
│              Kubernetes API Server                       │
│  - IAM 用户映射 → K8s RBAC                               │
│  - ClusterRole / Role 绑定                               │
│  - 命名空间隔离                                          │
└─────────────────────────────────────────────────────────┘
```

## 2. IAM 权限策略

### 2.1 系统策略

CCE 提供以下预置系统策略：

| 策略名称 | 权限说明 |
|---------|---------|
| CCE Administrator | CCE 完全管理权限（集群、节点、工作负载等） |
| CCE FullAccess | CCE 所有只读 + 写操作 |
| CCE ReadOnlyAccess | CCE 所有资源只读访问 |
| CCE TunnelFullAccess | CCE 隧道代理完全管理 |
| CCE TunnelReadOnlyAccess | CCE 隧道代理只读访问 |

### 2.2 自定义策略

```json
{
  "Version": "1.1",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "cce:cluster:get",
        "cce:cluster:list"
      ],
      "Resource": [
        "*"
      ]
    },
    {
      "Effect": "Allow",
      "Action": [
        "cce:node:list",
        "cce:node:get"
      ],
      "Resource": [
        "cce:cluster:<cluster-id>:*"
      ],
      "Condition": {
        "StringEquals": {
          "g:EnterpriseProjectId": ["ep-001"]
        }
      }
    }
  ]
}
```

### 2.3 细粒度操作权限

```json
{
  "Version": "1.1",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "cce:workload:create",
        "cce:workload:update",
        "cce:workload:delete",
        "cce:workload:get",
        "cce:workload:list"
      ],
      "Resource": [
        "cce:cluster:<cluster-id>:namespace/<namespace>:workload/*"
      ]
    },
    {
      "Effect": "Deny",
      "Action": [
        "cce:node:delete",
        "cce:node:update"
      ],
      "Resource": [
        "*"
      ]
    }
  ]
}
```

## 3. 企业项目隔离

### 3.1 企业项目概述

企业项目是华为云的资源隔离机制，实现多团队/多业务的资源和权限隔离：

```
企业项目 A (生产环境)          企业项目 B (开发环境)
├── CCE 集群 prod-cluster      ├── CCE 集群 dev-cluster
├── ECS 实例                   ├── ECS 实例
├── RDS 实例                   ├── RDS 实例
└── EVS 卷                     └── EVS 卷

IAM 用户组 A → 绑定企业项目 A 策略
IAM 用户组 B → 绑定企业项目 B 策略
```

### 3.2 集群与企业项目绑定

```bash
# 创建集群时指定企业项目
# 控制台 → 创建集群 → 高级配置 → 企业项目

# 通过 API 创建集群时指定
{
  "kind": "Cluster",
  "spec": {
    "enterpriseProjectId": "ep-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
  }
}

# 集群创建后查看企业项目
kubectl get cluster <cluster-id> -o jsonpath='{.spec.enterpriseProjectId}'
```

### 3.3 命名空间级别隔离

```yaml
# 通过 IAM + RBAC 实现命名空间级隔离
# 步骤 1: 创建命名空间
apiVersion: v1
kind: Namespace
metadata:
  name: team-a
  labels:
    enterprise-project: ep-001
    team: team-a

---
# 步骤 2: 创建 RBAC Role
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: team-a-admin
  namespace: team-a
rules:
  - apiGroups: ["", "apps", "batch"]
    resources: ["*"]
    verbs: ["*"]

---
# 步骤 3: 绑定到 IAM 用户组映射的 K8s Group
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: team-a-admin-binding
  namespace: team-a
subjects:
  - kind: Group
    name: "iam-team-a"      # IAM 用户组映射的 K8s Group
    apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: team-a-admin
  apiGroup: rbac.authorization.k8s.io
```

### 3.4 资源配额

```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-a-quota
  namespace: team-a
spec:
  hard:
    requests.cpu: "20"
    requests.memory: 40Gi
    limits.cpu: "40"
    limits.memory: 80Gi
    pods: "50"
    services: "20"
    persistentvolumeclaims: "10"
---
apiVersion: v1
kind: LimitRange
metadata:
  name: team-a-limits
  namespace: team-a
spec:
  limits:
    - default:
        cpu: "500m"
        memory: 512Mi
      defaultRequest:
        cpu: "100m"
        memory: 128Mi
      type: Container
```

## 4. 统一身份认证 (AAD 集成)

### 4.1 企业 IdP 联邦认证

CCE 支持通过华为云 AAD (Application Admin Directory) 对接企业 LDAP/AD：

```
企业用户 → 企业 IdP (LDAP/AD/SAML)
         → 华为云 AAD 联邦认证
         → IAM Token
         → CCE API Server
         → RBAC 授权
```

### 4.2 联邦认证配置

```bash
# 1. 在华为云 IAM 控制台创建身份提供商
#    - 类型: SAML 2.0 或 OIDC
#    - 上传 IdP 元数据

# 2. 创建映射规则
#    IAM 联邦用户 → IAM 用户组 → IAM 策略

# 3. 用户通过 SSO 获取临时 Token
#    curl -X POST https://iam.myhuaweicloud.com/v3/auth/tokens \
#      -d '{
#        "auth": {
#          "identity": {
#            "methods": ["token"],
#            "token": { "id": "<federation-token>" }
#          },
#          "scope": {
#            "project": { "name": "cn-north-4" }
#          }
#        }
#      }'
```

### 4.3 OIDC Token 集成

```yaml
# 使用 OIDC Token 访问 CCE (适用于 CI/CD)
# 环境变量:
#   HUAWEI_CLOUD_OIDC_TOKEN: 由 CI/CD 平台注入的 OIDC Token

# kubectl 配置
apiVersion: v1
kind: Config
clusters:
  - cluster:
      server: https://<cluster-endpoint>
      certificate-authority-data: <ca-data>
    name: my-cluster
users:
  - name: oidc-user
    user:
      auth-provider:
        name: oidc
        config:
          client-id: "cce-client"
          idp-issuer-url: "https://auth.example.com"
          id-token: "${HUAWEI_CLOUD_OIDC_TOKEN}"
contexts:
  - context:
      cluster: my-cluster
      user: oidc-user
    name: my-context
```

## 5. API 密钥与临时凭证

### 5.1 AK/SK 长期凭证

```bash
# 创建 IAM 用户的 AK/SK
# 控制台 → IAM → 用户 → 安全凭证 → 访问密钥

# 配置 kubectl 使用 AK/SK
# 方式一: 环境变量
export HUAWEI_ACCESS_KEY_ID="AK..."
export HUAWEI_SECRET_ACCESS_KEY="SK..."

# 方式二: 配置文件 (~/.huaweicloud/credentials)
[default]
ak = AK...
sk = SK...
region = cn-north-4
project_id = <project-id>
```

### 5.2 临时安全凭证 (STS)

```bash
# 通过 AssumeRole 获取临时凭证 (推荐用于应用)
curl -X POST https://iam.myhuaweicloud.com/v3.0/OS-CREDENTIAL/assume \
  -H "Content-Type: application/json" \
  -H "X-Auth-Token: <iam-token>" \
  -d '{
    "duration_seconds": 3600,
    "assume_role": {
      "role_name": "cce-operator",
      "domain_name": "my-domain"
    }
  }'

# 返回:
# {
#   "access_key": "临时 AK",
#   "secret_key": "临时 SK",
#   "security_token": "临时 Token",
#   "expires_at": "2026-07-02T12:00:00Z"
# }
```

### 5.3 委托 (Agency) 认证

```bash
# 创建 CCE 委托 (集群使用委托访问其他云服务)
# 控制台 → IAM → 委托 → 创建委托
# 委托类型: 云服务
# 被委托方: CCE
# 委托权限: 根据需要选择 (如 OBS ReadOnly、EVS FullAccess 等)

# 集群关联委托
# 创建集群时选择已创建的委托

# 验证委托权限 (Pod 内通过 metadata 获取)
curl -s http://169.254.169.254/openstack/latest/meta_data.json | jq '.meta'
```

## 6. Kubernetes RBAC 集成

### 6.1 IAM 用户到 K8s RBAC 的映射

```
IAM 用户 → IAM 用户组 → K8s Group → ClusterRole/Role
```

```yaml
# 开发者角色 - 限制在特定命名空间
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: developer
  namespace: development
rules:
  - apiGroups: ["", "apps", "batch"]
    resources: ["pods", "services", "deployments", "configmaps", "secrets", "jobs"]
    verbs: ["get", "list", "watch", "create", "update", "patch"]
  - apiGroups: [""]
    resources: ["pods/log", "pods/exec"]
    verbs: ["get", "create"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: developer-binding
  namespace: development
subjects:
  - kind: Group
    name: "iam-developers"    # IAM 用户组名称映射
    apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: developer
  apiGroup: rbac.authorization.k8s.io
```

### 6.2 SRE 全局只读

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: sre-readonly
rules:
  - apiGroups: ["*"]
    resources: ["*"]
    verbs: ["get", "list", "watch"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: sre-readonly-binding
subjects:
  - kind: Group
    name: "iam-sre-team"
    apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: ClusterRole
  name: sre-readonly
  apiGroup: rbac.authorization.k8s.io
```

### 6.3 审计 RBAC 配置

```bash
# 检查某用户的权限
kubectl auth can-i create deployments --namespace production --as=iam-user@example.com

# 列出所有 RoleBinding
kubectl get rolebindings -A -o wide

# 查看某用户的所有权限
kubectl get clusterrolebindings,rolebindings -A -o json | \
  jq '.items[] | select(.subjects[]?.name=="iam-user@example.com")'
```

## 7. 安全最佳实践

### 7.1 最小权限原则

```
用户角色          IAM 策略                    K8s RBAC
─────────────────────────────────────────────────────────
集群管理员        CCE Administrator           cluster-admin
SRE 工程师        CCE FullAccess              自定义 ClusterRole (只读 + 有限操作)
开发者            CCE FullAccess              Namespace Role (限制资源类型)
CI/CD 流水线      CCE FullAccess + 委托       ServiceAccount + Role
监控系统          CCE ReadOnlyAccess          ClusterRole (get/list/watch)
```

### 7.2 凭证轮换

```bash
# AK/SK 轮换策略
# 1. 创建新 AK/SK
# 2. 更新所有使用旧 AK/SK 的应用
# 3. 验证新凭证工作正常
# 4. 删除旧 AK/SK
# 建议周期: 90 天

# STS 临时凭证
# 自动过期，无需手动轮换
# 建议有效期: 1 小时 (最长 24 小时)
```

### 7.3 审计日志

```yaml
# CCE 审计日志配置
# 在集群配置中启用 API Server 审计
# 控制台 → 集群 → 配置中心 → 审计日志

# 审计策略示例
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
  # 记录所有写操作
  - level: RequestResponse
    verbs: ["create", "update", "patch", "delete"]
    resources:
      - group: ""
        resources: ["secrets", "configmaps"]
  # 记录认证失败
  - level: Metadata
    stages:
      - ResponseComplete
    omitStages:
      - RequestReceived
```

## 8. 常见问题排查

### 8.1 权限不足

```bash
# 错误: Forbidden (403)
# 排查步骤:
# 1. 确认 IAM 用户/组是否有对应系统策略
# 2. 确认企业项目是否匹配
# 3. 确认 K8s RBAC 绑定是否正确

kubectl auth can-i <verb> <resource> --namespace <ns> --as=<iam-user>
```

### 8.2 Token 过期

```bash
# 错误: Unauthorized (401)
# Token 默认有效期 24 小时

# 重新获取 Token
# 控制台 → 右上角 → API凭证 → 获取 Token

# 配置 kubectl 刷新 Token
# 使用 kubelogin 插件自动刷新
```

### 8.3 委托权限不足

```bash
# 错误: Pod 内访问其他云服务失败
# 排查:
# 1. 确认集群已关联委托
# 2. 确认委托包含所需权限
# 3. 确认 Pod 注解包含委托信息

kubectl get pod <pod-name> -o jsonpath='{.metadata.annotations}'
```

---

*本文档描述 CCE 身份认证与授权体系。具体参数以华为云官方文档为准。*
