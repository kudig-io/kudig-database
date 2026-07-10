---
title: OpenTofu IaC 工具
description: OpenTofu 是 Terraform 的开源分支（Linux Foundation 托管），在 HashiCorp 更改许可证后由社区发起，保持
  MPL 2...
summary: OpenTofu 是 Terraform 的开源分支（Linux Foundation 托管），在 HashiCorp 更改许可证后由社区发起，保持
  MPL 2...
category: dictionary
tags:
- k8s
- glossary
- tooling
- iac
- open-source
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenTofu IaC 工具 是什么
- OpenTofu 详解
trigger_keywords:
- OpenTofu IaC 工具
- OpenTofu
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# OpenTofu IaC 工具（OpenTofu）

## 概述

OpenTofu 是 Terraform 的开源分支（Linux Foundation 托管），在 HashiCorp 更改许可证后由社区发起，保持 MPL 2.0 开源许可，API 与 Terraform 1.x 兼容。

## 核心概念/原理

- **Terraform 分支**：从 Terraform 1.5.x fork，保持 API 兼容
- **MPL 2.0 许可**：保持真正的开源许可，无商业限制
- **Linux Foundation**：由 Linux Foundation 托管，社区治理
- **Provider 兼容**：可使用现有 Terraform Provider 生态

## 关键机制或特性

- `tofu init/plan/apply/destroy` 与 Terraform 命令兼容
- 支持 Terraform Registry 中的 Provider 和 Module
- State 后端兼容（S3、GCS、Consul 等）
- 社区驱动的 Provider 开发
- 与 Terragrunt 等工具链兼容
- OpenTofu Registry 独立 Provider 仓库

## 使用场景与最佳实践

- Terraform 的开源替代方案
- 需要真正开源许可的企业环境
- Kubernetes 基础设施管理（EKS/GKE/AKS）
- 多云基础设施编排
- GitOps 中的基础设施管理

## 参考链接

- https://opentofu.org/
- https://github.com/opentofu/opentofu

## Related

- [[domain-17-system-foundation/知识字典/platform-engineering/crossplane.md|Crossplane]]
- [[domain-17-system-foundation/知识字典/tooling/helm.md|Helm]]
- [[domain-17-system-foundation/知识字典/platform-engineering/backstage.md|Backstage]]


<!-- risk-assessed -->
