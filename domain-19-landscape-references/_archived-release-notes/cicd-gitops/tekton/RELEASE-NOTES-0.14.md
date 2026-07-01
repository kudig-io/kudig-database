---
title: tekton v0.14 Release Notes
description: tekton v0.14 Release Notes — Kubernetes 生产运维知识库
summary: tekton v0.14 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
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
- tekton v0.14 Release Notes 是什么
- 如何 tekton v0.14 Release Notes
trigger_keywords:
- tekton
- v0.14
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# tekton v0.14 Release Notes

Source: [v0.14.3](https://github.com/tektoncd/pipeline/releases/tag/v0.14.3)

# 🎉 Bugfix release 🎉

-[Docs @ v0.14.3](https://github.com/tektoncd/pipeline/tree/v0.14.3/docs)
-[Examples @ v0.14.3](https://github.com/tektoncd/pipeline/tree/v0.14.3/examples)

## Installation one-liner

```
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.14.3/release.yaml
```

## Upgrade Notices


Users need to pass in default parameter values or provide the required parameters in `Pipeline` spec as they now get validated by the controller.
The `creds-init` step is gone and its behavior is now handle by the entrypoint. This shouldn't have any impact except running as the same user as the step.

## Changes

# Fixes
* :bug: Fix some assignment to nil map issues (#3005)
  Fix a panic in the pipeline controller that may happen when a pipeline hangs in starting state, because of a malformed condition name.

## Thanks

Thanks to these contributors who cFix a panic in the pipeline controller that may happen when a pipeline hangs in starting state, because of a malformed condition name.ontributed to v0.14.3!

- :heart: @sbwsg
- :heart: @afrittoli 
