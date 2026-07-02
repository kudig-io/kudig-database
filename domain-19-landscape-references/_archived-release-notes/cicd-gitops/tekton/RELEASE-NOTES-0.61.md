---
title: tekton v0.61 Release Notes
description: tekton v0.61 Release Notes — Kubernetes 生产运维知识库
summary: tekton v0.61 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
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
- tekton v0.61 Release Notes 是什么
- 如何 tekton v0.61 Release Notes
trigger_keywords:
- tekton
- v0.61
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




# tekton v0.61 Release Notes

Source: [v0.61.1](https://github.com/tektoncd/pipeline/releases/tag/v0.61.1)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.61.1](https://github.com/tektoncd/pipeline/tree/v0.61.1/docs)
-[Examples @ v0.61.1](https://github.com/tektoncd/pipeline/tree/v0.61.1/examples)

## Installation one-liner

``` shell
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.61.1/release.yaml
```
## Attestation

The Rekor UUID for this release is `24296fb24b8ad77a7bf5b4e52e97f499e0dc71aed47d629395ba503bbc0cf8a16d8b49169d2db2f5`

Obtain the attestation:
```shell
REKOR_UUID=24296fb24b8ad77a7bf5b4e52e97f499e0dc71aed47d629395ba503bbc0cf8a16d8b49169d2db2f5
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.61.1/release.yaml
REKOR_UUID=24296fb24b8ad77a7bf5b4e52e97f499e0dc71aed47d629395ba503bbc0cf8a16d8b49169d2db2f5

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.61.1@sha256:" + .digest.sha256')

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

* :bug: [release-v0.61.x] Handle error conditions in CheckMissingResultReferences (#8105)

mproved error handling for some invalid result reference scenarios.




### Misc






### Docs




## Thanks

Thanks to these contributors who contributed to v0.61.1!
* :heart: @tekton-robot

Extra shout-out for awesome release notes:
* :heart_eyes: @tekton-robot

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->

<!-- risk-assessed -->
