---
title: tekton v0.64 Release Notes
description: tekton v0.64 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v0.64 Release Notes 是什么
- 如何 tekton v0.64 Release Notes
trigger_keywords:
- tekton
- v0.64
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
created: "2026-05-23"
---

# tekton v0.64 Release Notes

Source: [v0.64.0](https://github.com/tektoncd/pipeline/releases/tag/v0.64.0)

# 🎉 Released Container Images stored on ghcr.io  🎉

-[Docs @ v0.64.0](https://github.com/tektoncd/pipeline/tree/v0.64.0/docs)
-[Examples @ v0.64.0](https://github.com/tektoncd/pipeline/tree/v0.64.0/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.64.0/release.yaml
```

## Attestation

The Rekor UUID for this release is `108e9186e8c5677ab54c658d37a263dfad3c8244bbef3e63cced8ae2d37c05701abf89bc6fa1fdf8`

Obtain the attestation:
```shell
REKOR_UUID=108e9186e8c5677ab54c658d37a263dfad3c8244bbef3e63cced8ae2d37c05701abf89bc6fa1fdf8
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.64.0/release.yaml
REKOR_UUID=108e9186e8c5677ab54c658d37a263dfad3c8244bbef3e63cced8ae2d37c05701abf89bc6fa1fdf8

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.64.0@sha256:" + .digest.sha256')

# Download the release file
curl "$RELEASE_FILE" > release.yaml

# For each image in the attestation, match it to the release file
for image in $REKOR_ATTESTATION_IMAGES; do
  printf $image; grep -q $image release.yaml && echo " ===> ok" || echo " ===> no match";
done
```

## Changes

# Features

* :sparkles: Implement set-security-context feature for affinity assistant containers (#8182)

Affinity Assistant containers will now have a securityContext when feature flag `set-security-context` is enabled in ConfigMap `feature-flags`.

### Fixes

* :bug: Fix isolated workspaces ignored when using StepTemplate (#8272)

Isolated workspaces are now correctly set when using in conjuction with StepTemplate

* :bug: fix(TaskRun): fixed the issue where some step statuses might not be correctly updated in failed TaskRun (#8270)

fix: fixed the issue where some step statuses might not be correctly updated in failed TaskRun

* :bug: fix(pipelinerun): resolve issue with PipelineRun not timing out successfully (#8236)

fix(pipelinerun): resolve issue with PipelineRun not timing out successfully

* :bug: fix(e2e): stabilize TestTaskRunFailure test (#8174)
* :bug: Mark steps as deleted when TaskRun fails (#8294)

### Misc




* :hammer: Bump the all group across 1 directory with 4 updates (#8300)
* :hammer: Pin setup-go action (#8291)
* :hammer: Simply the path for the base image (#8290)
* :hammer: Bump github/codeql-action from 3.26.7 to 3.26.8 (#8289)
* :hammer: Pin alpine image used in release pipeline (#8287)
* :hammer: Update to the latest version of koparse for the release pipeline (#8285)
* :hammer: Bump google.golang.org/grpc from 1.64.1 to 1.67.0 (#8281)
* :hammer: Use the new version of koparse in the build (#8278)
* :hammer: Bump step-security/harden-runner from 2.9.1 to 2.10.1 (#8269)
* :hammer: Bump tj-actions/changed-files from 45.0.1 to 45.0.2 (#8268)
* :hammer: Bump github/codeql-action from 3.26.6 to 3.26.7 (#8267)
* :hammer: Bump the all group in /tekton with 4 updates (#8266)
* :hammer: Adapt koparse step to handle no import path (#8261)
* :hammer: Add KO_EXTRA_ARGS (#8260)
* :hammer: Propagate image registry regions to publish (#8259)
* :hammer: Fix the imageRegistryUser param usage in the release pipeline (#8256)
* :hammer: build(deps): bump the all group in /tekton with 4 updates (#8253)
* :hammer: Run build and tests conditionally (#8252)
* :hammer: Support separate bucket and image reg creds (#8251)
* :hammer: Add OCI source label to images (#8247)
* :hammer: Make image registry regions configurable (#8246)
* :hammer: build(deps): bump google.golang.org/grpc from 1.64.0 to 1.64.1 (#8245)
* :hammer: build(deps): bump github.com/Azure/azure-sdk-for-go/sdk/azidentity from 1.5.2 to 1.6.0 (#8244)
* :hammer: build(deps): bump github.com/hashicorp/go-retryablehttp from 0.7.6 to 0.7.7 (#8243)
* :hammer: build(deps): bump the all group across 1 directory with 4 updates (#8235)
* :hammer: build(deps): bump tj-actions/changed-files from 45.0.0 to 45.0.1 (#8233)
* :hammer: build(deps): bump github/codeql-action from 3.26.3 to 3.26.6 (#8232)

### Docs


* :book: Update releases for new Tekton Pipeline Releases 0.63 (#8229)

## Thanks

Thanks to these contributors who contributed to v0.64.0!
* :heart: @AlanGreene
* :heart: @afrittoli
* :heart: @chitrangpatel
* :heart: @dependabot[bot]
* :heart: @kristofferchr
* :heart: @l-qing
* :heart: @vdemeester

Extra shout-out for awesome release notes:
* :heart_eyes: @kristofferchr
* :heart_eyes: @l-qing
* :heart_eyes: @vdemeester