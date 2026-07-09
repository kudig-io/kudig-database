---
title: 任务
description: Job 是 Kubernetes 中用于运行一次性任务的工作负载控制器。它创建一个或多个 Pod 并确保指定数量的 Pod 成功完成后终止。...
summary: Job 是 Kubernetes 中用于运行一次性任务的工作负载控制器。它创建一个或多个 Pod 并确保指定数量的 Pod 成功完成后终止。...
category: dictionary
tags:
- k8s
- glossary
- job
- workload
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 任务 是什么
- Job 详解
trigger_keywords:
- 任务
- Job
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 任务

> **英文名**: Job

## 概述

Job 是 Kubernetes 中用于运行一次性任务的工作负载控制器。它创建一个或多个 Pod 并确保指定数量的 Pod 成功完成后终止。

## 核心概念/原理

### 核心特性

- **一次性执行**：Pod 成功完成后不会重启。
- **完成保证**：Job 确保 `completions` 指定数量的 Pod 成功执行。
- **并行控制**：`parallelism` 控制同时运行的 Pod 数量。
- **失败处理**：`backoffLimit` 限制最大重试次数。

### 执行模式

- **Non-parallel Job**：`completions=1`，一个 Pod 成功即完成。
- **Fixed completion count**：`completions=N`，需要 N 个 Pod 成功。
- **Work queue**：`completions` 未设置，由 Pod 自行协调。

## 关键机制或特性

- `activeDeadlineSeconds` 限制 Job 的最大运行时间。
- `ttlSecondsAfterFinished` 自动清理已完成的 Job。
- Job 支持 `suspend` 字段暂停执行。
- 失败后的重试间隔按指数退避（10s → 20s → 40s...最大 6min）。

## 使用场景与最佳实践

- 数据迁移、批处理、机器学习训练等一次性任务使用 Job。
- 设置 `backoffLimit` 防止无限重试。
- 使用 `activeDeadlineSeconds` 避免任务卡死。
- 大规模批处理考虑使用 Argo Workflows 或 Tekton。

## 参考链接

- [Job - Official Documentation](https://kubernetes.io/docs/concepts/workloads/controllers/job/)

## Related

- [[系统基础/topic-dictionary/workloads/pod.md|Pod]]
- [[系统基础/topic-dictionary/workloads/deployment.md|Deployment]]
- [[系统基础/topic-dictionary/workloads/statefulset.md|Statefulset]]
- [[系统基础/topic-dictionary/workloads/daemonset.md|Daemonset]]
- [[系统基础/topic-dictionary/workloads/replicaset.md|Replicaset]]


<!-- risk-assessed -->
