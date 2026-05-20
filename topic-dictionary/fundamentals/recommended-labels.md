---
title: 推荐标签
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- helm
- mysql
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 推荐标签 是什么
- 如何 推荐标签
trigger_keywords:
- 推荐标签
- dictionary
title_en: Labels
---


# 推荐标签

## 概述

除了 `kubectl` 和 Dashboard 之外，还有许多工具可以可视化和管理 Kubernetes 对象。一组通用的推荐标签（Recommended Labels）允许这些工具以可互操作的方式工作，用所有工具都能理解的通用方式描述对象。

## 核心概念/原理

### 应用概念

这些推荐标签的元数据围绕"应用"（application）概念组织。Kubernetes 不是 PaaS，也没有强制性的应用正式概念。应用是非正式的，通过元数据描述。

**注意**：这些是推荐标签，它们使应用管理更容易，但任何核心工具都不强制要求使用它们。

共享标签和注解使用共同的前缀：`app.kubernetes.io`。没有前缀的标签属于用户私有。共享前缀确保共享标签不会干扰用户自定义标签。

### 推荐标签列表

| 键 | 描述 | 示例 | 类型 |
|---|------|------|------|
| `app.kubernetes.io/name` | 应用名称 | `mysql` | string |
| `app.kubernetes.io/instance` | 标识应用实例的唯一名称 | `mysql-abcxyz` | string |
| `app.kubernetes.io/version` | 应用当前版本 | `5.7.21` | string |
| `app.kubernetes.io/component` | 架构中的组件 | `database` | string |
| `app.kubernetes.io/part-of` | 该应用所属的更高级别应用名称 | `wordpress` | string |
| `app.kubernetes.io/managed-by` | 用于管理应用操作的工具 | `Helm` | string |

### 应用与应用实例

一个应用可以在 Kubernetes 集群中安装一次或多次（甚至在同一命名空间中）。应用名称和实例名称是分开记录的：

- `app.kubernetes.io/name`：应用的名称（如 `wordpress`）
- `app.kubernetes.io/instance`：实例名称（如 `wordpress-abcxyz`）

这使得应用和应用实例都可以被识别。每个应用实例必须有唯一的名称。

## 使用场景

- 使用 Helm、Kustomize 等工具部署应用时，统一标记所有相关资源。
- 在监控和可观测性平台中按应用、版本、组件维度聚合数据。
- 在 CI/CD 和 GitOps 工作流中追踪应用实例和部署来源。

## 最佳实践/注意事项

- 为充分发挥这些标签的作用，应在每个资源对象上都应用它们。
- 优先使用带 `app.kubernetes.io/` 前缀的推荐标签，避免与私有标签冲突。
- `version` 可以是 SemVer、修订哈希或任何有意义的版本标识符。

## 参考链接

- [Recommended Labels - Official Documentation](https://kubernetes.io/docs/concepts/overview/working-with-objects/common-labels/)
