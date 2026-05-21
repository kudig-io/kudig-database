---
title: Controllers（控制器）
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- controller-manager
- job
- operator
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Controllers（控制器） 是什么
- 如何 Controllers（控制器）
trigger_keywords:
- Controllers
- 控制器
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

# Controllers（控制器）

## 概述

在 [[entities/kubernetes|kubernetes]] 中，控制器是监控集群状态的控制循环（control loop）。它们持续比较集群的**当前状态（current state）**与**期望状态（desired state）**，并在必要时采取措施使当前状态向期望状态靠拢。控制器本身通常不直接执行操作，而是通过向 API 服务器发送请求来产生副作用。

## 核心概念/原理

- **控制循环（Control Loop）**：一个永不终止的循环，用于调节系统状态。例如恒温器：设定温度是期望状态，室温是当前状态，恒温器通过开关设备来缩小两者差距。
- **期望状态 vs 当前状态**：Kubernetes 采用云原生视角，能够应对持续变化。只要控制器在运行并持续做出有用更改，集群整体是否达到稳定状态并不重要。
- **通过 API 服务器控制**：大多数内置控制器（如 Job Controller）通过读写 API 服务器来管理状态。例如 Job Controller 发现新 Job 后，会请求 API 服务器创建 Pod，而不是自己运行容器。
- **直接控制（Direct Control）**：某些控制器需要与集群外部系统交互。例如节点横向自动扩展控制器需要在节点不足时，调用云提供商 API 创建新节点，然后将结果报告回 API 服务器。

## 关键机制或特性

- **单一职责**：Kubernetes 设计了大量小型控制器，每个负责管理集群状态的某个特定方面，而不是一个庞大的控制循环。这种设计允许某个控制器失败时，其他控制器不受影响。
- **标签区分**：多个控制器可能创建或更新同一种对象（如 Deployment 和 Job 都会创建 Pod）。控制器通过标签（labels）和 owner references 来区分自己管理的资源，避免互相干扰。
- **内置控制器**：运行在 `kube-controller-manager` 中，如 Deployment Controller、Job Controller、Node Controller 等。控制平面具有高可用性，某个控制器失败时，其他副本会接管工作。
- **自定义控制器**：用户可以编写并部署自己的控制器，作为 Pod 运行在集群内，或作为外部进程运行。

## 使用场景

- 确保 Deployment 中始终维持指定数量的 Pod 副本
- 在 Job 完成后自动将其标记为 Finished
- 在云环境中根据负载自动扩缩容节点
- 自定义运维逻辑（如自定义 Operator）管理有状态应用

## 最佳实践/注意事项

- 控制器设计应尽量简单、职责单一，便于故障隔离和排查
- 自定义控制器应通过 owner references 和标签正确管理资源，避免误删他人创建的对象
- 控制器故障是设计预期内的，确保关键控制器以多副本高可用方式运行
- 如需编写自定义控制器，可参考 Kubernetes 扩展模式（extension patterns）和 sample-controller 仓库

## 参考链接

- https://kubernetes.io/docs/concepts/architecture/controller/
