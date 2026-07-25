---
title: Crossplane Compositions and Platform API Design Patterns
description: K8s 平台 API 设计 — Crossplane Compositions、XRD 设计、平台抽象层、自助服务 API、基础设施即产品
summary: 使用 Crossplane 构建平台工程 API 层，将基础设施封装为开发者友好的自助服务接口
category: practice
tags:
- crossplane
- compositions
- platform-api
- infrastructure-as-product
- xrd
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: manifest
---
# Crossplane Compositions 与平台 API 设计

> 将基础设施封装为开发者友好的平台 API，实现 Infrastructure as Product。

## 核心理念

```
传统: 开发者 → Terraform/Cloud Console → 基础设施（复杂、慢）
平台: 开发者 → kubectl apply XClaim → Crossplane → 基础设施（自助、快）

┌─────────────────────────────────────────────────────────┐
│  开发者视角（XClaim）                                    │
│  "我需要一个 PostgreSQL 数据库，10GB，生产级"            │
└────────────────────────┬────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────┐
│  平台 API（XRD + Composition）                           │
│  定义抽象资源 + 编排底层资源                             │
└────────────────────────┬────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────┐
│  基础设施（Managed Resources）                           │
│  RDS Instance + Security Group + Parameter Group + DNS  │
└─────────────────────────────────────────────────────────┘
```

## XRD 设计（Composite Resource Definition）

### 数据库平台 API

```yaml
apiVersion: apiextensions.crossplane.io/v1
kind: CompositeResourceDefinition
metadata:
  name: xdatabases.platform.example.com
spec:
  group: platform.example.com
  names:
    kind: XDatabase
    plural: xdatabases
  claimNames:
    kind: Database
    plural: databases
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
                engine:
                  type: string
                  enum: [postgresql, mysql]
                  description: "数据库引擎"
                version:
                  type: string
                  description: "引擎版本"
                  default: "16"
                size:
                  type: string
                  enum: [small, medium, large]
                  description: "规格（small=2C4G, medium=4C8G, large=8C16G）"
                  default: small
                storage:
                  type: string
                  description: "存储大小"
                  default: "20Gi"
                environment:
                  type: string
                  enum: [development, staging, production]
                  default: development
                multiAZ:
                  type: boolean
                  default: false
              required: [engine]
            status:
              type: object
              properties:
                ready:
                  type: boolean
                endpoint:
                  type: string
```

### 应用平台 API

```yaml
apiVersion: apiextensions.crossplane.io/v1
kind: CompositeResourceDefinition
metadata:
  name: xapplications.platform.example.com
spec:
  group: platform.example.com
  names:
    kind: XApplication
    plural: xapplications
  claimNames:
    kind: Application
    plural: applications
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
                image:
                  type: string
                  description: "容器镜像"
                replicas:
                  type: integer
                  minimum: 1
                  maximum: 20
                  default: 2
                port:
                  type: integer
                  default: 8080
                resources:
                  type: string
                  enum: [small, medium, large]
                  default: small
                ingress:
                  type: object
                  properties:
                    enabled:
                      type: boolean
                      default: false
                    host:
                      type: string
                database:
                  type: object
                  properties:
                    enabled:
                      type: boolean
                      default: false
                    engine:
                      type: string
                      enum: [postgresql, mysql]
              required: [image]
```

## Composition 实现

### 数据库 Composition（AWS）

