---
title: 基础设施即代码 x 多集群管理
description: 'title: 基础设施即代码 x 多集群管理'
category: general
tags:
- k8s
- helm
- argocd
- flux
- opa
- rbac
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 基础设施即代码 x 多集群管理 是什么
- 如何 基础设施即代码 x 多集群管理
trigger_keywords:
- 基础设施即代码
- 多集群管理
prerequisites:
- kubectl-basics
- helm-basics
- gitops-basics
- iac-basics
- policy-basics
created: "2026-05-23"
relationships:
  - target: "[[domain-07-platform-engineering/operate/13-multi-cluster-management]]"
    type: uses
  - target: "[[domain-17-system-foundation/topic-dictionary/configuration/secrets]]"
    type: uses
  - target: "[[entities/argocd]]"
    type: related_to
---

---
title: 基础设施即代码 x [[domain-07-platform-engineering/operate/13-multi-cluster-management|多集群管理]]
category: synthesis
tags:
- k8s
- iac
- multi-cluster
- crossplane
- gitops
- platform-engineering
- declarative
sources:
- concepts/infrastructure-as-code.md
- concepts/platform-engineering-idp.md
- concepts/gitops-principles.md
- concepts/multi-tenancy-isolation.md
- entities/crossplane.md
- entities/flux.md
created: 2026-05-21 14:00:00+00:00
updated: 2026-05-21 14:00:00+00:00
summary: "IaC 与多集群管理的交叉：Crossplane 将 IaC 模式引入 K8s 原生资源声明，GitOps 提供跨集群状态同步，两者结合实现声明式多集群编排——但集群生命周期与工作负载生命周期的治理差异仍是未解挑战。"
provenance:
  extracted: 0.2
  inferred: 0.7
  ambiguous: 0.1
base_confidence: 0.88
lifecycle: reviewed
lifecycle_changed: 2026-05-21

tier: supporting---

# 基础设施即代码 x 多集群管理

## 连接点

[[concepts/infrastructure-as-code|infrastructure-as-code]] 描述了 Terraform、Pulumi、Crossplane 等 IaC 工具的模式，[[concepts/platform-engineering-idp|platform-engineering-idp]] 提到 Crossplane 用于"平台抽象层——多云资源管理"，[[concepts/gitops-principles|gitops-principles]] 描述了 [[entities/argocd|ArgoCD]] 和 Flux 的多集群能力。但 wiki 中没有一页专门讨论 **IaC 如何从根本上改变多集群管理的范式**——从手动 kubectl 切换到声明式集群生命周期管理。

## 共现场景

- **Crossplane** 将云资源（RDS、S3、VPC）声明为 K8s CRD，通过 GitOps 在多个集群中同步——IaC 的"声明-应用"模式与 GitOps 的"协调-修复"模式在多集群场景下自然融合
- **ArgoCD ApplicationSet** 允许用一套模板在 N 个集群中部署应用，但 ApplicationSet 本身是 YAML 声明——这就是 IaC 模式
- **Flux Kustomization** 通过多目录/多仓库策略实现环境分层（dev/staging/prod），本质上是 IaC 的变量注入模式
- **Backstage Scaffolder + Crossplane** 组合实现"开发者请求资源 -> 自动生成 IaC 声明 -> GitOps 协调 -> 资源就绪"的全链路

## 交叉洞察

**核心洞察：IaC + GitOps + 多集群 = 平台工程的技术基石。** 这三者的组合产生了一个自洽的多集群管理模型：

```
开发者请求 (Backstage Scaffolder)
       ↓
IaC 声明 (Crossplane Composition / Helm + Kustomize)
       ↓
Git 提交 (单一事实源)
       ↓
GitOps 协调 (Flux/ArgoCD 跨 N 个集群同步)
       ↓
集群状态 = Git 声明 (持续修复漂移)
```

**这个模型解决了一个传统 IaC 无法解决的问题：漂移修复。** Terraform apply 是一次性的——apply 之后如果有人手动修改了资源，Terraform 不会自动修复。而 GitOps 的持续协调循环使集群状态始终趋近于 Git 声明，这是传统 IaC 在多集群场景下的根本性优势。

