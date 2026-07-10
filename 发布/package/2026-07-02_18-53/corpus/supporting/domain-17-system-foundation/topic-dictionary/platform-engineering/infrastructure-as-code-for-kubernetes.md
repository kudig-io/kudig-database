---
title: Kubernetes 基础设施即代码（IaC）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- helm
- argocd
- flux
- opa
- rbac
- crd
tier: supporting
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 基础设施即代码（IaC） 是什么
- 如何 Kubernetes 基础设施即代码（IaC）
trigger_keywords:
- Kubernetes
- 基础设施即代码
- IaC
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- helm-basics
- gitops-basics
- iac-basics
- etcd-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Kubernetes|Kubernetes]] 基础设施即代码（IaC）

## 概述

**基础设施即代码（[[concepts/infrastructure-as-code.md|Infrastructure as Code]], IaC）** 是通过代码和声明式配置文件来管理和配置基础设施的实践。在 Kubernetes 生态中，IaC 不仅包括集群本身的创建（Terraform / Pulumi / [[Crossplane|Crossplane]]），还包括集群内部资源的编排（YAML / [[Helm|Helm]] / Kustomize / GitOps）。2026 年的最佳实践要求企业建立**从底层云资源到 K8s 应用配置的完整 IaC 流水线**，实现版本控制、可审计、可重复和自动化的基础设施管理。

## 核心概念/原理

### 1. IaC 的两大范式

| 范式 | 特点 | 代表工具 |
|------|------|----------|
| **声明式（Declarative）** | 描述期望状态，由系统自动收敛 | Terraform、Pulumi（策略模式）、Crossplane、Kubernetes YAML |
| **命令式（Imperative）** | 描述执行步骤，按顺序操作 | Ansible、Chef、Pulumi（脚本模式） |

Kubernetes 原生就是声明式系统，因此声明式 IaC 工具与其理念高度契合。

### 2. Terraform

**Terraform** 是 HashiCorp 出品的声明式 IaC 工具，通过 **HCL（HashiCorp Configuration Language）** 定义基础设施：
- **Provider 生态**：支持 AWS、Azure、GCP、Kubernetes、Helm 等 3000+ Provider
- **状态管理（State）**：通过 `terraform.tfstate` 追踪实际资源与配置的差异
- **模块复用（Modules）**：将常用基础设施封装为可复用模块
- **工作区（Workspaces）**：管理 dev/staging/production 多环境配置

```hcl
# Terraform 创建 EKS 集群示例
module "eks" {
  source  = "terraform-aws-modules/eks/aws"
  version = "~> 20.0"

  cluster_name    = "production-cluster"
  cluster_version = "1.29"

  vpc_id     = module.vpc.vpc_id
  subnet_ids = module.vpc.private_subnets

  eks_managed_node_groups = {
    general = {
      desired_size = 3
      min_size     = 1
      max_size     = 10
      instance_types = ["m6i.large"]
    }
  }
}
```

### 3. Pulumi

**Pulumi** 允许使用熟悉的编程语言（TypeScript、Python、Go、C#）编写 IaC：
- **编程语言优势**：条件判断、循环、函数抽象、单元测试
- **策略即代码**：通过 CrossGuard 在部署前进行合规检查
- **与 CI/CD 深度集成**：可直接在应用代码仓库中管理基础设施

```python
# Pulumi Python 创建 GKE 集群示例
import pulumi
from pulumi_gcp import container

cluster = container.Cluster("my-cluster",
    initial_node_count=3,
    node_config=container.ClusterNodeConfigArgs(
        machine_type="n1-standard-2",
    ),
)
```

### 4. Crossplane

**Crossplane** 是 CNCF 孵化项目，将 IaC 的能力直接带入 Kubernetes：
- 通过 Kubernetes CRD 定义云资源（如 AWS RDS、GCP CloudSQL、Azure Blob Storage）
- 利用 Kubernetes 控制循环自动协调云资源状态
- 实现"用 Kubernetes 管理一切"的统一控制平面
- 与 GitOps 天然集成，云资源的变更也通过 Argo CD / [[flux|Flux]] 管理

```yaml
# Crossplane 创建 AWS RDS 示例
apiVersion: rds.aws.upbound.io/v1beta1
kind: Instance
metadata:
  name: production-postgres
spec:
  forProvider:
    engine: postgres
    instanceClass: db.t3.micro
    allocatedStorage: 20
  providerConfigRef:
    name: default
```

