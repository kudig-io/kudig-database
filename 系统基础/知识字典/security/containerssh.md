---
title: ContainerSSH SSH 代理
description: ContainerSSH 是开源的 SSH 服务器，将 SSH 连接代理到 Kubernetes Pod 或 Docker 容器中运行，为运维人员提供安全的容器...
summary: ContainerSSH 是开源的 SSH 服务器，将 SSH 连接代理到 Kubernetes Pod 或 Docker 容器中运行，为运维人员提供安全的容器...
category: dictionary
tags:
- k8s
- glossary
- security
- ssh
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
- ContainerSSH SSH 代理 是什么
- ContainerSSH 详解
trigger_keywords:
- ContainerSSH SSH 代理
- ContainerSSH
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# ContainerSSH SSH 代理（ContainerSSH）

## 概述

ContainerSSH 是开源的 SSH 服务器，将 SSH 连接代理到 Kubernetes Pod 或 Docker 容器中运行，为运维人员提供安全的容器 Shell 访问方式。

## 核心概念/原理

- **SSH 代理**：SSH 连接到容器/Pod 内部
- **认证代理**：支持 OIDC/LDAP/Kerberos 认证
- **安全审计**：完整的 SSH 会话审计和录制
- **多后端**：Kubernetes/Docker/本地 Shell

## 关键机制或特性

- SSH 协议服务器（标准 SSH 客户端连接）
- 后端：Kubernetes/Docker/Local
- OIDC/LDAP 认证后端
- 会话录制和回放
- 配置注入（环境变量/卷）
- 速率限制和访问控制
- Prometheus 指标

## 使用场景与最佳实践

- 运维人员的安全 Shell 访问
- 替代 `kubectl exec` 的 SSH 方案
- 合规要求下的会话审计
- 开发团队的容器远程访问
- 跳板机/堡垒机的容器化替代

## 参考链接

- https://containerssh.github.io/
- https://github.com/ContainerSSH/ContainerSSH

## Related

- [[系统基础/topic-dictionary/tooling/kubectl.md|kubectl]]
- [[系统基础/topic-dictionary/tooling/stern.md|Stern]]
- [[系统基础/topic-dictionary/security/rbac.md|RBAC]]


<!-- risk-assessed -->
