---
title: Kubernetes 部署策略最佳实践
description: '# Kubernetes 部署策略最佳实践'
summary: '本指南提供生产环境 Kubernetes 部署策略配置的最佳实践，涵盖从滚动更新到金丝雀部署的全方位内容 ^[inferred]。'
category: skills
tags:
- k8s
- deployment
- rolling-update
- blue-green
- canary
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 部署策略最佳实践 是什么
- 如何 Kubernetes 部署策略最佳实践
trigger_keywords:
- Kubernetes
- 部署策略最佳实践
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes 部署策略最佳实践

## 概述

本指南提供生产环境 Kubernetes 部署策略配置的最佳实践，涵盖从滚动更新到金丝雀部署的全方位内容 ^[inferred]。

## 部署策略对比

| 策略 | 优点 | 缺点 | 适用场景 |
|------|------|------|---------|
| **滚动更新** | 零停机，资源效率高 | 版本共存，回滚慢 | 大多数应用 |
| **蓝绿部署** | 快速回滚，零停机 | 资源需求高（双倍） | 关键应用 |
| **金丝雀部署** | 风险低，可验证 | 流程复杂 | 高风险应用 |
| **A/B 测试** | 灵活，可验证功能 | 配置复杂 | 功能验证 |

### 策略选择

- **低风险**：滚动更新（`maxUnavailable: 1`, `maxSurge: 1`）^[inferred]
- **中风险**：金丝雀部署（通过 Flagger 逐步放量，stepWeight: 10）^[inferred]
- **高风险**：蓝绿部署（双版本并行，通过 [[Service|Service]] selector 切换流量）^[inferred]

## 关键配置

### 滚动更新

```yaml
strategy:
  type: RollingUpdate
  rollingUpdate:
    maxUnavailable: 1
    maxSurge: 1

```

### 金丝雀部署（Flagger）

- `stepWeight: 10` — 每次增加 10% 流量 ^[inferred]
- `maxWeight: 50` — 最大 50% 流量 ^[inferred]
- `threshold: 5` — 失败阈值 5 次 ^[inferred]
- 监控指标：request-success-rate >= 99%，request-duration <= 500ms ^[inferred]

### 蓝绿部署

通过 Service 的 selector 在 blue/green 版本间切换 ^[inferred]。需要双倍资源 ^[ambiguous]。

## 健康检查配置

部署策略必须配合正确的健康检查 ^[inferred]：

- **livenessProbe**：initialDelaySeconds: 30, periodSeconds: 10 — 检测进程是否存活
- **readinessProbe**：initialDelaySeconds: 5, periodSeconds: 5 — 检测是否接收流量
- initialDelaySeconds 过短会导致部署中断 ^[inferred]

## 回滚策略

- 保留部署历史用于回滚 ^[inferred]
- `kubectl rollout undo deployment/<name>` — 回滚到上一版本
- `kubectl rollout undo deployment/<name> --to-revision=N` — 回滚到指定版本 ^[inferred]

## 常见陷阱

### 健康检查配置不当

initialDelaySeconds 设置过短会导致容器未完全启动就被判定为失败，引发部署中断 ^[inferred]。

### 资源限制配置不当

maxSurge 和 maxUnavailable 配置不当会导致部署时资源不足，Pod 无法调度 ^[inferred]。

### 回滚策略缺失

未配置回滚策略会导致故障恢复困难。应保留部署历史并测试回滚功能 ^[inferred]。

## 验证方法

- 检查 Deployment 状态和滚动更新进度
- 检查金丝雀状态：`kubectl get canary -n production`
- 测试部署后服务可用性 ^[inferred]

## 相关资源

- [[概念/k8s-production-best-practices.md|[[Kubernetes 生产环境最佳实践|Kubernetes 生产环境最佳实践]]]]
- [[deployment|Deployment]]
- [[技能/configure-health-probes.md|[[Configure Health Probes|Configure Health Probes]]]]
- [[概念/gitops-principles.md|[[GitOps 速查卡|GitOps]]ps Principles and Practice|GitOps Principles]]]]

## Related

- [[技能/deployment-workload-selection.md|deployment-workload-selection]] — 工作负载控制器选型
- [[技能/configure-health-probes.md|configure-health-probes]] — Configure Health Probes
- [[deployment]] — Deployment
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/k8s-production-best-practices.md|k8s-production-best-practices]] — Kubernetes 生产环境最佳实践

- [[平台工程/代码分析/deployment-create/09-canary-bluegreen.md|Deployment 金丝雀与蓝绿发布模式]]
```

<!-- risk-assessed -->
