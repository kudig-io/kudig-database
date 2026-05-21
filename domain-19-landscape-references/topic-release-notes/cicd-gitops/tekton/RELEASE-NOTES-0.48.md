---
title: tekton v0.48 Release Notes
description: tekton v0.48 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
- crd
- webhook
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- tekton v0.48 Release Notes 是什么
- 如何 tekton v0.48 Release Notes
trigger_keywords:
- tekton
- v0.48
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

# tekton v0.48 Release Notes

Source: [v0.48.0](https://github.com/tektoncd/pipeline/releases/tag/v0.48.0)

## 🎉 Provenance Beta, Resilient Affinity Assistant and Array Params in Matrix 🎉

-[Docs @ v0.48.0](https://github.com/tektoncd/pipeline/tree/v0.48.0/docs)
-[Examples @ v0.48.0](https://github.com/tektoncd/pipeline/tree/v0.48.0/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.48.0/release.yaml
```

## Attestation

The Rekor UUID for this release is `24296fb24b8ad77aadae6008428d822bb60159ae252ba66b61d276e7836b724a5cd7c7402aeb0527`

Obtain the attestation:
```shell
REKOR_UUID=24296fb24b8ad77aadae6008428d822bb60159ae252ba66b61d276e7836b724a5cd7c7402aeb0527
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.48.0/release.yaml
REKOR_UUID=24296fb24b8ad77aadae6008428d822bb60159ae252ba66b61d276e7836b724a5cd7c7402aeb0527

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.48.0@sha256:" + .digest.sha256')

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

* :sparkles: [TEP-0091] use VerificationResult in verify (#6673)

VerificationResult is the return value for instead of error for VerifyTask and VerifyPipeline.

* :sparkles: feat: support to produce results from a failed task (#6510)

PipelineRun can produce task results from the failed tasks, and the final task can reference those results.

* :sparkles: Promote the provenance field in status (#6495)

Promote `provenance` field to beta by setting the existing feature flag `enable-provenance-in-status` to be true by default with the installation of Tekton Pipeline. This feature flag will be completely removed once we consider this as a stable feature. That said, users can choose to opt out this by setting this feature flag to false. 
* :sparkles: [TEP-0091] add VerificationResult (#6663)
* :sparkles: [TEP-0089] Inject SpireControllerAPIClient into the Taskrun controller and reconciler. (#6627)
* :sparkles: [TEP-0089] SPIRE for non-falsifiable provenance. Setup the test environment. (#6553)
* :sparkles: [TEP-0089] Add CSI volumes to the Pods which provide the SPIRE workload API (#6539)
* :sparkles: Add matrix support for using references to entire PipelineRun array parameters (#6516)

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

* :bug: Bug Fixes: Update Status for Matrixed PipelineTask (#6661)

Bug Fix: A matrixed pipelineTask will accurately reflect the status of isStarted(), isScheduled(), IsBeforeFirstTaskRun(), IsConditionStatusFalse() with the correct start time based on it's TaskRuns or custom RunObjects.

* :bug: Sync pipelinerun validation between v1beta1 and v1 (#6656)

Sync pipelinerun validation between v1beta1 and v1

* :bug: Split array param indexing validation between reconciler and webhook (#6652)

bug fix: always perform validation of array parameter index bounds checking

* :bug: remove beta flag check for v1beta1 object param,results and array result (#6644)

Remove beta feature flag check for v1beta1 object param, results and array result. Object param, results and array result will be enabled if the  enable-api-fields feature flag is not alpha for v1beta1 CRDs (e.g. Tasks and Pipelines)

* :bug: Add validation for array indexing in finally when expressions (#6638)

Bug fix: add validation for out-of-bounds indexing into array parameters referenced in pipeline.spec.finally.when.inputs

* :bug: Don't mark done PipelineRuns as timed out (#6622)

Completed PipelineRuns are not anymore changed to PipelineRunTimeout status

* :bug: Support context variable replacements in custom tasks (#6620)

A user can now define context variables in inline pipeline specs for custom tasks.

* :bug: check beta feature flag for v1 TaskSpec's ValidateParamArrayIndex (#6613)

check beta feature flag for v1 TaskSpec's ValidateParamArrayIndex instead of alpha flag, since array indexing is beta feature

* :bug: Fix conversion of non-object results declared in Tasks (#6606)

Fix conversion bug preventing tasks with non-object results and parameters successfully round-tripping between api versions

* :bug: update affinity assistant creation implementation (#6596)

Resilient Affinity Assistant - make sure the Affinity Assistant pod is always on a healthy node during the entire life cycle of the pipelineRun

* :bug: Allow references to ClusterTasks in v1 Pipeline Tasks (#6588)

Continue to allow v1beta1 ClusterTasks (deprecated) to be referenced in v1 Pipelines

* :bug: Custom task without api version return validation error (#6505)

Custom task without api version returns validation error

* :bug: don't return validation error when taskrun failed/skipped (#6395)

If taskrun fails and task results not emitted, pipelinerun fails because of taskrun fails rather than results validation error.

* :bug: Remove enable-api-fields validation for array index replacements (#6646)
* :bug: Keeps Deprecated Fields in Step and StepTemplate When Switching Versions (#6623)
* :bug: Refactor Sidecar Containers Construction If Script Exists (#6619)
* :bug: Add Unit Tests for Array Results using [] notation (#6577)


### Misc

* :hammer: Clean up Task parameter validation logic (#6650)

Some functions in pkg/substitution have been removed or renamed.
* :hammer: Run events controller as separate binary (#6529)

The cloudevents controller for `Run` has been moved to its own binary, with dedicated deployment, service, pod, service account, roles and role bindings. No functional change, no configuration change.
* :hammer: Add results-from feature flag to config-feature-flags.yaml (#6692)
* :hammer: Cleanup context-based validation of propagated params/workspaces (#6684)
* :hammer: Test refactor: separate Task validation tests for propagation (#6677)
* :hammer: Cleanup: Remove "substituted context" task validation (#6671)
* :hammer: Refactor validation of propagated parameters and workspaces (#6660)
* :hammer: Rename function that replaces variables in When Expressions (#6658)
* :hammer: Refactor substituting variables in Parameter values (#6657)
* :hammer: Split Pipeline validation tests into separate test classes (#6653)
* :hammer: Refactor ResolvedPipelineTask and remove redundant fields (#6649)
* :hammer: Refactor SequentialTasks & SequentialRuns Tests (#6648)
* :hammer: Simplify + add docstrings for PipelineRun resolution (#6643)
* :hammer: Refactor validation functions for indexing into array params (#6642)
* :hammer: Cleanp - Adding a single variable for default configmaps. (#6639)
* :hammer: Remove docstrings indicating that there is a 24h limit on timeouts (#6585)
* :hammer: Bump github.com/sigstore/sigstore from 1.6.2 to 1.6.4 (#6629)
* :hammer: move trusted resources verification after we resolve the remote resources (#6621)
* :hammer: Clean up metrics code slightly. (#6609)
* :hammer: Bump github.com/tektoncd/pipeline from 0.46.0 to 0.47.0 in /test/custom-task-ctrls/wait-task-beta (#6582)
* :hammer: Bump github.com/spiffe/spire-api-sdk from 1.6.2 to 1.6.3 (#6544)

### Docs


* :book: Docs update: CSI + projected workspaces are beta (#6700)
* :book: Fix code blocks in the Tasks page (#6676)
* :book: Update documentation to reflect stability levels and deprecations (#6568)

## Thanks

Thanks to these contributors who contributed to v0.48.0!
* :heart: @EmmaMunley
* :heart: @SaschaSchwarze0
* :heart: @XinruZhang
* :heart: @Yongxuanzhang
* :heart: @afrittoli
* :heart: @chitrangpatel
* :heart: @chuangw6
* :heart: @dependabot[bot]
* :heart: @ijschwabacher
* :heart: @jagathprakash
* :heart: @jerop
* :heart: @lbernick
* :heart: @pritidesai
* :heart: @rh-hectormartinezdev

Extra shout-out for awesome release notes:
* :heart_eyes: @EmmaMunley
* :heart_eyes: @SaschaSchwarze0
* :heart_eyes: @Yongxuanzhang
* :heart_eyes: @afrittoli
* :heart_eyes: @chitrangpatel
* :heart_eyes: @chuangw6
* :heart_eyes: @lbernick
* :heart_eyes: @pritidesai

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->