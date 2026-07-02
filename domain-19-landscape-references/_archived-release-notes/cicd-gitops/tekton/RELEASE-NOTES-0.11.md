---
title: tekton v0.11 Release Notes
description: tekton v0.11 Release Notes — Kubernetes 生产运维知识库
summary: tekton v0.11 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v0.11 Release Notes 是什么
- 如何 tekton v0.11 Release Notes
trigger_keywords:
- tekton
- v0.11
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




# tekton v0.11 Release Notes

Source: [v0.11.3](https://github.com/tektoncd/pipeline/releases/tag/v0.11.3)

# 🎉 Timeout and Result Fixes 🎉

-[Docs @ v0.11.3](https://github.com/tektoncd/pipeline/tree/v0.11.3/docs)
-[Examples @ v0.11.3](https://github.com/tektoncd/pipeline/tree/v0.11.3/examples)

0.11.3 is likely to be the final patch release before 0.12.  Included here is a fix for Timeouts in PipelineTasks and several fixes for Task Results.  This release also adds support for using Task Results in the parameters of Conditions.

## Upgrade Notices

🚨 If you are upgrading from a version of Tekton Pipelines older than v0.11.0 then you may need to delete your existing tekton-pipeline [[Deployments|deployments]] before applying v0.11.3.

## Changes

### Fixes

* :bug: Fix 3 bugs with Task Results #2471 
* :bug: Fix PipelineTask timeout not correctly set #2468 

## Thanks

Thanks to these contributors who contributed to v0.11.3!

- :heart: @bobcatfish
- :heart: @othomann 
- :heart: @vdemeester

<!-- risk-assessed -->
