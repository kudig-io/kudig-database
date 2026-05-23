---
title: tekton v0.36 Release Notes
description: tekton v0.36 Release Notes — Kubernetes 生产运维知识库
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
- tekton v0.36 Release Notes 是什么
- 如何 tekton v0.36 Release Notes
trigger_keywords:
- tekton
- v0.36
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# tekton v0.36 Release Notes

Source: [v0.36.1](https://github.com/tektoncd/pipeline/releases/tag/v0.36.1)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.36.1](https://github.com/tektoncd/pipeline/tree/v0.36.1/docs)
-[Examples @ v0.36.1](https://github.com/tektoncd/pipeline/tree/v0.36.1/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.36.1/release.yaml
```

## Attestation

The Rekor UUID for this release is `362f8ecba72f4326ba7c696af10c9c634aa17d43f9ffc6e7c877d332b0e0f634434f09904b654e8c`

Obtain the attestation:
```shell
REKOR_UUID=362f8ecba72f4326ba7c696af10c9c634aa17d43f9ffc6e7c877d332b0e0f634434f09904b654e8c
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.36.1/release.yaml
REKOR_UUID=362f8ecba72f4326ba7c696af10c9c634aa17d43f9ffc6e7c877d332b0e0f634434f09904b654e8c

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.36.1@sha256:" + .digest.sha256')

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

* :bug: [release-v0.36.x] de-dupe order and resource dependencies (#5479)

De-dupe task dependencies - order and resource dependencies all together. It's very common to have a task with multiple when expressions referring to the same task but different results. Maintain a set of dependencies and add only a new parent.

* :bug: [release-v0.36.x] Improve DAG validation for pipelines with hundreds of tasks (#5461)

bug fixes:
- https://github.com/tektoncd/pipeline/issues/5420 - Improve DAG validation for pipelines with hundreds of tasks (validation wehbook performance)

* :bug: [release-v0.36.x] Fix the issue with empty array replacement  (#5442)

After the replacement with an empty array, the original array will be empty.
Example:
```
params:
  - name: myarray
     value: "$(params.anEmptyArray[*])"
```

* :bug: [0.36: cherry-pick] cmd/entrypoint: do not interpret anything after  (#5096)

Binary file (standard input) matches

* :bug: Relax result type validation to avoid nightly build failure (#5068)

Relax the validation of result type: allow for no type specified to support resources created before result types were introduced.




### Misc




* :hammer: [release-v0.36.x] Fix TestYamls for change in `[[ko|ko]] create` (#5445)
* :hammer: [0.36: cherry-pick] Fix TestTaskRunRetry for k8s 1.22.9 and later (#5148)

### Docs




## Thanks

Thanks to these contributors who contributed to v0.36.1!
* :heart: @abayer
* :heart: @afrittoli
* :heart: @pritidesai
* :heart: @rafalbigaj 
* :heart: @vdemeester

Extra shout-out for awesome release notes:
* :heart_eyes: @afrittoli
* :heart_eyes: @pritidesai
* :heart_eyes: @rafalbigaj 
* :heart_eyes: @vdemeester

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->