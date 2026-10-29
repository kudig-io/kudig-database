---
title: tekton v0.28 Release Notes
description: tekton v0.28 Release Notes — Kubernetes 生产运维知识库
summary: tekton v0.28 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- opa
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
- tekton v0.28 Release Notes 是什么
- 如何 tekton v0.28 Release Notes
trigger_keywords:
- tekton
- v0.28
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# tekton v0.28 Release Notes

Source: [v0.28.3](https://github.com/tektoncd/pipeline/releases/tag/v0.28.3)

# 🎉 Label Propagation Fix and Changes to Implicit Params 🎉

-[Docs @ v0.28.3](https://github.com/tektoncd/pipeline/tree/v0.28.3/docs)
-[Examples @ v0.28.3](https://github.com/tektoncd/pipeline/tree/v0.28.3/examples)

## Installation one-liner

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.28.3/release.yaml
```
# Fixes

* #4478 Fix Pipeline/Task to *Run label/annotation propagation
* #4484 Implicit params: don't apply PipelineSpec params to TaskRefs
* #4511 Implicit params: Disable implicit param behavior for Pipeline Objects
* #4521 Update Dockerfiles using golang images to Go 1.16.13

## Thanks

Thanks to these contributors who contributed to v0.32.1!
* :heart: @vdemeester 
* :heart: @wlynch 
* :heart: @sbwsg

<!-- risk-assessed -->
