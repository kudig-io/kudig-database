---
title: Deployment 滚动更新策略
description: '## 概述'
summary: 'RollingUpdate 是 Deployment 最常用的更新策略，通过逐步替换 Pod 实现零停机更新。核心参数 `maxSurge` 和 `maxUnavailable` 控制替换速度和可用性保障。'
category: skills
tags:
- k8s
- deployment
- rolling-update
- maxSurge
- maxUnavailable
- rollout
- pause-resume
tier: core
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
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

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
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

- [[26-技能/04-工作负载/deployment/deployment-canary-and-bluegreen.md|[[金丝雀与蓝绿发布|金丝雀与蓝绿发布]]]]
- [[26-技能/04-工作负载/deployment/deployment-workload-selection.md|[[工作负载控制器选型|工作负载控制器选型]]]]
- [[deployment|Deployment]]
- [[22-概念/01-核心架构/controller-pattern.md|控制器模式]]

## 生产案例

### 案例 1: 滚动更新期间新 Pod 健康检查失败导致回滚

| 时间 | 事件 |
|------|------|
| 14:00 | 执行滚动更新，新 Pod 启动后 readinessProbe 失败 |
| 14:05 | 新 Pod 被标记为 NotReady，旧 Pod 继续服务 |
| 14:08 | progressDeadlineSeconds 超时，Deployment 标记为 Failed |
| 14:10 | 🟡 `kubectl rollout undo deployment/app` 回滚 |

**根因**: 新版本应用启动时间超过 readinessProbe 的 initialDelaySeconds。

### 案例 2: 滚动更新导致数据库连接池耗尽

**现象**: 更新期间数据库连接数飙升，旧 Pod 未完全终止时新 Pod 已启动。

**诊断**: maxSurge=100% 导致新旧 Pod 同时运行，连接数翻倍

**修复**: 🟢 设置 maxSurge=25%，配置 preStop hook 优雅关闭连接

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 更新导致服务中断 | 立即 rollout undo |
| P1 | 更新卡住 | 检查新 Pod 状态 |
| P2 | 更新策略优化 | 调整 maxSurge/maxUnavailable |

## 面试要点

1. **Q: 滚动更新的零停机关键配置？**
   A: ① readinessProbe 确保新 Pod 就绪后才接收流量 ② preStop hook 等待存量请求处理完 ③ terminationGracePeriodSeconds 足够长 ④ maxUnavailable 控制最小可用数。

2. **Q: preStop hook 与 SIGTERM 的执行顺序？**
   A: 终止流程: ① Pod 标记为 Terminating ② 从 Service Endpoints 移除 ③ 执行 preStop hook ④ 发送 SIGTERM ⑤ 等待 terminationGracePeriodSeconds ⑥ 发送 SIGKILL。

3. **Q: 如何实现金丝雀发布？**
   A: 原生: 创建两个 Deployment(不同 label)，通过 Service 权重分配；进阶: Argo Rollouts/Flagger 自动分析指标并决定继续/回滚。

## Related

- [[23-实体/15-参考与索引/k8s-workloads-domain-guide.md|k8s-workloads-domain-guide]] — [[Kubernetes|Kubernetes]]es Workloads Domain Guide|Kubernetes Workloads Domain Guide]]
- [[deployment]] — Deployment

- [[22-概念/01-核心架构/controller-pattern.md|controller-pattern]]
```

<!-- risk-assessed -->
