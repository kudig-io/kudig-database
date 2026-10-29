---
title: tekton v1.4 Release Notes
description: tekton v1.4 Release Notes — Kubernetes 生产运维知识库
summary: tekton v1.4 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- apiserver
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
- tekton v1.4 Release Notes 是什么
- 如何 tekton v1.4 Release Notes
trigger_keywords:
- tekton
- v1.4
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




# tekton v1.4 Release Notes

Source: [v1.4.0](https://github.com/tektoncd/pipeline/releases/tag/v1.4.0)

# 🎉 Improved remote resolution and timeout configuration 🎉

-[Docs @ v1.4.0](https://github.com/tektoncd/pipeline/tree/v1.4.0/docs)
-[Examples @ v1.4.0](https://github.com/tektoncd/pipeline/tree/v1.4.0/examples)

## Installation one-liner

``` shell
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v1.4.0/release.yaml
```
## Attestation

The Rekor UUID for this release is `108e9186e8c5677a040c237838848039376864340e5217f6c7c23f294d61437c3d196cb1112b91f1`

Obtain the attestation:
```shell
REKOR_UUID=108e9186e8c5677a040c237838848039376864340e5217f6c7c23f294d61437c3d196cb1112b91f1
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v1.4.0/release.yaml
REKOR_UUID=108e9186e8c5677a040c237838848039376864340e5217f6c7c23f294d61437c3d196cb1112b91f1

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v1.4.0@sha256:" + .digest.sha256')

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

* :sparkles: feat: resolve steps referencing StepActions concurrently (#8925)

The resolution of `StepActions` within a `TaskRun` is now performed concurrently, which can significantly reduce the time it takes for a `TaskRun` to start, especially when using multiple remote `StepActions`.

* :sparkles: Do not fail PipelineRun if pvc creation error is because of exceeded quotas (#8903)

PipelineRun do not fail anymore if the pvc creation is due to an exceeded quota ; it will be requeued instead (until quota is available or it times out)

* :sparkles: feat: override task timeouts in pipelineruns (#8636)

feature: PipelineRun can now override individual task timeouts with spec.taskRunSpecs[].timeout 


<!-- Fill in deprecation notices when applicable
# Deprecation Notices

* :rotating_light: [Deprecation Notice Title]

[Detailed deprecation notice description] (#Number).

[Fill list here]
-->

<!-- Fill in backward incompatible changes when applicable
# Backwards incompatible changes

In current release:

* :rotating_light: [Change Title]

[Detailed change description] (#Number).

[Fill list here]
-->

### Fixes

* :bug: check  for the [[Kubernetes|kubernetes]] sidecar implementation (#8986)

Updated the sidecar implementation to check the completion status of initContainers before marking the taskRun complete. 

* :bug: fix: exclude pending PipelineRuns from  metric (#8951)

Fixed tekton_pipelines_controller_running_pipelineruns metric to exclude pending PipelineRuns, it now counts only running PipelineRuns

* :bug: Fix tini-git image to be multi-arch (#8944)

Updating tini-git base image to be multi-platform, also fixing the resolvers image.

* :bug: fix(#8940): token-authentication header typo in git resolver (#8937)

Bug fix: Before this change, there was a regression in which the git resolver was not authenticating with the provided `gitToken` and `gitTokenKey`, breaking the git resolver's http token-based auth. After this change, all git operations performed by the git resolver use the provided `gitToken` for remote authentication.

* :bug: fix: allow finalizer updates on completed TaskRun and PipelineRuns (#9011)
* :bug: fix nightly-build workflow to use kind setup action from allowed list (#8939)


### Misc



* :hammer: [FIX] Remove the apt warning (#8624)
* :hammer: build(deps): bump chainguard-dev/actions from 1.4.10 to 1.4.12 (#8989)
* :hammer: build(deps): bump google.golang.org/protobuf from 1.36.7 to 1.36.8 (#8985)
* :hammer: build(deps): bump google.golang.org/grpc from 1.74.2 to 1.75.0 (#8984)
* :hammer: build(deps): bump the all group in /tekton with 3 updates (#8978)
* :hammer: build(deps): bump github/codeql-action from 3.29.8 to 3.29.10 (#8977)
* :hammer: build(deps): bump chainguard-dev/actions from 1.4.9 to 1.4.10 (#8976)
* :hammer: build(deps): bump tj-actions/changed-files from f963b3f3562b00b6d2dd25efc390eb04e51ef6c6 to 2036da178f85576f1940fedb74bb93a36cd89ab7 (#8975)
* :hammer: build(deps): bump actions/dependency-review-action from 4.7.1 to 4.7.2 (#8974)
* :hammer: build(deps): bump k8s.io/apiextensions-apiserver from 0.32.7 to 0.32.8 (#8973)
* :hammer: build(deps): bump tj-actions/changed-files from 055970845dd036d7345da7399b7e89f2e10f2b04 to f963b3f3562b00b6d2dd25efc390eb04e51ef6c6 (#8964)
* :hammer: build(deps): bump github/codeql-action from 3.29.3 to 3.29.8 (#8963)
* :hammer: build(deps): bump actions/checkout from 4 to 5 (#8962)
* :hammer: build(deps): bump the all group in /tekton with 3 updates (#8961)
* :hammer: build(deps): bump chainguard-dev/actions from 1.4.6 to 1.4.9 (#8960)
* :hammer: build(deps): bump actions/cache from 4.2.3 to 4.2.4 (#8959)
* :hammer: build(deps): bump google.golang.org/protobuf from 1.36.6 to 1.36.7 (#8956)
* :hammer: build(deps): bump golang.org/x/crypto from 0.39.0 to 0.41.0 (#8954)
* :hammer: .github/workflows/nightly-builds: only run on tektoncd org (#8950)
* :hammer: build(deps): bump k8s.io/apiextensions-apiserver from 0.32.6 to 0.32.7 (#8894)

### Docs

* :book: docs: Switch from deprecated Tekton Hub to ArtifactHub (#8967)

Update examples and documentation to use ArtifactHub instead of the deprecated Tekton Hub for resource discovery and management.
* :book: release.md: update releases with 1.2.x and 1.3.x (#8952)

## Thanks

Thanks to these contributors who contributed to v1.4.0!
* :heart: @Maximilien-R
* :heart: @aThorp96
* :heart: @anithapriyanatarajan
* :heart: @dependabot[bot]
* :heart: @divyansh42
* :heart: @infernus01
* :heart: @khrm
* :heart: @leshikus
* :heart: @pritidesai
* :heart: @vdemeester
* :heart: @waveywaves

Extra shout-out for awesome release notes:
* :heart_eyes: @Maximilien-R
* :heart_eyes: @aThorp96
* :heart_eyes: @divyansh42
* :heart_eyes: @infernus01
* :heart_eyes: @pritidesai
* :heart_eyes: @vdemeester
* :heart_eyes: @waveywaves

<!--
## Unsorted PR List
- Disable the Gitea e2e tests temporarily to unblock (#9012)

To Be Done: Deprecation Notices, Backward Incompatible Changes
-->

<!-- risk-assessed -->