### 5. Cluster API（CAPI）

Cluster API 是 Kubernetes 官方的声明式集群生命周期管理项目（详见 `cluster-api-and-fleet-management.md`），本质上也属于 IaC 范畴：
- 用 Kubernetes YAML 创建其他 Kubernetes 集群
- 实现"Meta-Cluster"模式，用 K8s 管理 K8s

## 关键机制或特性

### 分层 IaC 架构

2026 年的最佳实践通常采用三层 IaC 架构：

```
# 🟢 低风险：只读/信息收集，通常无副作用
Layer 3: 应用配置（Application Config）
    └── Helm / Kustomize / Plain YAML（由 ArgoCD / Flux 管理）
    
Layer 2: 平台资源（Platform Resources）
    └── Crossplane / Terraform（创建数据库、Load Balancer、IAM Role）
    
Layer 1: 基础设施（Foundation Infrastructure）
    └── Terraform / Pulumi / Cluster API（创建 VPC、K8s 集群、节点池）
```
### State 管理与协作

- **Terraform Cloud / Enterprise**：提供远程 State 存储、状态锁定、RBAC 和审批工作流
- **S3 Backend + DynamoDB**：开源方案，使用 S3 存储 State，DynamoDB 实现状态锁定
- **Pulumi Service**：Pulumi 官方的 State 托管和团队协作者方案
- **Git 作为 Crossplane 的 State**：Crossplane 的状态直接反映在 Kubernetes etcd 和云资源的实际状态中

### IaC 安全扫描

在 `terraform apply` 或 `pulumi up` 之前，应通过安全扫描工具检查配置：
- **Checkov / Terrascan**：扫描 Terraform / CloudFormation 中的安全配置问题
- **tfsec**：轻量级 Terraform 安全扫描器
- **Snyk IaC**：检测 IaC 模板中的漏洞和合规问题
- **OPA / Sentinel**：在 CI 阶段强制执行组织策略

## 使用场景

1. **多云环境标准化**：使用 Terraform 模块在 AWS、Azure、GCP 上创建配置一致的 VPC 和 Kubernetes 集群
2. **GitOps 管理云资源**：使用 Crossplane + Argo CD，让数据库、对象存储的创建和更新完全通过 Git PR 驱动
3. **开发环境自动化**：开发者在 Backstage 门户提交环境申请，触发 Terraform 自动创建独立的 Namespace 和依赖资源
4. **灾难恢复重建**：区域级灾难后，通过存储在 Git 中的 Terraform 配置在 1 小时内重建完整基础设施
5. **合规即代码**：使用 Terraform Sentinel 或 OPA 阻止创建公开可访问的 S3 Bucket 或未加密的 RDS 实例

## 最佳实践/注意事项

- **State 文件是机密**：Terraform State 中可能包含数据库密码等敏感信息，必须加密存储并限制访问
- **模块化设计**：将基础设施拆分为 VPC、K8s、数据库等独立模块，降低耦合并提高复用性
- **最小权限原则**：Terraform/Pulumi 使用的云凭证应仅拥有创建所需资源的最低权限
- **CI/CD 集成**：所有 IaC 变更必须通过 CI Pipeline 的 `terraform plan` / `pulumi preview` 审查后才能执行
- **Drift Detection**：定期运行 `terraform plan` 检测手动修改导致的配置漂移，并自动修复
- **环境隔离**：使用独立的工作区或目录管理 dev/staging/production 环境，State 文件必须分离
- **版本锁定**：锁定 Provider 和 Module 的版本，避免上游变更导致意外破坏
- **文档化变量**：所有输入变量都应有清晰的描述、类型约束和默认值，降低使用门槛
- **备份 State 文件**：State 文件损坏或丢失可能导致资源孤儿化，必须定期备份

## 故障排查

