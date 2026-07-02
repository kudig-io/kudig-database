---
title: EKS 身份认证 — IRSA 与 Pod Identity
description: 'EKS IRSA 配置、Pod Identity Agent、最小权限 IAM 策略设计及 Cross-Account 访问'
summary: 'EKS IRSA 配置、Pod Identity Agent、最小权限 IAM 策略设计及 Cross-Account 访问'
category: cloud-providers
tags:
- cloud
- k8s
- aws
- eks
- iam
- irsa
- security
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 安全工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- EKS IRSA 是什么
- 如何配置 EKS Pod Identity
trigger_keywords:
- irsa
- pod-identity
- iam-roles-for-service-accounts
- cross-account
- least-privilege
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

# EKS 身份认证 — IRSA 与 Pod Identity

## 1. IRSA（IAM Roles for Service Accounts）

IRSA 通过 OIDC Federation 将 IAM Role 绑定到 Kubernetes Service Account，是 EKS 推荐的 Pod 级身份认证方式。

### 1.1 IRSA 工作原理

```
1. EKS 集群启用 OIDC Provider
2. 创建 IAM Role，Trust Policy 指向 OIDC Provider
3. IAM Role 通过 Condition 限定特定 Service Account
4. Pod 使用该 Service Account 运行
5. VPC CNI 自动注入 AWS_WEB_IDENTITY_TOKEN_FILE 和 AWS_ROLE_ARN
6. AWS SDK 使用 Token 向 STS AssumeRoleWithWebIdentity
7. 获取临时凭证访问 AWS 服务
```

### 1.2 启用 OIDC Provider

```bash
# 检查是否已启用
aws eks describe-cluster --name prod-cluster \
  --query "cluster.identity.oidc.issuer" --output text

# 创建 OIDC Provider（如果未启用）
eksctl utils associate-iam-oidc-provider \
  --cluster prod-cluster \
  --approve

# 验证
aws iam list-open-id-connect-providers \
  | jq '.OpenIDConnectProviderList[] | select(.Arn | contains("prod-cluster"))'
```

### 1.3 创建 IRSA Role

```bash
# eksctl 方式（推荐）
eksctl create iamserviceaccount \
  --name s3-reader \
  --namespace production \
  --cluster prod-cluster \
  --role-name eks-s3-reader-role \
  --attach-policy-arn arn:aws:iam::aws:policy/AmazonS3ReadOnlyAccess \
  --approve

# Terraform 方式
module "irsa_s3_reader" {
  source = "terraform-aws-modules/iam/aws//modules/iam-role-for-service-accounts-eks"
  
  role_name = "eks-s3-reader-role"
  
  attach_s3_read_only_policy = true
  
  oidc_providers = {
    main = {
      provider_arn               = module.eks.oidc_provider_arn
      namespace_service_accounts = ["production:s3-reader"]
    }
  }
}
```

### 1.4 手动创建 IRSA（理解原理）

```yaml
# Step 1: 创建 IAM Role
# Trust Policy（关键）
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Principal": {
        "Federated": "arn:aws:iam::123456789012:oidc-provider/oidc.eks.ap-southeast-1.amazonaws.com/id/EXAMPLED539D4633E53DE1B71EXAMPLE"
      },
      "Action": "sts:AssumeRoleWithWebIdentity",
      "Condition": {
        "StringEquals": {
          "oidc.eks.ap-southeast-1.amazonaws.com/id/EXAMPLED539D4633E53DE1B71EXAMPLE:sub": "system:serviceaccount:production:s3-reader",
          "oidc.eks.ap-southeast-1.amazonaws.com/id/EXAMPLED539D4633E53DE1B71EXAMPLE:aud": "sts.amazonaws.com"
        }
      }
    }
  ]
}

---
# Step 2: Service Account 注解
apiVersion: v1
kind: ServiceAccount
metadata:
  name: s3-reader
  namespace: production
  annotations:
    eks.amazonaws.com/role-arn: arn:aws:iam::123456789012:role/eks-s3-reader-role
```

### 1.5 Pod 使用 IRSA

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: s3-reader-app
  namespace: production
