---
title: tekton v0.18 Release Notes
description: tekton v0.18 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- webhook
- rag
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
---

# tekton v0.18 Release Notes

Source: [v0.18.1](https://github.com/tektoncd/pipeline/releases/tag/v0.18.1)

# 🎉 fix larger pipelines (>40 tasks) and updated the webhook name 🎉

-[Docs @ v0.18.1](https://github.com/tektoncd/pipeline/tree/v0.18.1/docs)
-[Examples @ v0.18.1](https://github.com/tektoncd/pipeline/tree/v0.18.1/examples)

## Installation one-liner

```
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
