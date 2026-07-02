---
title: tekton v0.2 Release Notes
description: tekton v0.2 Release Notes — Kubernetes 生产运维知识库
summary: tekton v0.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rbac
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
- tekton v0.2 Release Notes 是什么
- 如何 tekton v0.2 Release Notes
trigger_keywords:
- tekton
- v0.2
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




# tekton v0.2 Release Notes

Source: [v0.2.0](https://github.com/tektoncd/pipeline/releases/tag/v0.2.0)

# 🎉 Tekton Pipelines with Graphs; without [[domain-17-system-foundation/topic-dictionary/workloads/init-containers.md|init containers]]! 🎉

* [Docs @ v0.2.0](https://github.com/tektoncd/pipeline/tree/v0.2.0/docs#tekton-pipelines)
* [Examples @ v0.2.0](https://github.com/tektoncd/pipeline/tree/v0.2.0/examples)

This is the first dogfood released version of Tekton Pipelines, where the images were built, pushed and tagged using a Task!

## Changes

### Features
* :sparkles: Allow default system namespace to be overridden using an env var (#427)
* :sparkles: Use DAG (graph) based execution for Pipelines instead of linear one (#473, #578 )
* :sparkles: Use truncated base TaskRun name for lookup (#565)
* :sparkles: The containers executing the Task steps are changed from using InitContainers to Containers in the TaskRun pod. This is achieved by wrapping each container by a custom binary entrypoint to manage the order of the steps. As such, it is always recommended to explicitly specify a command for a Task step. If a step's image is in a private registry, a ServiceAccount with ImagePullSecrets should be provided in the Task  (#564, #620, #634, #647, #686, #687)
* :sparkles: Pipeline doesn't depend on [[Knative|knative]]/build anymore 💃 (#636, #648) 
* :sparkles: Update taskrun/pipelinerun timeout logic to not rely on resync behavior (#621,  #674, #681)
* :sparkles: cmd/git-init: check for errors or at least log them (#677)

### Fixes
* :bug: Make sure TaskSpec step container names aren't too long (#550)
* :bug: Arbitrary git commits can now be specified in Git PipelineResources (previously it only supported commits which can be `git fetch`-ed) (#555)
* :bug: PipelineResources storage GCS for directories now preserves directory structures during copies. (#566)
* :bug: Updates `tekton-pipelines-admin` clusterrole to have full access to `deployments/finalizers` (#572)
* :bug: Container names must end with an alphanumeric character (#580)
* :bug: ServiceAccount without ImagePullSecrets should not fail to fetch (#585)
* :bug: Fixes RBAC permissions for task and pipeline runs for openshift (#583)
* :bug: Port duplicate pod test from knative/build 🏗 (#603)
* :bug: Use full command field for internal containers (#605)

### Misc
* :hammer: Remove the `logsURL` field from `TaskRun` status. This field was never populated and was leftover from the original POC version of the API. #107 may add something like this in the future but that design remains TBD (#563)
* :arrow_double_up: Bump kubernetes dependecies to 1.12.6 (#662)

## Thanks

Thanks to these contributors who contributed to v0.2.0!

* :heart: @abayer
* :heart: @cagiti 
* :heart: @chmouel
* :heart: @Conky5
* :heart: @bobcatfish
* :heart: @dlorenc
* :heart: @dibbles
* :heart: @hrishin
* :heart: @ImJasonH 
* :heart: @khrm
* :heart: @skeeey
* :heart: @mattmoor-sockpuppet
* :heart: @mikeykhalil
* :heart: @pivotal-nader-ziada
* :heart: @nbarthwal
* :heart: @rawlingsj
* :heart: @rhuss 
* :heart: @shashwathi 
* :heart: @sthaha
* :heart: @vbatts
* :heart: @vdemeester
* :heart: @assertion 

Extra shout-out for awesome release notes:
* :heart_eyes: @dibbles 
* :heart_eyes: @Conky5 
* :heart_eyes: @sthaha 
* :heart_eyes: @pivotal-nader-ziada 


<!-- risk-assessed -->
