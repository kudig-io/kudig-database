---
title: tekton v1.3 Release Notes
description: tekton v1.3 Release Notes — Kubernetes 生产运维知识库
summary: tekton v1.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- apiserver
- docker
- job
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v1.3 Release Notes 是什么
- 如何 tekton v1.3 Release Notes
trigger_keywords:
- tekton
- v1.3
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# tekton v1.3 Release Notes

Source: [v1.3.3](https://github.com/tektoncd/pipeline/releases/tag/v1.3.3)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v1.3.3](https://github.com/tektoncd/pipeline/tree/v1.3.3/docs)
-[Examples @ v1.3.3](https://github.com/tektoncd/pipeline/tree/v1.3.3/examples)

## Installation one-liner

```shell
kubectl apply -f https://infra.tekton.dev/tekton-releases/pipeline/previous/v1.3.3/release.yaml
```

## Attestation

The Rekor UUID for this release is `108e9186e8c5677a876fa60f37e6445548fabd0dc463c51c7f8b06b07c242eb0921ee277008b088c`

Obtain the attestation:
```shell
REKOR_UUID=108e9186e8c5677a876fa60f37e6445548fabd0dc463c51c7f8b06b07c242eb0921ee277008b088c
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://infra.tekton.dev/tekton-releases/pipeline/previous/v1.3.3/release.yaml
REKOR_UUID=108e9186e8c5677a876fa60f37e6445548fabd0dc463c51c7f8b06b07c242eb0921ee277008b088c

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v1.3.3@sha256:" + .digest.sha256')

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

- **[GHSA-cv4x-93xx-wgfj](https://github.com/tektoncd/pipeline/security/advisories/GHSA-cv4x-93xx-wgfj)** / CVE-2026-33022 (Medium): Controller panic via long resolver name in TaskRun/PipelineRun. A user with permission to create TaskRuns or PipelineRuns could crash the controller into a restart loop by setting a resolver name of 31+ characters, causing denial of [[Service|service]] cluster-wide. Thanks to @1seal for reporting this vulnerability.

### Features

* :sparkles: Add support for `hostUsers` field in PodTemplate to control user namespace isolation (#9323)

### Fixes

* :bug: fix: panic in v1beta1 matrix validation for invalid result refs (#9260) — Resolved an issue where Pipelines with invalid result references in matrix parameters would cause a panic during validation (v1beta1 API)
* :bug: fix(pipelinerun): fix the issue of massive invalid status updates caused by unordered arrays, which will greatly impact the resource load and stability of the apiserver (#9312)
* :bug: fix(#8940): Fix token-authentication header in git resolver (#9115) — Before this change, there was a regression in which the git resolver was not authenticating with the provided `gitToken` and `gitTokenKey`, breaking the git resolver's http token-based auth. After this change, all git operations performed by the git resolver use the provided `gitToken` for remote authentication.
* :bug: fix: Prevent excessive reconciliation when timeout disabled (#9355)

### Misc

* :hammer: ci: add CI summary fan-in job for branch protection (#9409)
* :hammer: tekton: update plumbing ref to latest commit (#9414)
* :hammer: tekton: update plumbing ref to include full image references fix (#9403)
* :hammer: Backported test reliability fixes including reduced test parallelism, image migration from DockerHub to mirror.gcr.io, and improved dind-sidecar probe configuration (#9250)

## Thanks

Thanks to these contributors who contributed to v1.3.3!

* :heart: @1seal
* :heart: @aThorp96
* :heart: @tekton-robot
* :heart: @vdemeester
* :heart: @waveywaves

