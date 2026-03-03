# Crossplane

> **成熟度**: Graduated | **加入时间**: 2020-06 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://crossplane.io |
| **GitHub** | https://github.com/crossplane/crossplane |
| **文档** | https://docs.crossplane.io |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | App Definition & Development |

---

## 项目概述

### 简介
Crossplane 是一个开源的云原生控制平面框架，通过扩展 Kubernetes API 来管理任何基础设施和云服务。它将基础设施即代码(IaC)提升为基础设施即数据(Infrastructure as Data)，使团队能够用 Kubernetes 原生方式管理云资源。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2018-12 | Upbound 公司创建 Crossplane |
| 2020-06 | 加入 CNCF Sandbox |
| 2021-09 | 晋升为 CNCF Incubating |
| 2024-07 | 晋升为 CNCF Graduated |

### 核心定位
Crossplane 是构建内部开发者平台(IDP)的基础，让平台团队可以定义抽象，让开发者通过简单的 Kubernetes 资源请求基础设施，实现自助服务。

---

## 架构设计

### 整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                    Crossplane 架构                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                   Kubernetes Cluster                         ││
│  │                                                              ││
│  │  ┌─────────────────────────────────────────────────────────┐││
│  │  │                    Crossplane Core                       │││
│  │  │  ┌───────────────┐  ┌───────────────┐                   │││
│  │  │  │  Composition  │  │    Package    │                   │││
│  │  │  │   Engine      │  │   Manager     │                   │││
│  │  │  └───────────────┘  └───────────────┘                   │││
│  │  └─────────────────────────────────────────────────────────┘││
│  │                            │                                 ││
│  │         ┌──────────────────┼──────────────────┐             ││
│  │         ▼                  ▼                  ▼             ││
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐     ││
│  │  │ AWS Provider│    │ GCP Provider│    │Azure Provider│    ││
│  │  │             │    │             │    │             │     ││
│  │  │ • EC2       │    │ • GCE       │    │ • VM        │     ││
│  │  │ • RDS       │    │ • CloudSQL  │    │ • SQL DB    │     ││
│  │  │ • S3        │    │ • GCS       │    │ • Blob      │     ││
│  │  └──────┬──────┘    └──────┬──────┘    └──────┬──────┘     ││
│  │         │                  │                  │             ││
│  └─────────┼──────────────────┼──────────────────┼─────────────┘│
│            │                  │                  │              │
│            ▼                  ▼                  ▼              │
│    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐       │
│    │    AWS      │    │    GCP      │    │   Azure     │       │
│    │   Cloud     │    │   Cloud     │    │   Cloud     │       │
│    └─────────────┘    └─────────────┘    └─────────────┘       │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 核心概念

```
┌─────────────────────────────────────────────────────────────────┐
│                  Crossplane 资源层次                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  开发者视角 (Consumer)                                           │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                        Claim (XRC)                          ││
│  │                 例: PostgreSQLInstance                      ││
│  │  apiVersion: database.example.org/v1alpha1                  ││
│  │  kind: PostgreSQLInstance                                   ││
│  │  spec:                                                      ││
│  │    parameters:                                              ││
│  │      storageGB: 20                                          ││
│  └───────────────────────────┬─────────────────────────────────┘│
│                              │                                   │
│  平台团队视角 (Provider)      │                                   │
│  ┌───────────────────────────┼─────────────────────────────────┐│
│  │        Composite Resource │(XR)                             ││
│  │        例: XPostgreSQLInstance                              ││
│  │                           │                                 ││
│  │    ┌──────────────────────┴──────────────────────┐         ││
│  │    │              Composition                     │         ││
│  │    │       (定义 XR 如何映射到 MR)                │         ││
│  │    └──────────────────────┬──────────────────────┘         ││
│  │                           │                                 ││
│  │         ┌─────────────────┼─────────────────┐              ││
│  │         ▼                 ▼                 ▼              ││
│  │   ┌──────────┐      ┌──────────┐      ┌──────────┐        ││
│  │   │ RDSInstance│     │  VPC     │      │ SubnetGroup│      ││
│  │   │   (MR)   │      │   (MR)   │      │   (MR)    │       ││
│  │   └──────────┘      └──────────┘      └──────────┘        ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
│  MR = Managed Resource (实际云资源)                              │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 安装部署

### 安装 Crossplane

```bash
# 使用 Helm 安装
helm repo add crossplane-stable https://charts.crossplane.io/stable
helm repo update

helm install crossplane crossplane-stable/crossplane \
  --namespace crossplane-system \
  --create-namespace

# 验证安装
kubectl get pods -n crossplane-system
```

### 安装 Provider

```yaml
# 安装 AWS Provider
apiVersion: pkg.crossplane.io/v1
kind: Provider
metadata:
  name: provider-aws
spec:
  package: xpkg.upbound.io/upbound/provider-aws:v0.47.0

---
# 配置 Provider 凭证
apiVersion: aws.upbound.io/v1beta1
kind: ProviderConfig
metadata:
  name: default
spec:
  credentials:
    source: Secret
    secretRef:
      namespace: crossplane-system
      name: aws-secret
      key: credentials
