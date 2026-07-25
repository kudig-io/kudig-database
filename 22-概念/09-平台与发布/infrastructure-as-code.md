---
title: Infrastructure as Code
description: '- [[22-概念/11-交叉分析/IaC × 多集群管理.md|IaC x 多集群管理]] — synthesis'
summary: '- [[22-概念/11-交叉分析/IaC × 多集群管理.md|IaC x 多集群管理]] — synthesis'
category: concepts
tags:
- k8s
- iac
- terraform
- pulumi
- crossplane
- automation
- etcd
- helm
- argocd
- flux
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Infrastructure as Code 是什么
- 如何 Infrastructure as Code
trigger_keywords:
- Infrastructure
- as
- Code
prerequisites:
- kubectl-basics
- helm-basics
- gitops-basics
- iac-basics
- etcd-basics
- policy-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Infrastructure as Code

## IaC Tool Comparison

| Tool | Language | Paradigm | State Management | Best For |
|------|----------|----------|-----------------|----------|
| Terraform | HCL | Declarative | Local, S3, Terraform Cloud | Multi-cloud infrastructure |
| Pulumi | TypeScript/Python/Go | Imperative/Declarative | [[Service|Service]], local, S3 | Developer-centric IaC |
| Ansible | YAML | Imperative (config mgmt) | None (idempotent) | Configuration management |
| Crossplane | YAML (K8s CRDs) | Declarative | etcd (K8s native) | K8s-native infra orchestration |
| AWS CDK | TypeScript/Python | Imperative | CloudFormation | AWS-only infrastructure |

## Core IaC Patterns

**Modular Design**: Break infrastructure into reusable modules (networking, compute, storage). Each module has inputs, outputs, and internal resource definitions. Enables consistent patterns across environments.

**State Management**: IaC tools track actual vs desired state. Terraform stores state in backends (S3+DynamoDB for locking, Terraform Cloud for team collaboration). Crossplane stores state in K8s etcd, naturally integrating with [[22-概念/09-平台与发布/gitops-principles.md|GitOps]].

**Policy as Code**: Enforce infrastructure standards through automated policy checks:
- **Sentinel**: HashiCorp policy framework (Terraform Enterprise)
- **OPA**: Open Policy Agent with Rego language (cross-platform)
- **Conftest**: Configuration testing tool for CI/CD pipelines

## Terraform + GitOps Integration

Modern IaC pipelines combine Terraform (cloud resources) with GitOps (K8s resources):
1. Terraform provisions cloud infrastructure (VPCs, load balancers, managed K8s clusters)
2. Crossplane or Helm manages in-cluster resources
3. Both pipelines are GitOps-managed with ArgoCD/Flux
4. PR review catches misconfigurations before deployment

## Crossplane: K8s-Native IaC

Crossplane extends K8s API with custom resources for cloud infrastructure. A `Bucket` CRD provisions S3 storage, a `Database` CRD provisions RDS instances. Benefits:
- Unified API: same kubectl workflow for cloud and cluster resources
- GitOps native: state stored in etcd, reconciled by controllers
- Composition: combine multiple resources into higher-level abstractions

## 源码实现分析

### Terraform 执行模型

```
terraform plan
    │
    ▼
读取 State (S3/DynamoDB) + 读取 HCL 配置
    │
    ▼
构建 Resource Graph (DAG) → 计算 Diff
    │
    ▼
输出执行计划 (create/update/delete)
    │
    ▼
terraform apply
    │
    ▼
按 DAG 顺序调用 Provider API (AWS/GCP/Azure)
    │
    ▼
更新 State 文件 → 锁定释放
```

### Crossplane 调谐循环

```go
// crossplane/internal/controller/managed/reconciler.go
func (r *Reconciler) Reconcile(ctx context.Context, req Request) {
    // 1. 获取 Managed Resource (e.g., RDSInstance CR)
    managed := r.GetManagedResource(req)
    
    // 2. 调用云 API 获取外部资源状态
    external, err := r.client.Observe(ctx, managed)
    // → AWS SDK: DescribeDBInstance()
    
    // 3. 判断是否需要创建/更新/删除
    switch {
    case !external.Exists:
        r.client.Create(ctx, managed)  // CreateDBInstance
    case external.NeedsUpdate:
        r.client.Update(ctx, managed)  // ModifyDBInstance
    }
    
    // 4. 更新 Status (Ready/Synced conditions)
    managed.SetConditions(Ready(), Synced())
    r.UpdateStatus(ctx, managed)
}
```