**但 IaC 和 GitOps 的边界在多集群场景下变得模糊：**
- Crossplane 是 IaC 还是 GitOps？它用 K8s CRD 声明基础设施（IaC），由 Operator 持续协调（GitOps）
- Helm + Kustomize 是 IaC 还是部署模板？它们描述期望状态（IaC），由 ArgoCD 持续同步（GitOps）
- ArgoCD Application 本身是 K8s 资源，可以被 Crossplane 管理——IaC 管理 GitOps，GitOps 部署 IaC

**关键区别在于责任边界：**
- **集群生命周期**（创建/销毁/升级节点池）仍然由传统 IaC（Terraform）处理，因为这是面向云 API 的操作
- **集群内工作负载**（应用部署/配置/策略）由 GitOps 处理，因为这是面向 K8s API 的操作
- **跨集群资源**（数据库、消息队列、DNS）由 Crossplane 处理，因为它在两者之间架起桥梁

## 张力与权衡

| 张力 | 详情 |
|------|------|
| **集群引导悖论** | GitOps 需要集群已经存在才能部署 ArgoCD/Flux。但集群本身的配置（CNI 选择、节点池规格、RBAC 初始角色）需要 IaC。谁来管理 IaC 的 IaC？常见的解决方案是用 Terraform 创建集群 + 安装 GitOps operator，之后一切交给 GitOps。 |
| **跨集群状态一致性** | 当 Flux 在 10 个集群中同步同一个 Kustomization 时，如果 3 个集群同步失败（网络中断、API Server 不可用），如何保证一致性？GitOps 本身不提供跨集群事务保证。 |
| **多集群 Git 策略** | 单仓库（所有集群共用一套 Kustomize overlay）vs. 多仓库（每个集群独立仓库）。单仓库简单但耦合；多仓库灵活但管理开销大。没有统一的最佳实践。 |
| **Secret 跨集群分发** | 同一 Secret 需要在多个集群中存在。SOPS 加密后提交 Git，每个集群的 Flux 用各自的 decryption key 解密。但 key 的管理和轮换在多集群场景下复杂度线性增长。 |
| **集群漂移的责任归属** | 当集群实际状态与 Git 声明不一致时，是 GitOps 的协调循环出了问题，还是 IaC 的底层基础设施发生了变化（如云厂商自动升级了节点 OS）？根因分析变得困难。 |

## 开放问题

- **GitOps 管理的集群数量上限：** ArgoCD 和 Flux 在管理 100+ 集群时的性能表现如何？是否需要分层架构（区域级 hub 集群管理边缘集群）？
- **Crossplane 与 Terraform 的长期关系：** Crossplane 声称可以替代 Terraform，但目前 Crossplane 的 Provider 生态远小于 Terraform。企业是否应该采用"集群内 Crossplane + 集群外 Terraform"的混合模式？
- **多集群 GitOps 的灾难恢复：** 当 Git 仓库不可用（GitHub outage）时，多集群 GitOps 的降级策略是什么？集群能否在离线状态下维持已知最后一次的良好状态？
- **策略即代码在多集群中的执行：** OPA Gatekeeper / Kyverno 策略如何在多集群中统一管理？策略本身是否需要通过 GitOps 分发？策略违反事件是否需要跨集群聚合？

## 相关

- [[concepts/infrastructure-as-code|infrastructure-as-code]]
- [[concepts/gitops-principles|gitops-principles]]
- [[concepts/platform-engineering-idp|platform-engineering-idp]]
- [[concepts/multi-tenancy-isolation|multi-tenancy-isolation]]
- [[crossplane]]
- [[flux]]
- [[entities/argocd|argocd]]

> *This page synthesizes patterns across multiple sources and domains.* ^[inferred]

## Related

- [[synthesis/kubeadm-cluster-operations|kubeadm-cluster-operations]]

- [[kyverno]] — Kyverno
- [[crossplane]] — Crossplane
- [[cni]] — CNI (Container Network Interface)
- [[sops]] — SOPS ([[domain-17-system-foundation/topic-dictionary/configuration/secrets|Secrets]] OPerationS)
- [[entities/argocd|argocd]] — ArgoCD
- [[entities/helm|Helm (entities)]]
- [[entities/argo|Argo Workflows]]
