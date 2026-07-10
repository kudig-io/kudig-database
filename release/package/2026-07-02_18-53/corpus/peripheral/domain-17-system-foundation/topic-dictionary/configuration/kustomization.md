---
title: Kustomization 配置清单
description: Kustomization 是 Kustomize 的核心配置文件，通过 kustomization.yaml 定义基础资源（bases）和叠加层（overla...
summary: Kustomization 是 Kustomize 的核心配置文件，通过 kustomization.yaml 定义基础资源（bases）和叠加层（overla...
category: dictionary
tags:
- k8s
- glossary
- configuration
- kustomize
- overlay
tier: peripheral
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kustomization 配置清单 是什么
- Kustomization 详解
trigger_keywords:
- Kustomization 配置清单
- Kustomization
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kustomization 配置清单（Kustomization）

## 概述

Kustomization 是 Kustomize 的核心配置文件，通过 kustomization.yaml 定义基础资源（bases）和叠加层（overlays），实现声明式、无模板的 K8s 配置管理。

## 核心概念/原理

- **无模板**：直接操作 YAML，不使用模板语言
- **叠加模式**：base + overlay 的分层配置
- **K8s 内置**：kubectl apply -k 原生支持
- **声明式**：所有变更通过 patch 声明

## 关键机制或特性

- kustomization.yaml 入口文件
- resources 声明基础资源列表
- bases 引入其他 kustomization
- patchesStrategicMerge 策略合并补丁
- patchesJson6902 JSON Patch
- commonLabels/commonAnnotations 全局标签
- namePrefix/nameSuffix 名称前缀
- generators（ConfigMap/Secret 生成器）

## 使用场景与最佳实践

- 多环境配置的差异化管理
- 上游 YAML 的定制化修改
- GitOps 配置管理（Flux/ArgoCD）
- 团队间的配置隔离
- 最佳实践：overlay 不超过 3 层、bases 保持纯净、用 components 替代重复 overlay

## 参考链接

- https://kubectl.docs.kubernetes.io/references/kustomize/kustomization/
- https://kustomize.io/

## Related

- [[domain-17-system-foundation/知识字典/tooling/kustomize.md|Kustomize]]
- [[domain-17-system-foundation/知识字典/tooling/kpt.md|kpt]]
- [[domain-17-system-foundation/知识字典/tooling/helm.md|Helm]]


<!-- risk-assessed -->
