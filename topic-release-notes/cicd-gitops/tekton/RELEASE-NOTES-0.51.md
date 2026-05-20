---
title: tekton v0.51 Release Notes
description: tekton v0.51 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
- webhook
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v0.51 Release Notes 是什么
- 如何 tekton v0.51 Release Notes
trigger_keywords:
- tekton
- v0.51
- Release
- Notes
- release
- notes
---

# tekton v0.51 Release Notes

Source: [v0.51.0](https://github.com/tektoncd/pipeline/releases/tag/v0.51.0)

# 🎉 Co-schedule option and bugfixes 🎉

-[Docs @ v0.51.0](https://github.com/tektoncd/pipeline/tree/v0.51.0/docs)
-[Examples @ v0.51.0](https://github.com/tektoncd/pipeline/tree/v0.51.0/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.51.0/release.yaml
```

## Attestation

The Rekor UUID for this release is `24296fb24b8ad77af0123195ea84840480151ea9735ca9e2f869d262e403dad6fa6c42c32bc04193`

Obtain the attestation:
```shell
REKOR_UUID=24296fb24b8ad77af0123195ea84840480151ea9735ca9e2f869d262e403dad6fa6c42c32bc04193
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.51.0/release.yaml
REKOR_UUID=24296fb24b8ad77af0123195ea84840480151ea9735ca9e2f869d262e403dad6fa6c42c32bc04193

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.51.0@sha256:" + .digest.sha256')

# Download the release file
curl "$RELEASE_FILE" > release.yaml

# For each image in the attestation, match it to the release file
for image in $REKOR_ATTESTATION_IMAGES; do
  printf $image; grep -q $image release.yaml && echo " ===> ok" || echo " ===> no match";
done
```

## Upgrade Notices

**With this release, the minimun Kubernetes version supported is now 1.25.**

## Changes

# Features

* :sparkles: [TEP-0135] Coschedule per (Isolated) PipelineRun e2e support (#6927)

[TEP-0135]: Support `coschedule: pipelineruns` and `coschedule: isolate-pipelinerun` coschedule modes.
Users can now opt in this new feature to schedule all the pods in the same node and to optionally enforce one running pipelinerun in a node at the same time.
* :sparkles: Add service for Resolvers - metrics, probes and tracing (#6973)


# Deprecation Notices


* 🚨 Mark disable-affinity-assistant as deprecated (#6991)

The `disable-affinity-assistant` feature flag is deprecated in favour of the new `coschedule` feature flag. The `disable-affinity-assistant` feature flag will be removed in 9 months. 

<!--
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

* :bug: Fix release pipeline (publish-to-bucket) (#7044)

Binary file (standard input) matches

* :bug: Make sure we use the correct namespace for remote Pipeline validation (#7017)

ix validation errors when using the cluster resolver

* :bug: Create a separate role for Events Controller (#7016)

The role for Events Controller is now `tekton-events-controller`,  and the Rolebinding is now `tekton-pipelines-events-controller.`

* :bug: fix: add taskRunTemplate field validation (#6983)

Validate forbidden envs in TaskRunTemplate.PodTemplate.

* :bug: Fail fast on invalid image (#6982)

The Pod reason InvalidImageName is treated now as a permanent issue, so that TaskRuns that include a step with an invalid image reference are failed immediately and the corresponding Pod is deleted.

* :bug: Sync checksum between trusted resources and cluster resolver (#6964)

Cluster resolver now computes the checksum of the pre-processed Tekton object instead of just the spec.

* :bug: Fix Taskrun Failure for Preempted Pod of Taskrun (#6962)

This fixes the Taskrun failure for Preempted Pod of Taskrun which uses PVC. 

* :bug: fix: refine error resean with invalid pipelinename in taskrunspecs (#6957)

When the taskRunSpecs of the pipelineRun contains an invalid pipeline task name, the cause of the `InvalidTaskRunSpecs` error is displayed.

* :bug: Fix enforce-nonfalsifiability feature flag in configmap (#6937)


### Misc


* :hammer: Mark disable-affinity-assistant as deprecated (#6991)

action required: The `disable-affinity-assistant` feature flag is deprecated in favour of the new `coschedule` feature flag. The `disable-affinity-assistant` feature flag will be removed in 9 months. 
The Affinity Assistant behaviour should now be configured by the `coschedule` feature flag.

* :hammer: Bump knative/pkg to 1.11 (#6975)

Bump knative.dev/pkg to 1.11 so the Kubernetes min version is now 1.25

* :hammer: Add webhook validation for remote Tasks (#6942)

Remote tasks are now validated by any validating admission webhooks.
* :hammer: [TEP-0135] Refactor CreatePVCsForWorkspaces (#6921)
* :hammer: Bump github.com/golangci/golangci-lint from 1.54.1 to 1.54.2 in /tools (#7057)
* :hammer: Bump github.com/golangci/golangci-lint from 1.54.0 to 1.54.1 in /tools (#7047)
* :hammer: Bump github.com/golangci/golangci-lint from 1.53.3 to 1.54.0 in /tools (#7039)
* :hammer: Bump github.com/hashicorp/golang-lru from 0.5.4 to 1.0.2 (#7031)
* :hammer: Bump github.com/cloudflare/circl from 1.1.0 to 1.3.3 (#7026)
* :hammer: Bump github.com/google/go-containerregistry from 0.15.2 to 0.16.1 (#7021)
* :hammer: Bump go.uber.org/zap from 1.24.0 to 1.25.0 (#7018)
* :hammer: Bump github.com/containerd/containerd from 1.6.19 to 1.7.3 (#7002)
* :hammer: Bump github.com/spiffe/spire-api-sdk from 1.7.0 to 1.7.1 (#6997)
* :hammer: Bump github.com/go-git/go-git/v5 from 5.6.1 to 5.8.1 (#6980)
* :hammer: Add E2E Testing for Matrix (#6944)

### Docs


* :book: docs: Update references to examples from v1beta1 to v1 (#7050)
* :book: Fix typos and formatting in TaskRuns doc (#7020)
* :book: Update Matrix Documentation for Results (#7012)
* :book: [TEP-0135] Improve workspace related documentation (#6994)
* :book: Remove warnings about matrix being non-functional (#6986)
* :book: Add v0.50 to releases.md (#6967)
* :book: [TEP-0135] Update Affinity Assistant documentation (#6892)

## Thanks

Thanks to these contributors who contributed to v0.51.0!
* :heart: @AlanGreene
* :heart: @EmmaMunley
* :heart: @HamzaMateen
* :heart: @QuanZhang-William
* :heart: @afrittoli
* :heart: @chitrangpatel
* :heart: @cugykw
* :heart: @dependabot[bot]
* :heart: @khrm
* :heart: @lbernick
* :heart: @vdemeester

Extra shout-out for awesome release notes:
* :heart_eyes: @QuanZhang-William
* :heart_eyes: @afrittoli
* :heart_eyes: @chitrangpatel
* :heart_eyes: @cugykw
* :heart_eyes: @khrm
* :heart_eyes: @lbernick
* :heart_eyes: @vdemeester

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->