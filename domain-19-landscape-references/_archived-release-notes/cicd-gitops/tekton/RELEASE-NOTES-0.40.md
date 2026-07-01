---
title: tekton v0.40 Release Notes
description: tekton v0.40 Release Notes — Kubernetes 生产运维知识库
summary: tekton v0.40 Release Notes — Kubernetes 生产运维知识库
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
- tekton v0.40 Release Notes 是什么
- 如何 tekton v0.40 Release Notes
trigger_keywords:
- tekton
- v0.40
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# tekton v0.40 Release Notes

Source: [v0.40.2](https://github.com/tektoncd/pipeline/releases/tag/v0.40.2)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.40.2](https://github.com/tektoncd/pipeline/tree/v0.40.2/docs)
-[Examples @ v0.40.2](https://github.com/tektoncd/pipeline/tree/v0.40.2/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.40.2/release.yaml
```

## Attestation

The Rekor UUID for this release is `24296fb24b8ad77a2573afa5bfbd4582c0eb8c844009ee685a7e9abf6ae42b4d00b20c7485096315`

Obtain the attestation:
```shell
REKOR_UUID=24296fb24b8ad77a2573afa5bfbd4582c0eb8c844009ee685a7e9abf6ae42b4d00b20c7485096315
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.40.2/release.yaml
REKOR_UUID=24296fb24b8ad77a2573afa5bfbd4582c0eb8c844009ee685a7e9abf6ae42b4d00b20c7485096315

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.40.2@sha256:" + .digest.sha256')

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

* :bug: [release-v0.40.x] Write TaskRun.Status.TaskSpec with replaced spec on every reconcile run (#5584)

ix TaskRun parameter etc replacement logic to persist in the TaskRun's Status properly




### Misc

* :hammer: [release-v0.40.x] tekton: make sure the git workingdir is not dirty… (#5583)

ix the `-dirty` suffix in `pipeline.tekton.dev/release` annotation


* :hammer: [release-v0.40.x] Update [[ko|ko]] to v0.12.0 (#5567)

### Docs




## Thanks

Thanks to these contributors who contributed to v0.40.2!
* :heart: @tekton-robot

Extra shout-out for awesome release notes:
* :heart_eyes: @tekton-robot

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->