```

---

## 核心功能

### 1. Managed Resources (托管资源)

```yaml
# 直接创建 AWS RDS 实例
apiVersion: rds.aws.upbound.io/v1beta1
kind: Instance
metadata:
  name: my-database
spec:
  forProvider:
    region: us-west-2
    instanceClass: db.t3.micro
    engine: postgres
    engineVersion: "14"
    allocatedStorage: 20
    username: admin
    skipFinalSnapshot: true
    publiclyAccessible: false
  writeConnectionSecretToRef:
    name: db-connection
    namespace: default
```

### 2. Compositions (组合)

```yaml
# 定义组合资源类型
apiVersion: apiextensions.crossplane.io/v1
kind: CompositeResourceDefinition
metadata:
  name: xpostgresqlinstances.database.example.org
spec:
  group: database.example.org
  names:
    kind: XPostgreSQLInstance
    plural: xpostgresqlinstances
  claimNames:
    kind: PostgreSQLInstance
    plural: postgresqlinstances
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
                    storageGB:
                      type: integer
                      default: 20
                    instanceClass:
                      type: string
                      default: db.t3.micro
              required:
                - parameters

---
# 定义组合逻辑
apiVersion: apiextensions.crossplane.io/v1
kind: Composition
metadata:
  name: postgresql-aws
spec:
  compositeTypeRef:
    apiVersion: database.example.org/v1alpha1
    kind: XPostgreSQLInstance
  
  resources:
    # VPC
    - name: vpc
      base:
        apiVersion: ec2.aws.upbound.io/v1beta1
        kind: VPC
        spec:
          forProvider:
            region: us-west-2
            cidrBlock: 10.0.0.0/16
            enableDnsHostnames: true
    
    # Subnet
    - name: subnet
      base:
        apiVersion: ec2.aws.upbound.io/v1beta1
        kind: Subnet
        spec:
          forProvider:
            region: us-west-2
            cidrBlock: 10.0.1.0/24
            vpcIdSelector:
              matchControllerRef: true
    
    # RDS Instance
    - name: rdsinstance
      base:
        apiVersion: rds.aws.upbound.io/v1beta1
        kind: Instance
        spec:
          forProvider:
            region: us-west-2
            engine: postgres
            engineVersion: "14"
            username: admin
            skipFinalSnapshot: true
      patches:
        - fromFieldPath: spec.parameters.storageGB
          toFieldPath: spec.forProvider.allocatedStorage
        - fromFieldPath: spec.parameters.instanceClass
          toFieldPath: spec.forProvider.instanceClass
      connectionDetails:
        - name: host
          fromFieldPath: status.atProvider.address
        - name: port
          fromFieldPath: status.atProvider.port
```

### 3. Claims (声明)

```yaml
# 开发者使用的简单接口
apiVersion: database.example.org/v1alpha1
kind: PostgreSQLInstance
metadata:
  name: my-app-db
  namespace: default
spec:
  parameters:
    storageGB: 50
    instanceClass: db.t3.small
  writeConnectionSecretToRef:
    name: my-app-db-conn
```

---

## 使用场景

### 1. 内部开发者平台 (IDP)

```
┌─────────────────────────────────────────────────────────────────┐
│                   内部开发者平台架构                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  开发者                                                          │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │  kubectl apply -f my-app-db.yaml                            ││
│  │                                                              ││
│  │  apiVersion: database.example.org/v1                        ││
│  │  kind: PostgreSQLInstance                                   ││
│  │  spec:                                                      ││
│  │    parameters:                                              ││
│  │      size: medium                                           ││
│  └─────────────────────────────────────────────────────────────┘│
│                              │                                   │
│                              ▼                                   │
│  平台团队定义的抽象                                              │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │  Compositions                                               ││
│  │  • production-postgresql (RDS + 备份 + 监控)                ││
│  │  • development-postgresql (RDS 最小配置)                    ││
│  │  • staging-postgresql (RDS + 加密)                          ││
│  └─────────────────────────────────────────────────────────────┘│
│                              │                                   │
│                              ▼                                   │
│  实际云资源                                                      │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │  AWS: RDS + SecurityGroup + SubnetGroup + IAM               ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 2. GitOps 基础设施管理

```yaml
# 与 ArgoCD 集成
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: infrastructure
spec:
  source:
    repoURL: https://github.com/myorg/infrastructure
    path: crossplane/
    targetRevision: main
  destination:
    server: https://kubernetes.default.svc
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
```

### 3. 多云抽象

```yaml
# 同一 Claim 可以映射到不同云
apiVersion: database.example.org/v1alpha1
kind: PostgreSQLInstance
metadata:
  name: app-db
  labels:
    cloud: aws  # 或 gcp、azure
spec:
  compositionSelector:
    matchLabels:
      provider: aws  # 选择对应云的 Composition
  parameters:
    storageGB: 100
```

---

## 参考资源

- [官方文档](https://docs.crossplane.io)
- [GitHub Repo](https://github.com/crossplane/crossplane)
- [CNCF 项目页面](https://www.cncf.io/projects/crossplane/)
- [Upbound Marketplace](https://marketplace.upbound.io/)
- [Provider 列表](https://github.com/crossplane-contrib)

---

**维护者**: Kudig Team | **许可证**: MIT
