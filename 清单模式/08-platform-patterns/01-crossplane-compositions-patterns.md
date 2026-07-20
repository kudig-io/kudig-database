---
title: "Crossplane 组合模式：Composition/XRD 设计与平台抽象"
description: "Crossplane Composition 和 XRD 设计模式，Provider 配置、平台抽象层构建及与 Terraform 对比"
summary: "系统讲解 Crossplane 的组合模式：CompositeResourceDefinition 设计、Composition 模板编写、Provider 配置管理、平台抽象层构建，以及与 Terraform 的定位差异和协作方式"
category: 清单模式
tags:
- crossplane
- composition
- xrd
- platform-engineering
- iac
- provider
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
estimated_read_time: 20min
intent_queries:
- "Crossplane Composition 怎么设计"
- "XRD 和 Composition 是什么关系"
- "Crossplane 和 Terraform 怎么选"
trigger_keywords:
- crossplane
- composition
- xrd
- provider
- platform-abstraction
prerequisites:
- kubectl-basics
- k8s-crd
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

# Crossplane 组合模式

## 概述

Crossplane 是 CNCF 毕业项目，将 Kubernetes API 扩展为通用的基础设施控制面。通过 CompositeResourceDefinition（XRD）和 Composition，平台团队可以定义自定义 API（如 `XPostgreSQLInstance`），将底层云资源（RDS/Cloud SQL/Azure Database）抽象为统一的 Kubernetes 资源，开发者只需提交一个 Claim 即可获得完整的基础设施。

Crossplane 的核心价值是**平台抽象**：将"基础设施即代码"从 Terraform 的 HCL 脚本提升为 Kubernetes 原生的声明式 API，具备 GitOps 友好、持续调谐（reconcile）、RBAC 集成和可扩展性。

## 核心概念

### Crossplane 架构

```
开发者提交 Claim（XR 实例）
    ↓
Crossplane Controller
    ↓ 匹配 Composition
Composition（模板）
    ↓ 渲染
Managed Resources（MR）
    ↓ Provider 调谐
云 API（AWS/GCP/Azure/...）

核心组件：
├── Provider（云厂商适配器）：provider-aws, provider-gcp, provider-azure
├── XRD（自定义 API 定义）：定义 Claim 的 schema
├── Composition（组合模板）：定义 XR 如何映射到 MR
├── Claim（用户请求）：开发者提交的资源请求
└── Managed Resource（MR）：实际的云资源
```

### XRD vs Composition vs Claim

| 概念 | 角色 | 类比 | 谁创建 |
|------|------|------|--------|
| XRD | API 定义 | CRD | 平台团队 |
| Composition | 实现模板 | Helm Template | 平台团队 |
| Claim (XR) | 用户请求 | Helm Values | 开发者 |
| Managed Resource | 实际资源 | K8s 资源 | Crossplane 自动 |
| Provider | 云适配器 | CSI Driver | 平台团队 |

### Crossplane vs Terraform

| 维度 | Crossplane | Terraform |
|------|-----------|-----------|
| 运行模式 | 持续调谐（Controller Loop） | 一次性执行（Plan/Apply） |
| 状态管理 | K8s etcd（实时状态） | State 文件（快照） |
| 漂移检测 | 持续（默认 10min 轮询） | 手动（terraform plan） |
| 抽象能力 | XRD + Composition（强类型） | Module（弱类型） |
| 多租户 | K8s RBAC + Namespace | Workspace / 目录隔离 |
| GitOps | 原生（ArgoCD/Flux） | 需额外工具（Atlantis） |
| 学习曲线 | 高（K8s + Crossplane 概念） | 中（HCL + Provider） |
| 适用场景 | 平台工程、自助服务 | 一次性基础设施、脚本化 |
| 回滚 | K8s 声明式回滚 | State 回滚（复杂） |
| 扩展性 | 自定义 Provider（Go） | 自定义 Provider（Go） |

## 生产部署

