---
title: tekton v1.11 Release Notes
description: tekton v1.11 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- apiserver
- opa
- operator
- webhook
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- tekton v1.11 Release Notes 是什么
- 如何 tekton v1.11 Release Notes
trigger_keywords:
- tekton
- v1.11
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
- observability-basics
created: "2026-05-23"
---

# tekton v1.11 Release Notes

Source: [v1.11.0](https://github.com/tektoncd/pipeline/releases/tag/v1.11.0)

# 🎉 🐱 TaskRun pending parity, multiple Git credentials, and PVC auto-cleanup 🤖  🎉

-[Docs @ v1.11.0](https://github.com/tektoncd/pipeline/tree/v1.11.0/docs)
-[Examples @ v1.11.0](https://github.com/tektoncd/pipeline/tree/v1.11.0/examples)

## Installation one-liner

```shell
kubectl apply -f https://infra.tekton.dev/tekton-releases/pipeline/previous/v1.11.0/release.yaml
```

## Attestation

The Rekor UUID for this release is `108e9186e8c5677ae7cc1db0d04d478cc74a86ca458747f1ca41fe102d4ec5f14a6f8ec59c48facd`

Obtain the attestation:
```shell
REKOR_UUID=108e9186e8c5677ae7cc1db0d04d478cc74a86ca458747f1ca41fe102d4ec5f14a6f8ec59c48facd
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://infra.tekton.dev/tekton-releases/pipeline/previous/v1.11.0/release.yaml
REKOR_UUID=108e9186e8c5677ae7cc1db0d04d478cc74a86ca458747f1ca41fe102d4ec5f14a6f8ec59c48facd

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v1.11.0@sha256:" + .digest.sha256')

# Download the release file
curl -L "$RELEASE_FILE" > release.yaml

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

* :sparkles: feat(webhook): Bump [[Knative|knative]].dev/pkg to enable centrally managed WEBHOOK_* TLS for the webhook (#9466)

Bump knative.dev/pkg to enable centralized WEBHOOK_* TLS configuration for the webhook (min/max version, cipher suites, curves).
Webhook now inherits TLS policy from environment (operator/cluster); defaults remain TLS 1.3 when unset.

* :sparkles: Add multi-URL support and per-resolution url param to Hub Resolver (#9465)

dd multi-URL support and per-resolution url parameter to Hub Resolver, enabling ordered fallback across multiple hub instances and explicit URL targeting per resolution request.

* :sparkles: Add pending status support for TaskRun (parity with PipelineRun) (#9464)

TaskRun now supports spec.status: TaskRunPending to defer execution.
When pending, no Pod is created and status.startTime is not set.
Clearing spec.status starts execution, or setting TaskRunCancelled cancels without running.

* :sparkles: feat: add optional PVC auto-cleanup annotation for workspaces mode (#9354)

Add optional PVC auto-cleanup for workspaces mode via `tekton.dev/auto-cleanup-pvc: "true"` annotation. When set on a PipelineRun using `coschedule: workspaces`, PVCs created from `volumeClaimTemplate` workspaces are automatically deleted on completion. User-provided `persistentVolumeClaim` workspaces are never affected.
* :sparkles: Add Gitea e2e tests to CI (#9442)

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

* :bug: Fix: Add SSH Host aliases to support multiple SSH credentials on same host (#9643)

Fixed SSH credential matching to support multiple repositories on the same host with different SSH keys. Previously, when using multiple SSH auth [[Secrets|secrets]] for different repositories on the same Git server (e.g., github.com/org/repo1 and github.com/org/repo2), SSH would use the first key for all repositories, causing authentication failures with deploy keys. SSH Host aliases and Git `url.*.insteadOf` rewriting now enable per-repository SSH key selection when the secret annotation URL includes a repo path.

* :bug: fix: make step-init symlink creation idempotent (#9600)

ix entrypoint step-init to handle container restarts gracefully. Previously, if a container restarted within a pod (e.g. due to OOM or eviction), the init process would fail with "symlink: file exists" because symlinks from the previous run persisted on the shared volume.

* :bug: fix: replace silent default namespace fallback with explicit error in GetNameAndNamespace (#9594)

eplace silent "default" namespace fallback in GetNameAndNamespace with an explicit error, preventing potential ResolutionRequest creation in wrong namespace.

* :bug: fix: resolve context key collision and ownerRef nil panic in resolution framework (#9593)

ix context key collision in resolution framework where RequestName() silently returned the namespace value, and fix nil pointer panic in ownerRefsAreEqual when both Controller fields are nil.

* :bug: fix: cluster resolver namespace access control whitespace and wildcard bugs (#9592)

ix cluster resolver namespace access control: trim whitespace in allowed/blocked namespace lists, fix wildcard (*) handling when combined with explicit entries, and reject empty default-namespace values.

* :bug: fix: convert pod latency metric to histogram and remove pod label (#9530)

ction required: The `tekton_pipelines_controller_taskruns_pod_latency_milliseconds` metric has been converted from a Gauge to a Histogram and the `pod` label has been removed. Dashboards or alerts referencing this metric will need to be updated to use `histogram_quantile()` instead of direct value queries.

* :bug: fix: use hashed volume names to prevent credential volume name collisions (#9528)

ix credential volume name collisions when namespaces have many (118+)
annotated secrets. Volume names now use deterministic SHA-256 hashing
instead of truncation with random suffix.

* :bug: Fix running_taskruns metric overcounting TaskRuns with no condition (#9485)

Fixed overcounting in the `running_taskruns` metric for `TaskRun`s with no condition set yet.

* :bug: fix: propagate PipelineRun tasks/finally timeout to child TaskRuns (#9419)

When `spec.timeouts.tasks` or `spec.timeouts.finally` on a PipelineRun exceeds the global default timeout, the value is now propagated to individual child TaskRuns that do not have an explicit per-task timeout. This prevents TaskRuns from being prematurely canceled at the global default (e.g., 1h) when the PipelineRun allows a longer duration.

* :bug: Bugfix: deduplicate concurrent resolver cache requests with singleflight. (#9365)

Fix resolver cache race condition causing duplicate upstream pulls under concurrent load.

* :bug: Fix: Add useHttpPath to support multiple Git credentials on same host (#9143)

Fixed Git credential matching to support multiple repositories on the same host with different credentials. Previously, when using multiple secrets for different repositories on the same Git server (e.g., github.com/org/repo1 and github.com/org/repo2), it incorrectly use the first credential for all repositories, causing authentication failures. Git credential contexts now include `useHttpPath = true`, enabling proper per-repository credential selection.

* :bug: fix: record metrics for cancelled PipelineRuns (#9658)
* :bug: Add explicit permissions blocks to workflows missing them (#9562)
* :bug: fix: revert mistaken metadata changes in resolvers config-observability (#9468)
* :bug: fix: update default tracing endpoint to http protobuf endpoint (#9141)
* :bug: fix: Pin Ubuntu,Bash,Python, Node & Perl container images to digests in examples/v1/taskruns/step-script.yaml (#9618)
* :bug: fix: Pin alpine-git-nonroot,alpine/git,busybox & nop container images to digests in examples/v1/taskruns (#9614)
* :bug: fix: Pin Bash,Alpine & Busybox container images to digests in examples/v1/taskruns (#9610)
* :bug: fix: Pin Ubuntu container images to digests in examples/v1/taskruns (#9607)

### Misc

* :hammer: perf(pipelinerun): hoist VerificationPolicy list out of per-task loop in resolvePipelineState (#9601)

* :hammer: ci: fix GitHub Actions security issues found by zizmor (#9667)
* :hammer: Extract memberOfLookup from createChildResourceLabels to reduce nested loop (#9596)
* :hammer: cleanup: replace GCS release URLs with infra.tekton.dev (#9569)
* :hammer: fix: Upgrade Gitea test infrastructure from v1.17.1 to latest (#9568)
* :hammer: chore: bump knative.dev/pkg to main and k8s libs to 0.35.1 (#9470)
* :hammer: Update stale comment about storing TaskSpec in status (#9661)
* :hammer: build(deps): bump the all group in /tekton with 4 updates (#9652)
* :hammer: build(deps): bump github/codeql-action from 4.33.0 to 4.34.1 (#9651)
* :hammer: build(deps): bump actions/cache from 5.0.3 to 5.0.4 (#9650)
* :hammer: build(deps): bump chainguard-dev/actions from 1.6.8 to 1.6.9 (#9649)
* :hammer: build(deps): bump github.com/spiffe/spire-api-sdk from 1.14.3 to 1.14.4 (#9648)
* :hammer: build(deps): bump k8s.io/apimachinery from 0.35.2 to 0.35.3 (#9639)
* :hammer: build(deps): bump k8s.io/client-go from 0.35.2 to 0.35.3 (#9638)
* :hammer: build(deps): bump k8s.io/api from 0.34.5 to 0.34.6 in /test/custom-task-ctrls/wait-task-beta (#9637)
* :hammer: build(deps): bump k8s.io/client-go from 0.34.5 to 0.34.6 in /test/custom-task-ctrls/wait-task-beta (#9634)
* :hammer: build(deps): bump github.com/spiffe/spire-api-sdk from 1.14.1 to 1.14.3 (#9629)
* :hammer: build(deps): bump google.golang.org/grpc from 1.79.2 to 1.79.3 (#9628)
* :hammer: build(deps): bump github.com/google/go-containerregistry from 0.21.2 to 0.21.3 (#9627)
* :hammer: build(deps): bump github.com/tektoncd/pipeline from 1.10.0 to 1.10.2 in /test/custom-task-ctrls/wait-task-beta (#9626)
* :hammer: build(deps): bump golang.org/x/sync from 0.19.0 to 0.20.0 (#9611)
* :hammer: build(deps): bump the all group in /tekton with 4 updates (#9587)
* :hammer: build(deps): bump github/codeql-action from 4.32.6 to 4.33.0 (#9586)
* :hammer: build(deps): bump fgrosse/go-coverage-report from 1.2.0 to 1.3.0 (#9585)
* :hammer: build(deps): bump step-security/harden-runner from 2.15.1 to 2.16.0 (#9584)
* :hammer: build(deps): bump chainguard-dev/actions from 1.6.7 to 1.6.8 (#9583)
* :hammer: Remove opencensus dependency from test files (#9553)
* :hammer: Update tj-actions/changed-files version comment to v47.0.5 (#9552)
* :hammer: build(deps): bump go.opentelemetry.io/otel/trace from 1.41.0 to 1.42.0 (#9549)
* :hammer: build(deps): bump github.com/google/go-containerregistry from 0.21.1 to 0.21.2 (#9548)
* :hammer: build(deps): bump google.golang.org/grpc from 1.79.1 to 1.79.2 (#9547)
* :hammer: build(deps): bump step-security/harden-runner from 2.15.0 to 2.15.1 (#9542)
* :hammer: build(deps): bump the all group in /tekton with 4 updates (#9541)
* :hammer: build(deps): bump tj-actions/changed-files from 47.0.4 to 47.0.5 (#9540)
* :hammer: build(deps): bump chainguard-dev/actions from 1.6.5 to 1.6.7 (#9539)
* :hammer: build(deps): bump github/codeql-action from 4.32.5 to 4.32.6 (#9538)
* :hammer: build(deps): bump actions/dependency-review-action from 4.8.3 to 4.9.0 (#9536)
* :hammer: Nominate khrm and aThorp96 as pipeline approvers (#9519)
* :hammer: Move inactive approvers to alumni (#9518)
* :hammer: build(deps): bump k8s.io/apiextensions-apiserver from 0.35.1 to 0.35.2 (#9487)
* :hammer: build(deps): bump the all group in /tekton with 4 updates (#9483)
* :hammer: build(deps): bump github/codeql-action from 4.32.4 to 4.32.5 (#9482)
* :hammer: build(deps): bump step-security/harden-runner from 2.14.2 to 2.15.0 (#9481)
* :hammer: build(deps): bump actions/setup-go from 6.2.0 to 6.3.0 (#9480)
* :hammer: build(deps): bump chainguard-dev/actions from 1.6.4 to 1.6.5 (#9479)
* :hammer: build(deps): bump actions/upload-artifact from 6.0.0 to 7.0.0 (#9478)
* :hammer: build(deps): bump go.opentelemetry.io/otel/metric from 1.40.0 to 1.41.0 (#9477)
* :hammer: build(deps): bump k8s.io/apimachinery from 0.35.1 to 0.35.2 (#9476)
* :hammer: build(deps): bump k8s.io/client-go from 0.34.3 to 0.34.5 in /test/custom-task-ctrls/wait-task-beta (#9475)
* :hammer: build(deps): bump k8s.io/code-generator from 0.35.1 to 0.35.2 (#9473)
* :hammer: build(deps): bump k8s.io/api from 0.34.3 to 0.34.5 in /test/custom-task-ctrls/wait-task-beta (#9472)
* :hammer: build(deps): bump k8s.io/apiextensions-apiserver from 0.34.3 to 0.34.5 (#9455)
* :hammer: build(deps): bump github.com/tektoncd/pipeline from 1.9.1 to 1.10.0 in /test/custom-task-ctrls/wait-task-beta (#9453)
* :hammer: build(deps): bump k8s.io/client-go from 0.34.3 to 0.34.4 (#9447)
* :hammer: build(deps): bump go.opentelemetry.io/otel/trace from 1.39.0 to 1.40.0 (#9445)
* :hammer: fix: release cheat sheet doc typos (#9415)

### Docs

* :book: Re-enable pipeline-api.md generation (#9604)

Update the pipeline API published at https://tekton.dev/docs/pipelines/pipeline-api/

* :book: docs(auth): clean stale TODO (#9504)

Clean up stale TODO in auth.md

* :book: doc: Clarify scope of auth documentation (#9461)

Added auth doc scope to distinguish credentials for processes inside Steps from Kubernetes imagePullSecrets for pulling Step images.
* :book: docs: update releases.md with security patch releases (#9616)
* :book: docs: add 4 undocumented metrics to docs/metrics.md (#9512)
* :book: docs: fix broken internal markdown links (#9507)
* :book: docs: add README files for pipelinerun and taskrun examples (#9505)
* :book: doc: Fix broken Tekton Bundles example link in taskruns.md (#9462)
* :book: docs: update releases.md for v1.10.0 (#9448)

## Thanks

Thanks to these contributors who contributed to v1.11.0!
* :heart: @AiswaryaR6
* :heart: @BizerNotNull
* :heart: @ChinonsoNwakudu
* :heart: @Goutham-AR
* :heart: @Paramesh324
* :heart: @ab-ghosh
* :heart: @adityavshinde
* :heart: @afrittoli
* :heart: @anirudh242
* :heart: @ankrsinha
* :heart: @app/dependabot
* :heart: @infernus01
* :heart: @jkhelil
* :heart: @jorqen
* :heart: @khrm
* :heart: @ngelman1
* :heart: @sahilleth
* :heart: @srivickynesh
* :heart: @twoGiants
* :heart: @vdemeester
* :heart: @waveywaves

Extra shout-out for awesome release notes:
* :heart_eyes: @BizerNotNull
* :heart_eyes: @ab-ghosh
* :heart_eyes: @afrittoli
* :heart_eyes: @ankrsinha
* :heart_eyes: @infernus01
* :heart_eyes: @jkhelil
* :heart_eyes: @jorqen
* :heart_eyes: @sahilleth
* :heart_eyes: @twoGiants
* :heart_eyes: @vdemeester
* :heart_eyes: @waveywaves

<!--
## Unsorted PR List
- docs: replace 'coming soon' with tkn bundle link in taskruns.md (#9509)

To Be Done: Deprecation Notices, Backward Incompatible Changes
-->