## 使用场景

### 场景一：Terraform 模块化设计

```hcl
# modules/eks-cluster/main.tf
module "eks" {
  source          = "terraform-aws-modules/eks/aws"
  version         = "~> 20.0"
  cluster_name    = var.cluster_name
  cluster_version = "1.30"
  vpc_id          = var.vpc_id
  subnet_ids      = var.private_subnets
  
  eks_managed_node_groups = {
    default = {
      instance_types = ["m5.xlarge"]
      min_size       = 3
      max_size       = 10
      desired_size   = 3
    }
  }
}
```

### 场景二：Crossplane Composition（平台抽象）

```yaml
apiVersion: apiextensions.crossplane.io/v1
kind: Composition
metadata:
  name: database-aws
spec:
  compositeTypeRef:
    apiVersion: platform.example.com/v1
    kind: Database
  resources:
  - name: rdsinstance
    base:
      apiVersion: rds.aws.upbound.io/v1beta1
      kind: Instance
      spec:
        forProvider:
          engine: postgres
          instanceClass: db.t3.medium
          allocatedStorage: 100
    patches:
    - fromFieldPath: spec.parameters.engineVersion
      toFieldPath: spec.forProvider.engineVersion
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| Terraform 可以管理 K8s 内部资源 | Terraform 适合云基础设施，K8s 内部资源应用 GitOps/Helm |
| IaC 就是写脚本 | IaC 核心是声明式+状态管理+幂等性，与脚本本质不同 |
| State 文件不重要 | State 是 IaC 的核心，丢失/损坏会导致资源失控 |
| Crossplane 可以替代 Terraform | 两者互补：Terraform 管理云资源，Crossplane 统一 K8s+云的 API |
| IaC 不需要 Code Review | IaC 代码同样需要 PR 审查，基础设施变更影响更大 |
| apply 总是安全的 | 必须先 plan 审查，生产环境用 -auto-approve 是危险操作 |

## 面试要点

1. **Terraform 与 Crossplane 如何选择？** — Terraform：成熟、多云、生态丰富，适合云基础设施；Crossplane：K8s 原生、GitOps 集成、统一 API，适合平台工程。生产环境常组合使用：Terraform 建集群，Crossplane 管理集群内云资源。

2. **Terraform State 管理最佳实践？** — 远程后端（S3 + DynamoDB 锁定）；状态加密；永远不要提交到 Git；使用 workspace 隔离环境；定期备份；大团队用 Terraform Cloud/Enterprise。

3. **IaC 与 GitOps 的关系？** — IaC 定义“期望状态”，GitOps 确保“实际状态向期望收敛”。Terraform 可被 GitOps 触发（Atlantis/drift detection），Crossplane 天然 GitOps（控制器调谐）。

4. **Policy as Code 如何集成？** — OPA/Conftest 在 CI 中检查 Terraform plan；Kyverno 在 K8s 准入层拒绝不合规资源；Sentinel 在 Terraform Enterprise 中强制执行策略。实现“左移”安全。

## Related

- [[helm]] — Helm
- [[etcd]] — etcd
- [[23-实体/08-交付与制品/argocd.md|argocd]] — ArgoCD
- [[22-概念/09-平台与发布/platform-engineering-idp.md|platform-engineering-idp]] — Platform Engineering and Internal Developer Platforms
- [[crossplane]] — Crossplane
- [[22-概念/09-平台与发布/gitops-principles.md|GitOps Principles]]
- [[22-概念/09-平台与发布/platform-engineering-idp.md|Platform Engineering and IDP]]
- [[crossplane|Crossplane]]
- [[22-概念/11-交叉分析/IaC × 多集群管理.md|IaC x 多集群管理]] — synthesis

- 05-crossplane-enterprise-orchestration
- 99-crossplane-platform-guide
- 00-open-source-projects-index
- 11-infrastructure-as-code
- [[11-发布变更/README.md|Domain 08: 基础设施即代码 (Infrastructure as Code)]]
- 03-pulumi-enterprise-iac
- 02-ansible-enterprise-automation
- 04-azure-resource-manager-enterprise
- 01-terraform-enterprise-iac
- domain-24-infrastructure-as-code MOC

<!-- risk-assessed -->
