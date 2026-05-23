---
title: tekton v0.43 Release Notes
description: tekton v0.43 Release Notes — Kubernetes 生产运维知识库
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
- tekton v0.43 Release Notes 是什么
- 如何 tekton v0.43 Release Notes
trigger_keywords:
- tekton
- v0.43
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# tekton v0.43 Release Notes

Source: [v0.43.2](https://github.com/tektoncd/pipeline/releases/tag/v0.43.2)

-[Docs @ v0.43.2](https://github.com/tektoncd/pipeline/tree/v0.43.2/docs)
-[Examples @ v0.43.2](https://github.com/tektoncd/pipeline/tree/v0.43.2/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.43.2/release.yaml
```

## Attestation

The Rekor UUID for this release is `24296fb24b8ad77ac03a9e9a1de2842d68ade2674937db60c0150c990bf5e0347dec1edcad393407`

Obtain the attestation:
```shell
REKOR_UUID=24296fb24b8ad77ac03a9e9a1de2842d68ade2674937db60c0150c990bf5e0347dec1edcad393407
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.43.2/release.yaml
REKOR_UUID=24296fb24b8ad77ac03a9e9a1de2842d68ade2674937db60c0150c990bf5e0347dec1edcad393407

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.43.2@sha256:" + .digest.sha256')

# Download the release file
curl "$RELEASE_FILE" > release.yaml

# For each image in the attestation, match it to the release file
for image in $REKOR_ATTESTATION_IMAGES; do
  printf $image; grep -q $image release.yaml && echo " ===> ok" || echo " ===> no match";
done
```

## Changes

### Fixes

* :bug: [release-v0.43.x] Fix embedded-status conversion for PipelineRuns (#5974)

Update PipelineRun conversion between API versions to account for embedded-status feature flag

## Thanks

Thanks to these contributors who contributed to v0.43.2!
* :heart: @lbernick

Extra shout-out for awesome release notes:
* :heart_eyes: @lbernick