# Kubernetes 基础设施即代码（IaC）

## 概述

**基础设施即代码（Infrastructure as Code, IaC）** 是通过代码和声明式配置文件来管理和配置基础设施的实践。在 Kubernetes 生态中，IaC 不仅包括集群本身的创建（Terraform / Pulumi / Crossplane），还包括集群内部资源的编排（YAML / Helm / Kustomize / GitOps）。2026 年的最佳实践要求企业建立**从底层云资源到 K8s 应用配置的完整 IaC 流水线**，实现版本控制、可审计、可重复和自动化的基础设施管理。

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
- 与 GitOps 天然集成，云资源的变更也通过 Argo CD / Flux 管理

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

## 参考链接

- [Terraform Documentation](https://developer.hashicorp.com/terraform/docs)
- [Pulumi Documentation](https://www.pulumi.com/docs/)
- [Crossplane Documentation](https://docs.crossplane.io/)
- [Checkov - Policy as Code for IaC](https://www.checkov.io/)
- [OpenTofu - Open Source Terraform Fork](https://opentofu.org/docs/)
