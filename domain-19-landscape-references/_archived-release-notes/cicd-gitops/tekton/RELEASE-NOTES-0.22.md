---
title: tekton v0.22 Release Notes
description: tekton v0.22 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- pdb
- webhook
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v0.22 Release Notes 是什么
- 如何 tekton v0.22 Release Notes
trigger_keywords:
- tekton
- v0.22
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# tekton v0.22 Release Notes

Source: [v0.22.0](https://github.com/tektoncd/pipeline/releases/tag/v0.22.0)

# 🎉 Stop API conversion spam and Disable webhook PodDisruptionBudget🎉

-[Docs @ v0.22.0](https://github.com/tektoncd/pipeline/tree/v0.22.0/docs)
-[Examples @ v0.22.0](https://github.com/tektoncd/pipeline/tree/v0.22.0/examples)

## Installation one-liner

```
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.22.0/release.yaml
```

# Features

N/A

# Deprecation Notices

N/A

# Backwards incompatible changes

N/A

# Fixes

* :bug: Serialize and deserialize Finally in PipelineRuns too (#3816)

Fix issue where PipelineRun with finally in nested pipeline spec would lose those finally tasks when converted down to v1alpha1 and back up to v1beta1 again.

* :bug: Change to avoid error when [[Service|service]] account has empty secret name (#3795)

In case of an empty secret name, the taskRun was failing with CouldntGetTask as the GET request with an empty secret name return an error saying resource name may not be empty. Fixing such failing of a taskRun by avoiding an empty secret name in the kubeclient GET request.

* :bug: Disable Webhook PDB by default, document enabling it (#3787)

Disable PodDisruptionBudget for the webhook deployment by default

* :bug: Set minAvailable:1 to unblock node upgrades (#3784)

Modify webhook PodDisruptionBudget minAvailable to 1, so node upgrades aren't blocked

* :bug: Losslessly roundtrip Pipelines with Finally from beta to alpha and back (#3779)

v1beta1 Pipelines can now be requested with v1alpha1 version without losing Finally tasks. Applying the returned v1alpha1 version will store the resource as v1beta1 with the Finally section restored to its original state.

* :bug: Short term fix for Cloud Event Source (#3761)

Resolves #2676 by providing Cloud Event source value when selfLink unset

* :bug: Fix nightly builds (again) (#3776)
* :bug: Fix the cr URL for crane (#3775)
* :bug: Fix the pipeline nightly (#3772)
* :bug: Pin golang to avoid breakages. (#3766)


# Misc

* :hammer: Remove Test Builders from pipelinerunstate_test.go (#3802)



* :hammer: Remove Test Builders from remote_tests.go (#3801)



* :hammer: Remove support for build-gcs and the gcs-fetcher image (#3771)

Remove support for build-gcs and the gcs-fetcher image

* :hammer: Remove tekton.dev/task label from taskrun of clustertasks (#3764)

Remove tekton.dev/task label from taskrun of clustertasks

* :hammer: Closes #3262: Modify unnecessarily exported methods to unexported (#3289)
* :hammer: Add PipelineRun and TaskRun Status work to the Roadmap (#3793)



* :hammer: Refine the comment for git init base image (#3791)
* :hammer: cleaning up the function parameter (#3808)
* :hammer: Cleanup s390x exclude test list with build-gcs tests (#3783)
* :hammer: Re-enable test now that HEAD is fixed (#3768)
* :hammer: Update roadmap for 2021! 🛣️ (#3789)
* :hammer: Rework the release pipeline to use workspaces (#3788)
* :hammer: Use legacy build and test golang tasks (#3780)
* :hammer: Move the secret mount to the correct step (#3777)
* :hammer: Adding power (ppc64le) architecture image mappings (#3630)

# Docs

* :book: Fix typo in Code Blocks (#3814)



* :book: Add v0.21.0 to the README  (#3765)



* :book: Add the missing imagePullSecret configuration guidance for a developer (#3699)
* :book: Add documenation about setting resource limits on a Task step (#3809)
* :book: Update docs to use Kubernetes 1.17 as the minimum version (#3805)
* :book: Fix a broken link to the service account docs (#3773)

## Thanks

Thanks to these contributors who contributed to v0.22.0!
* :heart: @DanArlowski
* :heart: @ImJasonH
* :heart: @afrittoli
* :heart: @bahetiamit
* :heart: @barthy1
* :heart: @bobcatfish
* :heart: @cqbqdd11519
* :heart: @jerop
* :heart: @jmcshane
* :heart: @kobayashi
* :heart: @kscherer
* :heart: @mattmoor
* :heart: @piyush-garg
* :heart: @popcor255
* :heart: @pritidesai
* :heart: @sbwsg
* :heart: @wlynch
* :heart: @xiujuan95
* :heart: @zhangtbj

Extra shout-out for awesome release notes:
* :heart_eyes: @DanArlowski
* :heart_eyes: @ImJasonH
* :heart_eyes: @cqbqdd11519
* :heart_eyes: @jerop
* :heart_eyes: @jmcshane
* :heart_eyes: @kobayashi
* :heart_eyes: @piyush-garg
* :heart_eyes: @popcor255
* :heart_eyes: @sbwsg
* :heart_eyes: @xiujuan95
* :heart_eyes: @zhangtbj