```yaml
apiVersion: apiextensions.crossplane.io/v1
kind: Composition
metadata:
  name: xdatabases.aws.platform.example.com
  labels:
    provider: aws
    environment: production
spec:
  compositeTypeRef:
    apiVersion: platform.example.com/v1alpha1
    kind: XDatabase
  mode: Pipeline
  pipeline:
    - step: patch-and-transform
      functionRef:
        name: function-patch-and-transform
      input:
        apiVersion: pt.fn.crossplane.io/v1beta1
        kind: Resources
        resources:
          # RDS 实例
          - name: rdsinstance
            base:
              apiVersion: rds.aws.upbound.io/v1beta2
              kind: Instance
              spec:
                forProvider:
                  region: cn-east-1
                  engine: PostgreSQL
                  engineVersion: "16"
                  instanceClass: db.t3.micro
                  allocatedStorage: 20
                  storageType: gp3
                  dbSubnetGroupNameSelector:
                    matchLabels:
                      environment: production
                  vpcSecurityGroupIDSelector:
                    matchLabels:
                      role: database
                  publiclyAccessible: false
                  storageEncrypted: true
                  backupRetentionPeriod: 7
                  deletionProtection: true
                  skipFinalSnapshot: false
            patches:
              - type: FromCompositeFieldPath
                fromFieldPath: spec.engine
                toFieldPath: spec.forProvider.engine
              - type: FromCompositeFieldPath
                fromFieldPath: spec.version
                toFieldPath: spec.forProvider.engineVersion
              - type: FromCompositeFieldPath
                fromFieldPath: spec.storage
                toFieldPath: spec.forProvider.allocatedStorage
                transforms:
                  - type: string
                    string:
                      type: TrimSuffix
                      trim: "Gi"
              - type: FromCompositeFieldPath
                fromFieldPath: spec.size
                toFieldPath: spec.forProvider.instanceClass
                transforms:
                  - type: map
                    map:
                      small: db.t3.micro
                      medium: db.m5.large
                      large: db.m5.xlarge
              - type: FromCompositeFieldPath
                fromFieldPath: spec.multiAZ
                toFieldPath: spec.forProvider.multiAZ
          # 安全组规则
          - name: securitygrouprule
            base:
              apiVersion: ec2.aws.upbound.io/v1beta1
              kind: SecurityGroupIngressRule
              spec:
                forProvider:
                  region: cn-east-1
                  ipProtocol: tcp
                  fromPort: 5432
                  toPort: 5432
                  securityGroupIdSelector:
                    matchLabels:
                      role: database
            patches:
              - type: FromCompositeFieldPath
                fromFieldPath: spec.engine
                toFieldPath: spec.forProvider.fromPort
                transforms:
                  - type: map
                    map:
                      postgresql: "5432"
                      mysql: "3306"
```

## 开发者使用（Claim）

```yaml
# 开发者只需提交这个简单的 Claim
apiVersion: platform.example.com/v1alpha1
kind: Database
metadata:
  name: order-db
  namespace: team-commerce
spec:
  engine: postgresql
  version: "16"
  size: medium
  storage: 50Gi
  environment: production
  multiAZ: true
  compositionSelector:
    matchLabels:
      provider: aws
---
# 应用 Claim（含数据库）
apiVersion: platform.example.com/v1alpha1
kind: Application
metadata:
  name: order-service
  namespace: team-commerce
spec:
  image: registry.example.com/order-service:v2.1.0
  replicas: 3
  port: 8080
  resources: medium
  ingress:
    enabled: true
    host: orders.example.com
  database:
    enabled: true
    engine: postgresql
```

## 平台 API 设计原则

| 原则 | 说明 | 示例 |
|------|------|------|
| 抽象适度 | 隐藏复杂度但保留必要选项 | size: small/medium/large |
| 合理默认 | 大多数参数有默认值 | 只需指定 engine |
| 环境感知 | 不同环境不同配置 | dev 单 AZ，prod 多 AZ |
| 可组合 | 资源可组合使用 | Application 包含 Database |
| 可观测 | 状态可查询 | status.ready + events |
| 安全默认 | 加密、私有、最小权限 | 默认 encrypted + private |

## 与 Terraform 对比

| 维度 | Crossplane | Terraform |
|------|-----------|-----------|
| 运行模式 | 持续调谐（GitOps） | 一次性执行 |
| 漂移检测 | 自动修复 | 需手动 plan |
| 抽象层 | XRD + Composition | Module |
| 自助服务 | kubectl + Claim | 需 CI/CD 触发 |
| 学习曲线 | K8s 原生 | HCL 语言 |
| 适用场景 | K8s 平台团队 | 多云/非 K8s 基础设施 |

## 监控与治理

```yaml
# 查看 Claim 状态
kubectl get databases -A
kubectl describe database order-db -n team-commerce

# 查看底层资源
kubectl get managed -l crossplane.io/claim-name=order-db

# 成本标签（自动添加）
# 所有资源自动标记: team, environment, cost-center
```

## Related

- [[03-清单模式/09-平台模式/index.md|平台模式]]
- [[03-清单模式/09-平台模式/01-crossplane-compositions-patterns.md|Crossplane 基础]]
- [[10-平台工程/05-内部开发者平台/02-platform-governance-golden-path.md|平台治理]]
