---
title: Tekton
description: Tekton 是 CNCF 孵化项目，提供 Kubernetes 原生的 CI/CD 流水线框架。它将 CI/CD 的每一步建模为 Kubernetes
  CRD...
summary: Tekton 是 CNCF 孵化项目，提供 Kubernetes 原生的 CI/CD 流水线框架。它将 CI/CD 的每一步建模为 Kubernetes
  CRD...
category: dictionary
tags:
- k8s
- glossary
- tekton
- cicd
- pipeline
- cncf
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Tekton 是什么
- Tekton 详解
trigger_keywords:
- Tekton
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Tekton

> **英文名**: Tekton

## 概述

Tekton 是 CNCF 孵化项目，提供 Kubernetes 原生的 CI/CD 流水线框架。它将 CI/CD 的每一步建模为 Kubernetes CRD（Task、Pipeline），实现了真正云原生的持续集成和持续交付。

## 核心概念/原理

### 核心 CRD

| 资源 | 功能 |
|------|------|
| Task | 最小执行单元（一组有序的 Steps） |
| Pipeline | 多个 Task 的编排（DAG 依赖图） |
| TaskRun | Task 的一次执行实例 |
| PipelineRun | Pipeline 的一次执行实例 |
| Trigger | 外部事件触发 PipelineRun |

### 与 Jenkins 对比

| 特性 | Jenkins | Tekton |
|------|---------|--------|
| 运行环境 | 独立 VM/容器 | K8s 原生 Pod |
| 扩展性 | 插件（Groovy） | CRD + 容器 |
| 弹性 | Master-Agent | Serverless Pod |

## 关键机制或特性

- **Task 共享 Workspace**：通过 PVC 在 Task 之间传递数据。
- **Catalog**：社区贡献的预构建 Task（如 git-clone、buildpacks）。
- **Results**：Task 输出结果供下游 Task 引用。
- **When Expressions**：条件执行 Task。
- **Finally**：Pipeline 结束后的清理/通知 Task。

## 使用场景与最佳实践

- 云原生 CI/CD 优先选择 Tekton 替代 Jenkins。
- 使用 Tekton Catalog 复用社区 Task 减少重复开发。
- 配合 Triggers 实现 Webhook 触发的自动构建。
- 使用 Tekton Dashboard 或 Tekton Results 查看执行历史。
- 为 Pipeline 设置合理的超时时间和重试策略。

## 参考链接

- [Tekton Official](https://tekton.dev/)

## Related

- [[domain-17-system-foundation/topic-dictionary/operations/argo.md|Argo]]
- [[domain-17-system-foundation/topic-dictionary/operations/gitops.md|GitOps]]
- [[domain-17-system-foundation/topic-dictionary/tooling/helm.md|Helm]]
- [[domain-17-system-foundation/topic-dictionary/workloads/job.md|Job]]
- [[domain-17-system-foundation/topic-dictionary/workloads/deployment.md|Deployment]]


<!-- risk-assessed -->
