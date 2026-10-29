---
title: tekton v0.45 Release Notes
description: tekton v0.45 Release Notes — Kubernetes 生产运维知识库
summary: tekton v0.45 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- jaeger
- containerd
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
estimated_read_time: 10min
intent_queries:
- tekton v0.45 Release Notes 是什么
- 如何 tekton v0.45 Release Notes
trigger_keywords:
- tekton
- v0.45
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
- tracing-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# tekton v0.45 Release Notes

Source: [v0.45.0](https://github.com/tektoncd/pipeline/releases/tag/v0.45.0)

# 🎉 Propagated Parameters promoted to stable, Array Results promoted to beta, Propagated Workspaces to promoted to beta 🎉

-[Docs @ v0.45.0](https://github.com/tektoncd/pipeline/tree/v0.45.0/docs)
-[Examples @ v0.45.0](https://github.com/tektoncd/pipeline/tree/v0.45.0/examples)

## Installation one-liner

``` shell
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.45.0/release.yaml
```
## Attestation

The Rekor UUID for this release is `24296fb24b8ad77a1081521e24e0bdb57359c801e63b84d7de2b1cd4ef8c25004336d6fc9717ec65`

Obtain the attestation:
```shell
REKOR_UUID=24296fb24b8ad77a1081521e24e0bdb57359c801e63b84d7de2b1cd4ef8c25004336d6fc9717ec65
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.45.0/release.yaml
REKOR_UUID=24296fb24b8ad77a1081521e24e0bdb57359c801e63b84d7de2b1cd4ef8c25004336d6fc9717ec65

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.45.0@sha256:" + .digest.sha256')

# Download the release file
curl "$RELEASE_FILE" > release.yaml

# For each image in the attestation, match it to the release file
for image in $REKOR_ATTESTATION_IMAGES; do
  printf $image; grep -q $image release.yaml && echo " ===> ok" || echo " ===> no match";
done
```

## Upgrade Notices

### Action Required

* Please migrate off of `pullrequests` `pipelineresources` as it is removed, please refer to the doc at https://github.com/tektoncd/pipeline/blob/main/docs/pipelineresources.md#replacing-an-pullrequest-resource


## Backwards incompatible changes

In current release:

* :rotating_light: `pipelinerun.status.taskRuns` and `pipelinerun.status.runs` have been removed.

The `pipelinerun.status.taskRuns` and `pipelinerun.status.runs` fields have been removed, along with the `embedded-status` feature flag. If you are using these fields in your own tooling, please migrate to using `pipelinerun.status.childReferences` instead. (#6099).

## Changes

# Features

* :sparkles: TEP-0076 (array results and indexing into array) promoted to beta 🎓 (#6103)

TEP-0076 promoted to beta - Array results and indexing into an array is now possible with enable-api-fields set to beta.

* :sparkles: [TEP 0122] Add FeatureFlags Field to TaskRun.Status.Provenance (#6072)

Add FeatureFlags field to TaskRun.Status.Provenance

* :sparkles: Propagated Parameters - Stable (#6050)

Propagated Parameters is stable. This feature enables interpolating Parameters in embedded specifications to reduce verbosity in Tekton resources. 

* :sparkles: Beta Example Tests (#6031)

Run beta example tests when `enable-api-fields: beta` is set.

* :sparkles: Propagated Workspaces to Beta (#6030)

The feature propagated workspaces is now enabled if the feature flag, enable-api-field value is set to alpha or beta.

* :sparkles: Add prow env for beta integration test (#5737)

Adds pull-tekton-pipelines-beta-integration-test

* :sparkles: feat: support to extract init container failure message (#5646)

When an error occurs in the init container, the taskrun clearly displays the error message.
* :sparkles: [TEP-0089] Add a config map to support [[SPIRE|SPIRE]] initialization. (#5902)

## Deprecation Notices

* :rotating_light: `config-trusted-resources` is deprecated

config-trusted-resources of trusted resources to store public key is deprecated and will be removed in release 0.46, please use VerificationPolicy instead (#6134).

### Fixes

* :bug: validate emit results are defined in taskspec (#6129)

Tasks results emitted in script but don't defined in taskspec will fail

* :bug: remove results from status when type mismatched and add more logs (#6073)

taskrun results which have type mismatched are removed from taskrun status

* :bug: Fix converted legacy bundle->remote resolver syntax to be case-insensitive for kind (#6061)

The remote bundles resolver now matches the kind value in its parameters and in bundle layers in a case-insensitive manner.

* :bug: Increase memory limit for remote resolvers deployment (#6028)

Prevent OOM-kills of remote resolver pod when cloning large git repositories by increasing container's memory limit.

* :bug: Add security context for example PRs using catalog git-clone (#6138)
* :bug: Use remote resolution in release pipeline (#6027)
* :bug: fix:the pvc creation failure does not print the success log (#5777)
* :bug: increase the timer for reading k8s event in test (#6063)

### Misc


* :hammer: Mark config-trusted-resources as deprecated (#6134)

action required: config-trusted-resources of trusted resources to store public key is deprecated and will be removed in release 0.46, please use VerificationPolicy instead 

* :hammer: [TEP100] Remove  and  Fields for  (#6099)

`pipelinerun.status.taskruns` and `pipelinerun.status.runs` are removed

* :hammer: [TEP074] Remove Pullrequest-init Image (#6078)

Pullrequest image is removed. Please migrate off the Pullrequest Image.

* :hammer: [TEP0100] Remove  Feature Flag (#6049)

`embedded-status` feature flag is removed. TaskRun and Run status will be populated only in `pipelineRun.status.childReferences`.

* :hammer: [TEP074] Remove Pullrequest pipelineResources (#6011)

action required: please migrate off of `pullrquests` `pipelineresources` as it is removed, please refer to the doc at https://github.com/tektoncd/pipeline/blob/main/docs/pipelineresources.md#replacing-an-pullrequest-resource
* :hammer: cleaning up examples to avoid writing to results using path (#6098)
* :hammer: Migrate Kaniko Task off ImageDigestExporter (#6094)
* :hammer: [pkg/reconciler] clean up io/ioutil package (#6069)
* :hammer: Clean up cloudevents package (#6065)
* :hammer: Fix example customrun.yaml (#6041)
* :hammer: TEP-0114: Switch to use FilterCustomRunRef - Wait Custom Task Beta (#6040)
* :hammer: update object-param-result example (#6170)
* :hammer: update pipeline-object-param-and-result example (#6169)
* :hammer: [TEP074] Cleanup variables.md after Removal of PipelineResources (#6158)
* :hammer: Bump github.com/jenkins-x/go-scm from 1.13.1 to 1.13.2 (#6156)
* :hammer: Bump golang.org/x/crypto from 0.5.0 to 0.6.0 (#6149)
* :hammer: Bump go.opentelemetry.io/otel/exporters/jaeger from 1.12.0 to 1.13.0 (#6148)
* :hammer: Bump github.com/containerd/containerd from 1.6.16 to 1.6.17 (#6147)
* :hammer: Bump github.com/emicklei/go-restful from 2.15.0+incompatible to 2.16.0+incompatible in /test/custom-task-ctrls/wait-task-beta (#6146)
* :hammer: Bump github.com/go-git/go-billy/v5 from 5.4.0 to 5.4.1 (#6132)
* :hammer: Bump go.opentelemetry.io/otel/sdk from 1.12.0 to 1.13.0 (#6131)
* :hammer: Include CustomRuns in failed integration test YAML dump (#6124)
* :hammer: Replace hardcoded value (#6122)
* :hammer: add more error messages for results validation (#6119)
* :hammer: Bump github.com/golangci/golangci-lint from 1.50.1 to 1.51.1 in /tools (#6115)
* :hammer: Bump github.com/containerd/containerd from 1.6.15 to 1.6.16 (#6113)
* :hammer: Bumb up dependency versions for wait-task custom controllers (#6108)
* :hammer: Bump github.com/google/go-containerregistry from 0.12.1 to 0.13.0 (#6106)
* :hammer: unit test added for task result type mismatch (#6104)
* :hammer: Switch the release pipeline to use GitHub API for task resolution (#6100)
* :hammer: Bump github.com/tektoncd/pipeline from 0.43.2 to 0.44.0 in /test/custom-task-ctrls/wait-task-beta (#6087)
* :hammer: matrix with array results indexing (#6077)
* :hammer: Bump go.opentelemetry.io/otel/exporters/jaeger from 1.11.1 to 1.12.0 (#6074)
* :hammer: Add dependabot workflows for submodules. (#6068)
* :hammer: updating an example pipelinerun with array indexing (#6060)
* :hammer: updating an existing example pipelinerun with array results (#6055)
* :hammer: Fix spammy logs (#6051)
* :hammer: Adds label kind/misc to dependentbot (#6047)
* :hammer: Bump github.com/jenkins-x/go-scm from 1.12.3 to 1.13.1 (#6043)
* :hammer: Increase `timeout` in Conversion Integration Test to Prevent Flake (#6026)
* :hammer: Bump github.com/spiffe/spire-api-sdk from 1.5.2 to 1.5.4 (#5992)

### Docs

* :book: when expressions with array results (doc and example) (#6092)

Doc update for - How to refer to an array result in when expressions?

* :book: Add details about deprecating feature flags  and  (#6070)

Deprecation Notice: the feature flag `custom-task-version` will be removed in release v0.47.0.
* :book: updating the list of beta features (#6181)
* :book: Remove PipelineRunStatus from deprecation.md (#6164)
* :book: Move Propagated Workspaces to list of beta features (#6163)
* :book: chore: add link to tekton v0.40 version (#6151)
* :book: Add release info for v0.41.1 (#6126)
* :book: update pre-requsites to include checking docker is installed (#6121)
* :book: Pipelines Docs Cleanup (#6117)
* :book: Fix outdated remote resolution documentation (#6059)
* :book: updating an example taskrun with array parameters (#6057)
* :book: Update task-level compute resources docs (#6052)
* :book: adding section for emitting array results (#6039)
* :book: minor doc update for array and object results (#6038)
* :book: Add v0.44.0 to releases.md (#6035)
* :book: Update git resolver documentation (#6029)
* :book: Update pipelineResources deprecation (#6022)
* :book: Document behavior of feature-flag flips (#6017)
* :book: Refactor installation documentation (#5806)

## Thanks

Thanks to these contributors who contributed to v0.45.0!
* :heart: @Abirdcfly
* :heart: @JeromeJu
* :heart: @XinruZhang
* :heart: @Yongxuanzhang
* :heart: @abayer
* :heart: @afrittoli
* :heart: @chitrangpatel
* :heart: @cugykw
* :heart: @dependabot[bot]
* :heart: @dibyom
* :heart: @geriom
* :heart: @jagathprakash
* :heart: @jerop
* :heart: @lbernick
* :heart: @my-git9
* :heart: @pavanstarmanwar
* :heart: @pritidesai
* :heart: @wlynch

Extra shout-out for awesome release notes:
* :heart_eyes: @JeromeJu
* :heart_eyes: @XinruZhang
* :heart_eyes: @Yongxuanzhang
* :heart_eyes: @abayer
* :heart_eyes: @chitrangpatel
* :heart_eyes: @cugykw
* :heart_eyes: @jerop
* :heart_eyes: @pritidesai

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->

<!-- risk-assessed -->
