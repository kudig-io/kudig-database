---
title: tekton v0.23 Release Notes
description: tekton v0.23 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- rbac
- crd
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v0.23 Release Notes 是什么
- 如何 tekton v0.23 Release Notes
trigger_keywords:
- tekton
- v0.23
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# tekton v0.23 Release Notes

Source: [v0.23.0](https://github.com/tektoncd/pipeline/releases/tag/v0.23.0)

# 🎉 Spring Cleaning Edition: reduce controller permissions, remove unused Image CRD, add non-root user to git-init and pullrequest-init  🎉

-[Docs @ v0.23.0](https://github.com/tektoncd/pipeline/tree/v0.23.0/docs)
-[Examples @ v0.23.0](https://github.com/tektoncd/pipeline/tree/v0.23.0/examples)

## Installation one-liner

```
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.23.0/release.yaml
```

## Upgrade Notices

None in this release.

## Changes

# Features

* :sparkles: Remove YAML merge and variable from config YAML (#3842)

Make release.yaml more easily consumed by tools like kustomize and ytt


# Deprecation Notices

None in this release.

# Backwards incompatible changes

None in this release.

# Fixes

* :bug: PVC: Use Owner UIDs instead of Owner names. (#3856)




# Misc

* :hammer: Use v1 instead of v1beta1 for  🤖 (#3859)

Using `rbac.authorization.k8s.io/v1` instead of `rbac.authorization.k8s.io/v1beta1` for `ClusterRoleBinding` as it is being deprecated starting in 1.17.

* :hammer: Further limit cluster-wide read-write permissions (#3832)

Remove cluster-wide write access to ConfigMaps, LimitRanges and remove all cluster-wide access to Deployments

* :hammer: Remove cluster-wide write access to ServiceAccounts and Secrets (#3831)

Remove cluster-wide write access to ServiceAccounts and Secrets
* :hammer: Remove unused image-cache CRD (#3850)

The `Image` CRD in the `caching.internal.knative.dev` group is not used by `Tekton` and is not included anymore in the release.

* :hammer: Change to use new base images for git and pullrequest images (#3828)

Add nonroot user in the PullRequest init base image

* :hammer: Add Dockerfile for pullrequest nonroot build base image (#3810)

Add Dockerfile for pullrequset nonroot build base image
* :hammer: Pull request template spring cleaning 🌷 (#3866)
* :hammer: refactor pipelineTask validation (#3848)
* :hammer: refactoring validate pipelineTask name (#3818)
* :hammer: Add ability to run e2e tests on top of nightly release (#3847)
* :hammer: Use "no-cache" in the buildkit based builds (#3834)
* :hammer: Update self, community and plumbing reference to use main… 🧙 (#3821)

# Docs

* :book: Correct url in migration doc and modify to main branch in doc urls (#3860)

Correct the links in migration doc and modify url to use main branch

* :book: Remove mention of absolute vs relative path from workspaces doc (#3852)

Removed incorrect doc that stated workspaces with relative mountPath would be mounted relative to /workspace
* :book: add the latest release - 0.22.0 (#3826)
* :book: updating release cheat sheet (#3825)
* :book: Fix the pipeline release cheat-sheet (#3823)

## Thanks

Thanks to these contributors who contributed to v0.23.0!
* :heart: @ImJasonH
* :heart: @afrittoli
* :heart: @barthy1
* :heart: @bobcatfish
* :heart: @pritidesai
* :heart: @sbwsg
* :heart: @vdemeester
* :heart: @wlynch
* :heart: @zhangtbj

Extra shout-out for awesome release notes:
* :heart_eyes: @ImJasonH
* :heart_eyes: @afrittoli
* :heart_eyes: @sbwsg
* :heart_eyes: @vdemeester
* :heart_eyes: @wlynch
* :heart_eyes: @zhangtbj