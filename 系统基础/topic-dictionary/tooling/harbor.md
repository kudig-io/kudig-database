---
title: Harbor
description: Harbor 是 CNCF 毕业项目，提供企业级容器镜像和 Helm Chart 的托管、扫描和分发服务。它内置漏洞扫描（Trivy）、镜像签名（Notary）...
summary: Harbor 是 CNCF 毕业项目，提供企业级容器镜像和 Helm Chart 的托管、扫描和分发服务。它内置漏洞扫描（Trivy）、镜像签名（Notary）...
category: dictionary
tags:
- k8s
- glossary
- harbor
- container-registry
- security
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Harbor 是什么
- Harbor 详解
trigger_keywords:
- Harbor
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Harbor

> **英文名**: Harbor

## 概述

Harbor 是 CNCF 毕业项目，提供企业级容器镜像和 Helm Chart 的托管、扫描和分发服务。它内置漏洞扫描（Trivy）、镜像签名（Notary）、RBAC 和复制策略，是私有容器仓库的首选方案。

## 核心概念/原理

### 核心功能

| 功能 | 说明 |
|------|------|
| Image Repository | 容器镜像托管 |
| Vulnerability Scanning | 自动 Trivy/Clair 扫描 |
| Image Signing | Notary/Cosign 签名验证 |
| Replication | 跨地域镜像复制策略 |
| RBAC | 项目级别的访问控制 |
| Webhook | 镜像推送/拉取事件通知 |

### 与 Docker Hub 对比

| 特性 | Harbor | Docker Hub |
|------|--------|------------|
| 部署方式 | 自建/私有 | 公有云 |
| 漏洞扫描 | 内置 | 无 |
| 镜像签名 | 内置 | 无 |
| 复制策略 | 灵活 | 无 |

## 关键机制或特性

- **Project**：Harbor 的逻辑隔离单元（类似 K8s Namespace）。
- **Tag Retention**：自动清理过期或多余的镜像 Tag。
- **Proxy Cache**：代理缓存公共 Registry 加速拉取。
- **P2P 分发**：通过 Dragonfly 实现高效镜像分发。
- 支持 OIDC/LDAP/AD 认证集成。

## 使用场景与最佳实践

- 企业环境部署 Harbor 作为私有容器镜像仓库。
- 配置自动漏洞扫描，阻止高风险镜像部署。
- 使用复制策略同步镜像到多个数据中心。
- 启用镜像签名验证确保部署的镜像未被篡改。
- 配置 Tag Retention 策略自动清理过期镜像。

## 参考链接

- [Harbor Official](https://goharbor.io/)

## Related

- [[系统基础/topic-dictionary/security/trivy.md|Trivy]]
- [[系统基础/topic-dictionary/tooling/helm.md|Helm]]
- [[系统基础/topic-dictionary/security/certificate.md|Certificate]]
- [[系统基础/topic-dictionary/security/rbac.md|RBAC]]
- [[系统基础/topic-dictionary/workloads/deployment.md|Deployment]]


<!-- risk-assessed -->
