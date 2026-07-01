---
title: tekton v0.15 Release Notes
description: tekton v0.15 Release Notes — Kubernetes 生产运维知识库
summary: tekton v0.15 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
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
- tekton v0.15 Release Notes 是什么
- 如何 tekton v0.15 Release Notes
trigger_keywords:
- tekton
- v0.15
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# tekton v0.15 Release Notes

Source: [v0.15.2](https://github.com/tektoncd/pipeline/releases/tag/v0.15.2)

# 🎉 Fix a bug in the pullrequest pipelineresource 🎉

The previous release, 0.15.1, was supposed to include a fix for the PullRequest Resource but was not published correctly with the new docker image.  This release fixes that problem so that the released YAML includes the correct docker images.

-[Docs @ v0.15.2](https://github.com/tektoncd/pipeline/tree/v0.15.2/docs)
-[Examples @ v0.15.2](https://github.com/tektoncd/pipeline/tree/v0.15.2/examples)

## Installation one-liner

```
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.15.2/release.yaml
```

# Fixes

* :bug: Ensure pullrequest-init is based on a root image (#3055)

## Thanks

Thanks to these contributors who contributed to v0.15.2!

- :heart: @sbwsg