### Crossplane 安装与 Provider 配置

```yaml
# 🟡 中风险：安装 Crossplane 和 AWS Provider
# helm install crossplane crossplane-stable/crossplane -n crossplane-system --create-namespace

# AWS Provider 配置
apiVersion: pkg.crossplane.io/v1
kind: Provider
metadata:
  name: provider-aws
spec:
  package: xpkg.upbound.io/crossplane-contrib/provider-aws:v1.1.0
---
# ProviderConfig（AWS 凭证）
apiVersion: aws.upbound.io/v1beta1
kind: ProviderConfig
metadata:
  name: default
spec:
  credentials:
    source: Secret
    secretRef:
      namespace: crossplane-system
      name: aws-credentials
      key: credentials
---
# AWS 凭证 Secret
apiVersion: v1
kind: Secret
metadata:
  name: aws-credentials
  namespace: crossplane-system
type: Opaque
stringData:
  credentials: |
    [default]
    aws_access_key_id = AKIAIOSFODNN7EXAMPLE
    aws_secret_access_key = wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY
    region = us-east-1
```

### XRD 设计（自定义 API）

```yaml
# 🟡 中风险：定义 XRD（平台 API）
apiVersion: apiextensions.crossplane.io/v1
kind: CompositeResourceDefinition
metadata:
  name: xpostgresqlinstances.database.platform.io
spec:
  group: database.platform.io
  names:
    kind: XPostgreSQLInstance
    plural: xpostgresqlinstances
  claimNames:
    kind: PostgreSQLInstance
    plural: postgresqlinstances
  connectionSecretKeys:
  - username
  - password
  - endpoint
  - port
  versions:
  - name: v1alpha1
    served: true
    referenceable: true
    schema:
      openAPIV3Schema:
        type: object
        properties:
          spec:
            type: object
            properties:
              parameters:
                type: object
                properties:
                  version:
                    type: string
                    description: "PostgreSQL 版本"
                    enum: ["14", "15", "16"]
                    default: "16"
                  storageGB:
                    type: integer
                    description: "存储大小（GB）"
                    minimum: 20
                    maximum: 10000
                    default: 100
                  instanceSize:
                    type: string
                    description: "实例规格"
                    enum: ["small", "medium", "large"]
                    default: "small"
                  region:
                    type: string
                    description: "部署区域"
                    default: "us-east-1"
                  multiAZ:
                    type: boolean
                    description: "是否多 AZ"
                    default: false
                required:
                - version
                - storageGB
            required:
            - parameters
          status:
            type: object
            properties:
              ready:
                type: boolean
              endpoint:
                type: string
  # 组合标签（用于匹配 Composition）
  defaultCompositionRef:
    name: xpostgresqlinstance-aws
```

### Composition 设计（实现模板）