| 症状 | 可能原因 | 排查命令/方法 |
|------|---------|-------------|
| `terraform apply` 失败 | Provider 认证过期或 API 配额超限 | `terraform plan` 查看错误详情；检查云凭证有效性 |
| State 文件锁定无法释放 | 上次操作异常中断 | `terraform force-unlock <lock-id>`；检查 DynamoDB 锁表 |
| State 中资源与实际不一致（Drift） | 手动修改了云资源 | `terraform plan` 检测漂移；`terraform refresh` 更新 State |
| Crossplane 资源长时间 Pending | Provider 配置错误或 CRD 未安装 | `kubectl describe <managed-resource>`；检查 Provider Pod 日志 |
| Pulumi up 报 conflict 错误 | 并发操作或 State 不一致 | `pulumi refresh`；确保同一时间只有一个操作 |
| Module 版本升级后出错 | 上游 Module 引入破坏性变更 | 锁定 Module 版本；使用 `~>` 版本约束 |
| IaC 安全扫描大量告警 | 默认配置不满足安全基线 | 根据 Checkov/tfsec 输出逐项修复；配置 baseline 忽略规则 |
| Terraform Plan 显示大量不必要变更 | Provider 版本升级导致 Schema 变化 | 锁定 Provider 版本；使用 `ignore_changes` 排除特定字段 |

## 生产检查清单

- [ ] State 文件加密存储在远程后端（S3 + DynamoDB / Terraform Cloud）
- [ ] State 文件定期备份，丢失恢复流程已验证
- [ ] Terraform/Pulumi 使用的云凭证遵循最小权限原则
- [ ] 所有 IaC 变更必须通过 CI Pipeline 的 `plan`/`preview` 审查
- [ ] Provider 和 Module 版本已锁定（`.terraform.lock.hcl` / `go.sum`）
- [ ] IaC 安全扫描（Checkov/tfsec/Snyk）集成到 CI Pipeline
- [ ] dev/staging/production 使用独立的 State 文件和工作区
- [ ] 定期运行 `terraform plan` 检测配置漂移（建议每日）
- [ ] 所有输入变量有清晰的描述、类型约束和默认值
- [ ] 三层 IaC 架构已建立（Foundation → Platform → Application）
- [ ] Crossplane 管理的云资源与 GitOps 集成

## 命令快速参考

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# --- Terraform ---
# 初始化工作区
terraform init

# 查看执行计划
terraform plan

# 应用变更
terraform apply

# 检测配置漂移
terraform plan -detailed-exitcode

# 更新 State 与实际资源同步
terraform refresh

# 释放锁定
terraform force-unlock <lock-id>

# 导入已有资源到 State
terraform import <resource_type>.<name> <cloud_resource_id>

# 查看 State 中的资源列表
terraform state list

# 删除 State 中的资源记录（不删除实际资源）
terraform state rm <resource_address>

# 输出格式化的 Plan 文件
terraform plan -out=tfplan && terraform show -json tfplan > plan.json

# --- Pulumi ---
# 预览变更
pulumi preview

# 应用变更
pulumi up

# 刷新 State
pulumi refresh

# 查看 Stack 输出
pulumi stack output

# --- Crossplane ---
# 查看 Crossplane 管理的云资源
kubectl get managed

# 查看特定 Provider 的资源状态
kubectl get <provider-resource> -A

# 查看 Crossplane Provider 状态
kubectl get providers

# 查看 Composition 和 XRD
kubectl get compositions
kubectl get compositeresourcedefinitions

# --- IaC 安全扫描 ---
# Checkov 扫描 Terraform
checkov -d .

# tfsec 扫描
tfsec .

# Snyk IaC 扫描
snyk iac test
```
## 交叉引用

- [gitops-and-continuous-delivery.md](./gitops-and-continuous-delivery.md) — GitOps 与 IaC 的分层协作
- [cluster-api-and-fleet-management.md](./cluster-api-and-fleet-management.md) — Cluster API 作为 IaC 管理集群
- [developer-portal-and-platform-metrics.md](./developer-portal-and-platform-metrics.md) — 开发者自助申请触发 IaC 自动化
- [custom-resources.md](./custom-resources.md) — Crossplane 使用 CRD 管理云资源
- [operator-pattern.md](./operator-pattern.md) — Crossplane Provider 的 Operator 模式

## 参考链接

- [Terraform Documentation](https://developer.hashicorp.com/terraform/docs)
- [Pulumi Documentation](https://www.pulumi.com/docs/)
- [Crossplane Documentation](https://docs.crossplane.io/)
- [Checkov - Policy as Code for IaC](https://www.checkov.io/)
- [OpenTofu - Open Source Terraform Fork](https://opentofu.org/docs/)

## Related
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
