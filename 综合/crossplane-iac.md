---
title: Crossplane × Infrastructure as Code
summary: Crossplane 与基础设施即代码的交叉：Kubernetes 原生 IaC 控制器与 Terraform 类声明式工具的范式之争。
category: synthesis
tags:
- crossplane
- iac
- terraform
- kubernetes-native
- gitops
tier: supporting
sources:
- 实体/crossplane.md
- 概念/infrastructure-as-code.md
- 概念/IaC x 多集群管理.md
- 概念/controller-pattern.md
- 概念/declarative-api.md
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
provenance:
  extracted: 0.3
  inferred: 0.6
  ambiguous: 0.1
base_confidence: 0.74
lifecycle: draft
lifecycle_changed: '2026-07-11'
---

# Crossplane × Infrastructure as Code

## The Connection

传统 IaC（Terraform/OpenTofu/Pulumi）用外部进程对云资源做声明式 apply，状态存于独立的 state 文件。Crossplane 把"云资源"建模为 Kubernetes CRD，由集群内的控制器（control plane）持续 reconcile，状态存于 etcd。二者都声明式，但执行模型截然不同：IaC 是"外部 push 一次"（CI/CD pipeline 运行 `terraform apply` 后退出），Crossplane 是"集群内持续协调"（控制器 watch CRD 变更，持续比对期望状态与实际状态并自动纠偏）。这种差异决定了谁能与 GitOps/平台工程无缝融合——Crossplane 的 Provider CRD 本身就是 K8s 对象，天然可被 ArgoCD/Flux 同步，而 Terraform 需要额外的 Atlantis 或 TF Controller 才能进入 GitOps 流水线。从更深层的架构视角看，IaC 的状态管理是"离线"的（state 文件存在于 CI 运行时或远程后端），如果有人通过云控制台直接修改了资源，IaC 不会自动发现漂移；而 Crossplane 的控制器在每个 reconcile 周期都会对比 etcd 中的期望状态与云 API 返回的实际状态，发现偏差即纠偏——这种"在线 reconcile"是 Kubernetes 声明式模型向基础设施领域的延伸。^[inferred]

## Where They Co-occur

- **声明式抽象**：Terraform 用 HCL，Crossplane 用 CRD（YAML/Composition），都把"期望状态"与"实际资源"解耦。
- **GitOps 集成**：Crossplane 的 Provider CRD 本身就是 K8s 对象，天然可被 ArgoCD/Flux 同步；Terraform 需借助 Atlantis/TF Controller 才能进入 GitOps 流水线。
- **Composition（组合）**：Crossplane 用 Composition 把多个底层资源封装成一个"平台 API"，相当于 Terraform Module 的运行时版本。
- **状态管理**：Terraform state 易漂移、需后端加锁；Crossplane 状态在 etcd，控制器自动纠偏。
- **混合使用**：常见模式是用 Terraform 建集群、用 Crossplane 管集群内 + 云资源，或用 Terraform Provider for Kubernetes 调和。
- **Composition 的 XR (Composite Resource)**：Crossplane 的 `Composition` 定义了多个底层 Managed Resource（如 RDS + S3 + IAM）如何组装为一个面向应用的 `CompositeResourceDefinition`（XRD），相当于 Terraform Module 的运行时版本。
- **ProviderConfig 凭证隔离**：每个 Crossplane Provider 通过独立的 `ProviderConfig` 管理云凭证，可按 namespace 隔离——租户 A 的 Crossplane 资源使用 tenant-a AWS credentials，租户 B 使用 tenant-b。
- **Crossplane Function**：Crossplane v2 引入 Function（Pipeline 模式），允许用 Go/Python 编写动态 composition 逻辑（如根据 values 动态选择 Instance 类型），比静态 Composition 模板更灵活。
- **Watch-based Reconciliation**：Crossplane Provider 的 reconcile 频率影响云 API 调用量——过于频繁的 reconcile 会触发云 API rate limit，需通过 `--sync-period` 调优。
- **Composition 版本管理**：Composition 变更（如修改 XR 模板）时，已 provision 的 Managed Resource 不受影响（`UpdatePolicy: NoUpdate`），但新创建的 XR 会使用新模板——需通过版本化 XRD（`v1alpha1`/`v1beta1`）管理向后兼容。

