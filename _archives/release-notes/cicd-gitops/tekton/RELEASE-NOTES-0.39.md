---
title: tekton v0.39 Release Notes
description: tekton v0.39 Release Notes — Kubernetes 生产运维知识库
summary: tekton v0.39 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
- opa
- crd
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- tekton v0.39 Release Notes 是什么
- 如何 tekton v0.39 Release Notes
trigger_keywords:
- tekton
- v0.39
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




# tekton v0.39 Release Notes

Source: [v0.39.0](https://github.com/tektoncd/pipeline/releases/tag/v0.39.0)

# 🎉 Parameterize onError, finally task results in pipeline results, and many more  🎉

-[Docs @ v0.39.0](https://github.com/tektoncd/pipeline/tree/v0.39.0/docs)
-[Examples @ v0.39.0](https://github.com/tektoncd/pipeline/tree/v0.39.0/examples)

## Installation one-liner

``` shell
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.39.0/release.yaml
```
## Attestation

The Rekor UUID for this release is `362f8ecba72f43268e217c4700290e118237bd958b73e4b539da850cfacd12ff6719e20dcde99540`

Obtain the attestation:
```shell
REKOR_UUID=362f8ecba72f43268e217c4700290e118237bd958b73e4b539da850cfacd12ff6719e20dcde99540
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.39.0/release.yaml
REKOR_UUID=362f8ecba72f43268e217c4700290e118237bd958b73e4b539da850cfacd12ff6719e20dcde99540

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.39.0@sha256:" + .digest.sha256')

# Download the release file
curl "$RELEASE_FILE" > release.yaml

# For each image in the attestation, match it to the release file
for image in $REKOR_ATTESTATION_IMAGES; do
  printf $image; grep -q $image release.yaml && echo " ===> ok" || echo " ===> no match";
done
```

<!-- Any special upgrade notice
## Upgrade Notices
-->

## Changes

# Features

* :sparkles: build entrypoint image for Windows ltsc2019 and ltsc2022 (#5333)

Tekton support for Windows includes support for ltsc2019 in addition to ltsc2022

* :sparkles: support parameterization in `onError` (#5307)

Support variables in steps[].onError, for example, $(params.CONTINUE)

* :sparkles: Make pipeline cancel robust to missing resources (#5288)

A PipelineRun can be cancelled even if some of its owned resources have been deleted.

* :sparkles: Add TopologySpreadConstraints in PodTemplate (#5204)

Added TopologySpreadConstraints in PodTemplate to enable spread [[Pods|Pods]] across clusters among topology domains.

* :sparkles: TEP-0116: Referencing Finally Task Results in Pipeline Results (#5170)

Allow users to use `results` from `finally` in `PipelineResults` using `$(finally.<pipelinetask-name>.results.<result-name>)`

* :sparkles: [TEP-0075]Add more setdefaults features for taskresults (#5142)

taskresults is inferred as object if Properties is set, and Properties's value by default is string

* :sparkles: [TEP-0104] Populate Task-level Resource Requirements from PipelineRun to TaskRun (#5212)

* :sparkles: Add validation for results object properties types (#5169)

* :sparkles: [TEP-0104] Update Pod with Task-level Resource Requirements (#5082)

# Deprecation Notices

* :rotating_light: Rename ArrayOrString to ParamValues (#5304)

Rename ArrayOrString to ParamValues, NewArrayOrString to NewStructuredValues

This deprecation notice is applicable to the projects such CLI, Dashboard, Chains, etc which are dependent on the go types defined in the Pipeline.


<!-- Fill in backward incompatible changes when applicable
# Backwards incompatible changes

In current release:

* :rotating_light: [Change Title]

[Detailed change description] (#Number).

[Fill list here]
-->

### Fixes

* :bug: TEP-0090: Matrix - Retries (#5305)

Each retry for each matrixed `TaskRun` is completed before it is reattempted; failure in one matrixed `TaskRun` no longer affects retries for other matrixed `TaskRuns` from the same `PipelineTask`.

* :bug: Move validation from  to  when propagating parameters (#5291)

Move parameter validation from `pipelinespec` to `pipelinerunspec` when propagating parameters

* :bug: Add a status_msg field to fix issue 5150. (#5224)

Users can now differentiate if a TaskRun was cancelled by the user or by cancellation of a PipelineRun of which the TaskRun was a part of, by looking at the TaskRun's spec.StatusMessage field.

* :bug: Fix the Tekton controller panic for Metrics (#5166)

Fix the Tekton controller panic for Metrics.

* :bug: Move parameter validation from  to  when propagating parameters (#5143)

Move parameter validation from `taskspec` to `taskrunspec` when propagating parameters

* :bug: Make update-reference-docs.sh OSX compatible. (#5326)
* :bug: Uncomment tests in v1 task_validation_test.go (#5323)
* :bug: Modified test to allow for validation (#5284)
* :bug: Skip validation for deletion of v1 task (#5231)
* :bug: Fail taskrun when results validation fails (#5198)
* :bug: Fix ApplyTaskResultsToPipelineResults missing object validation (#5167)
* :bug: Implement stderr/stdout copying with exec pipes. (#5261)
* :bug: Bump TestSidecarTaskSupport test timeout to 2m. (#5260)

### Misc

* :hammer: Convert Step OnError from string to OnErrorType type (#5322)

Convert step.OnError from string to type: OnErrorType

* :hammer: Determine changeset from build information (#5311)

Version informaiton added to workload labels is determined from information embedded by Go, instead of relying on symlinks to Git information in our build process.

* :hammer: Rename ArrayOrString to ParamValues (#5304)

Rename ArrayOrString to ParamValues, NewArrayOrString to NewStructuredValues

* :hammer: [TEP-0075] Add variable usage and links to examples in docs (#5222)

Update docs

* :hammer: Do not validate anything on delete ✂ (#5210)

Do not try to convert object on deletion, and do not validate names on deletion as well.

* :hammer: TEP-0075: Add a pipeline run example with both object param and result (#5197)

Add a pipeline run example with both object param and result

* :hammer: Bump knative/pkg to release-1.6 (#4928)

Bump knative/pkg dependency to 1.15.
action required: this will bring up the minimum version for Kubernetes to 1.22
* :hammer: Move pod template to pod package (#5329)
* :hammer: Move version validation tests into separate package (#5319)
* :hammer: Reformat CustomTask within Test (#5314)
* :hammer: Refactor compute resources compare functions (#5257)
* :hammer: Bump github.com/cloudevents/sdk-go/v2 from 2.10.1 to 2.11.0 (#5321)
* :hammer: Test feature flags (#5312)
* :hammer: Bump golangci-lint to v1.47.2 to support Go 1.18.x (#5310)
* :hammer: Fix typo in tekton/publish.yaml (#5301)
* :hammer: Bump go.uber.org/zap from 1.21.0 to 1.22.0 (#5293)
* :hammer: Add tests for limitrange transformer (#5279)
* :hammer: Fix task conversion test typo (#5277)
* :hammer: Fix a typo in 5080-entrypoint-init-regression.yaml (#5276)
* :hammer: Fix conversion related typos (#5272)
* :hammer: Add conversion for v1 Pipeline (#5258)
* :hammer: V1: Add conversion for Task.Resources (#5253)
* :hammer: TEP-0115: Update Git Resolver example to use revision and pathInRepo fields (#5238)
* :hammer: Make sure that OpenAPI rules violations errors show up in Prow build logs (#5237)
* :hammer: Add V1 version to Task CRD (#5234)
* :hammer: Rename tests in TestPipelineTaskList_Deps (#5228)
* :hammer: Add V1 Pipeline Golang structs (#5219)
* :hammer: Bump go-scm to 1.11.19 (#5213)
* :hammer: Bump containerd to 1.5.13 (#5209)
* :hammer: Update docs with matrix csi workspace release number (#5207)
* :hammer: Add conversion for v1 Task (#5202)

### Docs

* :book: Results Lifecycle (#5070)

Documenting the results lifecycle.

* :book: Update a few missing versions in the README (#5299)
* :book: Updates Default Fields When Creating a GKE Cluster (#5273)
* :book: Docs: Updated installation customizations link (#5244)
* :book: Add instructions for creating a new API version (#5235)
* :book: Remove not supported results and params variables doc (#5227)
* :book: Add Tep 75&76 to install doc (#5216)
* :book: Add v0.38.0, v0.37.1, and v0.37.2 to README (#5206)
* :book: Update release cheat sheet (#5205)
* :book: Make container docs more tailored to Tekton (#5124)

## Thanks

Thanks to these contributors who contributed to v0.39.0!
* :heart: @JeromeJu
* :heart: @PrajwalBorkar
* :heart: @QuanZhang-William
* :heart: @XinruZhang
* :heart: @Yongxuanzhang
* :heart: @abayer
* :heart: @afrittoli
* :heart: @austinzhao-go
* :heart: @chitrangpatel
* :heart: @chuangw6
* :heart: @dependabot[bot]
* :heart: @imjasonh
* :heart: @jagathprakash
* :heart: @jerop
* :heart: @khrm
* :heart: @lbernick
* :heart: @pritidesai
* :heart: @seongpyoHong
* :heart: @vdemeester
* :heart: @vsinghai
* :heart: @wlynch

Extra shout-out for awesome release notes:
* :heart_eyes: @QuanZhang-William
* :heart_eyes: @Yongxuanzhang
* :heart_eyes: @afrittoli
* :heart_eyes: @chitrangpatel
* :heart_eyes: @chuangw6
* :heart_eyes: @imjasonh
* :heart_eyes: @jagathprakash
* :heart_eyes: @jerop
* :heart_eyes: @khrm
* :heart_eyes: @pritidesai
* :heart_eyes: @seongpyoHong
* :heart_eyes: @vdemeester
* :heart_eyes: @vsinghai

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->

<!-- risk-assessed -->
