---
title: tekton v0.13 Release Notes
description: tekton v0.13 Release Notes — Kubernetes 生产运维知识库
summary: tekton v0.13 Release Notes — Kubernetes 生产运维知识库
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
- tekton v0.13 Release Notes 是什么
- 如何 tekton v0.13 Release Notes
trigger_keywords:
- tekton
- v0.13
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# tekton v0.13 Release Notes

Source: [v0.13.2](https://github.com/tektoncd/pipeline/releases/tag/v0.13.2)

# 🎉 Fix to notags release 🎉

-[Docs @ v0.13.2](https://github.com/tektoncd/pipeline/tree/v0.13.2/docs)
-[Examples @ v0.13.2](https://github.com/tektoncd/pipeline/tree/v0.13.2/examples)

## Installation one-liner

```
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.13.2/release.yaml
```

## Upgrade Notices

none

## Changes

# Fixes

* :bug: Remove tag+digest on shell-image 🍶 (#2810)

## Thanks

Thanks to these contributors who contributed to v0.13.2!

- :heart: @vdemeester