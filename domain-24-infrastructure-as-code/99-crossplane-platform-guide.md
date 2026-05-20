---
title: Crossplane 平台工程实践指南
description: '# Crossplane 平台工程实践指南'
category: infrastructure-as-code
tags:
- k8s
- iac
- terraform
- pulumi
- helm
- opa
- postgresql
- rag
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- SRE
- DevOps 工程师
estimated_read_time: 5min
intent_queries:
- Crossplane 平台工程实践指南 是什么
- 如何 Crossplane 平台工程实践指南
- Kubernetes 24 infrastructure as code 最佳实践
trigger_keywords:
- Crossplane
- 平台工程实践指南
- infrastructure
- as
- code
---

# Crossplane 平台工程实践指南

> **适用版本**: Crossplane v1.19.0  
> **最后更新**: 2026-04-24  
> **难度**: 高级

---

## 📋 目录

- [一、核心概念](#一核心概念)
- [二、安装部署](#二安装部署)
- [三、Provider 配置](#三provider-配置)
- [四、Composition 构建平台 API](#四composition-构建平台-api)
- [五、GitOps 集成](#五gitops-集成)
- [六、多租户与治理](#六多租户与治理)
- [七、与 Terraform 对比](#七与-terraform-对比)

---

## 一、核心概念

```
开发者视角 (Claim)
  ├── DatabaseClaim ──► 平台工程师预定义
  │
  平台工程师视角
    ├── CompositeResourceDefinition (XRD): 模式定义
    ├── Composition: 实现映射 (AWS RDS / GCP CloudSQL / Azure PG)
    └── Provider: 云厂商插件
        └── Managed Resource (MR): 底层云资源
```

---

## 二、安装部署

```bash
helm repo add crossplane-stable https://charts.crossplane.io/stable
helm install crossplane crossplane-stable/crossplane \
  --namespace crossplane-system \
  --create-namespace \
  --version 1.19.0

# 安装 CLI
 curl -sL https://raw.githubusercontent.com/crossplane/crossplane/master/install.sh | sh
 sudo mv crossplane /usr/local/bin/
```

---

## 三、Provider 配置

```yaml
# AWS Provider
apiVersion: pkg.crossplane.io/v1
kind: Provider
metadata:
  name: provider-aws-rds
spec:
  package: xpkg.upbound.io/upbound/provider-aws-rds:v1.21.0
---
# ProviderConfig (凭证)
apiVersion: aws.upbound.io/v1beta1
kind: ProviderConfig
metadata:
  name: default
spec:
  credentials:
    source: Secret
    secretRef:
      namespace: crossplane-system
      name: aws-creds
      key: creds
```

---

## 四、Composition 构建平台 API

```yaml
# XRD: 平台工程师定义 API
apiVersion: apiextensions.crossplane.io/v1
kind: CompositeResourceDefinition
metadata:
  name: xpostgresqls.database.example.org
spec:
  group: database.example.org
  names:
    kind: XPostgreSQL
    plural: xpostgresqls
  claimNames:
    kind: PostgreSQL
    plural: postgresqls
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
                  region:
                    type: string
                    enum: ["us-east-1", "eu-west-1"]
                  storageGB:
                    type: integer
                    default: 20
                  version:
                    type: string
                    enum: ["14", "15", "16"]
                    default: "16"
---
# Composition: AWS RDS 实现
apiVersion: apiextensions.crossplane.io/v1
kind: Composition
metadata:
  name: aws-rds-postgresql
  labels:
    provider: aws
    db: postgresql
spec:
  compositeTypeRef:
    apiVersion: database.example.org/v1alpha1
    kind: XPostgreSQL
  resources:
  - name: rds-instance
    base:
      apiVersion: rds.aws.upbound.io/v1beta1
      kind: Instance
      spec:
        forProvider:
          engine: postgres
          instanceClass: db.t3.micro
          allocatedStorage: 20
          region: us-east-1
    patches:
    - fromFieldPath: spec.parameters.region
      toFieldPath: spec.forProvider.region
    - fromFieldPath: spec.parameters.storageGB
      toFieldPath: spec.forProvider.allocatedStorage
    - fromFieldPath: spec.parameters.version
      toFieldPath: spec.forProvider.engineVersion
```

---

## 五、GitOps 集成

```yaml
# 开发者提交 Claim
apiVersion: database.example.org/v1alpha1
kind: PostgreSQL
metadata:
  name: myapp-db
  namespace: dev-team
spec:
  parameters:
    region: us-east-1
    storageGB: 50
    version: "16"
```

**与 Argo CD 集成**
- Crossplane 资源为原生 K8s YAML
- 直接通过 Argo CD 管理
- 利用 Argo CD 的 drift detection 检测云资源漂移

---

## 六、多租户与治理

```yaml
# 配额与治理 (通过 XRD validation 或 OPA Gatekeeper)
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sRequiredLabels
metadata:
  name: require-cost-center
spec:
  match:
    kinds:
    - apiGroups: ["database.example.org"]
      kinds: ["PostgreSQL"]
  parameters:
    labels:
    - key: cost-center
```

---

## 七、与 Terraform 对比

| 维度 | Crossplane | Terraform |
|:---|:---|:---|
| 控制平面 | K8s 原生 | 外部 CLI |
| GitOps | 原生支持 | 需 wrapper |
| 漂移检测 | 自动调和 | 需手动 plan |
| 组合抽象 | XRD + Composition | Module |
| 多租户 | 命名空间隔离 | 需额外管理 |
| 云资源状态 | K8s CR 实时反映 | state 文件 |
| 团队分工 | 平台/开发解耦 | 通常集中管理 |

---

## 参考链接

- [Crossplane 官方文档](https://docs.crossplane.io/)
- [Upbound Marketplace](https://marketplace.upbound.io/)
- [Crossplane 架构](https://docs.crossplane.io/latest/concepts/)
