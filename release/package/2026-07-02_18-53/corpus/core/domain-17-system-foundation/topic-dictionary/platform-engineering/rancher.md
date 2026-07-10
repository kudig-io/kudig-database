---
title: Rancher
description: Rancher 是 SUSE 的企业级 Kubernetes 管理平台，提供多集群管理、安全策略、应用目录和运维工具的统一界面。它降低了管理多个
  K8s 集群的...
summary: Rancher 是 SUSE 的企业级 Kubernetes 管理平台，提供多集群管理、安全策略、应用目录和运维工具的统一界面。它降低了管理多个
  K8s 集群的...
category: dictionary
tags:
- k8s
- glossary
- rancher
- multi-cluster
- management
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Rancher 是什么
- Rancher 详解
trigger_keywords:
- Rancher
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Rancher

> **英文名**: Rancher

## 概述

Rancher 是 SUSE 的企业级 Kubernetes 管理平台，提供多集群管理、安全策略、应用目录和运维工具的统一界面。它降低了管理多个 K8s 集群的复杂度，是企业多集群运维的主流方案之一。

## 核心概念/原理

### 核心功能

| 功能 | 说明 |
|------|------|
| Multi-Cluster | 统一管理 EKS/AKS/GKE/自建集群 |
| RKE2/K3s | 内置轻量级 K8s 发行版 |
| App Catalog | Helm Chart 应用市场 |
| Security | 全局 RBAC + OPA Gatekeeper |
| Monitoring | Prometheus + Grafana 一键启用 |
| Logging | 集中式日志收集 |

### Rancher 架构

```
Rancher Server → 管理多个 Downstream Clusters
                    ├── EKS
                    ├── AKS
                    ├── RKE2 (on-prem)
                    └── K3s (edge)
```

## 关键机制或特性

- **Fleet**：大规模多集群 GitOps 部署引擎。
- **Harvester**：HCI 超融合基础设施管理。
- **Longhorn**：内置分布式块存储。
- **NeuVector**：容器安全扫描和运行时保护。
- **Elemental**：边缘节点的 OS 管理。

## 使用场景与最佳实践

- 企业多集群管理统一使用 Rancher。
- 使用 Fleet 实现跨集群的 GitOps 部署。
- 边缘场景使用 K3s + Rancher 统一管理。
- 启用 Rancher 的 Monitoring 和 Logging 快速搭建可观测性。
- 配置全局安全策略确保所有集群一致性。

## 参考链接

- [Rancher Official](https://www.rancher.com/)

## Related

- [[domain-17-system-foundation/知识字典/tooling/k3s.md|K3s]]
- [[domain-17-system-foundation/知识字典/storage/longhorn.md|Longhorn]]
- [[domain-17-system-foundation/知识字典/operations/argo.md|Argo]]
- [[domain-17-system-foundation/知识字典/security/rbac.md|RBAC]]
- [[domain-17-system-foundation/知识字典/observability/prometheus.md|Prometheus]]


<!-- risk-assessed -->
