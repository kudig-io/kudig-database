---
title: tekton v0.35 Release Notes
description: tekton v0.35 Release Notes — Kubernetes 生产运维知识库
summary: tekton v0.35 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- statefulset
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
- tekton v0.35 Release Notes 是什么
- 如何 tekton v0.35 Release Notes
trigger_keywords:
- tekton
- v0.35
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# tekton v0.35 Release Notes

Source: [v0.35.1](https://github.com/tektoncd/pipeline/releases/tag/v0.35.1)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.35.1](https://github.com/tektoncd/pipeline/tree/v0.35.1/docs)
-[Examples @ v0.35.1](https://github.com/tektoncd/pipeline/tree/v0.35.1/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.35.1/release.yaml
```

## Attestation

The Rekor UUID for this release is `de02942bd2a6ebca8c094b7e69d31ccbc38d528d37f1b18d2f008e3710779f10`

Obtain the attestation:
```shell
REKOR_UUID=de02942bd2a6ebca8c094b7e69d31ccbc38d528d37f1b18d2f008e3710779f10
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | base64 --decode | jq
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.35.1/release.yaml
REKOR_UUID=de02942bd2a6ebca8c094b7e69d31ccbc38d528d37f1b18d2f008e3710779f10

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | base64 --decode | jq -r '.subject[]|.name + ":v0.35.1@sha256:" + .digest.sha256')

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

* :bug: [PATCH] Patch [[Knative|knative]]/pkg to fix HA via [[StatefulSet|StatefulSet]] (#4864)

Restores the HA Setup via StatefulSet which was broken in v0.35.0

* :bug: Fix bug where PipelineRun hangs after task failure (#4854)

[Bug fix] Prevent PipelineRun from hanging when a PipelineTask fails and another PipelineTask depends on it




### Misc






### Docs




## Thanks

Thanks to these contributors who contributed to v0.35.1!
* :heart: @lbernick 
* :heart: @afrittoli

Extra shout-out for awesome release notes:
* :heart_eyes: @lbernick 
* :heart_eyes: @afrittoli

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->