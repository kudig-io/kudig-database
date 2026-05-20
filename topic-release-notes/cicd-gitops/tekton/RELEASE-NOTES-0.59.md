---
title: tekton v0.59 Release Notes
description: tekton v0.59 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v0.59 Release Notes 是什么
- 如何 tekton v0.59 Release Notes
trigger_keywords:
- tekton
- v0.59
- Release
- Notes
- release
- notes
---

# tekton v0.59 Release Notes

Source: [v0.59.6](https://github.com/tektoncd/pipeline/releases/tag/v0.59.6)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.59.6](https://github.com/tektoncd/pipeline/tree/v0.59.6/docs)
-[Examples @ v0.59.6](https://github.com/tektoncd/pipeline/tree/v0.59.6/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.59.6/release.yaml
```

## Attestation

The Rekor UUID for this release is `108e9186e8c5677a3b6f02cfe11967af07cb62f2f7ade3716195d8cd4e8ae04533c64fab45285a74`

Obtain the attestation:
```shell
REKOR_UUID=108e9186e8c5677a3b6f02cfe11967af07cb62f2f7ade3716195d8cd4e8ae04533c64fab45285a74
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.59.6/release.yaml
REKOR_UUID=108e9186e8c5677a3b6f02cfe11967af07cb62f2f7ade3716195d8cd4e8ae04533c64fab45285a74

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.59.6@sha256:" + .digest.sha256')

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

* :bug: [release-v0.59.x] fix: avoid panic when validate enum param with special matrix task (#8517)

ix: avoid panic when validate enum param with special matrix task




### Misc




* :hammer: [release-v0.59.x] .github/workflows: add a build and test workflows (#8580)
* :hammer: [release-v0.59.x] .github/workflows: backport e2e workflows (#8450)

### Docs




## Thanks

Thanks to these contributors who contributed to v0.59.6!
* :heart: @tekton-robot
* :heart: @vdemeester

Extra shout-out for awesome release notes:
* :heart_eyes: @tekton-robot

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->