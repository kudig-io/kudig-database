---
title: tekton v0.26 Release Notes
description: tekton v0.26 Release Notes — Kubernetes 生产运维知识库
summary: tekton v0.26 Release Notes — Kubernetes 生产运维知识库
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
- tekton v0.26 Release Notes 是什么
- 如何 tekton v0.26 Release Notes
trigger_keywords:
- tekton
- v0.26
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




# tekton v0.26 Release Notes

Source: [v0.26.0](https://github.com/tektoncd/pipeline/releases/tag/v0.26.0)

# 🎉  Debugging taskRuns and Merging podTemplates with default 🎉

-[Docs @ v0.26.0](https://github.com/tektoncd/pipeline/tree/v0.26.0/docs)
-[Examples @ v0.26.0](https://github.com/tektoncd/pipeline/tree/v0.26.0/examples)

## Installation one-liner

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.26.0/release.yaml
```
## Upgrade Notices

N/A

# Features

* :sparkles: Merge *Run's PodTemplate with Default 🧙 (#4057)

TaskRun and PipelineRun's PodTemplate are now merge with the default PodTemplate. This means any field that is not specified in a TaskRun or PipelineRun's PodTemplate will come from the configured default PodTemplate (if defined).

* :sparkles: Add debug with breakpoint onFailure to TaskRun Spec (#3857)

  * Add -breakpoint_on_failure to entrypointer which disables exit of container upon failure
  * Add debug helper scripts to /tekton/debug/scripts
  * Add /tekton/debug/info/ mount which is used by helper scripts to understand which step they are running in where denotes the step number. First step = 0, Second step = 1 and so on.

# Deprecation Notices

N/A

# Backwards incompatible changes

N/A

# Fixes

* :bug: Set HOME=/tekton/home for GCS PipelineResources (#4081)

Fixed bug where GCS PipelineResource failed to upload unless HOME="/tekton/home" was explicitly set in task's pod template.

* :bug: Fix the cp command in the release publish task (#4051)


# Misc


* :hammer: Set fake AWS credentials on controller to workaround aws-sdk bug (#4073)

Work around a bug in the AWS go SDK that causes extremely long delays in task startup times.

* :hammer: Update [[Knative|knative]]/pkg to v0.23 and k8s.io to v0.20.7 (#4044)

Bump k8s.io and knative dependencies to v0.20.7 and v0.23.0 respectively, taking latest fixes and features in.

* :hammer: cleaning up task results constant (#4070)

* :hammer: Replace tmpfile with scriptfile in script init container (#4041)


# Docs

* :book: Updates to pipeline tutorial to handle skaffold Dockerfile and keep the yq syntax working (#4058)

  1. Updated kaniko executor image to latest
  2. Added --build-arg=BASE=alpine:3 to have kaniko correctly handle the skaffold leeroy-web Dockerfile (had the issue described in https://github.com/GoogleContainerTools/kaniko/issues/1271  before)
  3. Changed mikefarah/yq image to 3.4.1 (the latest working with the provided command) in deploy-using-kubectl task to get this working (ver. 4 changed the syntax completely: https://mikefarah.gitbook.io/yq/upgrading-from-v3, so the command gave a syntax error)

* :book: Add v0.25.0 to the README (#4043)

* :book: Update comment on RunControllerName const (#4065)

* :book: Adjust weight in hermetic.md (#4056)

* :book: Fix broken link (#4054)

* :book: docs: update metrics name (#3945)

## Thanks

Thanks to these contributors who contributed to v0.26.0!
* :heart: @afrittoli
* :heart: @eliasnorrby
* :heart: @ggalloro
* :heart: @jerop
* :heart: @pritidesai
* :heart: @pugangxa
* :heart: @sbwsg
* :heart: @vdemeester
* :heart: @waveywaves
* :heart: @zhouhaibing089

Extra shout-out for awesome release notes:
* :heart_eyes: @ggalloro
* :heart_eyes: @jerop
* :heart_eyes: @sbwsg
* :heart_eyes: @vdemeester
* :heart_eyes: @waveywaves



<!-- risk-assessed -->
