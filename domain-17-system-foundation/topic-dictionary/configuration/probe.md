---
title: 探针
description: Probe（探针）是 Kubernetes 中容器健康检查机制的统称。kubelet 通过定期执行探针来检测容器的运行状态，决定容器是否需要重启、是否可以接收流...
summary: Probe（探针）是 Kubernetes 中容器健康检查机制的统称。kubelet 通过定期执行探针来检测容器的运行状态，决定容器是否需要重启、是否可以接收流...
category: dictionary
tags:
- k8s
- glossary
- probe
- health-check
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 探针 是什么
- Probe 详解
trigger_keywords:
- 探针
- Probe
- dictionary
prerequisites:
- kubectl-basics
---



# 探针

> **英文名**: Probe

## 概述

Probe（探针）是 Kubernetes 中容器健康检查机制的统称。kubelet 通过定期执行探针来检测容器的运行状态，决定容器是否需要重启、是否可以接收流量、或是否已完成启动。

## 核心概念/原理

### 三种探针类型

| 探针 | 作用 | 失败行为 |
|------|------|---------|
| Liveness Probe | 检测容器是否存活 | 终止并重启容器 |
| Readiness Probe | 检测容器是否就绪 | 从 Service Endpoints 移除 |
| Startup Probe | 检测容器是否启动完成 | 禁用其他探针直到成功 |

### 四种探测方式

| 方式 | 说明 | 成功条件 |
|------|------|---------|
| httpGet | 发送 HTTP GET 请求 | 状态码 200-399 |
| tcpSocket | 尝试建立 TCP 连接 | 连接成功 |
| exec | 执行命令 | 退出码 0 |
| grpc | gRPC Health Check | Health 状态 SERVING |

### 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| initialDelaySeconds | 0 | 容器启动后等待时间 |
| periodSeconds | 10 | 探测间隔 |
| timeoutSeconds | 1 | 探测超时 |
| successThreshold | 1 | 连续成功次数 |
| failureThreshold | 3 | 连续失败次数判定 |

## 关键机制或特性

- 探针从 K8s v1.0 开始支持，Startup Probe 在 v1.20 达到 stable。
- 探针的成功和失败由 kubelet 在节点本地执行，不经过 API Server。
- 每个探针的结果会记录在 Pod 的 Events 中。

## 使用场景与最佳实践

- 所有生产容器至少配置 Readiness Probe。
- 慢启动应用使用 Startup Probe 防止启动期间被误杀。
- 探针检查路径应反映应用的真实健康状态，避免过于简单的检查（如仅检查端口开放）。
- 合理设置探测间隔和阈值，平衡检测灵敏度和资源开销。

## 参考链接

- [Probe - Official Documentation](https://kubernetes.io/docs/concepts/workloads/pods/pod-lifecycle/#container-probes)

## Related

[[domain-17-system-foundation/topic-dictionary/configuration/liveness-probe.md|Liveness Probe]] | [[domain-17-system-foundation/topic-dictionary/configuration/readiness-probe.md|Readiness Probe]]
