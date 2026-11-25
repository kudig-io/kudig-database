---
title: tekton v1.10 Release Notes
description: tekton v1.10 Release Notes — Kubernetes 生产运维知识库
summary: tekton v1.10 Release Notes — Kubernetes 生产运维知识库
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
- tekton v1.10 Release Notes 是什么
- 如何 tekton v1.10 Release Notes
trigger_keywords:
- tekton
- v1.10
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# tekton v1.10 Release Notes

Source: [v1.10.2](https://github.com/tektoncd/pipeline/releases/tag/v1.10.2)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v1.10.2](https://github.com/tektoncd/pipeline/tree/v1.10.2/docs)
-[Examples @ v1.10.2](https://github.com/tektoncd/pipeline/tree/v1.10.2/examples)

## Installation one-liner

``` shell
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f https://infra.tekton.dev/tekton-releases/pipeline/previous/v1.10.2/release.yaml
```
## Attestation

The Rekor UUID for this release is `108e9186e8c5677a104b9492904b91b09e714ee02dae9637eee78dfd892d6ca7cab46ce0208fd387`

Obtain the attestation:
```shell
REKOR_UUID=108e9186e8c5677a104b9492904b91b09e714ee02dae9637eee78dfd892d6ca7cab46ce0208fd387
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://infra.tekton.dev/tekton-releases/pipeline/previous/v1.10.2/release.yaml
REKOR_UUID=108e9186e8c5677a104b9492904b91b09e714ee02dae9637eee78dfd892d6ca7cab46ce0208fd387

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v1.10.2@sha256:" + .digest.sha256')

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

Thanks to these contributors who contributed to v1.10.2!

* :heart: @vdemeester
* :heart: @1seal




<!-- risk-assessed -->