## Cross-cutting Insight

IaC 把基础设施变成"代码仓库里的产物"，Crossplane 把基础设施变成"集群里的一等公民"。后者让平台团队能像对待 Deployment 一样对待数据库、VPC、Bucket——用同一套 RBAC、GitOps、可观测性治理。这是"基础设施即应用"的范式跃迁：基础设施不再由独立流水线驱动，而由控制平面持续守护。更深层的意义在于自助化（self-service）：在 IaC 模式下，应用团队要新建一个 RDS 实例需要提交 PR → 等基础设施团队审批 → 等 Terraform pipeline 运行——流程耗时数天。在 Crossplane 模式下，平台团队预定义好 Composition（如"标准 PostgreSQL"），应用团队只需在自己的 namespace 中创建一个 `XPostgreSQL` 实例（类似创建一个 Deployment），Crossplane 控制器自动 provision 底层云资源并注入连接 Secret——流程缩短到分钟级。这种"基础设施自助"的能力是平台工程黄金路径的终极形态。但 Crossplane 也有其局限：Provider 生态不如 Terraform 丰富（尤其是小众云厂商或私有云），Composition 的抽象设计需要深厚的领域知识，且所有基础设施状态依赖 etcd 健康——如果 etcd 挂了，不仅 K8s 挂，所有云资源的 reconcile 也停了。^[inferred]

## Tensions and Trade-offs

| 维度 | Terraform/OpenTofu (IaC) | Crossplane (K8s-native) | 结合注意事项 |
|---|---|---|---|
| 执行模型 | 外部进程 push | 集群控制器 reconcile | 纠偏实时性差异大 |
| 状态存储 | 独立 state 文件 + 锁 | etcd | state 漂移是 IaC 痛点 |
| GitOps 契合 | 需额外控制器 | 原生 CRD，天然契合 | 平台优先选 Crossplane |
| 生态成熟度 | 资源极全，Provider 最多 | Provider 在补齐 | 偏门资源仍需 Terraform |
| 删除/导入 | terraform import 复杂 | controller 自管生命周期 | 迁移需双轨过渡 |
| 团队边界 | 基础设施团队 owns | 平台/应用团队可 self-serve | 影响 Org 结构 |
| 回滚 | terraform destroy/apply 回滚 | 删除 CRD 触发控制器回收 | Crossplane 回滚更声明式但依赖控制器健康 |

## Open Questions

- 当云资源由 Crossplane 管理后，如何与 Terraform 已有 state 做安全的"导入 + 接管"迁移？是否有标准化的迁移工具？
- Crossplane Composition 的"平台 API"设计如何避免成为又一个泄漏抽象？Composition 版本演进如何兼容下游消费方？
- 在多集群环境下，一个 Crossplane 控制面管理多集群资源，其故障爆炸半径如何控制？是否需要 per-cluster Crossplane 实例？
- etcd 不可用时，Crossplane 管理的云资源如何保证不被误删或漂移？Crossplane 控制面宕机期间的云资源状态如何审计？

## Related

- [[实体/crossplane.md|Crossplane]]
- [[实体/opentofu.md|OpenTofu]]
- [[实体/kpt.md|kpt]]
- [[实体/carvel.md|Carvel]]
- [[实体/cdk8s.md|cdk8s]]
- [[概念/infrastructure-as-code.md|基础设施即代码]]
- [[概念/IaC x 多集群管理.md|IaC × 多集群管理]]
- [[概念/controller-pattern.md|控制器模式]]
- [[概念/declarative-api.md|声明式 API]]
- [[综合/argocd-gitops.md|ArgoCD × GitOps]]


<!-- risk-assessed -->
