---
title: tekton v0.12 Release Notes
description: tekton v0.12 Release Notes — Kubernetes 生产运维知识库
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
- tekton v0.12 Release Notes 是什么
- 如何 tekton v0.12 Release Notes
trigger_keywords:
- tekton
- v0.12
- Release
- Notes
- release
- notes
---

# tekton v0.12 Release Notes

Source: [v0.12.1](https://github.com/tektoncd/pipeline/releases/tag/v0.12.1)

# 🎉 Bug Fixes 🎉

-[Docs @ v0.12.1](https://github.com/tektoncd/pipeline/tree/v0.12.1/docs)
-[Examples @ v0.12.1](https://github.com/tektoncd/pipeline/tree/v0.12.1/examples)

## Installation one-liner

```
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.12.1/release.yaml
```

## Upgrade Notices

N/A

## Changes

# Fixes


> ⚠️ **弃用警告**: `PodSecurityPolicy` 已在 Kubernetes v1.25 中正式移除。
> 请使用 [Pod Security Admission (PSA)](https://kubernetes.io/docs/concepts/security/pod-security-admission/) 替代。

* :bug: Add PodSecurityPolicy access to webhook's clusterrole (#2620)
* :bug: Fix typo introduced in git-init  (#2620)

[Fill list here]

# Misc

* :hammer: Revert "config: prefixes image names with ko:// scheme 📠" (#2625)
* :hammer: Revert "config: prefixes image names with ko:// scheme" (#2624)
* :hammer: Update golangci configuration (#2620)
* :hammer: Replace devel on all yamls (#2620)

## Thanks

Thanks to these contributors who contributed to v0.12.1!

- :heart: @ad22
- :heart: @afrittoli
- :heart: @sbwsg 
- :heart: @vdemeester

Extra shout-out for awesome release notes:

* :heart_eyes: @afrittoli
* :heart_eyes: @sbwsg