```yaml
# 🟡 中风险：Composition（AWS RDS 实现）
apiVersion: apiextensions.crossplane.io/v1
kind: Composition
metadata:
  name: xpostgresqlinstance-aws
  labels:
    provider: aws
    environment: production
spec:
  compositeTypeRef:
    apiVersion: database.platform.io/v1alpha1
    kind: XPostgreSQLInstance
  mode: Pipeline
  pipeline:
  - step: patch-and-transform
    functionRef:
      name: function-patch-and-transform
    input:
      apiVersion: pt.fn.crossplane.io/v1beta1
      kind: Resources
      resources:
      # VPC Security Group
      - name: securitygroup
        base:
          apiVersion: ec2.aws.upbound.io/v1beta1
          kind: SecurityGroup
          spec:
            forProvider:
              region: us-east-1
              description: "PostgreSQL security group"
              vpcIdSelector:
                matchControllerRef: true
        patches:
        - type: FromCompositeFieldPath
          fromFieldPath: spec.parameters.region
          toFieldPath: spec.forProvider.region
      # Security Group Rule（仅允许 VPC 内访问）
      - name: securitygrouprule
        base:
          apiVersion: ec2.aws.upbound.io/v1beta1
          kind: SecurityGroupIngressRule
          spec:
            forProvider:
              ipProtocol: tcp
              fromPort: 5432
              toPort: 5432
              cidrIpv4: "10.0.0.0/8"
        patches:
        - type: FromCompositeFieldPath
          fromFieldPath: spec.parameters.region
          toFieldPath: spec.forProvider.region
      # RDS Subnet Group
      - name: dbsubnetgroup
        base:
          apiVersion: rds.aws.upbound.io/v1beta1
          kind: DBSubnetGroup
          spec:
            forProvider:
              region: us-east-1
              description: "Managed by Crossplane"
              subnetIdRefs:
              - name: subnet-a
              - name: subnet-b
        patches:
        - type: FromCompositeFieldPath
          fromFieldPath: spec.parameters.region
          toFieldPath: spec.forProvider.region
      # RDS Instance
      - name: rdsinstance
        base:
          apiVersion: rds.aws.upbound.io/v1beta1
          kind: Instance
          spec:
            forProvider:
              region: us-east-1
              engine: postgres
              engineVersion: "16"
              instanceClass: db.t3.micro
              allocatedStorage: 100
              dbSubnetGroupNameSelector:
                matchControllerRef: true
              vpcSecurityGroupIDSelector:
                matchControllerRef: true
              skipFinalSnapshot: true
              publiclyAccessible: false
              storageEncrypted: true
              autoMinorVersionUpgrade: true
            writeConnectionSecretToRef:
              namespace: crossplane-system
        patches:
        - type: FromCompositeFieldPath
          fromFieldPath: spec.parameters.version
          toFieldPath: spec.forProvider.engineVersion
        - type: FromCompositeFieldPath
          fromFieldPath: spec.parameters.storageGB
          toFieldPath: spec.forProvider.allocatedStorage
        - type: FromCompositeFieldPath
          fromFieldPath: spec.parameters.instanceSize
          toFieldPath: spec.forProvider.instanceClass
          transforms:
          - type: map
            map:
              small: db.t3.micro
              medium: db.m5.large
              large: db.m5.2xlarge
        - type: FromCompositeFieldPath
          fromFieldPath: spec.parameters.region
          toFieldPath: spec.forProvider.region
        - type: FromCompositeFieldPath
          fromFieldPath: spec.parameters.multiAZ
          toFieldPath: spec.forProvider.multiAZ
        - type: ToCompositeFieldPath
          fromFieldPath: status.atProvider.endpoint
          toFieldPath: status.endpoint
        connectionDetails:
        - type: FromConnectionSecretKey
          name: username
          key: username
        - type: FromConnectionSecretKey
          name: password
          key: password
        - type: FromFieldPath
          name: endpoint
          fromFieldPath: status.atProvider.endpoint
        - type: FromValue
          name: port
          value: "5432"
```

### 开发者 Claim（自助服务）

```yaml
# 🟢 低风险：开发者提交 Claim
apiVersion: database.platform.io/v1alpha1
kind: PostgreSQLInstance
metadata:
  name: my-app-db
  namespace: team-backend
spec:
  parameters:
    version: "16"
    storageGB: 200
    instanceSize: medium
    region: us-east-1
    multiAZ: true
  # 连接信息写入 Secret
  writeConnectionSecretToRef:
    name: my-app-db-conn
---
# 应用使用连接信息
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
  namespace: team-backend
spec:
  replicas: 2
  selector:
    matchLabels:
      app: my-app
  template:
    metadata:
      labels:
        app: my-app
    spec:
      containers:
      - name: app
        image: registry.example.com/my-app:v1
        env:
        - name: DATABASE_URL
          valueFrom:
            secretKeyRef:
              name: my-app-db-conn
              key: endpoint
        - name: DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: my-app-db-conn
              key: password
```

## 运维操作

### 状态检查

