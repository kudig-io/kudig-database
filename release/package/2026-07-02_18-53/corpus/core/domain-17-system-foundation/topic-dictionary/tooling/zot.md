---
title: zot OCI 注册表
description: zot 是 Cisco 开源的 CNCF Sandbox 项目，轻量级 OCI 原生容器注册表，专为边缘和嵌入式场景优化，资源占用极低，支持
  OCI 1.1 规...
summary: zot 是 Cisco 开源的 CNCF Sandbox 项目，轻量级 OCI 原生容器注册表，专为边缘和嵌入式场景优化，资源占用极低，支持 OCI
  1.1 规...
category: dictionary
tags:
- k8s
- glossary
- tooling
- registry
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
- zot OCI 注册表 是什么
- zot 详解
trigger_keywords:
- zot OCI 注册表
- zot
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# zot OCI 注册表（zot）

## 概述

zot 是 Cisco 开源的 CNCF Sandbox 项目，轻量级 OCI 原生容器注册表，专为边缘和嵌入式场景优化，资源占用极低，支持 OCI 1.1 规范。

## 核心概念/原理

- **轻量级**：单二进制，极低资源占用
- **OCI 原生**：完整实现 OCI Distribution Spec
- **CNCF Sandbox**：Cisco 主导
- **边缘优化**：适用于资源受限的环境

## 关键机制或特性

- 支持 OCI Image/Artifact/Index
- Referrers API（OCI 1.1 附件引用）
- 搜索 API（OCI 搜索规范）
- 多架构镜像支持
- 同步复制（zot-to-zot）
- 认证（Bearer/Basic/LDAP）
- 存储驱动（文件系统/S3）

## 使用场景与最佳实践

- 边缘设备的本地 OCI Registry
- 开发环境的轻量镜像仓库
- CI/CD Pipeline 的临时 Registry
- IoT 设备的镜像分发
- OCI 制品（Helm/WASM/SBOM）的存储

## 参考链接

- https://zotregistry.dev/
- https://github.com/project-zot/zot

## Related

- [[domain-17-system-foundation/知识字典/tooling/distribution.md|Distribution]]
- [[domain-17-system-foundation/知识字典/tooling/harbor.md|Harbor]]
- [[domain-17-system-foundation/知识字典/security/notary-project.md|Notary Project]]


<!-- risk-assessed -->
