---
title: tekton v0.46 Release Notes
description: tekton v0.46 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- jaeger
- containerd
- opa
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- tekton v0.46 Release Notes 是什么
- 如何 tekton v0.46 Release Notes
trigger_keywords:
- tekton
- v0.46
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
---

# tekton v0.46 Release Notes

Source: [v0.46.0](https://github.com/tektoncd/pipeline/releases/tag/v0.46.0)

# 🎉 Object Params and Results promoted to beta, context variables in `pipelineRun.workspaces[].subPath` 🎉

-[Docs @ v0.46.0](https://github.com/tektoncd/pipeline/tree/v0.46.0/docs)
-[Examples @ v0.46.0](https://github.com/tektoncd/pipeline/tree/v0.46.0/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.46.0/release.yaml
```

## Attestation

The Rekor UUID for this release is `24296fb24b8ad77a5430edfba7dc256e01b47fa5bde58abfa1c2c6307271140d90ce0236fbb3ca3a`

Obtain the attestation:
```shell
REKOR_UUID=24296fb24b8ad77a5430edfba7dc256e01b47fa5bde58abfa1c2c6307271140d90ce0236fbb3ca3a
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.46.0/release.yaml
REKOR_UUID=24296fb24b8ad77a5430edfba7dc256e01b47fa5bde58abfa1c2c6307271140d90ce0236fbb3ca3a

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.46.0@sha256:" + .digest.sha256')

# Download the release file
curl "$RELEASE_FILE" > release.yaml

# For each image in the attestation, match it to the release file
for image in $REKOR_ATTESTATION_IMAGES; do
  printf $image; grep -q $image release.yaml && echo " ===> ok" || echo " ===> no match";
done
```


## Upgrade Notices

* Tekton Pipeline v0.46.0 requires Kubernetes version 1.24 or greater.
* Release EOL: April 17th, 2023

# Action Required

* Hygiene: enable linters for error conventions. (#6264)

  - `resolution.ErrorRequestedResourceIsNil` is deprecated; change references to `resolution.ErrNilResource`.
  - `remote.ErrorRequestInProgress` is deprecated; change references to `remote.ErrRequestInProgress`.
  - in package `cmd/entrypoint/subcommands`, `SubcommandSuccessful` type is deprecated, use type `OK`.
  - in package `resolution/common`, `ErrorRequestInProgress` is deprecated; change references to `ErrRequestInProgress`.
  - in package `resolution/common`, all errors of type `Error*` have been renamed to type `*Error`.
  - in package `resolution/framework`, `ErrorMissingTypeSelector` is deprecated; change references to `ErrMissingTypeSelector`

Trusted Resources are in alpha; the following breaking changes are included in this PR:

- in package `trustedresources`:
  - `ErrorResourceVerificationFailed` is replaced by `ErrResourceVerificationFailed`
  - `ErrorEmptyVerificationConfig` is replaced by `ErrEmptyVerificationConfig`
  - `ErrorNoMatchedPolicies` is replaced by `ErrNoMatchedPolicies`
  - `ErrorRegexMatch` is replaced by `ErrRegexMatch`
  - `ErrorSignatureMissing` is replaced by `ErrSignatureMissing`
- in package `trustedresources/verifier`:
  - `ErrorFailedLoadKeyFile` is replaced by `ErrFailedLoadKeyFile`
  - `ErrorDecodeKey` is replaced by `ErrDecodeKey`
  - `ErrorEmptyPublicKeys` is replaced by `ErrEmptyPublicKeys`
  - `ErrorEmptySecretData` is replaced by `ErrEmptySecretData`
  - `ErrorSecretNotFound` is replaced by `ErrSecretNotFound`
  - `ErrorMultipleSecretData` is replaced by `ErrMultipleSecretData`
  - `ErrorEmptyKey` is replaced by `ErrEmptyKey`
  - `ErrorK8sSpecificationInvalid` is replaced by `ErrK8sSpecificationInvalid`
  - `ErrorLoadVerifier` is replaced by `ErrLoadVerifier`
  - `ErrorAlgorithmInvalid` is replaced by `ErrAlgorithmInvalid`

* [TEP074] Remove Image pipelineResources (#6002)

Please migrate off of `image` `pipelineresources` as it is removed, please refer to the doc at https://github.com/tektoncd/pipeline/blob/main/docs/pipelineresources.md#replacing-an-image-resource
`imagedigestexporter` image will not be built nor maintained from now on

* [TEP074] Remove Storage, Git and Generic PipelineResources (#6150)

Please migrate off of `git` and `storage` `pipelineresources` as they are removed, please refer to the doc at https://github.com/tektoncd/pipeline/blob/main/docs/pipelineresources.md#replacing-an-git-resource  https://github.com/tektoncd/pipeline/blob/main/docs/pipelineresources.md#replacing-an-storage-resource

Installing this release on a cluster with tasks and pipelines that use pipeline resources may break them. `pipelineresources` are removed in this release.

## Changes

# Features

* :sparkles: TEP-0118: Apply Param and Result Replacements in Matrix (#6345)

Matrix.params can have array replacements from array params or string replacements from string, array and object params.
Matrix.Include params can only have string replacements from string, array and object params

* :sparkles: [TEP-0133]: Configure Default Resolver (#6317)

[TEP-0133] Add "default-resolver-type" field in the "default-configs" ConfigMap to configure default resolver

* :sparkles: TEP-0075(object params and results) promoted to beta (#6211)

TEP-0075 promoted to beta - object params and results is now possible with enable-api-fields set to beta.

* :sparkles: TEP-0118: Added Matrix.Include field in preview mode (#6188)

API Change: Added Matrix.Include in preview mode (Not yet functional)

* :sparkles: context variables in  workspaces (#6165)

Support for using pipelineRun context variables in pipelineRun.workspaces[].subPath.

* :sparkles: TEP-0118: Apply PipelineTask Context Replacements in Matrix Include (#6358)

* :sparkles: TEP-0118: Validate Matrix Include Parameters are unique in Matrix and Pipeline Task Parameters (#6349)

* :sparkles: TEP-0118: Update PipelineTaskResultRefs for Matrix Include Parameters (#6348)

* :sparkles: TEP-0118: Update Pipeline Conversion for Matrix Include Parameters (#6346)

* :sparkles: TEP-0118: Add validation for matrix pipeline context parameter variables (#6238)

* :sparkles: TEP-0118: Add validation for matrix combination count with matrix.include params (#6237)

* :sparkles: TEP-0118: Add validation for matrix include pipeline parameter variables  (#6235)

* :sparkles: TEP-0118: Add validation for Matrix.Include.Params of type string (#6230)

* :sparkles: TEP-0118: Add exported functions for validating Matrix.Include and Matrix.Params (#6229)

<!-- Fill in deprecation notices when applicable
# Deprecation Notices

* :rotating_light: [Deprecation Notice Title]

[Detailed deprecation notice description] (#Number).

[Fill list here]
-->

# Backwards incompatible changes

In current release:

* :rotating_light: remove config-trusted-resources (#6305)

BREAKING CHANGE: [alpha] config-trusted-resources is removed, please refer to https://github.com/tektoncd/pipeline/blob/main/docs/trusted-resources.md for migrating public keys in VerificationPolicy

### Fixes

* :bug: validate matrix for duplicate params (#6308)

Validate matrix.params and matrix.include.params such that specifying duplicate parameters is caught by the validation.

* :bug: Add conversion for remote tasks and pipelines to support v1 (#6254)

V1 tasks and pipelines are supported in remote resolution

* :bug: Revert removal of embedded TaskRun status in API. (#6206)

PipelineRun Embedded TaskRun statuses reintroduced as deprecated to allow compatibility with older server versions.

* :bug: Skip  annotation when merging them. (#6161)

Skip `kubectl.kubernetes.io/last-applied-configuration` annotation when merging them in `PipelineRun` and `TaskRun`.

* :bug: don't validate skipped task results for pipeline results (#6157)

A fix for regression issue that previously we don't fail the run when skipped task results don't emit, but failed when validation is introduced.

* :bug: fix param array indexing validation error reason and error log (#6179)
* :bug: If pod was evicted, prefer pod status message. (#6153)
* :bug: Add exact comparison for events (#6296)

### Misc

* :hammer: Hygiene: enable linters for error conventions. (#6264)


action required: 

- `resolution.ErrorRequestedResourceIsNil` is deprecated; change references to `resolution.ErrNilResource`.
- `remote.ErrorRequestInProgress` is deprecated; change references to `remote.ErrRequestInProgress`.
- in package `cmd/entrypoint/subcommands`, `SubcommandSuccessful` type is deprecated, use type `OK`.
- in package `resolution/common`, `ErrorRequestInProgress` is deprecated; change references to `ErrRequestInProgress`.
- in package `resolution/common`, all errors of type `Error*` have been renamed to type `*Error`.
- in package `resolution/framework`, `ErrorMissingTypeSelector` is deprecated; change references to `ErrMissingTypeSelector`

Trusted Resources are in alpha; the following breaking changes are included in this PR:

- in package `trustedresources`:
  - `ErrorResourceVerificationFailed` is replaced by `ErrResourceVerificationFailed`
  - `ErrorEmptyVerificationConfig` is replaced by `ErrEmptyVerificationConfig`
  - `ErrorNoMatchedPolicies` is replaced by `ErrNoMatchedPolicies`
  - `ErrorRegexMatch` is replaced by `ErrRegexMatch`
  - `ErrorSignatureMissing` is replaced by `ErrSignatureMissing`
- in package `trustedresources/verifier`:
  - `ErrorFailedLoadKeyFile` is replaced by `ErrFailedLoadKeyFile`
  - `ErrorDecodeKey` is replaced by `ErrDecodeKey`
  - `ErrorEmptyPublicKeys` is replaced by `ErrEmptyPublicKeys`
  - `ErrorEmptySecretData` is replaced by `ErrEmptySecretData`
  - `ErrorSecretNotFound` is replaced by `ErrSecretNotFound`
  - `ErrorMultipleSecretData` is replaced by `ErrMultipleSecretData`
  - `ErrorEmptyKey` is replaced by `ErrEmptyKey`
  - `ErrorK8sSpecificationInvalid` is replaced by `ErrK8sSpecificationInvalid`
  - `ErrorLoadVerifier` is replaced by `ErrLoadVerifier`
  - `ErrorAlgorithmInvalid` is replaced by `ErrAlgorithmInvalid`

* :hammer: remove config-trusted-resources (#6305)

BREAKING CHANGE: [alpha] config-trusted-resources is removed, please refer to https://github.com/tektoncd/pipeline/blob/main/docs/trusted-resources.md for migrating public keys in VerificationPolicy

* :hammer: change param array indexing validation functions to member functions (#6180)

Make ValidateParamArrayIndex as member functions for TaskSpec and PipelineSpec
Change []ParamSpec to ParamSpecs and []Param to Params 

* :hammer: [TEP074] Remove Storage, Git and Generic PipelineResources (#6150)

action required: please migrate off of `git` and `storage` `pipelineresources` as they are removed, please refer to the doc at https://github.com/tektoncd/pipeline/blob/main/docs/pipelineresources.md#replacing-an-git-resource  https://github.com/tektoncd/pipeline/blob/main/docs/pipelineresources.md#replacing-an-storage-resource

installing this release on a cluster with tasks and pipelines that use pipeline resources may break them

pipelineresources are removed in this release

* :hammer: Upgrade to knative/pkg 1.9 (#6062)

Bump knative/pkg to 1.9 and update the Kubernetes minimun version to be v1.24.x

* :hammer: [TEP074] Remove Image pipelineResources (#6002)

action required: please migrate off of `image` `pipelineresources` as it is removed, please refer to the doc at https://github.com/tektoncd/pipeline/blob/main/docs/pipelineresources.md#replacing-an-image-resource
`imagedigestexporter` image will not be built nor maintained from now on

* :hammer: drop git-init from the list of published images (#6378)
* :hammer: Refactor extracting parameters from a PipelineTask (#6344)
* :hammer: [TEP-0133] Refactor set default test helper (#6339)
* :hammer: Hygiene: removed unused `images` package variable. (#6337)
* :hammer: simplifying matrix combinations (#6312)
* :hammer: Restore timeouts default for v1 (#6311)
* :hammer: Stop populating resourceName in git-init image (#6310)
* :hammer: cleanup: deleting duplicate unit test (#6307)
* :hammer: Reorganizing Matrix Implementation (#6282)
* :hammer: Hygiene: enable `goconst`, `dogsled` linters. (#6262)
* :hammer: Hygiene: enable `containedctx` linter. (#6261)
* :hammer: Hygiene: enable `ineffassign` linter. 🧹🧹🧹 (#6259)
* :hammer: Hygiene: cleanup `golangci` configuration. (#6258)
* :hammer: Hygiene: enable additional lint presets. (#6247)
* :hammer: TEP-0090 Matrix: Refactor Matrix FanOut() to use the Matrix Struct (#6246)
* :hammer: Hygiene: enable additional linter presets. (#6236)
* :hammer: Add CustomRun to register.go (#6199)
* :hammer: typochanges in line 81 and 94. I have changed it to "ResourceRef" from "ResourseRef". (#6184)
* :hammer: updated spire unit test based on grpc update (#6168)
* :hammer: Bump github.com/spiffe/go-spiffe/v2 from 2.1.2 to 2.1.3 (#6370)
* :hammer: Bump github.com/google/go-containerregistry from 0.13.0 to 0.14.0 (#6369)
* :hammer: Bump github.com/go-git/go-git/v5 from 5.6.0 to 5.6.1 (#6368)
* :hammer: fix error msg in taskref.resolver.params validation (#6361)
* :hammer: fix error message for resolver params (#6360)
* :hammer: remove pipelineresources from delete_pipeline_resources (#6357)
* :hammer: Bump k8s.io/code-generator from 0.25.4 to 0.25.7 (#6338)
* :hammer: Check generated API docs during CI (#6336)
* :hammer: Bump k8s.io/apimachinery from 0.25.4 to 0.25.7 (#6334)
* :hammer: Bump k8s.io/api from 0.25.4 to 0.25.7 (#6333)
* :hammer: Bump k8s.io/client-go from 0.25.4 to 0.25.7 (#6332)
* :hammer: Fix the dependabot config (#6330)
* :hammer: refactor trusted resources get matching policies code (#6323)
* :hammer: Bump github.com/spiffe/spire-api-sdk from 1.5.5 to 1.6.1 (#6321)
* :hammer: Bump golang.org/x/crypto from 0.6.0 to 0.7.0 (#6320)
* :hammer: Bump google.golang.org/protobuf from 1.28.1 to 1.29.0 (#6319)
* :hammer: Bump github.com/containerd/containerd from 1.6.18 to 1.6.19 (#6318)
* :hammer: dependabot: do not update k8s.io/* major/minor updates (#6314)
* :hammer: Bump github.com/jenkins-x/go-scm from 1.13.4 to 1.13.9 (#6303)
* :hammer: Sync SpanContext field for v1 with v1beta1 API (#6300)
* :hammer: Bump github.com/go-git/go-git/v5 from 5.5.2 to 5.6.0 (#6289)
* :hammer: updating release instructions to include LTS suffix (#6283)
* :hammer: add missing test cases for matrix params array indexing validation (#6281)
* :hammer: Bump k8s.io/api from 0.26.1 to 0.26.2 in /test/custom-task-ctrls/wait-task-beta (#6272)
* :hammer: Bump k8s.io/apimachinery from 0.26.1 to 0.26.2 in /test/custom-task-ctrls/wait-task-alpha (#6270)
* :hammer: Bump k8s.io/api from 0.26.1 to 0.26.2 in /test/custom-task-ctrls/wait-task-alpha (#6269)
* :hammer: Bump golang.org/x/time from 0.2.0 to 0.3.0 (#6268)
* :hammer: Bump github.com/hashicorp/go-retryablehttp from 0.7.1 to 0.7.2 (#6265)
* :hammer: Remove Docs for Image Resources (#6263)
* :hammer: apply latest knative.dev/pkg 1.9 (#6256)
* :hammer: Bump github.com/sigstore/sigstore from 1.5.2 to 1.6.0 (#6253)
* :hammer: Bump go.opentelemetry.io/otel/exporters/jaeger from 1.13.0 to 1.14.0 (#6249)
* :hammer: [TEP074] Remove git-ssh Examples (#6239)
* :hammer: Bump github.com/sigstore/sigstore from 1.5.1 to 1.5.2 (#6231)
* :hammer: update go.mod to Go 1.19 (#6216)
* :hammer: Fix indent for the example of specifying Custom Task Spec (#6215)
* :hammer: [TEP074] Remove PipelineResourceResultType of PipelineResourceResult Struct (#6198)
* :hammer: Bump github.com/golangci/golangci-lint from 1.51.1 to 1.51.2 in /tools (#6196)
* :hammer: Bump github.com/jenkins-x/go-scm from 1.13.2 to 1.13.4 (#6195)
* :hammer: Bump github.com/tektoncd/pipeline from 0.44.0 to 0.45.0 in /test/custom-task-ctrls/wait-task-beta (#6191)
* :hammer: Bump github.com/tektoncd/pipeline from 0.44.0 to 0.45.0 in /test/custom-task-ctrls/wait-task-alpha (#6190)
* :hammer: run codegen against latest main to update licenses and api doc (#6189)
* :hammer: Add v0.45.0 to releases.md (#6187)
* :hammer: Make SA implicit in TestPropagatedParams (#6185)
* :hammer: Bump github.com/containerd/containerd from 1.6.17 to 1.6.18 (#6183)
* :hammer: Bump github.com/spiffe/spire-api-sdk from 1.5.4 to 1.5.5 (#6173)
* :hammer: update pipeline-object-results example (#6166)

### Docs

* :book: Update EOL releases (#6285)

Move v0.39 and v0.40 releases to the end-of-life section.
* :book: Update Removal of PipelineResources in Deprecation.md (#6343)
* :book: Fix resolution getting started docs with correct parameters (#6234)
* :book: Update Tekton development processes section URLs (#6225)
* :book: Create Removed Table for deprecation.md (#6209)
* :book: Add deprecation notes for developers. (#6208)
* :book: Fix godoc deprecated comments. (#6207)
* :book: Updated documentation of pipelines around approvals (#6205)
* :book: fix weight in update-reference-docs (#6200)
* :book: Fix broken links and delete troubleshooting section (#6172)
* :book: Clarify docs on pipelinerun timeouts (#6171)

## Thanks

Thanks to these contributors who contributed to v0.46.0!
* :heart: @Aathirajan
* :heart: @EmmaMunley
* :heart: @JeromeJu
* :heart: @QuanZhang-William
* :heart: @SaschaSchwarze0
* :heart: @XinruZhang
* :heart: @Yongxuanzhang
* :heart: @abayer
* :heart: @afrittoli
* :heart: @bendory
* :heart: @concaf
* :heart: @dependabot[bot]
* :heart: @drewbailey
* :heart: @ernesgonzalez33
* :heart: @geriom
* :heart: @jerop
* :heart: @khrm
* :heart: @l-qing
* :heart: @lbernick
* :heart: @mswiderski
* :heart: @pavanstarmanwar
* :heart: @pritidesai
* :heart: @pxp928
* :heart: @vdemeester
* :heart: @wlynch

Extra shout-out for awesome release notes:
* :heart_eyes: @EmmaMunley
* :heart_eyes: @JeromeJu
* :heart_eyes: @QuanZhang-William
* :heart_eyes: @Yongxuanzhang
* :heart_eyes: @afrittoli
* :heart_eyes: @bendory
* :heart_eyes: @pritidesai
* :heart_eyes: @vdemeester
* :heart_eyes: @wlynch

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->