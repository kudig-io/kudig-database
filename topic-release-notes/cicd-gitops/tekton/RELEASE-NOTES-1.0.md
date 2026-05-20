---
title: tekton v1.0 Release Notes
description: tekton v1.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- job
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v1.0 Release Notes 是什么
- 如何 tekton v1.0 Release Notes
trigger_keywords:
- tekton
- v1.0
- Release
- Notes
- release
- notes
---

# tekton v1.0 Release Notes

Source: [v1.0.1](https://github.com/tektoncd/pipeline/releases/tag/v1.0.1)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v1.0.1](https://github.com/tektoncd/pipeline/tree/v1.0.1/docs)
-[Examples @ v1.0.1](https://github.com/tektoncd/pipeline/tree/v1.0.1/examples)

## Installation one-liner

```shell
kubectl apply -f https://infra.tekton.dev/tekton-releases/pipeline/previous/v1.0.1/release.yaml
```

## Attestation

The Rekor UUID for this release is `108e9186e8c5677ac065838c723ea199e9f0cc152e2c53d2430fb164dfe15d3e59766eef70923f9f`

Obtain the attestation:
```shell
REKOR_UUID=108e9186e8c5677ac065838c723ea199e9f0cc152e2c53d2430fb164dfe15d3e59766eef70923f9f
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://infra.tekton.dev/tekton-releases/pipeline/previous/v1.0.1/release.yaml
REKOR_UUID=108e9186e8c5677ac065838c723ea199e9f0cc152e2c53d2430fb164dfe15d3e59766eef70923f9f

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v1.0.1@sha256:" + .digest.sha256')

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

### :warning: Security Fixes

- **[GHSA-j5q5-j9gm-2w5c](https://github.com/tektoncd/pipeline/security/advisories/GHSA-j5q5-j9gm-2w5c)** (Critical): Path traversal in git resolver allows reading arbitrary files from the resolver pod. Fixed by validating the `pathInRepo` parameter to prevent directory traversal.

- **[GHSA-cv4x-93xx-wgfj](https://github.com/tektoncd/pipeline/security/advisories/GHSA-cv4x-93xx-wgfj)** / CVE-2026-33022 (Medium): Controller panic via long resolver name in TaskRun/PipelineRun. A user with permission to create TaskRuns or PipelineRuns could crash the controller into a restart loop by setting a resolver name of 31+ characters, causing denial of service cluster-wide. Thanks to @1seal for reporting this vulnerability.

### Fixes

* :bug: fix: panic in v1beta1 matrix validation for invalid result refs (#9212) — Resolved an issue where Pipelines with invalid result references in matrix parameters would cause a panic during validation (v1beta1 API)
* :bug: fix(#8940): Fix token-authentication header in git resolver (#8947) — Before this change, there was a regression in which the git resolver was not authenticating with the provided `gitToken` and `gitTokenKey`, breaking the git resolver's http token-based auth. After this change, all git operations performed by the git resolver use the provided `gitToken` for remote authentication.
* :bug: fix: ensure git shell-out inherits environment variables (#8923) — The git resolver now respects environment variables on the pod
* :bug: fix(pipeline): support variables in onError for pipeline v1beta1 (#8932)
* :bug: fix: exclude pending PipelineRuns from `tekton_pipelines_controller_running_pipelineruns` metric (#8979) — Fixed metric to count only running PipelineRuns
* :bug: fix: allow finalizer updates on completed TaskRun and PipelineRuns (#9024)
* :bug: fix(ci): pin GitHub Actions to commit SHAs (#9305)

### Misc

* :hammer: The log results sidecar has been optimized to significantly reduce CPU utilization. Operators can tune the system for their environment—using a higher interval to reduce CPU load in production, or a lower interval for faster feedback in development (#8913)
* :hammer: ci: add CI summary fan-in job for branch protection (#9410)
* :hammer: build: bump Go version to 1.24.0 and fix e2e test infrastructure (#9318)
* :hammer: tekton: update plumbing ref to latest commit (#9411)
* :hammer: tekton: update plumbing ref to include full image references fix (#9404)
* :hammer: Docs: Switch from deprecated Tekton Hub to ArtifactHub, remove all references to gcr.io (#8918)

## Thanks

Thanks to these contributors who contributed to v1.0.1!

* :heart: @1seal
* :heart: @tekton-robot
* :heart: @vdemeester

