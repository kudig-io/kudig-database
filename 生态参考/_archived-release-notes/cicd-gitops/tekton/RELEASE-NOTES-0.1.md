---
title: tekton v0.1 Release Notes
description: tekton v0.1 Release Notes — Kubernetes 生产运维知识库
summary: tekton v0.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- crd
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v0.1 Release Notes 是什么
- 如何 tekton v0.1 Release Notes
trigger_keywords:
- tekton
- v0.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# tekton v0.1 Release Notes

Source: [v0.1.0](https://github.com/tektoncd/pipeline/releases/tag/v0.1.0)

# 🎉 First Tekton Pipelines Release! 🎉

* [Docs @ v0.1.0](https://github.com/tektoncd/pipeline/tree/v0.1.0/docs#tekton-pipelines)
* [Examples @ v0.1.0](https://github.com/tektoncd/pipeline/tree/v0.1.0/examples)

This is the very first release of Tekton Pipelines, which has built on the concept of the [Build](https://github.com/knative/build) CRD, allowing users to declare typed inputs and outputs, and to combine multiple Builds (now called `Tasks`) into a `Pipeline`.

## Features

* `Tasks` allow users to define `steps` (i.e. container images)
  * Steps which can be executed against typed Inputs and parameters to produce Outputs
  * Cluster wide `ClusterTasks` are supported
  * They can declare `Parameters` which can be used in templating `steps`
* `Pipelines` combine `Tasks` together
  * Optionally they linking inputs and outputs between Tasks
  * Tasks are executed sequentially in the order they are declared in the Pipeline.
  * They can declare `Parameters` which can be used in templating `Task` usage
* `PipelineResources` are used as inputs and outputs. The following types are supported:
  * Git
  * Image (templating of attributes only)
  * [[Kubernetes|Kubernetes]]es 集群配置最佳实践|Kubernetes Cluster]]
  * Storage (currently only GCS)
* `Tasks` and `Pipelines` can be used repeatedly by instantiating `PipelineRuns` and `TaskRuns`, which will cause these to execute.
  * Runs can be executed with user specified `ServiceAccounts`
  * Runs can be cancelled


## Fixes
n/a

## Misc
n/a

<!-- risk-assessed -->
