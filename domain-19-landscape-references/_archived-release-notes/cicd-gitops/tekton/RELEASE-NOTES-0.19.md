---
title: tekton v0.19 Release Notes
description: tekton v0.19 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- cri-o
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v0.19 Release Notes 是什么
- 如何 tekton v0.19 Release Notes
trigger_keywords:
- tekton
- v0.19
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# tekton v0.19 Release Notes

Source: [v0.19.0](https://github.com/tektoncd/pipeline/releases/tag/v0.19.0)

# 🎉 Custom Tasks 🎉

-[Docs @ v0.19.0](https://github.com/tektoncd/pipeline/tree/v0.19.0/docs)
-[Examples @ v0.19.0](https://github.com/tektoncd/pipeline/tree/v0.19.0/examples)

## Installation one-liner

```
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.19.0/release.yaml
```

## Upgrade Notices

N/A

# Features

* :sparkles: Do not allow use of deprecated Conditions with custom tasks (#3601)

   Disallow use of Conditions in pipeline tasks that reference custom tasks

* :sparkles: Plumb the TEKTON_RESOURCE_NAME env var into the cluster and PR reso… (#3555)

   Binary file (standard input) matches

* :sparkles: Plumb the TEKTON_RESOURCE_NAME environment variable into more resou… (#3532)

   Binary file (standard input) matches

* :sparkles: Add variable expansion for ImagePullPolicy in Tasks. See #3423 (#3488)

   Add variable expansion in Tasks for fields:
   - `spec.steps[].imagePullPolicy`
   - `spec.sidecar[].imagePullPolicy`

* :sparkles: Integrate custom tasks into Pipelines (#3463)

   In package pkg/apis/pipeline/v1alpha1, the type of field RunStatusFields.Results has changed from
   v1beta1.TaskRunResult to v1alpha1.RunResult.

* :sparkles: Add an unsuccessful TaskRun testcase in conformance tests (#3454)

# Deprecation Notices

* :rotating_light: The PascalCase fields in WhenExpressions is deprecated
   
   Tasks and Pipelines with WhenExpressions that were created using Tekton Pipelines v0.16.x need to be reapplied to fix the case of their json annotations (#3570)

# Backwards incompatible changes

N/A

# Fixes

* :bug: Fix PipelineRun serviceAccountNames for finally tasks (#3560)

   Fixes a bug where PipelineRun's serviceAccountNames and taskPodSpecs couldn't be applied on finally tasks and resulted in an error.

* :bug: Fix duplicate TaskRuns when Pipeline/PipelineRun labels are changed (#3558)

   Fix an issue where the PipelineRun controller could create duplicate TaskRuns if a Pipeline's or PipelineRun's labels are changed while          the PipelineRun is running

* :bug: Consider not-found pod as permanent error when taskrun is done (#3542)

   Fix an issue where the taskrun controller would continue reconciling completed taskruns if [[Pods|pods]] in case of evicted pods

* :bug: optimize cycle detection logic in dag (#3539)
* :bug: Fix script step on CRI-O runtime (#3526)


# Misc


* :hammer: Do not set the gitconfig option globally (#3574)

* :hammer: fix commen typo (#3546)
* :hammer: deleting redundant test file (#3556)
* :hammer: Use logstream in the duplicate_test (#3530)
* :hammer: pkg/apis: rename {Task,Pipeline}Interface into *Object (#3589)
* :hammer: retrieving deps outside of dag.build (#3583)
* :hammer: pkg/apis: remove duplicate substitution code. (#3579)
* :hammer: pkg/apis: unexport ApplyContainerReplacements (#3578)
* :hammer: pkg/apis: use pod.Template instead of v1beta1.PodTemplate (#3577)
* :hammer: tracker: use TrackReference instead of deprecated Track function (#3576)
* :hammer: pkg/apis: refactor GetTaskRunSpecs function (#3575)
* :hammer: pkg/apis: remove duplicate MergeStepsWithStepTemplate function (#3572)
* :hammer: Make examples runnable with kubectl 🐇 (#3564)
* :hammer: Refresh list of s390x excluded test (#3527)
* :hammer: Add some tests for pkg/git/git.go (#3523)
* :hammer: Use dogfooding skopeo image for e2e and examples tests (#3519)
* :hammer: Use dogfooding buildx image for multi-arch builds (#3514)
* :hammer: pkg/apis tests cleanup on duplicate test name 💅 (#3495)
* :hammer: Update plumbing dep to update google/go-licenses (#3422)

# Docs

* :book: Add disable-creds-init property for feature-flags setting (#3573)
* :book: Add deprecated PascalCase fields in WhenExpressions to deprecated table (#3570)

   action required: `Tasks` and `Pipelines` with `WhenExpressions` that were created using Tekton Pipelines v0.16.x need to be reapplied to fix the case of their json annotations

* :book: Fix the internal document link typo in docs/auth.md (#3567)
* :book: Clarify input and output pipeline resource usage with PVC (#3535)

   Added more detail to the documentation about input/output PipelineResources.

* :book: Update docs when with version info (#3528)
* :book: Fix markdown styling (#3520)
* :book: Add a document to tell users how to configure Thread, QPS and Burst (#3508)
* :book: Correct the disabled link (#3507)
* :book: fixing broken links (#3562)
* :book: Format workspaces description and correct unclear words (#3547)
* :book: fix link to owners file in release readme (#3538)
* :book: updating readme for 0.18.1 (#3537)
* :book: Move the High Availability doc out of the developers subdirectory (#3536)
* :book: updating readme with 0.18 (#3511)

## Thanks

Thanks to these contributors who contributed to v0.19.0!
* :heart: @GregDritschler
* :heart: @ImJasonH
* :heart: @NissesSenap
* :heart: @afrittoli
* :heart: @barthy1
* :heart: @chmouel
* :heart: @dlorenc
* :heart: @donglinjy
* :heart: @izhukov
* :heart: @jerop
* :heart: @linzhaoming
* :heart: @ljupchokotev
* :heart: @mattmoor
* :heart: @popcor255
* :heart: @pritidesai
* :heart: @rinckm
* :heart: @sbwsg
* :heart: @vdemeester
* :heart: @vincent-pli
* :heart: @xiujuan95
* :heart: @yaoxiaoqi
* :heart: @zhangtbj

Extra shout-out for awesome release notes:
* :heart_eyes: @GregDritschler
* :heart_eyes: @NissesSenap
* :heart_eyes: @afrittoli
* :heart_eyes: @chmouel
* :heart_eyes: @dlorenc
* :heart_eyes: @donglinjy
* :heart_eyes: @jerop
* :heart_eyes: @linzhaoming
* :heart_eyes: @ljupchokotev
* :heart_eyes: @popcor255
* :heart_eyes: @rinckm
* :heart_eyes: @vincent-pli
* :heart_eyes: @xiujuan95
* :heart_eyes: @zhangtbj
