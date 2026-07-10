---
title: CoreDNS
description: CoreDNS 是 Kubernetes 集群内置的 DNS 服务器，作为 kube-dns 的替代方案。它是 CNCF 毕业项目，以插件化架构提供灵活的
  DN...
summary: CoreDNS 是 Kubernetes 集群内置的 DNS 服务器，作为 kube-dns 的替代方案。它是 CNCF 毕业项目，以插件化架构提供灵活的
  DN...
category: dictionary
tags:
- k8s
- glossary
- coredns
- dns
- networking
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CoreDNS 是什么
- CoreDNS 详解
trigger_keywords:
- CoreDNS
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CoreDNS

> **英文名**: CoreDNS

## 概述

CoreDNS 是 Kubernetes 集群内置的 DNS 服务器，作为 kube-dns 的替代方案。它是 CNCF 毕业项目，以插件化架构提供灵活的 DNS 解析服务，是集群内服务发现的基础设施。

## 核心概念/原理

### 架构

CoreDNS 以 Deployment 形式运行在 kube-system 命名空间，通过 ConfigMap（`coredns`）配置插件链。

### 核心插件

| 插件 | 功能 |
|------|------|
| kubernetes | 解析集群内 Service/Pod DNS |
| forward | 转发外部 DNS 查询 |
| cache | DNS 响应缓存 |
| loop | 检测 DNS 转发循环 |
| errors | 错误日志 |
| health | 健康检查端点 |
| prometheus | 指标暴露 |

## 关键机制或特性

- 插件链按 Corefile 中的顺序执行。
- 支持 DNS-over-TLS 和 DNS-over-gRPC。
- 通过 `hosts` 插件可添加自定义 DNS 记录。
- `rewrite` 插件支持 DNS 记录重写。
- 指标通过 `/metrics` 端点暴露给 Prometheus。

## 使用场景与最佳实践

- 大集群启用 NodeLocal DNSCache 减少 CoreDNS 压力。
- 使用 `cache` 插件合理设置 TTL 减少查询量。
- 排查 DNS 问题时检查 CoreDNS Pod 日志和资源使用。
- 配置 `forward` 插件的上游 DNS 服务器。
- 使用 `rewrite` 插件处理内部域名映射。

## 参考链接

- [CoreDNS Official](https://coredns.io/)

## Related

- [[系统基础/topic-dictionary/networking/dns-resolution.md|DNS Resolution]]
- [[系统基础/topic-dictionary/networking/service.md|Service]]
- [[系统基础/topic-dictionary/networking/headless-service.md|Headless Service]]
- [[系统基础/topic-dictionary/networking/endpoint.md|Endpoints]]
- [[系统基础/topic-dictionary/observability/prometheus.md|Prometheus]]


<!-- risk-assessed -->
