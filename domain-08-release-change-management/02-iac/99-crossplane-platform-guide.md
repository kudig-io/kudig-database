---
title: Crossplane 平台工程实践指南
description: '# Crossplane 平台工程实践指南'
summary: 'helm repo add crossplane-stable https://charts.crossplane.io/stable'
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
tier: peripheral
created: '2026-05-23'
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
prerequisites:
- kubectl-basics
- gitops-basics
- helm-basics
- iac-basics
- policy-basics
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




# [[Crossplane|Crossplane]] 平台工程实践指南

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
# 🟢 低风险：只读/信息收集，通常无副作用
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

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
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

**与 [[Argo|Argo]] CD 集成**
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

---

## Obsidian 相关文档

- domain-24-infrastructure-as-code MOC
- [[domain-08-release-change-management/README.md|Domain 08: 基础设施即代码 (Infrastructure as Code)]]
- Domain-24 基础设施即代码 — 开源项目索引
- Terraform企业级基础设施即代码实践
- Ansible企业级自动化运维深度实践
- Pulumi Enterprise Infrastructure as Code Platform
- Azure Resource Manager (ARM) Enterprise 深度实践
- Crossplane Enterprise Infrastructure Orchestration 深度实践

## See Also

- 04-azure-resource-manager-enterprise
- 05-crossplane-enterprise-orchestration
- 01-terraform-enterprise-iac
- 02-ansible-enterprise-automation

```

<!-- risk-assessed -->
