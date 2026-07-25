---
title: 域名服务
description: Kubernetes DNS 是集群内部的域名解析服务，为 Service 和 Pod 提供自动的 DNS 记录。CoreDNS 是 Kubernetes
  的默...
summary: Kubernetes DNS 是集群内部的域名解析服务，为 Service 和 Pod 提供自动的 DNS 记录。CoreDNS 是 Kubernetes
  的默...
category: dictionary
tags:
- k8s
- glossary
- dns
- coredns
- networking
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 域名服务 是什么
- DNS 详解
trigger_keywords:
- 域名服务
- DNS
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 域名服务

> **英文名**: DNS

## 概述

Kubernetes DNS 是集群内部的域名解析服务，为 Service 和 Pod 提供自动的 DNS 记录。CoreDNS 是 Kubernetes 的默认 DNS 实现。

## 核心概念/原理

### DNS 记录格式

- **Service**：`<service-name>.<namespace>.svc.cluster.local`
- **Headless Service**：返回所有后端 Pod 的 IP 地址。
- **Pod**：`<pod-ip-dashed>.<namespace>.pod.cluster.local`
- **StatefulSet Pod**：`<pod-name>.<headless-service>.<namespace>.svc.cluster.local`

### CoreDNS

CoreDNS 是 CNCF 毕业项目，通过 Kubernetes 插件机制部署。它支持丰富的插件生态，包括缓存、转发、日志等。

## 关键机制或特性

- `ndots` 配置影响 DNS 查询行为（默认 5，可能导致额外查询）。
- DNS 缓存（NodeLocal DNSCache）可显著减少 CoreDNS 负载。
- CoreDNS 的 `forward` 插件可将外部域名转发到上游 DNS。
- `dnsConfig` 字段允许自定义 Pod 的 DNS 配置。

## 使用场景与最佳实践

- 生产环境部署 NodeLocal DNSCache 减少 CoreDNS 压力。
- 调整 `ndots: 2` 减少不必要的 DNS 查询。
- 监控 CoreDNS 的 QPS、延迟和缓存命中率。
- 为外部服务配置 ExternalName Service 或 CoreDNS rewrite 规则。

## 参考链接

- [DNS - Official Documentation](https://kubernetes.io/docs/concepts/services-networking/dns-pod-service/)

## Related

[[23-实体/02-K8s核心组件/coredns.md|CoreDNS]]


<!-- risk-assessed -->
