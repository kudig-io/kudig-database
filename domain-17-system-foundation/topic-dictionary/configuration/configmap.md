---
title: 配置映射
description: 'ConfigMap 是 Kubernetes 中用于存储非敏感配置数据的 API 资源。它将配置与容器镜像解耦，使应用配置可以被集中管理和动态更新。...'
category: dictionary
tags:
- k8s
- glossary
- configmap
- configuration
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 配置映射 是什么
- ConfigMap 详解
trigger_keywords:
- 配置映射
- ConfigMap
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# 配置映射

> **英文名**: ConfigMap

## 概述

ConfigMap 是 Kubernetes 中用于存储非敏感配置数据的 API 资源。它将配置与容器镜像解耦，使应用配置可以被集中管理和动态更新。

## 核心概念/原理

### 使用方式

1. **环境变量**：通过 `envFrom` 或 `env` 注入容器环境变量。
2. **命令行参数**：作为容器启动命令的参数。
3. **文件挂载**：作为 Volume 挂载到容器中的文件。

### 示例

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
data:
  database_host: "mysql.default.svc"
  log_level: "info"
  config.yaml: |
    server:
      port: 8080
```

## 关键机制或特性

- ConfigMap 大小限制 1MB。
- 通过 Volume 挂载的 ConfigMap 更新会自动传播（有延迟）。
- 通过环境变量注入的 ConfigMap 更新需要重启 Pod。
- `subPath` 挂载单个文件时不会自动更新。

## 使用场景与最佳实践

- 敏感数据使用 Secret 而非 ConfigMap。
- 使用 Kustomize 或 Helm 管理 ConfigMap 的版本。
- 为 ConfigMap 设置合理的标签便于管理。
- 考虑使用外部配置中心（如 Apollo、Nacos）管理动态配置。

## 参考链接

- [ConfigMap - Official Documentation](https://kubernetes.io/docs/concepts/configuration/configmap/)

## Related

[[domain-17-system-foundation/topic-dictionary/configuration/configmaps.md|ConfigMaps]]