spec:
  template:
    spec:
      serviceAccountName: s3-reader
      containers:
        - name: app
          image: my-app:latest
          env:
            # 以下环境变量由 VPC CNI webhook 自动注入
            # AWS_ROLE_ARN=arn:aws:iam::123456789012:role/eks-s3-reader-role
            # AWS_WEB_IDENTITY_TOKEN_FILE=/var/run/secrets/eks.amazonaws.com/serviceaccount/token
            - name: AWS_REGION
              value: ap-southeast-1
```

## 2. Pod Identity Agent（新方式）

Pod Identity 是 AWS 2023 年推出的新方案，简化了 IRSA 的配置流程。

### 2.1 安装 Pod Identity Agent

```bash
# 通过 EKS Addon 安装
aws eks create-addon \
  --cluster-name prod-cluster \
  --addon-name eks-pod-identity-agent \
  --addon-version v1.3.0-eksbuild.1

# 验证安装
kubectl get pods -n kube-system -l app.kubernetes.io/name=eks-pod-identity-agent
```

### 2.2 创建 Pod Identity Association

```bash
# 创建 IAM Role（Trust Policy 不同于 IRSA）
# Trust Policy 中 Principal 为 pods.eks.amazonaws.com

# 关联 Role 和 Service Account
aws eks create-pod-identity-association \
  --cluster-name prod-cluster \
  --namespace production \
  --service-account s3-reader \
  --role-arn arn:aws:iam::123456789012:role/eks-pod-identity-s3-reader
```

```json
// Pod Identity Trust Policy
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Principal": {
        "Service": "pods.eks.amazonaws.com"
      },
      "Action": [
        "sts:AssumeRole",
        "sts:TagSession"
      ],
      "Condition": {
        "StringEquals": {
          "aws:SourceArn": "arn:aws:eks:ap-southeast-1:123456789012:cluster/prod-cluster"
        }
      }
    }
  ]
}
```

### 2.3 IRSA vs Pod Identity 对比

| 特性 | IRSA | Pod Identity |
|------|------|-------------|
| 依赖 OIDC | 是 | 否 |
| 跨账户支持 | 原生支持 | 需额外配置 |
| 注入方式 | Webhook 环境变量 | Agent 元数据服务 |
| Token 轮换 | 由 kubelet 处理 | Agent 自动处理 |
| 命名空间限制 | Trust Policy 中指定 | API 关联时指定 |
| 多集群管理 | 每集群独立 OIDC | 统一 Principal |
| 推荐场景 | 跨账户、复杂策略 | 新集群、简单场景 |

## 3. 最小权限 IAM 策略设计

### 3.1 策略模板

```json
// S3 只读策略 — 限定 Bucket 和路径
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Sid": "S3ReadSpecificBucket",
      "Effect": "Allow",
      "Action": [
        "s3:GetObject",
        "s3:ListBucket",
        "s3:GetBucketLocation"
      ],
      "Resource": [
        "arn:aws:s3:::prod-data-bucket",
        "arn:aws:s3:::prod-data-bucket/app-data/*"
      ]
    }
  ]
}

// DynamoDB 读写策略 — 限定表名
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Sid": "DynamoDBSpecificTable",
      "Effect": "Allow",
      "Action": [
        "dynamodb:GetItem",
        "dynamodb:PutItem",
        "dynamodb:UpdateItem",
        "dynamodb:Query",
        "dynamodb:BatchGetItem",
        "dynamodb:BatchWriteItem"
      ],
      "Resource": [
        "arn:aws:dynamodb:ap-southeast-1:123456789012:table/prod-orders",
        "arn:aws:dynamodb:ap-southeast-1:123456789012:table/prod-orders/index/*"
      ]
    }
  ]
}

// SQS 策略 — 限定队列
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Sid": "SQSSpecificQueue",
      "Effect": "Allow",
      "Action": [
        "sqs:SendMessage",
        "sqs:ReceiveMessage",
        "sqs:DeleteMessage",
        "sqs:GetQueueAttributes"
      ],
      "Resource": "arn:aws:sqs:ap-southeast-1:123456789012:prod-task-queue"
    }
  ]
}
```

### 3.2 权限审计

```bash
# 查看 IAM Role 使用情况
aws iam generate-service-last-accessed-details \
  --arn arn:aws:iam::123456789012:role/eks-s3-reader-role

