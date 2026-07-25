---
title: DNS 解析
description: DNS Resolution（DNS 解析）在 Kubernetes 中指将 Service 名称或 Pod 的 DNS 记录转换为 IP
  地址的过程。集群内部...
summary: DNS Resolution（DNS 解析）在 Kubernetes 中指将 Service 名称或 Pod 的 DNS 记录转换为 IP 地址的过程。集群内部...
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
- DNS 解析 是什么
- DNS Resolution 详解
trigger_keywords:
- DNS 解析
- DNS Resolution
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# DNS 解析

> **英文名**: DNS Resolution

## 概述

DNS Resolution（DNS 解析）在 Kubernetes 中指将 Service 名称或 Pod 的 DNS 记录转换为 IP 地址的过程。集群内部 DNS 由 CoreDNS 提供，遵循 `<service>.<namespace>.svc.cluster.local` 的命名格式。

## 核心概念/原理

### DNS 记录格式

| 资源类型 | DNS 格式 | 示例 |
|----------|----------|------|
| Service (ClusterIP) | `<svc>.<ns>.svc.cluster.local` | `nginx.default.svc.cluster.local` |
| Headless Service | 返回所有 Pod IP | `db.default.svc.cluster.local` → 多个 A 记录 |
| Pod | `<pod-ip-dashed>.<ns>.pod.cluster.local` | `10-244-0-5.default.pod.cluster.local` |
| SRV 记录 | `_<port>._<proto>.<svc>.<ns>.svc.cluster.local` | 用于发现命名端口 |

### 解析流程

Pod 内的 DNS 查询 → Pod DNS Config → CoreDNS → 上游 DNS（如需要）

## 关键机制或特性

- CoreDNS 以 Deployment 形式运行在 kube-system 命名空间。
- Pod 的 `/etc/resolv.conf` 由 kubelet 自动配置指向 CoreDNS。
- `dnsPolicy` 控制 Pod 的 DNS 行为：`ClusterFirst`（默认）、`Default`、`None`。
- NodeLocal DNSCache 减少 CoreDNS 压力，提升解析性能。

## 使用场景与最佳实践

- 排查 DNS 问题时使用 `nslookup` 或 `dig` 测试解析。
- 大集群启用 NodeLocal DNSCache 避免 CoreDNS 成为瓶颈。
- 使用 `ndots:2` 减少不必要的域名后缀搜索。
- 外部 DNS 查询使用 ExternalDNS 管理云 DNS 记录。

## 参考链接

- [DNS for Services and Pods - Kubernetes Docs](https://kubernetes.io/docs/concepts/services-networking/dns-pod-service/)

## Related

- [[17-系统基础/06-知识字典/networking/coredns.md|CoreDNS]]
- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/networking/headless-service.md|Headless Service]]
- [[17-系统基础/06-知识字典/networking/endpoint.md|Endpoints]]
- [[17-系统基础/06-知识字典/networking/networkpolicy.md|NetworkPolicy]]


<!-- risk-assessed -->
