---
title: 指标服务器
description: 'Metrics Server 是 Kubernetes 集群的资源指标聚合器，收集节点和 Pod 的 CPU/内存使用数据。它是 HPA、VPA 和 `kube...'
category: dictionary
tags:
- k8s
- glossary
- observability
- metrics
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 指标服务器 是什么
- Metrics Server 详解
trigger_keywords:
- 指标服务器
- Metrics Server
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# 指标服务器

> **英文名**: Metrics Server

## 概述

Metrics Server 是 Kubernetes 集群的资源指标聚合器，收集节点和 Pod 的 CPU/内存使用数据。它是 HPA、VPA 和 `kubectl top` 命令的数据源。

## 核心概念/原理

### 核心功能

- **节点指标**：CPU 和内存使用率。
- **Pod 指标**：每个 Pod/容器的 CPU 和内存使用率。
- **API 暴露**：通过 `metrics.k8s.io` API Group 提供指标数据。

### 使用方式

```bash
# 查看节点资源使用
kubectl top nodes

# 查看 Pod 资源使用
kubectl top pods -n <namespace>

# 查看容器级别
kubectl top pods -n <namespace> --containers
```

## 关键机制或特性

- Metrics Server 不是 Kubernetes 核心组件，需要单独安装。
- 数据每 15 秒采集一次（可配置）。
- 仅保留最近的数据点，不提供历史查询。
- 使用 Summary API 从 kubelet 获取指标。

## 使用场景与最佳实践

- 每个集群都应部署 Metrics Server（HPA 依赖）。
- 配置 `--kubelet-insecure-tls` 仅用于开发环境。
- 生产环境需要配置正确的 TLS 证书。
- 监控 Metrics Server 自身的可用性和延迟。

## 参考链接

- [Metrics Server - Official Documentation](https://github.com/kubernetes-sigs/metrics-server)

## Related

- [[domain-17-system-foundation/topic-dictionary/observability/prometheus.md|Prometheus]]
- [[domain-17-system-foundation/topic-dictionary/observability/grafana.md|Grafana]]
- [[domain-17-system-foundation/topic-dictionary/observability/alertmanager.md|Alertmanager]]
- [[domain-17-system-foundation/topic-dictionary/observability/kubernetes-events.md|Kubernetes Events]]
- [[domain-17-system-foundation/topic-dictionary/observability/logging.md|Logging]]
