---
title: tekton v0.37 Release Notes
description: tekton v0.37 Release Notes — Kubernetes 生产运维知识库
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
- tekton v0.37 Release Notes 是什么
- 如何 tekton v0.37 Release Notes
trigger_keywords:
- tekton
- v0.37
- Release
- Notes
- release
- notes
---

# tekton v0.37 Release Notes

Source: [v0.37.5](https://github.com/tektoncd/pipeline/releases/tag/v0.37.5)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.37.5](https://github.com/tektoncd/pipeline/tree/v0.37.5/docs)
-[Examples @ v0.37.5](https://github.com/tektoncd/pipeline/tree/v0.37.5/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.37.5/release.yaml
```

## Attestation

The Rekor UUID for this release is `362f8ecba72f43263b14ba88ed3003f2038017cca2b180c14ad3e3263321a6a92ea4977c465b526d`

Obtain the attestation:
```shell
REKOR_UUID=362f8ecba72f43263b14ba88ed3003f2038017cca2b180c14ad3e3263321a6a92ea4977c465b526d
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.37.5/release.yaml
REKOR_UUID=362f8ecba72f43263b14ba88ed3003f2038017cca2b180c14ad3e3263321a6a92ea4977c465b526d

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.37.5@sha256:" + .digest.sha256')

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

* :bug: [release-v0.37.x] de-dupe order and resource dependencies (#5482)

De-dupe task dependencies - order and resource dependencies all together. It's very common to have a task with multiple when expressions referring to the same task but different results. Maintain a set of dependencies and add only a new parent.

* :bug: [release-v0.37.x] Improve DAG validation for pipelines with hundreds of tasks (#5430)

Fixes https://github.com/tektoncd/pipeline/issues/5420 - Improve DAG validation for pipelines with hundreds of tasks (validation wehbook performance)

* :bug: [release-v0.37.x] Fix the issue with empty array replacement  (#5394)

After the replacement with an empty array, the original array will be empty.

Example:

```yaml
params:
  - name: myarray
     value: "$(params.anEmptyArray[*])"
```



### Misc




* :hammer: [release-v0.37.x] Fix TestYamls for change in `ko create` (#5438)

### Docs




## Thanks

Thanks to these contributors who contributed to v0.37.5!
* :heart: @rafalbigaj 
* :heart: @abayer
* :heart: @pritidesai
* :heart: @vdemeester

Extra shout-out for awesome release notes:
* :heart_eyes: @rafalbigaj 
* :heart_eyes: @pritidesai
* :heart_eyes: @vdemeester

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->