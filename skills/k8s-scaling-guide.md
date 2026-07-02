---
title: Kubernetes 扩缩容最佳实践
description: '# Kubernetes 扩缩容最佳实践'
summary: '本指南提供生产环境 Kubernetes 扩缩容配置的最佳实践，涵盖从 HPA 到集群自动扩缩容的全方位内容 ^[inferred]。'
category: skills
tags:
- k8s
- scaling
- hpa
- vpa
- cluster-autoscaler
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 扩缩容最佳实践 是什么
- 如何 Kubernetes 扩缩容最佳实践
trigger_keywords:
- Kubernetes
- 扩缩容最佳实践
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes 扩缩容最佳实践

## 概述

本指南提供生产环境 Kubernetes 扩缩容配置的最佳实践，涵盖从 HPA 到集群自动扩缩容的全方位内容 ^[inferred]。

## 扩缩容架构

三层扩缩容体系 ^[inferred]：

- **Pod 水平扩缩容（HPA）**：基于 CPU/内存/自定义指标调整 Pod 副本数
- **Pod 垂直扩缩容（VPA）**：基于历史使用情况调整 Pod 资源请求/限制
- **集群自动扩缩容（Cluster Autoscaler）**：根据未调度 Pod 自动增减节点

## HPA 配置

### 推荐配置

```yaml
spec:
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        averageUtilization: 80
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 10
        periodSeconds: 60
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
      - type: Percent
        value: 100
        periodSeconds: 60
```

### 关键参数

- **扩容稳定窗口**：60 秒 — 快速响应负载增长 ^[inferred]
- **缩容稳定窗口**：300 秒（5 分钟）— 避免频繁缩容 ^[inferred]
- **每次最多缩容 10%** — 渐进式缩容避免服务中断 ^[inferred]
- **每次最多扩容 100%** — 快速响应流量高峰 ^[inferred]

## VPA 配置

- `updateMode: "Auto"` — 自动更新资源建议 ^[inferred]
- 配置 `minAllowed` 和 `maxAllowed` 边界 ^[inferred]
- **不应与 HPA 同时用于同一资源维度** — 会导致冲突和扩缩容行为异常 ^[inferred]

## Cluster Autoscaler 配置

- `--expander=least-waste` — 选择浪费最少的节点组 ^[inferred]
- `--balance-similar-node-groups` — 平衡相似节点组 ^[inferred]
- `--skip-nodes-with-system-[[Pods|pods]]=false` — 允许缩容有系统 Pod 的节点 ^[inferred]

## 常见陷阱

### HPA 指标配置不当导致震荡

未配置稳定窗口或阈值不合理会导致 Pod 数量频繁变化。应配置 scaleDown stabilizationWindowSeconds: 300 和渐进式缩容策略 ^[inferred]。

### VPA 与 HPA 冲突

VPA 和 HPA 同时配置到同一 Deployment 会导致扩缩容行为异常。推荐仅使用 HPA 进行副本扩缩容，VPA 仅用于资源建议（`updateMode: "Off"` 或 `"Initial"`）^[inferred]。

### Cluster Autoscaler 配置不当

未配置 `--balance-similar-node-groups` 会导致节点组间资源分配不均。未配置 `--expander=least-waste` 会导致成本增加 ^[inferred]。

## 验证方法

- 检查 Metrics Server 状态
- 检查 HPA/VPA/Cluster Autoscaler 状态
- 进行负载测试验证扩缩容响应 ^[inferred]

## 相关资源

- [[concepts/k8s-production-best-practices.md|[[Kubernetes 生产环境最佳实践|Kubernetes 生产环境最佳实践]]]]
- [[concepts/autoscaling-strategies.md|[[Autoscaling Strategies|Autoscaling Strategies]]]]
- [[concepts/resource-management.md|[[Resource Management (Requests, Limits, QoS)|Resource Management]]]]
- [[skills/k8s-deployment-strategies-guide.md|[[Kubernetes 部署策略最佳实践|Kubernetes 部署策略最佳实践]]]]

## Related

- [[deployment]] — Deployment
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[concepts/k8s-production-best-practices.md|k8s-production-best-practices]] — Kubernetes 生产环境最佳实践
- [[concepts/autoscaling-strategies.md|autoscaling-strategies]] — Autoscaling Strategies


<!-- risk-assessed -->