```bash
# 🟢 低风险：Crossplane 状态检查
# 查看 Provider 状态
kubectl get providers
kubectl get providerconfigs

# 查看 XRD
kubectl get xrd

# 查看 Composition
kubectl get compositions

# 查看 Claim 状态
kubectl get postgresqlinstances -A
kubectl describe postgresqlinstance my-app-db -n team-backend

# 查看 Managed Resources
kubectl get managed
kubectl get instances.rds.aws.upbound.io

# 查看事件（调谐错误）
kubectl get events -n crossplane-system --sort-by='.lastTimestamp' | tail -20
```

### 漂移检测与修复

```bash
# 🟢 低风险：漂移检测
# Crossplane 持续调谐，自动检测并修复漂移
# 查看最近调谐
kubectl get managed -o custom-columns=\
NAME:.metadata.name,READY:.status.conditions[?(@.type=="Ready")].status,SYNCED:.status.conditions[?(@.type=="Synced")].status

# 手动触发调谐
kubectl annotate managed <resource-name> crossplane.io/force-reconcile=$(date +%s) --overwrite

# 查看调谐日志
kubectl logs -n crossplane-system -l app=crossplane --tail=100
```

### 版本升级

```bash
# 🔴 高风险：Crossplane/Provider 升级
# 升级 Crossplane 核心
helm upgrade crossplane crossplane-stable/crossplane -n crossplane-system

# 升级 Provider
kubectl patch provider provider-aws --type merge \
  -p '{"spec":{"package":"xpkg.upbound.io/crossplane-contrib/provider-aws:v1.2.0"}}'

# 检查升级状态
kubectl get providerrevisions
kubectl get pods -n crossplane-system
```

## 故障排查

### 常见问题

```bash
# 🟢 低风险：Crossplane 问题诊断
# 问题 1：Claim 一直 Pending
kubectl describe postgresqlinstance my-app-db -n team-backend
# 检查是否有匹配的 Composition
kubectl get compositions -l provider=aws

# 问题 2：Managed Resource 创建失败
kubectl get events -n crossplane-system --field-selector reason=CannotCreate
# 常见原因：云 API 权限不足、配额超限

# 问题 3：Provider 不健康
kubectl get provider provider-aws
kubectl logs -n crossplane-system -l pkg.crossplane.io/provider=provider-aws

# 问题 4：连接 Secret 未生成
kubectl get secret my-app-db-conn -n team-backend
# 检查 Composition 中 connectionDetails 配置
```

## 最佳实践

### 设计原则

1. **XRD 面向用户**：XRD schema 只暴露用户需要选择的参数，隐藏基础设施细节
2. **Composition 面向平台**：Composition 包含所有基础设施最佳实践（加密、多 AZ、安全组）
3. **多 Composition 策略**：同一 XRD 可以有多个 Composition（AWS/GCP/Azure），通过 label 选择
4. **连接信息安全**：连接凭证通过 K8s Secret 传递，启用 encryption at rest
5. **与 [[平台工程/构建/07-crossplane-platform-composition|Crossplane 平台组合]] 配合**：了解更完整的平台设计
6. **与 [[清单模式/08-platform-patterns/02-cue-language-configuration|CUE]] 结合**：使用 CUE 验证 XRD schema
7. **参考 [[平台工程/构建/01-platform-engineering-overview|平台工程概述]] 了解全局**

## Related

- [[平台工程/构建/07-crossplane-platform-composition|Crossplane 平台组合]]
- [[清单模式/08-platform-patterns/02-cue-language-configuration|CUE 语言配置]]
- [[清单模式/08-platform-patterns/03-jsonnet-tanka-patterns|Jsonnet/Tanka 模式]]
- [[平台工程/构建/01-platform-engineering-overview|平台工程概述]]
- [[综合/crossplane-iac|Crossplane IaC 综合]]
- [[平台工程/构建/06-kratix-platform-as-code|Kratix 平台即代码]]
