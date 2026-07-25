---
title: 外部名称
description: ExternalName 是 Service 的一种特殊类型，它将集群内部的 DNS 名称映射到外部的 DNS 名称（CNAME 记录），而不是将流量转发到
  P...
summary: ExternalName 是 Service 的一种特殊类型，它将集群内部的 DNS 名称映射到外部的 DNS 名称（CNAME 记录），而不是将流量转发到
  P...
category: dictionary
tags:
- k8s
- glossary
- networking
- service
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 外部名称 是什么
- ExternalName 详解
trigger_keywords:
- 外部名称
- ExternalName
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 外部名称

> **英文名**: ExternalName

## 概述

ExternalName 是 Service 的一种特殊类型，它将集群内部的 DNS 名称映射到外部的 DNS 名称（CNAME 记录），而不是将流量转发到 Pod。

## 核心概念/原理

### 核心概念

```yaml
apiVersion: v1
kind: Service
metadata:
  name: external-db
spec:
  type: ExternalName
  externalName: db.example.com
```

查询 `external-db.default.svc.cluster.local` 会返回 `db.example.com` 的 CNAME 记录。

### 使用场景

- 将外部数据库映射为集群内部名称。
- 引用其他集群中的服务。
- 渐进式迁移：从外部服务迁移到集群内部服务时，只需修改 Service 类型。

## 关键机制或特性

- ExternalName 不创建 Endpoints，不进行流量转发。
- CoreDNS 直接返回 CNAME 记录。
- 不支持端口映射，客户端使用 `externalName` 的默认端口。

## 使用场景与最佳实践

- 使用 ExternalName 统一管理外部服务的访问方式。
- 迁移外部服务到集群内时只需修改 Service 类型。

## 参考链接

- [ExternalName - Official Documentation](https://kubernetes.io/docs/concepts/services-networking/service/#externalname)

## Related

- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/networking/ingress.md|Ingress]]
- [[17-系统基础/06-知识字典/networking/clusterip.md|Clusterip]]
- [[17-系统基础/06-知识字典/networking/nodeport.md|Nodeport]]
- [[17-系统基础/06-知识字典/networking/loadbalancer.md|Loadbalancer]]


<!-- risk-assessed -->
