---
title: tekton v1.9 Release Notes
description: tekton v1.9 Release Notes — Kubernetes 生产运维知识库
summary: tekton v1.9 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v1.9 Release Notes 是什么
- 如何 tekton v1.9 Release Notes
trigger_keywords:
- tekton
- v1.9
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# tekton v1.9 Release Notes

Source: [v1.9.2](https://github.com/tektoncd/pipeline/releases/tag/v1.9.2)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v1.9.2](https://github.com/tektoncd/pipeline/tree/v1.9.2/docs)
-[Examples @ v1.9.2](https://github.com/tektoncd/pipeline/tree/v1.9.2/examples)

## Installation one-liner

```shell
kubectl apply -f https://infra.tekton.dev/tekton-releases/pipeline/previous/v1.9.2/release.yaml
```

## Attestation

The Rekor UUID for this release is `108e9186e8c5677a6c7ee52741b34d7b7e9a3277e775365533a3669a49c3be92b372bcbda73ee439`

Obtain the attestation:
```shell
REKOR_UUID=108e9186e8c5677a6c7ee52741b34d7b7e9a3277e775365533a3669a49c3be92b372bcbda73ee439
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://infra.tekton.dev/tekton-releases/pipeline/previous/v1.9.2/release.yaml
REKOR_UUID=108e9186e8c5677a6c7ee52741b34d7b7e9a3277e775365533a3669a49c3be92b372bcbda73ee439

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v1.9.2@sha256:" + .digest.sha256')

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

## Thanks

Thanks to these contributors who contributed to v1.9.2!

* :heart: @vdemeester
* :heart: @1seal

