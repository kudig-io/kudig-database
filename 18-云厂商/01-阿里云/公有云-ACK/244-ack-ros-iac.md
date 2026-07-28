---
title: ACK 关联产品 - ROS 资源编排 (IaC)
description: ACK 基础设施即代码实践：ROS 与 Terraform 选型、ACK 集群 ROS 模板、堆栈部署与变更预览操作步骤、CI/CD 集成
summary: ACK 资源编排（ROS/IaC）实践指南，覆盖 ROS 与 Terraform 选型对比、ACK Pro 集群模板示例、堆栈创建/变更预览/回滚完整操作步骤与验证方法、GitOps 集成建议。
category: general
tags:
- cloud
- multi-cloud
- containerd
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- ACK 关联产品 - ROS 资源编排 (IaC) 是什么
- 如何 ACK 关联产品 - ROS 资源编排 (IaC)
- Kubernetes 12 cloud providers 最佳实践
trigger_keywords:
- ACK
- 关联产品
- ROS
- 资源编排
- IaC
- cloud
- providers
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- iac-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# ACK 关联产品 - ROS 资源编排 (IaC)

> **适用版本**: ACK v1.25 - v1.32 | **最后更新**: 2026-01

---

## 目录

- [IaC 方案对比 (ROS vs Terraform)](#iac-方案对比-ros-vs-terraform)
- [ROS 基本概念与架构](#ros-基本概念与架构)
- [ACK 集群 ROS 模板示例](#ack-集群-ros-模板示例)
- [堆栈操作步骤与验证](#堆栈操作步骤与验证)
- [资源依赖管理 (DependsOn)](#资源依赖管理-dependson)
- [CI/CD 与 GitOps 集成](#cicd-与-gitops-集成)

---

## IaC 方案对比 (ROS vs Terraform)

| 维度 | ROS (资源编排) | Terraform |
|:---|:---|:---|
| **托管方式** | 阿里云全托管，无需管理状态文件 | 自行管理或使用 TF Cloud 管理 State |
| **语言规范** | JSON / YAML (标准阿里云规范) | HCL (HashiCorp 专有语言) |
| **生态支持** | 阿里云产品深度集成，支持嵌套堆栈 | 多云支持，插件丰富 |
| **学习曲线** | 低 (如果你熟悉 YAML/JSON) | 中 (需学习 HCL 语法) |

---

## ROS 基本概念与架构

### 核心要素

- **模板 (Template)**: 定义资源的文本文件。
- **资源 (Resource)**: 如 `ALIYUN::CS::ManagedKubernetesCluster`。
- **参数 (Parameters)**: 动态输入。
- **输出 (Outputs)**: 返回值 (如集群 ID, Kubeconfig)。
- **堆栈 (Stack)**: 真正运行起来的资源集合。

### ACK 创建链路

1. **VPC/vSwitch**: 创建网络基础设施。
2. **Managed Kubernetes**: 定义控制面类型及参数。
3. **NodePool**: 定义计算节点规格及数量。

---

## ACK 集群 ROS 模板示例

```yaml
ROSTemplateFormatVersion: '2015-09-01'
Description: 创建一个 ACK Pro 托管集群
Parameters:
  VpcId:
    Type: String
    Description: 指定已有的 VPC ID
  VSwitchIds:
    Type: CommaDelimitedList
    Description: 指定交换机列表
Resources:
  AckCluster:
    Type: ALIYUN::CS::ManagedKubernetesCluster
    Properties:
      Name: my-ros-cluster
      VpcId: !Ref VpcId
      VSwitchIds: !Ref VSwitchIds
      ClusterSpec: ack.pro.small
      WorkerInstanceTypes: 
        - ecs.c7.xlarge
      ContainerRuntime: containerd
      RuntimeVersion: 1.6.20
Outputs:
  ClusterId:
    Value: !GetAtt AckCluster.ClusterId
```

---

## 堆栈操作步骤与验证

### 创建堆栈

```bash
# 🟢 低风险：先验证模板语法
aliyun ros ValidateTemplate --TemplateBody "$(cat ack-cluster.yaml)"

# 🟡 中风险：创建堆栈（会实际创建 ACK 集群与 ECS 资源）
aliyun ros CreateStack --StackName prod-ack-stack \
  --TemplateBody "$(cat ack-cluster.yaml)" \
  --Parameters.1.ParameterKey=VpcId --Parameters.1.ParameterValue=vpc-xxx \
  --Parameters.2.ParameterKey=VSwitchIds --Parameters.2.ParameterValue=vsw-xxx,vsw-yyy \
  --TimeoutInMinutes 60
```

### 验证堆栈状态

```bash
# 🟢 低风险：轮询堆栈状态，期望 CREATE_COMPLETE
aliyun ros GetStack --StackId <stack-id> | jq '{Status, StatusReason}'

# 🟢 低风险：查看输出值（集群 ID 等）
aliyun ros GetStack --StackId <stack-id> | jq '.Outputs'

# 🟢 低风险：创建失败时定位具体资源的失败原因
aliyun ros ListStackEvents --StackId <stack-id> | jq '.Events[] | select(.Status | endswith("FAILED")) | {LogicalResourceId, StatusReason}'
```

### 变更预览与更新（生产推荐流程）

```bash
# 🟢 低风险：生成变更集，预览受影响资源，不实际执行
aliyun ros CreateChangeSet --StackId <stack-id> --ChangeSetName pre-check-$(date +%m%d) \
  --TemplateBody "$(cat ack-cluster.yaml)"
aliyun ros GetChangeSet --ChangeSetId <changeset-id> | jq '.Changes'

# 🔴 高风险：确认变更集无意外删除（Action 为 Remove 的资源）后才执行
aliyun ros ExecuteChangeSet --ChangeSetId <changeset-id>
```

> 关键检查点：变更集中任何 `"Action": "Remove"` 或 `"Replacement": "True"` 的条目都意味着资源重建，对集群/节点池类资源将造成业务中断，必须走变更窗口。

---

## 资源依赖管理 (DependsOn)

在 IaC 编排中，资源创建的顺序至关重要。

### 处理逻辑

- **显式依赖**: 使用 `DependsOn` 关键字。
- **隐式依赖**: 通过 `!Ref` 或 `!GetAtt` 引用其他资源属性时，ROS 自动计算顺序。

| 场景 | 依赖关系 |
|:---|:---|
| **存储挂载** | ECS 节点池依赖于 NAS/OSS 的创建完成 |
| **网络隔离** | 安全组规则依赖于 VPC 和安全组 ID |
| **监控部署** | ARMS 组件依赖于 ACK 集群状态为 Running |

---

## CI/CD 与 GitOps 集成

### 工作流建议

```mermaid
graph LR
    A[代码提交 Git] --> B[Jenkins/Tekton 调用 CLI]
    B --> C[aliyun ros UpdateStack]
    C --> D[阿里云堆栈自动更新]
```

### 最佳实践

1. **版本化模板**: 将 ROS 模板存放在 Git 中，通过版本号控制环境一致性。
2. **变更预览 (ChangeSet)**: 在应用更新前生成变更集，预览受影响的资源，防止意外删除。
3. **标签管理**: 为所有通过 ROS 创建的资源打上 `iac:ros` 标签，方便成本分摊和资源审计。

---

## 相关文档

- [[11-发布变更/02-IaC/index|IaC 专题索引]]
- [[11-发布变更/02-IaC/01-terraform-enterprise-iac|Terraform 企业级 IaC]]
- [[18-云厂商/01-阿里云/index|阿里云域索引]]

## Related

- [[17-系统基础/05-速查卡/k8s.md|k8s]]
- [[17-系统基础/05-速查卡/gitops.md|gitops]]
- [[17-系统基础/05-速查卡/git.md|git]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]

## See Also

- [[18-云厂商/01-阿里云/公有云-ACK/242-ack-vpc-network.md|242-ack-vpc-network]]
- [[18-云厂商/01-阿里云/公有云-ACK/243-ack-ram-authorization.md|243-ack-ram-authorization]]
- [[18-云厂商/01-阿里云/公有云-ACK/245-ack-ebs-storage.md|245-ack-ebs-storage]]
- [[18-云厂商/01-阿里云/公有云-ACK/alicloud-ack-overview.md|alicloud-ack-overview]]


<!-- risk-assessed -->
