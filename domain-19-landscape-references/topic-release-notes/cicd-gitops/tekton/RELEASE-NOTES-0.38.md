---
title: tekton v0.38 Release Notes
description: tekton v0.38 Release Notes — Kubernetes 生产运维知识库
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
- tekton v0.38 Release Notes 是什么
- 如何 tekton v0.38 Release Notes
trigger_keywords:
- tekton
- v0.38
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# tekton v0.38 Release Notes

Source: [v0.38.4](https://github.com/tektoncd/pipeline/releases/tag/v0.38.4)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.38.4](https://github.com/tektoncd/pipeline/tree/v0.38.4/docs)
-[Examples @ v0.38.4](https://github.com/tektoncd/pipeline/tree/v0.38.4/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.38.4/release.yaml
```

## Attestation

The Rekor UUID for this release is `362f8ecba72f4326fa5b24a3cce6792d794726e3efd6e3c151eaa96ef7dfdc1ccf8ffc2230201d18`

Obtain the attestation:
```shell
REKOR_UUID=362f8ecba72f4326fa5b24a3cce6792d794726e3efd6e3c151eaa96ef7dfdc1ccf8ffc2230201d18
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.38.4/release.yaml
REKOR_UUID=362f8ecba72f4326fa5b24a3cce6792d794726e3efd6e3c151eaa96ef7dfdc1ccf8ffc2230201d18

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.38.4@sha256:" + .digest.sha256')

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

* :bug: [release-v0.38.x] de-dupe order and resource dependencies (#5483)

De-dupe task dependencies - order and resource dependencies all together. It's very common to have a task with multiple when expressions referring to the same task but different results. Maintain a set of dependencies and add only a new parent.

* :bug: [release-v0.38.x] Improve DAG validation for pipelines with hundreds of tasks (#5431)

Fixes https://github.com/tektoncd/pipeline/issues/5420 - Improve DAG validation for pipelines with hundreds of tasks (validation wehbook performance)




### Misc




* :hammer: [release-v0.38.x] Fix TestYamls for change in `ko create` (#5439)

### Docs




## Thanks

Thanks to these contributors who contributed to v0.38.4!
* :heart: @abayer
* :heart: @pritidesai
* :heart: @rafalbigaj 

Extra shout-out for awesome release notes:
* :heart_eyes: @pritidesai
* :heart_eyes: @rafalbigaj 

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->