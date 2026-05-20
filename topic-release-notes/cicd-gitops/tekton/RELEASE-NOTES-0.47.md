---
title: tekton v0.47 Release Notes
description: tekton v0.47 Release Notes — Kubernetes 生产运维知识库
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
- tekton v0.47 Release Notes 是什么
- 如何 tekton v0.47 Release Notes
trigger_keywords:
- tekton
- v0.47
- Release
- Notes
- release
- notes
---

# tekton v0.47 Release Notes

Source: [v0.47.9](https://github.com/tektoncd/pipeline/releases/tag/v0.47.9)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.47.9](https://github.com/tektoncd/pipeline/tree/v0.47.9/docs)
-[Examples @ v0.47.9](https://github.com/tektoncd/pipeline/tree/v0.47.9/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.47.9/release.yaml
```

## Attestation

The Rekor UUID for this release is `24296fb24b8ad77a97b4cab5b45b50c5ccf7747c415168ca5a52a02bf17db08b9289c8518215ab4e`

Obtain the attestation:
```shell
REKOR_UUID=24296fb24b8ad77a97b4cab5b45b50c5ccf7747c415168ca5a52a02bf17db08b9289c8518215ab4e
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.47.9/release.yaml
REKOR_UUID=24296fb24b8ad77a97b4cab5b45b50c5ccf7747c415168ca5a52a02bf17db08b9289c8518215ab4e

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.47.9@sha256:" + .digest.sha256')

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



- [release-v0.47.x] chore(deps): Migrate to github.com/go-jose/go-jose/v3 (#7859)



### Misc






### Docs




## Thanks

Thanks to these contributors who contributed to v0.47.9!
* :heart: @vdemeester

Extra shout-out for awesome release notes:
* :heart_eyes: @vdemeester

<!--
## Unsorted PR List

To Be Done: Deprecation Notices, Backward Incompatible Changes
-->