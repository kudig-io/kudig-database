---
title: tekton v0.41 Release Notes
description: tekton v0.41 Release Notes — Kubernetes 生产运维知识库
summary: tekton v0.41 Release Notes — Kubernetes 生产运维知识库
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
- tekton v0.41 Release Notes 是什么
- 如何 tekton v0.41 Release Notes
trigger_keywords:
- tekton
- v0.41
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# tekton v0.41 Release Notes

Source: [v0.41.3](https://github.com/tektoncd/pipeline/releases/tag/v0.41.3)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.41.3](https://github.com/tektoncd/pipeline/tree/v0.41.3/docs)
-[Examples @ v0.41.3](https://github.com/tektoncd/pipeline/tree/v0.41.3/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.41.3/release.yaml
```

## Attestation

The Rekor UUID for this release is `24296fb24b8ad77ac1565c644cf61337d73f9ec1057463c2dae22579280825a8c1db1641fde00202`

Obtain the attestation:
```shell
REKOR_UUID=24296fb24b8ad77ac1565c644cf61337d73f9ec1057463c2dae22579280825a8c1db1641fde00202
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.41.3/release.yaml
REKOR_UUID=24296fb24b8ad77ac1565c644cf61337d73f9ec1057463c2dae22579280825a8c1db1641fde00202

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.41.3@sha256:" + .digest.sha256')

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

* :bug: [release v0.41.x] Fix v1beta1 pipelineref bundle conversion to resolver (#6807)

bug fix: bundle resolver type param value for pipelineRef conversion

* :bug: [release-v0.41.x] Fix spammy logs (#6787)

bug fix: reduce log spam




### Misc






### Docs




## Thanks

Thanks to these contributors who contributed to v0.41.3!
* :heart: @JeromeJu
* :heart: @lbernick

Extra shout-out for awesome release notes:
* :heart_eyes: @JeromeJu
* :heart_eyes: @lbernick

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->