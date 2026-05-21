---
title: tekton v1.5 Release Notes
description: tekton v1.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v1.5 Release Notes 是什么
- 如何 tekton v1.5 Release Notes
trigger_keywords:
- tekton
- v1.5
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# tekton v1.5 Release Notes

Source: [v1.5.0](https://github.com/tektoncd/pipeline/releases/tag/v1.5.0)

# 🎉 Use `managedBy` to delegate `pipelineRun` and `taskRun` lifecycle control 🎉

-[Docs @ v1.5.0](https://github.com/tektoncd/pipeline/tree/v1.5.0/docs)
-[Examples @ v1.5.0](https://github.com/tektoncd/pipeline/tree/v1.5.0/examples)

## Installation one-liner

```shell
kubectl apply -f https://infra.tekton.dev/tekton-releases/pipeline/previous/v1.5.0/release.yaml
```

## Attestation

The Rekor UUID for this release is `108e9186e8c5677a657cc892687dc9dbf41be24c29f51d2f5fc1092446b0739ec5280bb6b0bc1b82`

Obtain the attestation:
```shell
REKOR_UUID=108e9186e8c5677a657cc892687dc9dbf41be24c29f51d2f5fc1092446b0739ec5280bb6b0bc1b82
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://infra.tekton.dev/tekton-releases/pipeline/previous/v1.5.0/release.yaml
REKOR_UUID=108e9186e8c5677a657cc892687dc9dbf41be24c29f51d2f5fc1092446b0739ec5280bb6b0bc1b82

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v1.5.0@sha256:" + .digest.sha256')

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

* :sparkles: Add Support for managedBy field in TaskRun and PipelineRun (#8965)

Added a "managedBy" field to delegate responsibility of controlling the lifecycle of PipelineRuns/TaskRuns.

The semantics of the field:

Whenever the value is set, and it does not point to the built-in controller, then we skip the reconciliation.
* The field is immutable
* The field is not defaulted



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






### Misc




* :hammer: GHA label checker (#9050)
* :hammer: build(deps): bump github/codeql-action from 3.29.10 to 3.30.1 (#9030)
* :hammer: Set the user-agent in the release name tool (#9016)
* :hammer: add khrm to reviewers in OWNERS_ALIASES (#9026)


### Docs


* :book: document 1.3.2 patch (#9022)
* :book: Updates for release 1.4 (#9020)

## Thanks

Thanks to these contributors who contributed to v1.5.0!
* :heart: @afrittoli
* :heart: @dependabot[bot]
* :heart: @khrm
* :heart: @pritidesai
* :heart: @waveywaves

Extra shout-out for awesome release notes:
* :heart_eyes: @khrm

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->