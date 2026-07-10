---
title: CNCF Distribution 镜像仓库
description: Distribution 是 CNCF 毕业项目，提供 OCI 兼容的容器镜像仓库实现（即 Docker Registry v2），是大多数私有
  Registr...
summary: Distribution 是 CNCF 毕业项目，提供 OCI 兼容的容器镜像仓库实现（即 Docker Registry v2），是大多数私有
  Registr...
category: dictionary
tags:
- k8s
- glossary
- tooling
- registry
- container
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CNCF Distribution 镜像仓库 是什么
- Distribution 详解
trigger_keywords:
- CNCF Distribution 镜像仓库
- Distribution
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CNCF Distribution 镜像仓库（Distribution）

## 概述

Distribution 是 CNCF 毕业项目，提供 OCI 兼容的容器镜像仓库实现（即 Docker Registry v2），是大多数私有 Registry（Harbor、GHCR 等）的底层引擎。

## 核心概念/原理

- **OCI 标准**：完整实现 OCI Distribution Specification
- **Registry v2**：Docker Registry 的官方开源实现
- **广泛基础**：Harbor、GitLab Registry、AWS ECR 等基于此构建
- **CNCF 毕业**：经过大规模生产验证

## 关键机制或特性

- Pull/Push API（Manifest + Blob/Layer 管理）
- Tag 和 Digest 两种寻址方式
- Token-based Authentication（Bearer Token）
- 存储驱动（Filesystem/S3/GCS/Azure/OSS）
- 垃圾回收（`registry garbage-collect`）
- Referrers API（OCI 1.1 附件引用）

## 使用场景与最佳实践

- 企业内部私有镜像仓库
- 边缘场景的轻量镜像缓存
- CI/CD Pipeline 的镜像存储后端
- 开发环境 Registry 的本地替代
- OCI 制品（Helm Chart/WASM 等）存储

## 参考链接

- https://github.com/distribution/distribution
- https://distribution.github.io/distribution/

## Related

- [[系统基础/知识字典/tooling/harbor.md|Harbor]]
- [[系统基础/知识字典/fundamentals/docker.md|Docker]]
- [[系统基础/知识字典/security/notary-project.md|Notary Project]]


<!-- risk-assessed -->
