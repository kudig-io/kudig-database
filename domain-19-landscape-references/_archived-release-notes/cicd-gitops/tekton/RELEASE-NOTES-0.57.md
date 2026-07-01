---
title: tekton v0.57 Release Notes
description: tekton v0.57 Release Notes — Kubernetes 生产运维知识库
summary: tekton v0.57 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
- docker
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
- tekton v0.57 Release Notes 是什么
- 如何 tekton v0.57 Release Notes
trigger_keywords:
- tekton
- v0.57
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---



# tekton v0.57 Release Notes

Source: [v0.57.0](https://github.com/tektoncd/pipeline/releases/tag/v0.57.0)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.57.0](https://github.com/tektoncd/pipeline/tree/v0.57.0/docs)
-[Examples @ v0.57.0](https://github.com/tektoncd/pipeline/tree/v0.57.0/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.57.0/release.yaml
```

## Attestation

The Rekor UUID for this release is `24296fb24b8ad77add7b0a9a7946185efd5c044009544db4ec1a3799c4b6a95285f979f1fd78cc75`

Obtain the attestation:
```shell
REKOR_UUID=24296fb24b8ad77add7b0a9a7946185efd5c044009544db4ec1a3799c4b6a95285f979f1fd78cc75
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.57.0/release.yaml
REKOR_UUID=24296fb24b8ad77add7b0a9a7946185efd5c044009544db4ec1a3799c4b6a95285f979f1fd78cc75

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.57.0@sha256:" + .digest.sha256')

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

* :sparkles: Allow  for the specified duration (#7666)

Configure default-imagepullbackoff-timeout to allow imagePullBackOff to retry and wait for the specified duration before failing the pipeline.

* :sparkles: Add granular termination reason in container termination message (#7565)

Steps in a TaskRun will have more granular termination reasons indicating what exactly happened in new terminationReason field: Completed, Continued, Error, TimeoutExceeded, Skipped, TaskRunCancelled


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

* :bug: fix(pipeline): correct warning path for duplicate param name in pipeline tasks (#7651)

fix: correct warning path for duplicate param name in pipeline tasks

* :bug: The  field in Final Task cannot parse ordinary Task status information. (#7637)

The status of the referenced ordinary task is replaced before calculating the final task `when.cel`.

* :bug: fix: prevent modification of annotations on completed TaskRuns (#7603)

fix: the pipeline controller will no longer modify any annotation it has set on completed pipelineruns

* :bug: allow pipeline runs whose task/custom runs have been deleted still timeout (#7557)

PipelineRuns that timeout will no longer be blocked on reaching a terminal, cancelled state if their underlying TaskRuns or CustomRuns were deleted beforehand.

* :bug: update docker-in-docker testimage for s390x (#7652)


### Misc


* :hammer: Update releases.md (#7587)

Created v0.56 LTS release.

* :hammer: matrix  name updated to end with the instance count (#7563)

taskRun names updated to end with the instance count for all fan out instances of matrix.

* :hammer: Isolate new env nightly feature flag test (#7686)
* :hammer: chore(deps): bump github/codeql-action from 3.24.0 to 3.24.3 (#7685)
* :hammer: chore(deps): bump tj-actions/changed-files from 42.0.2 to 42.0.4 (#7684)
* :hammer: chore(deps): bump github.com/opencontainers/image-spec from 1.1.0-rc6 to 1.1.0 (#7682)
* :hammer: chore(deps): bump github.com/google/cel-go from 0.19.0 to 0.20.0 (#7681)
* :hammer: chore(deps): bump k8s.io/client-go from 0.27.8 to 0.27.11 in /test/custom-task-ctrls/wait-task-beta (#7673)
* :hammer: chore(deps): bump google.golang.org/grpc from 1.61.0 to 1.61.1 (#7670)
* :hammer: Patch Release v0.56.1 (#7665)
* :hammer: Patch Release v0.56.1 (#7663)
* :hammer: chore(deps): bump go.opentelemetry.io/otel/sdk from 1.22.0 to 1.23.1 (#7659)
* :hammer: chore(deps): bump actions/upload-artifact from 4.3.0 to 4.3.1 (#7658)
* :hammer: Update e2e-test script for per-feature flag test (#7657)
* :hammer: Fix typo in publish task (#7648)
* :hammer: Bump github.com/golangci/golangci-lint from 1.55.1 to 1.56.1 in /tools (#7646)
* :hammer: Bump go.opentelemetry.io/otel from 1.22.0 to 1.23.1 (#7645)
* :hammer: Bump github.com/opencontainers/image-spec from 1.1.0-rc3 to 1.1.0-rc6 (#7635)
* :hammer: Bump github/codeql-action from 3.23.1 to 3.24.0 (#7634)
* :hammer: TEP-0138 New features to use Per-feature flag struct (#7633)
* :hammer: Bump github.com/containerd/containerd from 1.6.19 to 1.7.13 (#7628)
* :hammer: Per-feature Flag Test Suite (#7627)
* :hammer: Bump github.com/google/go-containerregistry from 0.18.0 to 0.19.0 (#7624)
* :hammer: Bump tj-actions/changed-files from 42.0.0 to 42.0.2 (#7622)
* :hammer: Bump actions/upload-artifact from 4.2.0 to 4.3.0 (#7620)
* :hammer: Bump github.com/google/go-containerregistry from 0.17.0 to 0.18.0 (#7616)
* :hammer: Bump google.golang.org/grpc from 1.60.1 to 1.61.0 (#7612)
* :hammer: Bump github.com/google/uuid from 1.5.0 to 1.6.0 (#7611)
* :hammer: Bump github.com/opencontainers/image-spec from 1.1.0-rc3 to 1.1.0-rc.6 (#7610)
* :hammer: Bump github.com/containerd/containerd from 1.6.19 to 1.7.12 (#7609)
* :hammer: Bump go.opentelemetry.io/otel/sdk from 1.21.0 to 1.22.0 (#7606)
* :hammer: Bump github.com/jenkins-x/go-scm from 1.14.25 to 1.14.26 (#7605)
* :hammer: Bump github.com/opencontainers/image-spec from 1.1.0-rc5 to 1.1.0-rc.6 (#7604)
* :hammer: Bump code.gitea.io/sdk/gitea from 0.16.0 to 0.17.1 (#7597)
* :hammer: Bump github.com/containerd/containerd from 1.7.11 to 1.7.12 (#7596)
* :hammer: Bump github.com/google/cel-go from 0.18.1 to 0.19.0 (#7594)
* :hammer: Bump tj-actions/changed-files from 41.1.1 to 42.0.0 (#7593)
* :hammer: Bump github/codeql-action from 3.23.0 to 3.23.1 (#7592)
* :hammer: Bump actions/upload-artifact from 4.1.0 to 4.2.0 (#7591)
* :hammer: Bump go.opentelemetry.io/otel from 1.21.0 to 1.22.0 (#7586)
* :hammer: Bump github.com/jenkins-x/go-scm from 1.14.24 to 1.14.25 (#7585)
* :hammer: Bump github.com/spiffe/go-spiffe/v2 from 2.1.5 to 2.1.7 (#7584)
* :hammer: Bump github.com/google/go-containerregistry from 0.17.0 to 0.18.0 (#7583)
* :hammer: Bump github.com/go-git/go-git/v5 from 5.10.0 to 5.11.0 (#7582)
* :hammer: Error sweep: fix error messages for timing out Runs (#7572)
* :hammer: Label user error for failed TaskRunStatus message (#7543)
* :hammer: Add pre-commit rules (#7367)

### Docs

* :book: Pipeline v0.44.x LTS End of Life (#7613)

Release v0.44 LTS is EOL


## Thanks

Thanks to these contributors who contributed to v0.57.0!
* :heart: @AlanGreene
* :heart: @Basavaraju-G
* :heart: @JeromeJu
* :heart: @afrittoli
* :heart: @chitrangpatel
* :heart: @cugykw
* :heart: @dependabot[bot]
* :heart: @gabemontero
* :heart: @l-qing
* :heart: @pritidesai
* :heart: @renzodavid9
* :heart: @roman-kiselenko

Extra shout-out for awesome release notes:
* :heart_eyes: @afrittoli
* :heart_eyes: @chitrangpatel
* :heart_eyes: @cugykw
* :heart_eyes: @gabemontero
* :heart_eyes: @l-qing
* :heart_eyes: @pritidesai
* :heart_eyes: @renzodavid9

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->