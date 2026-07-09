---
title: SlimToolkit 容器优化
description: SlimToolkit（原 DockerSlim）是 CNCF Sandbox 项目，通过静态和动态分析自动缩小容器镜像体积（通常减少 10-30
  倍），同时保...
summary: SlimToolkit（原 DockerSlim）是 CNCF Sandbox 项目，通过静态和动态分析自动缩小容器镜像体积（通常减少 10-30
  倍），同时保...
category: dictionary
tags:
- k8s
- glossary
- tooling
- container
- optimization
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SlimToolkit 容器优化 是什么
- SlimToolkit 详解
trigger_keywords:
- SlimToolkit 容器优化
- SlimToolkit
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# SlimToolkit 容器优化（SlimToolkit）

## 概述

SlimToolkit（原 DockerSlim）是 CNCF Sandbox 项目，通过静态和动态分析自动缩小容器镜像体积（通常减少 10-30 倍），同时保持应用功能完整，是容器镜像优化的标准工具。

## 核心概念/原理

- **镜像瘦身**：自动分析并删除镜像中未使用的文件
- **CNCF Sandbox**：社区主导的容器优化工具
- **安全加固**：减少攻击面（删除不必要的包和工具）
- **零修改**：无需修改 Dockerfile 或应用代码

## 关键机制或特性

- `slim build` 分析并生成精简镜像
- 静态分析（文件系统扫描）
- 动态分析（运行容器并追踪文件访问）
- HTTP 探针（自动触发 API 端点）
- 安全配置文件生成（Seccomp/AppArmor）
- 镜像层级分析和可视化

## 使用场景与最佳实践

- CI/CD Pipeline 中的镜像自动优化
- 生产镜像的安全加固
- 镜像体积的成本优化
- 安全合规的攻击面减少
- 开发镜像到生产镜像的转换

## 参考链接

- https://slimtoolkit.org/
- https://github.com/slimtoolkit/slim

## Related

- [[系统基础/topic-dictionary/fundamentals/docker.md|Docker]]
- [[系统基础/topic-dictionary/tooling/buildpacks.md|Buildpacks]]
- [[系统基础/topic-dictionary/tooling/copa.md|Copa]]


<!-- risk-assessed -->
