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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# tekton v0.15 Release Notes

Source: [v0.15.2](https://github.com/tektoncd/pipeline/releases/tag/v0.15.2)

# 🎉 Fix a bug in the pullrequest pipelineresource 🎉

The previous release, 0.15.1, was supposed to include a fix for the PullRequest Resource but was not published correctly with the new docker image.  This release fixes that problem so that the released YAML includes the correct docker images.

-[Docs @ v0.15.2](https://github.com/tektoncd/pipeline/tree/v0.15.2/docs)
-[Examples @ v0.15.2](https://github.com/tektoncd/pipeline/tree/v0.15.2/examples)

## Installation one-liner

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.15.2/release.yaml
```
# Fixes

* :bug: Ensure pullrequest-init is based on a root image (#3055)

## Thanks

Thanks to these contributors who contributed to v0.15.2!

- :heart: @sbwsg

<!-- risk-assessed -->
