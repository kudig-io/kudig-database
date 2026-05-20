---
title: tekton v1.1 Release Notes
description: tekton v1.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- apiserver
- opa
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v1.1 Release Notes 是什么
- 如何 tekton v1.1 Release Notes
trigger_keywords:
- tekton
- v1.1
- Release
- Notes
- release
- notes
---


# tekton v1.1 Release Notes

Source: [v1.1.0](https://github.com/tektoncd/pipeline/releases/tag/v1.1.0)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v1.1.0](https://github.com/tektoncd/pipeline/tree/v1.1.0/docs)
-[Examples @ v1.1.0](https://github.com/tektoncd/pipeline/tree/v1.1.0/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v1.1.0/release.yaml
```

## Attestation

The Rekor UUID for this release is `108e9186e8c5677a4abf3bb44246e552fdd917a58075df15b5f99ad1aa9e1da6ffd3c6aebc69689d`

Obtain the attestation:
```shell
REKOR_UUID=108e9186e8c5677a4abf3bb44246e552fdd917a58075df15b5f99ad1aa9e1da6ffd3c6aebc69689d
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v1.1.0/release.yaml
REKOR_UUID=108e9186e8c5677a4abf3bb44246e552fdd917a58075df15b5f99ad1aa9e1da6ffd3c6aebc69689d

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v1.1.0@sha256:" + .digest.sha256')

# Download the release file
curl "$RELEASE_FILE" > release.yaml

# For each image in the attestation, match it to the release file
for image in $REKOR_ATTESTATION_IMAGES; do
  printf $image; grep -q $image release.yaml && echo " ===> ok" || echo " ===> no match";
done
```

## Changes

# Features

### Fixes

* :bug: fix: Ensure retryable errors during validation do not fail Runs (#8746)

Retryable errors during dry-run Task validation will no longer cause a PipelineRun to be failed.

* :bug: Add oomkilled reason (#8709)

TaskRuns that fail due to Out of Memory (OOM) conditions will now show the termination reason in their failure message.


### Misc

* :hammer: refactor: use os.UserHomeDir instead of go-homedir (#8774)
* :hammer: Remove temporary `GOPATH` generation in in `update-codegen.sh` and `update-openapigen.sh`. (#8719)
* :hammer: Refactor Step validation to implement apis.Validatable. (#8717)
* :hammer: Raise test coverage in `task_validation.go` and `container_validation.go`. (#8714)
* :hammer: Refactor sidecar validation to implement apis.Validatable. (#8710)
* :hammer: Move Steps and Sidecars validation to `container_validation.go`. (#8685)
* :hammer: build(deps): bump google.golang.org/grpc from 1.72.1 to 1.72.2 (#8801)
* :hammer: build(deps): bump the all group in /tekton with 4 updates (#8800)
* :hammer: build(deps): bump tj-actions/changed-files from 480f49412651059a414a6a5c96887abb1877de8a to c6634ca281a9fc05b03bee224ba00910cb78ab6e (#8799)
* :hammer: build(deps): bump go.opentelemetry.io/otel/trace from 1.35.0 to 1.36.0 (#8798)
* :hammer: build(deps): bump github.com/google/go-containerregistry from 0.20.3 to 0.20.5 (#8796)
* :hammer: build(deps): bump go.opentelemetry.io/otel/sdk from 1.35.0 to 1.36.0 (#8794)
* :hammer: build(deps): bump go.opentelemetry.io/otel/exporters/otlp/otlptrace/otlptracehttp from 1.35.0 to 1.36.0 (#8793)
* :hammer: Fix subpath capitalisation (#8790)
* :hammer: build(deps): bump k8s.io/code-generator from 0.32.4 to 0.32.5 (#8789)
* :hammer: build(deps): bump the all group in /tekton with 4 updates (#8787)
* :hammer: build(deps): bump actions/dependency-review-action from 4.7.0 to 4.7.1 (#8786)
* :hammer: build(deps): bump github/codeql-action from 3.28.17 to 3.28.18 (#8785)
* :hammer: build(deps): bump k8s.io/client-go from 0.32.4 to 0.32.5 (#8783)
* :hammer: build(deps): bump k8s.io/apiextensions-apiserver from 0.32.4 to 0.32.5 (#8781)
* :hammer: build(deps): bump k8s.io/api from 0.32.4 to 0.32.5 (#8780)
* :hammer: build(deps): bump google.golang.org/grpc from 1.72.0 to 1.72.1 (#8771)
* :hammer: build(deps): bump actions/setup-go from 5.4.0 to 5.5.0 (#8766)
* :hammer: build(deps): bump actions/dependency-review-action from 4.6.0 to 4.7.0 (#8765)
* :hammer: build(deps): bump tj-actions/changed-files from 4168bb487d5b82227665ab4ec90b67ce02691741 to 480f49412651059a414a6a5c96887abb1877de8a (#8764)
* :hammer: build(deps): bump the all group in /tekton with 4 updates (#8763)
* :hammer: build(deps): bump k8s.io/apiextensions-apiserver from 0.32.1 to 0.32.4 (#8762)
* :hammer: build(deps): bump github.com/jenkins-x/go-scm from 1.14.56 to 1.14.58 (#8754)
* :hammer: build(deps): bump go.opentelemetry.io/otel/exporters/otlp/otlptrace/otlptracehttp from 1.34.0 to 1.35.0 (#8753)
* :hammer: build(deps): bump github.com/sigstore/sigstore/pkg/signature/kms/hashivault from 1.8.15 to 1.9.4 (#8752)
* :hammer: build(deps): bump github.com/google/cel-go from 0.24.1 to 0.25.0 (#8751)
* :hammer: build(deps): bump google.golang.org/grpc from 1.71.1 to 1.72.0 (#8749)
* :hammer: build(deps): bump github.com/sigstore/sigstore/pkg/signature/kms/aws from 1.8.15 to 1.9.4 (#8748)
* :hammer: build(deps): bump golang.org/x/sync from 0.13.0 to 0.14.0 (#8747)
* :hammer: Migration to golangci-lint v2… (#8745)
* :hammer: Add @waveywaves as a maintainer (#8743)
* :hammer: build(deps): bump the all group in /tekton with 4 updates (#8742)
* :hammer: build(deps): bump github/codeql-action from 3.28.16 to 3.28.17 (#8741)
* :hammer: build(deps): bump tj-actions/changed-files from 5426ecc3f5c2b10effaefbd374f0abdc6a571b2f to 4168bb487d5b82227665ab4ec90b67ce02691741 (#8739)
* :hammer: build(deps): bump github.com/cloudevents/sdk-go/v2 from 2.15.2 to 2.16.0 (#8737)
* :hammer: build(deps): bump github.com/sigstore/sigstore/pkg/signature/kms/gcp from 1.8.15 to 1.9.4 (#8736)
* :hammer: build(deps): bump k8s.io/code-generator from 0.32.2 to 0.32.4 (#8735)
* :hammer: build(deps): bump go.opentelemetry.io/otel/sdk from 1.34.0 to 1.35.0 (#8734)
* :hammer: build(deps): bump code.gitea.io/sdk/gitea from 0.20.0 to 0.21.0 (#8733)
* :hammer: build(deps): bump k8s.io/client-go from 0.32.2 to 0.32.4 (#8732)
* :hammer: build(deps): bump github.com/spiffe/spire-api-sdk from 1.11.2 to 1.12.0 (#8731)
* :hammer: build(deps): bump tj-actions/changed-files from c34c1c13a740b06851baff92ab9a653d93ad6ce7 to 5426ecc3f5c2b10effaefbd374f0abdc6a571b2f (#8730)
* :hammer: build(deps): bump github.com/sigstore/sigstore/pkg/signature/kms/azure from 1.8.15 to 1.9.4 (#8725)
* :hammer: build(deps): bump golang.org/x/net from 0.33.0 to 0.36.0 in /test/resolver-with-timeout (#8708)
* :hammer: build(deps): bump golang.org/x/crypto from 0.31.0 to 0.35.0 in /test/resolver-with-timeout (#8706)
* :hammer: build(deps): bump github.com/google/go-cmp from 0.6.0 to 0.7.0 in /test/custom-task-ctrls/wait-task-beta (#8588)

### Docs

* :book: docs: add more explicit language in the Pipelines in Pipelines docs (#8767)
* :book: Update releases.md after v1.0.0 (#8761)
* :book: fix(docs): correct documentation link errors related to sidecar-logs (#8744)
* :book: Add ghcr.io migration banner to README.md. (#8693)

## Thanks

Thanks to these contributors who contributed to v1.1.0!
* :heart: @AlanGreene
* :heart: @aThorp96
* :heart: @afrittoli
* :heart: @dependabot[bot]
* :heart: @infernus01
* :heart: @l-qing
* :heart: @twoGiants
* :heart: @vdemeester

Extra shout-out for awesome release notes:
* :heart_eyes: @aThorp96
* :heart_eyes: @infernus01
