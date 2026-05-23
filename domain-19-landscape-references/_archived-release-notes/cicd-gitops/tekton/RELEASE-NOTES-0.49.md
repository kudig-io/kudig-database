---
title: tekton v0.49 Release Notes
description: tekton v0.49 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- jaeger
- opa
- statefulset
- webhook
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- tekton v0.49 Release Notes 是什么
- 如何 tekton v0.49 Release Notes
trigger_keywords:
- tekton
- v0.49
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

# tekton v0.49 Release Notes

Source: [v0.49.0](https://github.com/tektoncd/pipeline/releases/tag/v0.49.0)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.49.0](https://github.com/tektoncd/pipeline/tree/v0.49.0/docs)
-[Examples @ v0.49.0](https://github.com/tektoncd/pipeline/tree/v0.49.0/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.49.0/release.yaml
```

## Attestation

The Rekor UUID for this release is `24296fb24b8ad77a7568df3bfec7071c4ec0e2ce4f105b7e8f5749bdad0b5c1774ae7000ce62ac8f`

Obtain the attestation:
```shell
REKOR_UUID=24296fb24b8ad77a7568df3bfec7071c4ec0e2ce4f105b7e8f5749bdad0b5c1774ae7000ce62ac8f
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.49.0/release.yaml
REKOR_UUID=24296fb24b8ad77a7568df3bfec7071c4ec0e2ce4f105b7e8f5749bdad0b5c1774ae7000ce62ac8f

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.49.0@sha256:" + .digest.sha256')

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

* :sparkles: [TEP-0091] support remote v1 pipeline verification (#6765)

Trusted Resources supports v1 remote tasks verification

* :sparkles: [TEP-0091] support remote v1 task verification (#6764)

Trusted Resources supports v1 remote tasks verification

* :sparkles: [TEP-0091] update taskrun and pipelinerun condition based on VerificationResult (#6757)

TrustedResourcesVerified is added to TaskRun/PipelineRun status if trusted resources is enabled, the condition indicates the result of the verification.

* :sparkles: add taskrun gauge metrics for k8s throttling because of defined resource quotas or k8s node constraints (#6744)

A new gauge metric for both PipelineRun and TaskRun will indicate whether underlying [[Pods|Pods]] are being throttled by [[Kubernetes|Kubernetes]] because of either ResourceQuota policies defined in the namespace, or because the underlying node is experiencing resource constraints.

* :sparkles: Add more secure SecurityContext to injected pod containers (#6515)

Set new feature flag "set-security-context" to "true" to allow TaskRuns and PipelineRuns to be run in namespaces with restricted pod security admission

# Backwards incompatible changes

In current release:

* :rotating_light:  Enable beta features by default (#6732)

action required: "enable-api-fields" is set to "beta" by default. If you are using v1 APIs and would like to use only stable features, modify the "feature-flags" configmap in the "tekton-pipelines" namespace to set "enable-api-fields" to "stable". Example command: kubectl patch cm feature-flags -n tekton-pipelines -p '{"data":{"enable-api-fields":"stable"}}'
If you are using v1beta1 APIs, no action is needed.

### Fixes

* :bug: Conversion webhook fix for tasks with nil StepTemplate (#6825)

Conversion webhook fix for tasks with nil StepTemplate

* :bug: Add validation for beta features in v1 remote Tasks/Pipelines (#6725)

Bug fix: Apply validation for beta features for v1 remote pipelines and tasks in the same way as already exists for pipelines and tasks created directly on cluster

* :bug: Validate pipelineTask params usage only when explicit declaration is required (#6710)

Adds validation that parameters used in inline task specs within pipelines are declared by the pipeline.

* :bug: fix: taskrun still fails even with onerror set to continue (#6675)

bug fix: taskrun still fails even with onerror set to continue

* :bug: Ignore tekton reserved annotations (#6441)

Binary file (standard input) matches

* :bug: Fix v1beta1 pipelineref bundle conversion to resolver (#6791)
* :bug: Fix the key for Span of Tracing in pipelinerun reconciler (#6784)
* :bug: Remove reconciler check for enable-tekton-oci-bundles flag (#6777)
* :bug: Add Unit Tests for TestMissingResultWhenStepErrorIsIgnored and Update e2e test: TestFailingStepOnContinue (#6771)
* :bug: Issue#6697 Fix tab formatting for documentation (#6750)

### Misc

* :hammer: merge VerifyTask and VerifyPipeline into VerifyResource (#6724)

action required: VerifyTask and VerifyPipeline are now merged into 1 function VerifyResource, please update the usages if upgrade to the new release

* :hammer: Change the Storage Version to V1 Types (#6444)

action required:  for custom resolver users, please update to use v1.Param and v1.RefSource

* :hammer: TEP-0135: Refactor Affinity Assistant PVC creation (#6741)

TEP-0135: Update the owner of `PVCs` created by `pipelinerun VolumeClaimTemplate` to the affinity assistant `StatefulSet` when affinity assistant is enabled. The `PVCs` bounded to the `pipelinerun` is now in `terminating` state when the `pipelinerun` is completed but not deleted (when affinity assistant is enabled).

* :hammer: Clean up non-functional CloudEvents Metrics in Reconciler for Deprecated CloudEvents (#6827)
* :hammer: Refactor test cases for remote PipelineRef (#6805)
* :hammer:  Remove logic setting resolvers feature flag in e2e tests (#6786)
* :hammer: Fix apiVersion of Task to v1 in v1 examples (#6785)
* :hammer: Refactor TestReconcile_RemotePipelineRef bundle resolver test case (#6781)
* :hammer: Refactor test cases for remote TaskRef (#6778)
* :hammer: fix alpha propagated object params docs (#6753)
* :hammer: move tep75 tep76 and tep 107 examples from alpha to beta (#6747)
* :hammer: Cleanup outdated usage for functions in upgrade test (#6723)
* :hammer: Consolidate validation for Task/Pipeline beta features (#6719)
* :hammer: Cleanup: Use CustomRun instead of RunObject (#6718)
* :hammer: Add tests for ResolvedPipelineTask IsCancelled and IsCancelledForTimeout (#6703)
* :hammer: Cleanup: Move array indexing validation out of apis package (#6617)
* :hammer: Remove refs, HEAD symlinks in resolvers kodata (#6838)
* :hammer: Bump k8s.io/apimachinery from 0.26.5 to 0.26.6 in /test/custom-task-ctrls/wait-task-beta (#6836)
* :hammer: Bump github.com/golangci/golangci-lint from 1.53.2 to 1.53.3 in /tools (#6833)
* :hammer: TEP-0135: add affinity assistant cleanup unit tests (#6818)
* :hammer: Add apiVersions to TrustedResources Verification Helper Functions (#6803)
* :hammer: Add pod name to build_logs test output (#6796)
* :hammer: Bump github.com/sigstore/sigstore from 1.6.4 to 1.6.5 (#6789)
* :hammer: Bump github.com/golangci/golangci-lint from 1.52.2 to 1.53.2 in /tools (#6776)
* :hammer: RFC: Update Go compatibility policy (#6768)
* :hammer: Sync V1 apis with V1beta1 changes (#6766)
* :hammer: [TEP-0091] add more no error test cases for taskrun and pipelinerun (#6754)
* :hammer: Bump golang.org/x/sync from 0.1.0 to 0.2.0 (#6745)
* :hammer: Bump github.com/spiffe/go-spiffe/v2 from 2.1.4 to 2.1.5 (#6737)
* :hammer: Bump github.com/tektoncd/pipeline from 0.47.0 to 0.48.0 in /test/custom-task-ctrls/wait-task-beta (#6734)
* :hammer: Bump k8s.io/apimachinery from 0.26.4 to 0.26.5 (#6733)
* :hammer: Bump google.golang.org/grpc from 1.54.0 to 1.55.0 (#6721)
* :hammer: Bump github.com/google/go-containerregistry from 0.14.0 to 0.15.2 (#6720)
* :hammer: add missing unit test case for warn mode verification policy. (#6717)
* :hammer: Refactor PipelineRun and Run yamls in conversion_test to avoid flake (#6714)
* :hammer: Bump go.opentelemetry.io/otel from 1.14.0 to 1.16.0 (#6708)
* :hammer: Bump go.opentelemetry.io/otel/exporters/jaeger from 1.14.0 to 1.16.0 (#6706)
* :hammer: Bump go.opentelemetry.io/otel/sdk from 1.14.0 to 1.16.0 (#6705)
* :hammer: Validate beta features only when v1 Tasks and Pipelines are defined (#6701)
* :hammer: Bump k8s.io/api from 0.26.4 to 0.26.5 in /test/custom-task-ctrls/wait-task-beta (#6687)
* :hammer: Bump k8s.io/client-go from 0.25.9 to 0.25.10 in /test/custom-task-ctrls/wait-task-beta (#6686)
* :hammer: Bump k8s.io/apimachinery from 0.26.4 to 0.26.5 in /test/custom-task-ctrls/wait-task-beta (#6685)
* :hammer: Update pipelineTasks in Release-Pipeline to use Git Resolver (#6565)

### Docs


* :book: Remove cloudevent metrics from documentation (#6843)
* :book: Update broken links in api_compatibility_policy.md (#6840)
* :book: Updating release doc with recent releases (#6821)
* :book: Add instructions for cherry-picking commits for patch releases (#6788)
* :book: Added clarification and fix the Metrics doc (#6779)
* :book: remove tep75 in alpha feature list table (#6749)
* :book: chore: Add PROBES_PORT environment variable and update targetPorts for existing services (#6739)
* :book: clarify in docs to not use apiVersion for taskRef for non-customtask (#6704)

## Thanks

Thanks to these contributors who contributed to v0.49.0!
* :heart: @EmmaMunley
* :heart: @JeromeJu
* :heart: @QuanZhang-William
* :heart: @Yongxuanzhang
* :heart: @chitrangpatel
* :heart: @concaf
* :heart: @dependabot[bot]
* :heart: @gabemontero
* :heart: @jsminem
* :heart: @kahirokunn
* :heart: @khrm
* :heart: @l-qing
* :heart: @lbernick
* :heart: @vdemeester

Extra shout-out for awesome release notes:
* :heart_eyes: @JeromeJu
* :heart_eyes: @QuanZhang-William
* :heart_eyes: @Yongxuanzhang
* :heart_eyes: @chitrangpatel
* :heart_eyes: @gabemontero
* :heart_eyes: @l-qing
* :heart_eyes: @lbernick
* :heart_eyes: @vdemeester

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->