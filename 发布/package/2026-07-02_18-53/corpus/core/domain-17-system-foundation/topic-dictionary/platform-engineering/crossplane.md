---
title: Crossplane
description: Crossplane 是 CNCF 孵化项目，将 Kubernetes 的控制循环扩展到基础设施管理领域。它使用 K8s CRD 声明式管理云资源（AWS/Az...
summary: Crossplane 是 CNCF 孵化项目，将 Kubernetes 的控制循环扩展到基础设施管理领域。它使用 K8s CRD 声明式管理云资源（AWS/Az...
category: dictionary
tags:
- k8s
- glossary
- crossplane
- iac
- platform-engineering
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Crossplane 是什么
- Crossplane 详解
trigger_keywords:
- Crossplane
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Crossplane

> **英文名**: Crossplane

## 概述

Crossplane 是 CNCF 孵化项目，将 Kubernetes 的控制循环扩展到基础设施管理领域。它使用 K8s CRD 声明式管理云资源（AWS/Azure/GCP），实现了基础设施即代码（IaC）的 Kubernetes 原生化。

## 核心概念/原理

### 核心概念

- **Provider**：云厂商的 CRD 扩展（AWS/Azure/GCP 等）。
- **Managed Resource (MR)**：单个云资源的 K8s 表示（如 RDS、S3）。
- **Composite Resource (XR)**：组合多个 MR 的抽象层。
- **Composition**：定义 XR 如何映射到具体的 MR。
- **Claim**：命名空间级别的资源请求（XR 的简化接口）。

### 与 Terraform 对比

| 特性 | Terraform | Crossplane |
|------|-----------|------------|
| 模型 | Plan + Apply | 持续调谐 |
| 状态管理 | tfstate 文件 | K8s etcd |
| 漂移修复 | 手动 terraform apply | 自动 |
| 管理界面 | CLI | K8s API |

## 关键机制或特性

- **持续调谐**：Controller 持续将云资源推向期望状态。
- **Composition 抽象**：团队通过 Claim 请求资源，无需了解底层细节。
- **跨云管理**：同一套 API 管理 AWS/Azure/GCP 资源。
- **Provider Config**：管理云凭证和连接配置。
- **Observe-Only**：导入已有云资源到 Crossplane 管理。

## 使用场景与最佳实践

- 平台团队使用 Crossplane 构建自助式基础设施平台。
- 定义 Composition 让开发者通过 Claim 请求数据库/存储/网络。
- 配合 Argo CD 实现应用 + 基础设施的统一 GitOps。
- 从 Terraform 迁移时使用 `provider-terraform` 桥接。
- 使用 Crossplane 的 Drift Detection 自动修复配置漂移。

## 参考链接

- [Crossplane Official](https://www.crossplane.io/)

## Related

- [[domain-17-system-foundation/知识字典/platform-engineering/operator-pattern.md|Operator Pattern]]
- [[domain-17-system-foundation/知识字典/operations/argo.md|Argo]]
- [[domain-17-system-foundation/知识字典/operations/gitops.md|GitOps]]
- [[domain-17-system-foundation/知识字典/platform-engineering/custom-resource.md|Custom Resource]]
- [[domain-17-system-foundation/知识字典/platform-engineering/manifest.md|Manifest]]


<!-- risk-assessed -->
