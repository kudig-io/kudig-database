---
title: tekton v0.29 Release Notes
description: tekton v0.29 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- opa
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v0.29 Release Notes 是什么
- 如何 tekton v0.29 Release Notes
trigger_keywords:
- tekton
- v0.29
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

# tekton v0.29 Release Notes

Source: [v0.29.1](https://github.com/tektoncd/pipeline/releases/tag/v0.29.1)

# 🎉 Label Propagation Fix and Changes to Implicit Params 🎉

-[Docs @ v0.29.1](https://github.com/tektoncd/pipeline/tree/v0.29.1/docs)
-[Examples @ v0.29.1](https://github.com/tektoncd/pipeline/tree/v0.29.1/examples)

## Installation one-liner

```
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.29.1/release.yaml
```

# Fixes

* #4478 Fix Pipeline/Task to *Run label/annotation propagation
* #4484 Implicit params: don't apply PipelineSpec params to TaskRefs
* #4511 Implicit params: Disable implicit param behavior for Pipeline Objects
* #4521 Update Dockerfiles using golang images to Go 1.16.13

## Thanks

Thanks to these contributors who contributed to v0.29.1!
* :heart: @vdemeester 
* :heart: @wlynch 
* :heart: @sbwsg