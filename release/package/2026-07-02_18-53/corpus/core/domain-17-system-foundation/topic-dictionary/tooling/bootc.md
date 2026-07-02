---
title: bootc 容器启动系统
description: bootc 是 Red Hat 开源的项目，将 OCI 容器镜像作为操作系统的基础，实现以容器方式管理和更新整个操作系统，是 Fedora/CentOS
  的下一...
summary: bootc 是 Red Hat 开源的项目，将 OCI 容器镜像作为操作系统的基础，实现以容器方式管理和更新整个操作系统，是 Fedora/CentOS
  的下一...
category: dictionary
tags:
- k8s
- glossary
- tooling
- container
- os
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- bootc 容器启动系统 是什么
- bootc 详解
trigger_keywords:
- bootc 容器启动系统
- bootc
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# bootc 容器启动系统（bootc）

## 概述

bootc 是 Red Hat 开源的项目，将 OCI 容器镜像作为操作系统的基础，实现以容器方式管理和更新整个操作系统，是 Fedora/CentOS 的下一代系统交付方式。

## 核心概念/原理

- **容器即 OS**：使用 OCI 镜像定义完整的操作系统
- **原子更新**：通过 `bootc upgrade` 实现系统的原子更新和回滚
- **OSTree 底层**：基于 OSTree 的文件系统管理和引导
- **Red Hat 主导**：Fedora Bootc / RHEL Image Mode 的核心技术

## 关键机制或特性

- `bootc build` 从 Containerfile 构建可引导的系统镜像
- `bootc upgrade` 拉取新镜像并部署
- `bootc rollback` 回滚到上一版本
- `bootc switch` 切换到不同的镜像源
- 与 Podman / Buildah 集成
- 支持 Kubernetes 风格的配置注入（/usr 只读 + /var 可写）

## 使用场景与最佳实践

- 边缘设备和 IoT 的系统管理
- 不可变基础设施（Immutable Infrastructure）
- 大规模裸金属/VM 的系统交付
- 操作系统的安全补丁快速部署
- Kubernetes 节点操作系统的统一管理

## 参考链接

- https://containers.github.io/bootc/
- https://github.com/containers/bootc

## Related

- [[domain-17-system-foundation/topic-dictionary/fundamentals/docker.md|Docker]]
- [[domain-17-system-foundation/topic-dictionary/tooling/podman.md|Podman]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/runc.md|runc]]


<!-- risk-assessed -->
