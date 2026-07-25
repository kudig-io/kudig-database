---
title: Copa 容器补丁工具
description: Copa（Container Patching）是微软开源的 CNCF Sandbox 项目，无需访问源代码或 Dockerfile 即可直接修补容器镜像中的
  ...
summary: Copa（Container Patching）是微软开源的 CNCF Sandbox 项目，无需访问源代码或 Dockerfile 即可直接修补容器镜像中的
  ...
category: dictionary
tags:
- k8s
- glossary
- tooling
- security
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
- Copa 容器补丁工具 是什么
- Copa 详解
trigger_keywords:
- Copa 容器补丁工具
- Copa
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Copa 容器补丁工具（Copa）

## 概述

Copa（Container Patching）是微软开源的 CNCF Sandbox 项目，无需访问源代码或 Dockerfile 即可直接修补容器镜像中的 OS 包漏洞，大幅降低容器漏洞修复的门槛。

## 核心概念/原理

- **无源码修补**：直接修补已有镜像中的 OS 包漏洞
- **Trivy 集成**：使用 Trivy 扫描结果驱动修补
- **CNCF Sandbox**：微软主导，社区活跃
- **零重建**：无需重新构建镜像即可修复漏洞

## 关键机制或特性

- `copa patch` 根据扫描报告修补镜像
- 支持 Debian/Ubuntu/Alpine/RHEL/Amazon Linux
- Trivy SARIF/JSON 格式扫描报告输入
- 修补后的镜像验证（重新扫描确认修复）
- 支持自定义包源和镜像 Registry
- 批量修补（batch patching）

## 使用场景与最佳实践

- 紧急漏洞的快速修复（无需等待上游重建）
- 遗留镜像的漏洞修补
- CI/CD Pipeline 中的自动漏洞修补
- 合规要求下的漏洞 SLA 管理
- 第三方镜像的安全加固

## 参考链接

- https://project-copa.dev/
- https://github.com/project-copacetic/copacetic

## Related

- [[17-系统基础/06-知识字典/security/trivy.md|Trivy]]
- [[17-系统基础/06-知识字典/fundamentals/docker.md|Docker]]
- [[17-系统基础/06-知识字典/security/supply-chain-security.md|供应链安全]]


<!-- risk-assessed -->
