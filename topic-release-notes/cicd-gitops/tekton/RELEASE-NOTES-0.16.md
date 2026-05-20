---
title: tekton v0.16 Release Notes
description: tekton v0.16 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v0.16 Release Notes 是什么
- 如何 tekton v0.16 Release Notes
trigger_keywords:
- tekton
- v0.16
- Release
- Notes
- release
- notes
---

# tekton v0.16 Release Notes

Source: [v0.16.3](https://github.com/tektoncd/pipeline/releases/tag/v0.16.3)

# 🎉  Fix nil pointer with timeouts  🎉

-[Docs @ v0.16.3](https://github.com/tektoncd/pipeline/tree/v0.16.3/docs)
-[Examples @ v0.16.3](https://github.com/tektoncd/pipeline/tree/v0.16.3/examples)

## Installation one-liner

```
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.16.3/release.yaml
```

## Changes

# Fixes

* :bug: Fix `nil` pointer exception in case the PipelineRun timeout is not specified (nor default applied)⏲ (#3241)

## Thanks

Thanks for the bug report @dghubble 😻 !!

Thanks to these contributors who contributed to v0.16.3!

- :heart: @vdemeester