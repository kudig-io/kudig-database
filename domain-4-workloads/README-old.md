---
title: 'Domain-4: Kubernetes工作负载'
description: '## 概述'
category: workloads
tags:
- k8s
- workload
- pod
- deployment
- statefulset
- hpa
- vpa
- daemonset
- job
- cronjob
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 'Domain-4: Kubernetes工作负载 是什么'
- '如何 Domain-4: Kubernetes工作负载'
- Kubernetes 4 workloads 最佳实践
trigger_keywords:
- 'Domain-4:'
- Kubernetes工作负载
- workloads
cross_refs:
- type: domain
  path: ../domain-3-control-plane/
  label: '相关知识域: domain-3-control-plane'
- type: domain
  path: ../domain-8-observability/
  label: '相关知识域: domain-8-observability'
- type: cheatsheet
  path: ../topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
---

# Domain-4: Kubernetes工作负载

> **文档数量**: 24 篇 | **最后更新**: 2026-02 | **适用版本**: Kubernetes 1.20+

---

## 概述

Kubernetes工作负载域深入解析Pod、Deployment、StatefulSet、DaemonSet等核心工作负载资源的配置管理和最佳实践。

**核心价值**：
- 📦 **资源管理**：各类工作负载资源配置和管理
- 🔄 **生命周期**：Pod生命周期管理、健康检查
- 📊 **调度策略**：亲和性、污点容忍、资源约束
- 🛡️ **可靠性**：滚动更新、故障恢复、自愈机制

---

## 文档目录

### 核心工作负载 (01-08)
| # | 文档 | 关键内容 | 重要程度 |
|:---:|:---|:---|:---|
| 01 | [Pod配置详解](./01-pod-configuration.md) | 容器配置、卷挂载、环境变量 | ⭐⭐⭐⭐⭐ |
| 02 | [Deployment管理](./02-deployment-management.md) | 滚动更新、回滚策略、副本管理 | ⭐⭐⭐⭐⭐ |
| 03 | [StatefulSet配置](./03-statefulset-configuration.md) | 有状态应用、持久化存储、网络标识 | ⭐⭐⭐⭐⭐ |
| 04 | [DaemonSet部署](./04-daemonset-deployment.md) | 节点级守护进程、系统服务 | ⭐⭐⭐⭐ |
| 05 | [Job批处理](./05-job-batch-processing.md) | 批处理任务、定时任务、并行处理 | ⭐⭐⭐⭐ |
| 06 | [CronJob定时任务](./06-cronjob-scheduled-tasks.md) | 定时调度、任务管理、失败处理 | ⭐⭐⭐⭐ |
| 07 | [ReplicaSet管理](./07-replicaset-management.md) | 副本管理、标签选择器、扩缩容 | ⭐⭐⭐⭐ |
| 08 | [ReplicationController](./08-replicationcontroller.md) | 传统副本控制器、迁移指南 | ⭐⭐⭐ |

### 高级配置 (09-16)
| # | 文档 | 关键内容 | 重要程度 |
|:---:|:---|:---|:---|
| 09 | [资源请求与限制](./09-resource-requests-limits.md) | CPU/Memory配置、QoS等级 | ⭐⭐⭐⭐⭐ |
| 10 | [健康检查配置](./10-health-checks-configuration.md) | Liveness、Readiness、Startup探针 | ⭐⭐⭐⭐⭐ |
| 11 | [生命周期钩子](./11-lifecycle-hooks.md) | PostStart、PreStop钩子配置 | ⭐⭐⭐⭐ |
| 12 | [亲和性调度](./12-affinity-scheduling.md) | 节点亲和性、Pod亲和性、反亲和性 | ⭐⭐⭐⭐⭐ |
| 13 | [污点与容忍](./13-taints-tolerations.md) | 节点污点、Pod容忍、调度控制 | ⭐⭐⭐⭐⭐ |
| 14 | [拓扑分布约束](./14-topology-spread-constraints.md) | 跨区域部署、负载均衡 | ⭐⭐⭐⭐ |
| 15 | [优先级与抢占](./15-priority-preemption.md) | 优先级类、抢占机制、资源竞争 | ⭐⭐⭐⭐ |
| 16 | [服务质量等级](./16-quality-of-service.md) | QoS分类、驱逐策略、资源保障 | ⭐⭐⭐⭐⭐ |

### 工作负载模式 (17-20)
| # | 文档 | 关键内容 | 重要程度 |
|:---:|:---|:---|:---|
| 17 | [Init容器模式](./17-init-containers-pattern.md) | 初始化任务、依赖管理、配置准备 | ⭐⭐⭐⭐⭐ |
| 18 | [Sidecar模式](./18-sidecar-pattern.md) | 辅助容器、日志收集、监控代理 | ⭐⭐⭐⭐⭐ |
| 19 | [Ambassador模式](./19-ambassador-pattern.md) | 代理容器、服务网格集成 | ⭐⭐⭐⭐ |
| 20 | [Adapter模式](./20-adapter-pattern.md) | 接口适配、协议转换 | ⭐⭐⭐⭐ |

### 运维最佳实践 (21-24)
| # | 文档 | 关键内容 | 重要程度 |
|:---:|:---|:---|:---|
| 21 | [滚动更新策略](./21-rolling-update-strategies.md) | 更新策略、最大不可用、 surge控制 | ⭐⭐⭐⭐⭐ |
| 22 | [自动扩缩容](./22-auto-scaling.md) | HPA、VPA、Cluster Autoscaler | ⭐⭐⭐⭐⭐ |
| 23 | [故障排查指南](./23-troubleshooting-guide.md) | 工作负载相关故障诊断 | ⭐⭐⭐⭐⭐ |
| 24 | [生产最佳实践](./24-production-best-practices.md) | 生产环境配置、监控告警、安全加固 | ⭐⭐⭐⭐⭐ |

---

## 学习路径建议

### 🎯 基础入门路径
**01 → 02 → 09 → 10**  
掌握Pod和Deployment基础配置及资源管理

### 🔧 进阶配置路径  
**12 → 13 → 16 → 21**  
深入学习调度策略和更新管理

### 🏢 企业级应用路径
**17 → 18 → 22 → 24**  
掌握高级模式和生产最佳实践

---

## 相关领域

- **[Domain-1: 架构基础](../domain-1-architecture-fundamentals)** - K8s基础架构
- **[Domain-3: 控制平面](../domain-3-control-plane)** - 调度器配置
- **[Domain-5: 网络](../domain-5-networking)** - 服务网络配置

---

**维护者**: Kusheet Workloads Team | **许可证**: MIT