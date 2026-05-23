---
title: 注解
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- helm
- ingress
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 注解 是什么
- 如何 注解
trigger_keywords:
- 注解
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- helm-basics
created: "2026-05-23"
---

# 注解

## 概述

[[Kubernetes|Kubernetes]] 注解（Annotations）用于将任意非标识性元数据附加到对象上。与标签不同，注解不用于识别和选择对象，但可以包含标签不允许的字符，大小和结构也更灵活。

## 核心概念/原理

### 注解与标签的区别

- **标签**：用于选择对象和查找满足特定条件的对象集合，键/值有严格的格式和长度限制。
- **注解**：不用于标识和选择对象，可以是小或大、结构化或非结构化的元数据，支持标签不允许的字符。

注解和标签都是以键/值映射的形式存在于对象的 `metadata` 中：

```yaml
metadata:
  annotations:
    key1: "value1"
    key2: "value2"
```

**注意**：键和值必须是字符串，不能使用数字、布尔值、列表等类型。

### 常见注解用途

- 由声明式配置层管理的字段（用于区分默认值、自动生成字段和自动扩缩容系统设置的字段）。
- 构建、发布或镜像信息（时间戳、发布 ID、Git 分支、PR 编号、镜像哈希、仓库地址）。
- 指向日志、监控、分析或审计仓库的链接。
- 客户端库或工具的调试信息（名称、版本、构建信息）。
- 用户或工具的来源信息（如其他生态系统组件相关对象的 URL）。
- 轻量级部署工具的元数据（配置或检查点）。
- 负责人联系方式或团队网站链接。
- 最终用户向实现发出的指令（修改行为或启用非标准特性）。

## 关键机制或特性

- **语法规则**：与标签类似，注解键由可选前缀和名称组成，用 `/` 分隔。名称段最多 63 个字符，前缀必须是 DNS 子域名（最多 253 个字符）。`kubernetes.io/` 和 `k8s.io/` 前缀保留给 Kubernetes 核心组件。
- 将此类信息存储在注解中而非外部数据库，有助于产生共享的客户端库和工具，便于部署、管理和内省。

## 使用场景

- 在 CI/CD 流水线中记录镜像构建信息。
- 为 [[Ingress|Ingress]] 控制器、负载均衡器或证书管理器提供配置提示。
- 记录对象的创建来源或管理工具信息（如 [[Helm|Helm]] 发布的元数据）。
- 存储对运维和调试有帮助的非结构化信息。

## 最佳实践/注意事项

- 不要在注解中存储敏感信息（应使用 Secret）。
- 自动化系统组件向最终用户对象添加注解时，必须指定前缀。
- 注解的大小没有硬性上限，但过于庞大的注解可能影响对象性能，应保持合理。

## 参考链接

- [Annotations - Official Documentation](https://kubernetes.io/docs/concepts/overview/working-with-objects/annotations/)

## Related

- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
