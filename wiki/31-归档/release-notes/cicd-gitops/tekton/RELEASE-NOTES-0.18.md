---
title: tekton v0.18 Release Notes
description: tekton v0.18 Release Notes — Kubernetes 生产运维知识库
summary: tekton v0.18 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- webhook
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
- tekton v0.18 Release Notes 是什么
- 如何 tekton v0.18 Release Notes
trigger_keywords:
- tekton
- v0.18
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




# tekton v0.18 Release Notes

Source: [v0.18.1](https://github.com/tektoncd/pipeline/releases/tag/v0.18.1)

# 🎉 fix larger pipelines (>40 tasks) and updated the webhook name 🎉

-[Docs @ v0.18.1](https://github.com/tektoncd/pipeline/tree/v0.18.1/docs)
-[Examples @ v0.18.1](https://github.com/tektoncd/pipeline/tree/v0.18.1/examples)

## Installation one-liner

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.18.1/release.yaml
```
## Changes

# Fixes

* :bug: [cherry-pick] Fix recursion issue on Skip (#3534)

Fix and issue in the pipeline state resolution which lead to very long or failed start times and high controller CPU in case of pipelines with a large number of dependencies between tasks (40+).

* :bug: [cherry-pick] Change the webhook name to pipeline-webhook (#3533)

Fix an issue that caused the webhook, under certain conditions,  to fail to acquire a lease and not function correctly as a result. 

## Thanks

Thanks for the bug report @skaegi, @mattmoor, and @afrittoli  😻 !!

Thanks to these contributors who contributed to v0.18.1!
* :heart: @afrittoli 
* :heart: @sbwsg 
* :heart: @mattmoor 
* :heart: @pritidesai


Extra shout-out for awesome release notes:
* :heart_eyes: @afrittoli 
* :heart_eyes: @sbwsg 
* :heart_eyes: @mattmoor 
* :heart_eyes: @pritidesai


<!-- risk-assessed -->
