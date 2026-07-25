---
title: Crossplane (entities)
description: Crossplane — Kubernetes 生产运维知识库
summary: Crossplane — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- iac
- crossplane
- infrastructure
- composition
- etcd
- helm
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Crossplane 是什么
- 如何 Crossplane
trigger_keywords:
- Crossplane
prerequisites:
- kubectl-basics
- helm-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Crossplane

> Crossplane 是 CNCF 毕业项目，将 Kubernetes 扩展为通用基础设施控制平面，通过声明式 API 管理云资源、数据库、网络等任何基础设施。

## 基本信息

| 属性 | 值 |
|------|------|
| CNCF 状态 | 毕业 (Graduated, 2021 孵化) |
| 架构 | K8s 控制器 (状态存储在 etcd) |
| 语言 | Go |
| Provider 数量 | 200+ (AWS, GCP, Azure, Helm, SQL, Terraform...) |
| GitOps 集成 | 原生支持 (状态在 K8s，控制器协调) |
| 官网 | https://crossplane.io |
| GitHub | https://github.com/crossplane/crossplane |

## 架构设计

```
┌─────────────────────────────────────────────────────┐
│                  Kubernetes API Server               │
├─────────────────────────────────────────────────────┤
│                                                      │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐  │
│  │  Claim      │  │  XR        │  │  MR        │  │
│  │ (Namespaced)│→│(Composite) │→│(Managed)   │  │
│  └────────────┘  └────────────┘  └────────────┘  │
│        │               │               │          │
│        ▼               ▼               ▼          │
│  ┌──────────────────────────────────────────┐  │
│  │         Composition Engine              │  │
│  │  (XRD + Composition + PatchSet)        │  │
│  └──────────────────────────────────────────┘  │
│        │                                         │
│        ▼                                         │
│  ┌──────────────────────────────────────────┐  │
│  │         Provider Controllers             │  │
│  │  (AWS / GCP / Azure / Helm / SQL)       │  │
│  └──────────────────────────────────────────┘  │
│        │                                         │
└────────┼─────────────────────────────────────────┘
         ▼
┌─────────────────────────────────────────────────────┐
│              外部基础设施                          │
│  AWS RDS / GCP CloudSQL / Azure Blob / VPC / DNS  │
└─────────────────────────────────────────────────────┘
```

## 核心概念

| 概念 | 说明 | 作用域 |
|------|------|--------|
| Provider | 云平台插件，提供 MR 类型 | Cluster |
| Managed Resource (MR) | 单个外部资源实例 | Cluster |
| Composite Resource Definition (XRD) | 定义新的复合资源类型 | Cluster |
| Composition | 定义 XR 如何组合多个 MR | Cluster |
| Composite Resource (XR) | XRD 的实例 | Cluster |
| Claim | 命名空间级别的资源请求 | Namespace |
| PatchSet | 可复用的补丁集 | Cluster |
| Function | 组合逻辑扩展 (Pipeline 模式) | Cluster |

## 安装与配置

### 安装 Crossplane

```bash
# 🟢 使用 Helm 安装
helm repo add crossplane-stable https://charts.crossplane.io/stable
helm install crossplane crossplane-stable/crossplane \
  --namespace crossplane-system \
  --create-namespace \
  --set args='{--enable-usages}'

# 🟢 验证安装
kubectl get pods -n crossplane-system
kubectl get crd | grep crossplane.io
```

### 安装 Provider

```yaml
# 🟡 安装 AWS Provider
apiVersion: pkg.crossplane.io/v1
kind: Provider
metadata:
  name: provider-aws
spec:
  package: xpkg.upbound.io/crossplane-contrib/provider-aws:v0.47.0
---
# 配置 Provider 凭据
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
      key: credentials
```

### 定义 XRD 和 Composition

```yaml
# XRD: 定义 PostgreSQL 抽象
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
                  engineVersion:
                    type: string
                required: [storageGB]
            required: [parameters]
---
# Composition: 映射到 AWS RDS
apiVersion: apiextensions.crossplane.io/v1
kind: Composition
metadata:
  name: xpostgresqlinstances.aws.database.example.org
  labels:
    provider: aws
spec:
  compositeTypeRef:
    apiVersion: database.example.org/v1alpha1
    kind: XPostgreSQLInstance
  resources:
  - name: rdsinstance
    base:
      apiVersion: rds.aws.upbound.io/v1beta1
      kind: Instance
      spec:
        forProvider:
          region: us-east-1
          engine: postgres
          instanceClass: db.t3.micro
          allocatedStorage: 20
          skipFinalSnapshot: true
    patches:
    - fromFieldPath: spec.parameters.storageGB
      toFieldPath: spec.forProvider.allocatedStorage
    - fromFieldPath: spec.parameters.engineVersion
      toFieldPath: spec.forProvider.engineVersion
```

### 开发者使用 Claim

```yaml
# 开发者提交 Claim（无需知道底层是 AWS/GCP/Azure）
apiVersion: database.example.org/v1alpha1
kind: PostgreSQLInstance
metadata:
  name: my-db
  namespace: team-a
spec:
  parameters:
    storageGB: 50
    engineVersion: "15.4"
  compositionSelector:
    matchLabels:
      provider: aws
```

## 运维操作

### 常用命令

