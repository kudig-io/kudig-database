---
title: KitOps ML 打包
description: KitOps 是 Jozu 开源的 CNCF Sandbox 项目，为 AI/ML 模型和数据集提供 OCI 打包和分发能力，将 ML 模型管理纳入标准的
  De...
summary: KitOps 是 Jozu 开源的 CNCF Sandbox 项目，为 AI/ML 模型和数据集提供 OCI 打包和分发能力，将 ML 模型管理纳入标准的
  De...
category: dictionary
tags:
- k8s
- glossary
- specialized-workloads
- ai-ml
- oci
tier: supporting
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KitOps ML 打包 是什么
- KitOps 详解
trigger_keywords:
- KitOps ML 打包
- KitOps
- dictionary
prerequisites:
- kubernetes
---



# KitOps ML 打包（KitOps）

## 概述

KitOps 是 Jozu 开源的 CNCF Sandbox 项目，为 AI/ML 模型和数据集提供 OCI 打包和分发能力，将 ML 模型管理纳入标准的 DevOps 工具链。

## 核心概念/原理

- **ML OCI 打包**：模型/数据集/代码的 OCI 打包
- **DevOps 集成**：ML 资产纳入标准 CI/CD
- **CNCF Sandbox**：Jozu 主导
- **Kitfile 规范**：声明式定义 ML 包内容

## 关键机制或特性

- Kitfile YAML 定义 ML 包
- `kit pack` 打包为 OCI 镜像
- `kit push/pull` 推送/拉取到 Registry
- 模型/数据集/代码统一管理
- 签名和验证（Cosign/Notation）
- Kitfile 参数化
- 多 Registry 支持

## 使用场景与最佳实践

- AI 模型的 DevOps 管理
- 模型版本控制和分发
- ML Pipeline 的资产打包
- 团队协作的模型共享
- 合规要求下的模型审计

## 参考链接

- https://kitops.ml/
- https://github.com/jozu-ai/kitops

## Related

- [[domain-17-system-foundation/知识字典/specialized-workloads/modelpack.md|ModelPack]]
- [[domain-17-system-foundation/知识字典/specialized-workloads/kserve.md|KServe]]
- [[domain-17-system-foundation/知识字典/security/notary-project.md|Notary Project]]