# 获取访问报告
aws iam get-service-last-accessed-details \
  --job-id <job-id>

# 使用 IAM Access Analyzer
aws accessanalyzer create-analyzer \
  --analyzer-name eks-role-analyzer \
  --type ACCOUNT
```

### 3.3 策略版本控制

```yaml
# OPA/Gatekeeper 策略 — 禁止通配符权限
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sIAMPolicyNoWildcard
metadata:
  name: deny-wildcard-iam
spec:
  match:
    kinds:
      - apiGroups: ["iam.aws.crossplane.io"]
        kinds: ["Policy"]
  parameters:
    exemptRoles:
      - "system:*"
```

## 4. Cross-Account 访问

### 4.1 AssumeRole 模式

```json
// 账户 A 的 Role Trust Policy — 允许账户 B 的 EKS Role 假设
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Principal": {
        "AWS": "arn:aws:iam::ACCOUNT_B_ID:role/eks-app-role"
      },
      "Action": "sts:AssumeRole",
      "Condition": {
        "StringEquals": {
          "sts:ExternalId": "cross-account-access-id"
        }
      }
    }
  ]
}
```

### 4.2 IRSA Cross-Account 配置

```yaml
# 账户 B 的 Service Account — 假设账户 A 的 Role
apiVersion: v1
kind: ServiceAccount
metadata:
  name: cross-account-s3
  namespace: production
  annotations:
    # 指向账户 A 的 Role
    eks.amazonaws.com/role-arn: arn:aws:iam::ACCOUNT_A_ID:role/cross-account-s3-role

---
# Pod 中配置 STS AssumeRole
apiVersion: apps/v1
kind: Deployment
spec:
  template:
    spec:
      serviceAccountName: cross-account-s3
      containers:
        - name: app
          env:
            - name: AWS_ROLE_ARN
              value: arn:aws:iam::ACCOUNT_A_ID:role/cross-account-s3-role
            - name: AWS_DEFAULT_REGION
              value: ap-southeast-1
```

### 4.3 VPC Endpoint 跨账户共享

```bash
# 账户 A 创建 VPC Endpoint Service
aws ec2 create-vpc-endpoint-service-configuration \
  --network-load-balancer-arns arn:aws:elasticloadbalancing:... \
  --acceptance-required \
  --allowed-principals arn:aws:iam::ACCOUNT_B_ID:root

# 账户 B 创建 VPC Endpoint
aws ec2 create-vpc-endpoint \
  --vpc-id vpc-account-b \
  --vpc-endpoint-type Interface \
  --service-name com.amazonaws.vpce.ap-southeast-1.vpce-svc-0123456789abcdef0 \
  --subnet-ids subnet-aaaa subnet-bbbb
```

## 5. Secrets 管理集成

### 5.1 Secrets Store CSI Driver + AWS Provider

```bash
# 安装 Secrets Store CSI Driver
helm install csi-secrets-store \
  secrets-store-csi-driver/secrets-store-csi-driver \
  --namespace kube-system \
  --set syncSecret.enabled=true

# 安装 AWS Provider
helm install aws-secrets-manager \
  aws-secrets-manager/secrets-store-csi-driver-provider-aws \
  --namespace kube-system
```

```yaml
# SecretProviderClass
apiVersion: secrets-store.csi.x-k8s.io/v1
kind: SecretProviderClass
metadata:
  name: aws-secrets
  namespace: production
spec:
  provider: aws
  parameters:
    objects: |
      - objectName: "prod/database/credentials"
        objectType: "secretsmanager"
        jmesPath:
          - path: username
            objectAlias: db-username
          - path: password
            objectAlias: db-password
  secretObjects:
    - secretName: db-credentials
      type: Opaque
      data:
        - objectName: db-username
          key: username
        - objectName: db-password
          key: password
```

## Related

- [[02-eks-cluster-lifecycle-management]]
- [[06-eks-troubleshooting-playbook]]

## See Also

- AWS IRSA 文档
- EKS Pod Identity 文档
- IAM Best Practices