```bash
# 🟢 查看 Provider 状态
kubectl get providers
kubectl get providerrevisions

# 🟢 查看 Managed Resources
kubectl get managed
kubectl get rdsinstance.aws.upbound.io

# 🟢 查看 Composite Resources
kubectl get composite

# 🟢 查看 Claims
kubectl get claims -A

# 🟢 查看 Composition
kubectl get compositions
kubectl get xrd

# 🟢 查看资源事件
kubectl describe managed <resource-name>
kubectl events --for managed/<resource-name>

# 🟡 升级 Provider
kubectl patch provider provider-aws -p '{"spec":{"package":"xpkg.upbound.io/crossplane-contrib/provider-aws:v0.48.0"}}'

# 🔴 删除 Managed Resource (会删除外部资源!)
kubectl delete rdsinstance.aws.upbound.io my-db
# 安全删除 (保留外部资源)
kubectl annotate rdsinstance.aws.upbound.io my-db crossplane.io/deletion-policy=Orphan
kubectl delete rdsinstance.aws.upbound.io my-db
```

### 资源状态排查

```bash
# 🟢 检查资源是否 Ready
kubectl get managed -o wide
# SYNCED=True, READY=True 表示正常

# 🟢 查看资源详情
kubectl get rdsinstance.aws.upbound.io my-db -o yaml
# 检查 status.conditions

# 🟢 查看 Crossplane 日志
kubectl logs -n crossplane-system -l app=crossplane --tail=100

# 🟢 查看 Provider 日志
kubectl logs -n crossplane-system -l pkg.crossplane.io/provider=provider-aws --tail=100
```

## 故障排查

### 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| MR SYNCED=False | Provider 无法连接云 API | 检查 ProviderConfig 凭据 |
| MR READY=False | 外部资源创建中/失败 | 查看 events 和 Provider 日志 |
| Claim Pending | 无匹配的 Composition | 检查 compositionSelector labels |
| XR 未创建 | XRD 未就绪 | `kubectl get xrd` 检查 Established |
| Provider CrashLoop | 资源不足/配置错误 | 检查 Pod 日志和资源限制 |
| 资源漂移 | 外部手动修改 | Crossplane 自动回滚 (reconcile) |

### 排查流程

```
1. Claim 状态检查
   kubectl get claim <name> -n <ns> -o yaml
       │
2. XR 状态检查
   kubectl get xr <name> -o yaml
       │
3. MR 状态检查
   kubectl get managed -o wide
       │
4. Provider 日志
   kubectl logs -n crossplane-system -l pkg.crossplane.io/provider=<provider>
       │
5. 云平台控制台确认
   检查实际资源状态
```

## 生产案例

### 案例1：Provider 凭据过期导致所有 MR 失败

**症状：** 所有 AWS MR 状态变为 SYNCED=False

**根因：** IAM 临时凭据过期

**解决：** 使用 IRSA (IAM Roles for Service Accounts) 替代静态凭据

### 案例2：Composition 更新导致资源重建

**症状：** 修改 Composition 后 RDS 实例被删除重建

**根因：** 不可变字段变更触发删除重建

**解决：** 使用 `patchPolicy: FromCompositeFieldPath` + 充分测试

### 案例3：资源漂移检测

**症状：** 手动在 AWS 控制台修改了安全组，Crossplane 回滚了变更

**根因：** Crossplane 默认持续 reconcile，确保实际状态 = 期望状态

**解决：** 理解这是期望行为；如需手动管理，设置 `managementPolicies: [Observe]`

## Crossplane vs Terraform

| 特性 | Crossplane | Terraform |
|------|-----------|----------|
| 状态存储 | etcd (K8s) | 文件/远程 Backend |
| 操作方式 | 持续 Reconcile | 手动 Plan/Apply |
| 漂移检测 | 自动修复 | 手动检测 |
| 学习曲线 | K8s 原生 (YAML) | HCL 语言 |
| 抽象能力 | XRD + Composition | Module |
| 多集群 | 原生支持 | 需额外工具 |
| GitOps | 原生 (ArgoCD/Flux) | 需 Atlantis |
| 生态 | 200+ Provider | 3000+ Provider |

## 版本兼容矩阵

| Crossplane | K8s | 重要变化 |
|-----------|-----|----------|
| 1.14+ | 1.27+ | Composition Functions GA |
| 1.15+ | 1.28+ | Usages GA |
| 1.16+ | 1.29+ | 性能优化 |

## 检查清单

- [ ] 理解 Crossplane 架构 (Provider/XRD/Composition/Claim)
- [ ] 能安装和配置 Provider
- [ ] 能编写 XRD 和 Composition
- [ ] 掌握资源状态排查流程
- [ ] 理解 deletion-policy (Delete vs Orphan)
- [ ] 能处理资源漂移问题
- [ ] 了解 Crossplane vs Terraform 选型

## Related

- [[helm]] — Helm
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[22-概念/09-平台与发布/infrastructure-as-code.md|Infrastructure as Code]]
- [[22-概念/09-平台与发布/platform-engineering-idp.md|Platform Engineering and IDP]]
- [[22-概念/09-平台与发布/gitops-principles.md|GitOps Principles]]
- [[23-实体/15-参考与索引/cncf-orchestration.md|CNCF 编排与应用管理项目全景]]
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]

<!-- risk-assessed -->
