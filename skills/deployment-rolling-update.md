---
title: Deployment 滚动更新策略
description: '## 概述'
category: skills
tags:
- k8s
- deployment
- rolling-update
- maxSurge
- maxUnavailable
- rollout
- pause-resume
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Deployment 滚动更新策略 是什么
- 如何 Deployment 滚动更新策略
trigger_keywords:
- Deployment
- 滚动更新策略
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# Deployment 滚动更新策略

## 概述

RollingUpdate 是 Deployment 最常用的更新策略，通过逐步替换 Pod 实现零停机更新。核心参数 `maxSurge` 和 `maxUnavailable` 控制替换速度和可用性保障。

## 策略配置

```yaml
apiVersion: apps/v1
kind: Deployment
spec:
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 25%        # 更新期间可超出期望副本数的比例/数量
      maxUnavailable: 25%  # 更新期间允许不可用的最大比例/数量
```

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `maxSurge` | 25% | 更新期间允许创建的额外 Pod 数量。可以是绝对数或百分比 |
| `maxUnavailable` | 25% | 更新期间允许不可用的 Pod 数量 |

**约束**：`maxSurge` 和 `maxUnavailable` 不能同时为 0

## 滚动更新核心流程

```
1. 获取当前活跃的 ReplicaSet（最新的）
2. 扩容新 ReplicaSet → 确保有 Pod 可用
3. 缩容旧 ReplicaSet → 释放资源
4. 清理完成历史版本的老 ReplicaSet（超过 revisionHistoryLimit）
5. 更新 Deployment 状态
```

### 更新算法

```
设: desired = Deployment.Spec.Replicas
    maxSurge = N
    maxUnavailable = M

约束条件:
    totalPods <= desired + N          (maxSurge 上限)
    unavailablePods <= M              (maxUnavailable 上限)
    availablePods >= desired - M      (最少可用)
```

### maxUnavailable=0 的零停机更新

```yaml
spec:
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0  # 关键：不允许不可用
```

**行为**：新 Pod 必须 Ready 后才缩容旧 Pod，始终保持所有 Pod 可用。适合对可用性要求高的服务。

## 暂停与恢复

```bash
# 暂停滚动更新
kubectl rollout pause deployment/nginx

# 查看暂停状态
kubectl get deployment nginx -o jsonpath='{.spec.paused}'

# 恢复滚动更新
kubectl rollout resume deployment/nginx
```

**暂停期间的行为**：
- 不会创建新的 ReplicaSet
- 不会执行新旧 RS 之间的替换
- 但仍会响应 `kubectl scale` 命令

## 版本回滚

```bash
# 查看发布历史
kubectl rollout history deployment/nginx

# 回滚到上一个版本
kubectl rollout undo deployment/nginx

# 回滚到指定版本
kubectl rollout undo deployment/nginx --to-revision=2
```

## Progress Deadline

- 默认 `progressDeadlineSeconds = 600`（10 分钟）
- 如果滚动更新在 10 分钟内没有推进（新 Pod 没有变得 Available），Deployment Status 中 `Progressing=False`
- 可以通过 `kubectl rollout undo` 回滚

## 常见错误

| 错误 | 现象 | 原因 | 解决方案 |
|------|------|------|---------|
| selector 与 template.labels 不匹配 | 创建失败 | spec.selector 未包含 template.labels | 确保 selector 是 template.labels 的子集 |
| ProgressDeadlineSeconds 超时 | `Progressing=False` | 新 Pod 在 deadline 内未就绪 | 检查 readinessProbe、镜像拉取、资源限制 |
| maxSurge/maxUnavailable 配置冲突 | 更新卡住 | 同时为 0 | 至少一个值大于 0 |
| 回滚目标版本不存在 | `unable to find specified revision` | 目标 RS 已被清理 | 增大 `revisionHistoryLimit` |

## 相关技能

- [[skills/deployment-canary-and-bluegreen.md|[[金丝雀与蓝绿发布|金丝雀与蓝绿发布]]]]
- [[skills/deployment-workload-selection.md|[[工作负载控制器选型|工作负载控制器选型]]]]
- [[deployment|Deployment]]
- [[concepts/controller-pattern.md|控制器模式]]

## Related

- [[references/k8s-workloads-domain-guide.md|k8s-workloads-domain-guide]] — [[Kubernetes|Kubernetes]]es Workloads Domain Guide|Kubernetes Workloads Domain Guide]]
- [[deployment]] — Deployment

- [[concepts/controller-pattern.md|controller-pattern]]