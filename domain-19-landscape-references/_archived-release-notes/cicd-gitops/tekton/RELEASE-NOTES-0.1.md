---
title: tekton v0.1 Release Notes
description: tekton v0.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- crd
- rag
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
created: "2026-05-23"
---

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