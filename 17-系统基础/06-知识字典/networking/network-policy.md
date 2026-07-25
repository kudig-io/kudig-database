---
title: 网络策略
description: NetworkPolicy 是 Kubernetes 中用于控制 Pod 之间以及 Pod 与外部网络之间流量访问的网络安全资源。它基于标签选择器定义允许/拒绝...
summary: NetworkPolicy 是 Kubernetes 中用于控制 Pod 之间以及 Pod 与外部网络之间流量访问的网络安全资源。它基于标签选择器定义允许/拒绝...
category: dictionary
tags:
- k8s
- glossary
- network-policy
- security
- networking
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 网络策略 是什么
- NetworkPolicy 详解
trigger_keywords:
- 网络策略
- NetworkPolicy
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 网络策略

> **英文名**: NetworkPolicy

## 概述

NetworkPolicy 是 Kubernetes 中用于控制 Pod 之间以及 Pod 与外部网络之间流量访问的网络安全资源。它基于标签选择器定义允许/拒绝的入站和出站规则。

## 核心概念/原理

### 核心概念

- **默认策略**：Kubernetes 默认允许所有 Pod 之间的通信（无隔离）。
- **策略生效**：为 Pod 配置 NetworkPolicy 后，未明确允许的流量将被拒绝。
- **三要素**：
  - `podSelector`：选择策略适用的 Pod。
  - `ingress`：定义允许的入站规则。
  - `egress`：定义允许的出站规则。

### 示例：限制只允许特定 Pod 访问

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
spec:
  podSelector:
    matchLabels:
      app: database
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: backend
    ports:
    - port: 5432
```

## 关键机制或特性

- NetworkPolicy 需要 CNI 插件支持（Calico、Cilium 等），部分 CNI 不支持。
- 策略基于 IP CIDR 和标签选择器，不直接支持 FQDN（域名）策略。
- 空 `ingress: []` 表示拒绝所有入站，空 `egress: []` 表示拒绝所有出站。

## 使用场景与最佳实践

- 生产环境应为所有应用配置 NetworkPolicy 实现最小权限网络访问。
- 从默认拒绝策略开始，逐步添加允许规则。
- 使用 Cilium 的 FQDN Policy 实现基于域名的出站控制。
- 定期审计 NetworkPolicy 覆盖情况，确保无遗漏。

## 参考链接

- [NetworkPolicy - Official Documentation](https://kubernetes.io/docs/concepts/services-networking/network-policies/)

## Related

[[17-系统基础/06-知识字典/networking/network-policies.md|Network Policies]]


<!-- risk-assessed -->
