---
title: tekton v0.42 Release Notes
description: tekton v0.42 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
- crd
- webhook
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v0.42 Release Notes 是什么
- 如何 tekton v0.42 Release Notes
trigger_keywords:
- tekton
- v0.42
- Release
- Notes
- release
- notes
---


# tekton v0.42 Release Notes

Source: [v0.42.0](https://github.com/tektoncd/pipeline/releases/tag/v0.42.0)

# 🎉 Enforce resource verification, GA policy documented and Custom Task Beta 🎉

-[Docs @ v0.42.0](https://github.com/tektoncd/pipeline/tree/v0.42.0/docs)
-[Examples @ v0.42.0](https://github.com/tektoncd/pipeline/tree/v0.42.0/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.42.0/release.yaml
```

## Attestation

The Rekor UUID for this release is `24296fb24b8ad77a92f523df8531edb5cb063ec9ef24a9e652e0643ff0f7ac9ce89edc8aa9395ffd`

Obtain the attestation:
```shell
REKOR_UUID=24296fb24b8ad77a92f523df8531edb5cb063ec9ef24a9e652e0643ff0f7ac9ce89edc8aa9395ffd
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.42.0/release.yaml
REKOR_UUID=24296fb24b8ad77a92f523df8531edb5cb063ec9ef24a9e652e0643ff0f7ac9ce89edc8aa9395ffd

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.42.0@sha256:" + .digest.sha256')

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

* :sparkles: Make the webhook port number configurable (#5696)

The port on which the webhook server listens may be configured via the WEBHOOK_PORT environment variable.

* :sparkles: Set ConfigSource in clusterresolver (#5687)

Set source value for cluster resource to link back its origin i.e. url and checksum.

* :sparkles: Set ConfigSource in bundleresolver (#5684)

ociresolver captures correct source information about where remote image came from.

* :sparkles: Feature flag for provenance field in status (#5670)

Added a new boolean feature flag named "enable-provenance-in-status" in feature-flags configmap to enable the provenance field in status to be populated. This field in status aims to record authenticated metadata about how a software artifact was built i.e. the source where remote resource came from.

* :sparkles: Set ConfigSource in gitresolver (#5664)

Set ConfigSource value for gitresolver to record the source where the remote resource came from. 

* :sparkles: TEP-0114: Reconciler, event, config, webhook support for CustomRuns (#5662)

Reconciler, event, config, and webhook support for CustomRuns

* :sparkles: [TEP-0091] Add Verification at reconciler (#5581)

Trusted Resource feature enable tekton pipeline to verify the resources resolved from resolver. With trusted resource feature, users can configure public keys in configmap and choose to turn on/off this feature via feature flag `resource-verification-mode`. This commit enables mount public key files as secrets into Pipeline and used for verification. Taskrun/Pipelinerun that fail the verification will be marked as `failed` and be stopped from execution if `resource-verification-mode` is set to `enforce`

* :sparkles: Populate the  field (#5397)

Populate the TaskRun/PipelineRun's Status.Provenance.ConfigSource field with the value from the remote ResolutionRequest Status. 

Note: the feature flag `enable-provenance-in-status` needs to be set to "true" to enable this provenance field to be populated & available in *Run.Status.
* :sparkles: Bring `Retries` and `RetriesStatus` back (#5765)

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

* :bug: Add conversion config to ResolutionRequest CRD (#5742)

Properly configures conversion from v1alpha1.ResolutionRequest to v1beta1.ResolutionRequest

* :bug: fix: the pipelinerun never done due to repeated workspace (#5724)

Check for duplicate workspaces of pipeline task.

* :bug: Remove logging.request-log-template from resolvers config-observability CM (#5717)

Clean up example configuration in config-observability configmap for tekton-pipelines-resolvers namespace

* :bug: Fixes default value for enabling resolvers (#5725)
* :bug: Fix TaskRef and PipelineRef name with Resolver Conversion (#5702)


### Misc

* :hammer: TEP-0114 Serve Custom Task Beta (#5780)

Custom Task Beta is available.

* :hammer: Use SchemeGroupVersion for tekton objects in cluster resolver (#5705)

Use v1beta1.SchemeGroupVersion.String() for the APIVersion field in the tekton object retrieved by cluster resolver.

* :hammer: fix cloud event flaky unit tests by adding waitgroup to fakeclient (#5690)

Fix cloud event flacky unit tests by adding EventSender
* :hammer: Add unit tests for bundle resolver (#5704)

Add unit test for bundle resolver

* :hammer: Add GA API policy and describe feature gates (#5634)

Updates API compatibility policy for the V1 api version
* :hammer: main branch's codegen is out of sync. (#5764)
* :hammer: fix knative downstream tests (#5763)
* :hammer: TEP-0114: Stop serving v1beta1.CustomRun until we align on Retries (#5736)
* :hammer: Order methods to appear next to their receiving types (#5733)
* :hammer: [upgrade test] Change to Kind cluster and Unfixed upgrade test release version (#5726)
* :hammer: Remove `retriesStatus` from `CustomRunStatus` (#5719)
* :hammer: Add RunReason and CustomRunReason (#5718)
* :hammer: Stop using copy-paste of old git-clone catalog task in examples/tests (#5712)
* :hammer: Bump k8s.io/code-generator from 0.25.2 to 0.25.4 (#5762)
* :hammer: Bump k8s.io/client-go from 0.25.3 to 0.25.4 (#5761)
* :hammer: Bump k8s.io/api from 0.25.3 to 0.25.4 (#5759)
* :hammer: Bump github.com/containerd/containerd from 1.6.9 to 1.6.10 (#5758)
* :hammer: Bump k8s.io/apimachinery from 0.25.3 to 0.25.4 (#5745)
* :hammer: Bump github.com/google/go-containerregistry from 0.12.0 to 0.12.1 (#5743)
* :hammer: Bump golang.org/x/oauth2 from 0.1.0 to 0.2.0 (#5739)
* :hammer: Bump golang.org/x/crypto from 0.1.0 to 0.2.0 (#5738)
* :hammer: Bump go.opencensus.io from 0.23.0 to 0.24.0 (#5731)
* :hammer: Rename v1beta1 clients for test (#5701)
* :hammer: Bump github.com/jenkins-x/go-scm from 1.11.29 to 1.11.35 (#5642)

### Docs


* :book: Update Roadmap with link to project board (#5735)
* :book: Add finallystarttime to PipelineRun status docs (#5729)
* :book: Update README and releases for v0.41.0 (#5698)
* :book: TEP-0114: Custom Task Beta - User Guide (#5677)

## Thanks

Thanks to these contributors who contributed to v0.42.0!
* :heart: @JeromeJu
* :heart: @ScrapCodes
* :heart: @XinruZhang
* :heart: @Yongxuanzhang
* :heart: @abayer
* :heart: @afrittoli
* :heart: @chuangw6
* :heart: @cugykw
* :heart: @dependabot[bot]
* :heart: @dibyom
* :heart: @imjasonh
* :heart: @jerop
* :heart: @lbernick
* :heart: @sel
* :heart: @sm43
* :heart: @urbanikb

Extra shout-out for awesome release notes:
* :heart_eyes: @XinruZhang
* :heart_eyes: @Yongxuanzhang
* :heart_eyes: @abayer
* :heart_eyes: @chuangw6
* :heart_eyes: @cugykw
* :heart_eyes: @dibyom
* :heart_eyes: @sel

<!--
## Unsorted PR List
- Bump github.com/spiffe/spire-api-sdk from 1.4.5 to 1.5.0 (#5716)
- Bump github.com/spiffe/spire-api-sdk from 1.4.4 to 1.4.5 (#5711)
- Add prerequisites for running upgrade tests locally (#5649)

To Be Done: Deprecation Notices, Backward Incompatible Changes
-->