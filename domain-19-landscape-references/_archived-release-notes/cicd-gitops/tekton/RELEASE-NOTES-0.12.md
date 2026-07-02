---
title: tekton v0.12 Release Notes
description: tekton v0.12 Release Notes — Kubernetes 生产运维知识库
summary: tekton v0.12 Release Notes — Kubernetes 生产运维知识库
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
- tekton v0.12 Release Notes 是什么
- 如何 tekton v0.12 Release Notes
trigger_keywords:
- tekton
- v0.12
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




# tekton v0.12 Release Notes

Source: [v0.12.1](https://github.com/tektoncd/pipeline/releases/tag/v0.12.1)

# 🎉 Bug Fixes 🎉

-[Docs @ v0.12.1](https://github.com/tektoncd/pipeline/tree/v0.12.1/docs)
-[Examples @ v0.12.1](https://github.com/tektoncd/pipeline/tree/v0.12.1/examples)

## Installation one-liner

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.12.1/release.yaml
```
## Upgrade Notices

N/A

## Changes

# Fixes


> ⚠️ **弃用警告**: `PodSecurityPolicy` 已在 [[Kubernetes|Kubernetes]] v1.25 中正式移除。
> 请使用 Pod Securityod Security Admission]] (PSA)](https://kubernetes.io/docs/concepts/security/pod-security-admission/) 替代。

* :bug: Add PodSecurityPolicy access to webhook's clusterrole (#2620)
* :bug: Fix typo introduced in git-init  (#2620)

[Fill list here]

# Misc

* :hammer: Revert "config: prefixes image names with [[ko|ko]]:// scheme 📠" (#2625)
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

<!-- risk-assessed -->
