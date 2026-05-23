---
title: tekton v0.58 Release Notes
description: tekton v0.58 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- jaeger
- containerd
- docker
- opa
- crd
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v0.58 Release Notes 是什么
- 如何 tekton v0.58 Release Notes
trigger_keywords:
- tekton
- v0.58
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
- tracing-basics
- observability-basics
created: "2026-05-23"
---

# tekton v0.58 Release Notes

Source: [v0.58.0](https://github.com/tektoncd/pipeline/releases/tag/v0.58.0)

# 🎉 `displayName` in `childReferences` and dynamic specifications of `[[Secrets|secrets]]` and `[[ConfigMaps|configmaps]]` in `workspaces` 🎉

-[Docs @ v0.58.0](https://github.com/tektoncd/pipeline/tree/v0.58.0/docs)
-[Examples @ v0.58.0](https://github.com/tektoncd/pipeline/tree/v0.58.0/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.58.0/release.yaml
```

## Attestation

The Rekor UUID for this release is `24296fb24b8ad77ad32de0077ddf3d746f9072f2d536cec99e2add11d56d964943ea86f5265aec54`

Obtain the attestation:
```shell
REKOR_UUID=24296fb24b8ad77ad32de0077ddf3d746f9072f2d536cec99e2add11d56d964943ea86f5265aec54
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.58.0/release.yaml
REKOR_UUID=24296fb24b8ad77ad32de0077ddf3d746f9072f2d536cec99e2add11d56d964943ea86f5265aec54

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.58.0@sha256:" + .digest.sha256')

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

* :sparkles: TEP-0147: introduce feature flag to guard artifacts feature (#7705)

Introduces a feature flag enable-artifacts.

* :sparkles: TEP 0147: add inputs/outputs to stepState  (#7703)

introduce inputs/outputs to stepState for future artifacts work

* :sparkles: implementing TEP-0150 -  in  (#7683)

A fully resolved displayName is now available in childReferences along with the pipelineTaskName. This is mainly beneficial to parameterize and easily distinguish matrix instances of the task. 

* :sparkles: feat: support for variable interpolation in workspace.* (in PipelineRun and TaskRun) (#7671)

feat: support for variable interpolation in workspace.* (in PipelineRun and TaskRun)


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

* :bug: fix: avoid panic when used pipelineRef or pipelineSpec in pipeline task (#7722)

fix: avoid panic when used pipelineRef or pipelineSpec in pipeline task

* :bug: fix: pipeline execution status test case index error (#7742)
* :bug: Migrate [[Jaeger|jaeger]] to otel API (#7547)


### Misc




* :hammer: chore(deps): bump google.golang.org/grpc from 1.62.0 to 1.62.1 (#7774)
* :hammer: chore(deps): bump google.golang.org/protobuf from 1.32.0 to 1.33.0 (#7773)
* :hammer: chore(deps): bump github.com/stretchr/testify from 1.8.4 to 1.9.0 (#7772)
* :hammer: chore(deps): bump tj-actions/changed-files from 42.1.0 to 43.0.0 (#7771)
* :hammer: chore(deps): bump github.com/containerd/containerd from 1.7.13 to 1.7.14 (#7770)
* :hammer: chore(deps): bump github/codeql-action from 3.24.6 to 3.24.8 (#7769)
* :hammer: chore(deps): bump actions/checkout from 4.1.1 to 4.1.2 (#7768)
* :hammer: chore(deps): bump k8s.io/api from 0.27.11 to 0.27.12 in /test/custom-task-ctrls/wait-task-beta (#7767)
* :hammer: chore(deps): bump tj-actions/changed-files from 42.0.5 to 42.1.0 (#7747)
* :hammer: chore(deps): bump github/codeql-action from 3.24.5 to 3.24.6 (#7735)
* :hammer: chore(deps): bump github.com/sigstore/sigstore/pkg/signature/kms/aws from 1.8.1 to 1.8.2 (#7727)
* :hammer: chore(deps): bump github.com/sigstore/sigstore/pkg/signature/kms/hashivault from 1.8.1 to 1.8.2 (#7723)
* :hammer: chore(deps): bump github/codeql-action from 3.24.3 to 3.24.5 (#7719)
* :hammer: chore(deps): bump tj-actions/changed-files from 42.0.4 to 42.0.5 (#7718)
* :hammer: chore(deps): bump github.com/spiffe/spire-api-sdk from 1.8.7 to 1.9.0 (#7712)
* :hammer: chore(deps): bump go.opentelemetry.io/otel/sdk from 1.23.1 to 1.24.0 (#7710)
* :hammer: chore(deps): bump go.opentelemetry.io/otel from 1.23.1 to 1.24.0 (#7709)
* :hammer: chore(deps): bump google.golang.org/grpc from 1.61.1 to 1.62.0 (#7702)
* :hammer: chore(deps): bump go.uber.org/zap from 1.26.0 to 1.27.0 (#7696)
* :hammer: chore(deps): bump github.com/cloudevents/sdk-go/v2 from 2.14.0 to 2.15.1 (#7695)
* :hammer: chore(deps): bump github.com/golangci/golangci-lint from 1.56.1 to 1.56.2 in /tools (#7676)
* :hammer: fix: reduce warnings caused by woke scan results (#7558)
* :hammer: Bump github.com/docker/docker from 24.0.0+incompatible to 24.0.7+incompatible (#7526)

### Docs

* :book: [TEP-0129] Move CRDs definition and update multi-tenancy docs accordingly (#7598)

Document simple installation instructions for a Tekton multi-tenancy setup.
* :book: docs: changing the variable camel cases (#7701)
* :book: fix:add missing documentation link (#7697)
* :book: Fix link to CEL in WhenExpression docs (#7692)
* :book: Fix typo in additional configs doc (#7689)
* :book: Add release v0.57.0 to the list of releases (#7687)
* :book: Add feature flags recording demo for developer guide (#7662)
* :book: docs: optimize examples for propagating results (#7554)

## Thanks

Thanks to these contributors who contributed to v0.58.0!
* :heart: @AlanGreene
* :heart: @JeromeJu
* :heart: @afrittoli
* :heart: @cugykw
* :heart: @dependabot[bot]
* :heart: @ericzzzzzzz
* :heart: @katmutua
* :heart: @kmjayadeep
* :heart: @l-qing
* :heart: @pritidesai
* :heart: @vdemeester

Extra shout-out for awesome release notes:
* :heart_eyes: @afrittoli
* :heart_eyes: @ericzzzzzzz
* :heart_eyes: @l-qing
* :heart_eyes: @pritidesai

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->