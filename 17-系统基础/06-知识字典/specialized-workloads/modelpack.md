---
title: ModelPack 模型打包
description: ModelPack 是将 AI/ML 模型打包为 OCI 镜像的工具和规范，利用容器 Registry 分发和版本管理 AI 模型，实现模型的标准化管理和部署。...
summary: ModelPack 是将 AI/ML 模型打包为 OCI 镜像的工具和规范，利用容器 Registry 分发和版本管理 AI 模型，实现模型的标准化管理和部署。...
category: dictionary
tags:
- k8s
- glossary
- specialized-workloads
- ai-ml
- oci
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- ModelPack 模型打包 是什么
- ModelPack 详解
trigger_keywords:
- ModelPack 模型打包
- ModelPack
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# ModelPack 模型打包（ModelPack）

## 概述

ModelPack 是将 AI/ML 模型打包为 OCI 镜像的工具和规范，利用容器 Registry 分发和版本管理 AI 模型，实现模型的标准化管理和部署。

## 核心概念/原理

- **模型即镜像**：将 AI 模型打包为 OCI 镜像
- **Registry 分发**：通过标准容器 Registry 分发模型
- **版本管理**：利用镜像标签管理模型版本
- **跨平台**：与 K8s/Kserve/Seldon 集成

## 关键机制或特性

- 模型文件打包为 OCI Layer
- 模型元数据（框架/精度/指标）
- 模型签名和验证
- 多 Registry 同步
- 与 KServe/Seldon/BentoML 集成
- Helm Chart 模型部署
- 模型拉取和缓存

## 使用场景与最佳实践

- AI 模型的版本管理
- 模型的分发和部署
- MLOps 的模型 Registry
- 多环境模型的同步
- 模型的签名和合规管理

## 参考链接

- https://github.com/modelpack/modelpack

## Related

- [[17-系统基础/06-知识字典/specialized-workloads/kserve.md|KServe]]
- [[17-系统基础/06-知识字典/specialized-workloads/seldon.md|Seldon]]
- [[17-系统基础/06-知识字典/scheduling/kaito.md|KAITO]]


<!-- risk-assessed -->
