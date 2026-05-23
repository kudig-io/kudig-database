---
title: tekton v0.21 Release Notes
description: tekton v0.21 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- webhook
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- tekton v0.21 Release Notes 是什么
- 如何 tekton v0.21 Release Notes
trigger_keywords:
- tekton
- v0.21
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# tekton v0.21 Release Notes

Source: [v0.21.0](https://github.com/tektoncd/pipeline/releases/tag/v0.21.0)

# 🎉 WhenExpressions and Results in Finally Tasks 🎉

-[Docs @ v0.21.0](https://github.com/tektoncd/pipeline/tree/v0.21.0/docs)
-[Examples @ v0.21.0](https://github.com/tektoncd/pipeline/tree/v0.21.0/examples)

## Installation one-liner

```
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.21.0/release.yaml
```

## Upgrade Notices
* Requires [[Kubernetes|Kubernetes]] 1.17+ to run
* The controller now requires that the SYSTEM_NAMESPACE environment variable is set. This was set by default before, but is now required.

# Features

* :sparkles: WhenExpressions in Finally Tasks (#3738)

    WhenExpressions are supported in Finally Tasks not only to provide efficient guarded execution but also to improve the
reusability of `Tasks` in `Finally`

* :sparkles: Allow pipeline results to use custom task results (#3694)

    Pipeline results now can refer to pipeline tasks that run custom tasks and produce results.

* :sparkles: Support multiple [[Secrets|secrets]] of type dockercfg and dockerconfigjson (#3659)

    Adds support for multiple secrets of type dockercfg or dockerconfigjson

* :sparkles: add sparse checkout to git (#3646)

* :sparkles: Implement Pending PipelineRun status (TEP-0015) (#3522)

    Added PipelineRunPending setting to PipelineRun Spec Status to allow creating PipelineRuns in a Pending state.

* :sparkles: consuming task results in finally (#3242)

    Final tasks can be configured to consume `results` of `PipelineTasks` from `tasks` section. 


# Deprecation Notices

N/A

# Backwards incompatible changes

N/A

# Fixes

* :bug: Make volume mount names RFC1123 DNS label strict (#3740)

    Fixed a bug where a secret name with dots (e.g. `gcr.io`) led to a TaskRun creation failure, because the secret name was used internally as part of a volume mount name. These volume mount name have the be RFC1123 DNS label conform and therefore disallow dots as part of the name.

* :bug: Validate Context Variables in Finally Tasks (#3733)

    Added validation for Context variables in Finally Tasks

* :bug: Change to use the FilteredPodInformer to only watch Tekton relevant [[Pods|pods]] (#3725)

    Performance improvement: Only watch Pods managed by Tekton, instead of all Pods on the cluster

* :bug: Fix duplicate Pods when TaskRun or Pod labels are changed (#3708)

    Fix duplicate Pods when TaskRun or Pod labels are changed

* :bug: Fix stuck PipelineRun caused by optional workspace (#3702)

    Fixed bug where PipelineRun that didn't provide an optional workspace to the Pipeline would stall until timing out.

* :bug: taskrun: fix log message (#3689)

* :bug: Fix bug where step status ordering did not match step container ordering (#3679)

    Fixed bug where sorted order of step statuses did not match order of step containers.

* :bug: set task as failed  (#3571)

    Declare task failure when it hits `CreateContainerConfigError` instead of setting it to unknown i.e. running.

* :bug: Move resolution of Pipeline Results to end of completed/successful reconcile (#3684)

# Misc

* :hammer: Use crane cp to move images, instead of gcloud (#3755)

    Release: Copy and re-tag images using crane, instead of gcloud

* :hammer: Update TOC in Pipelines documentation (#3743)

* :hammer: Refactor IsFinallySkipped Tests (#3729)

* :hammer: Document deprecation of the build-gcs PipelineResource type (#3728)

    Deprecate the build-gcs sub-type of the storage PipelineResource

* :hammer: bump deps - k8s to 1.19.7 (#3724)

* :hammer: Issue #3553 reduce unit test execution time (#3652)

* :hammer: Remove pkg/system. (#3177)

    The controller now requires that the SYSTEM_NAMESPACE environment variable is set. This was set by default before, but is now required.

* :hammer: tekton controller logger key (#3752)

    The controller logger key is now set to "tekton-pipelines-controller" instead of "tekton" which is consistent with the webhook service name "tekton-pipelines-webhook".

* :hammer: promoting webhook service name in the logger (#3741)

    Promoting webhook service name in the logger instead of always defaulting to "webhook-pipeline". The service name is read from the env. variable `WEBHOOK_SERVICE_NAME` if specified else defaults to "tekton-pipelines-webhook"

* :hammer: Add nonroot user to pipeline's build-base image (#3727)

    Added non-root user 65532 to pipelines' build-base image. Git-init can now be used to clone repositories as a non-root user.

* :hammer: release: publish only on a given list of platforms 💻 (#3717)

    Publish images for a finite list of architecture instead of all the one supported by base images. The current list is : linux/amd64 (supported) and linux/arm64, linux/s390x, linux/ppc64le.

* :hammer: Validate dependencies between resolved resources in a PipelineRun (#3711)

    Added extra validations before PipelineRun can start: all result variables in the Pipeline must be valid and optional workspaces from a pipeline can only be passed to tasks expecting optional workspaces.

* :hammer: Fix replace go mod of client-go (#3668)

* :hammer: Use main branch for community repository 🧙 (#3747)

* :hammer: bump ggcr & k8schain module versions (#3736)

* :hammer: Add kustomize.yaml to the Tekton folder (#3754)

* :hammer: Small updates for s390x tests (#3714)

* :hammer: Ignore generated openapi code when running golangci-lint (#3685)

* :hammer: adding unit tests for pipelineTaskList.Deps (#3597)

* :hammer: Fix TestPipelineLevelFinally_OneDAGTaskFailed test to avoid false negative error (#3722)

* :hammer: Skip the dropNetworking test if it doesn't have the correct privileges. (#3653)


# Docs

* :book: Correct workspace doc on behaviour of mountPath (#3719)

    Fixed doc issue: relative workspace mountPaths are not prepended with "/workspace/" and never have been.

* :book: Fix typo in auth.md (#3709)

* :book: Update taskruns.md doc (#3696)

    Fix wrong path in the taskruns.md document.

* :book: Fix the broken link of pipeline previous yaml file (#3671)

* :book: updating doc - pipelineResult referencing results of a finally task (#3753)

* :book: Add docs about cloud events structure (#3735)

* :book: Add comment to PipelineRunSpecStatusPending (#3723)

* :book: updating readme with 0.20.1 (#3698)

* :book: Add link to docs and exmples for v0.20.0 (#3686)


## Thanks

Thanks to these contributors who contributed to v0.21.0!
* :heart: @GregDritschler
* :heart: @HeavyWombat
* :heart: @ImJasonH
* :heart: @LinuxSuRen
* :heart: @SaschaSchwarze0
* :heart: @Tomy2e
* :heart: @a-rothwell
* :heart: @afrittoli
* :heart: @barthy1
* :heart: @dlorenc
* :heart: @dprotaso
* :heart: @howardjohn
* :heart: @jbarrick-mesosphere
* :heart: @jerop
* :heart: @pritidesai
* :heart: @qu1queee
* :heart: @sbwsg
* :heart: @souleb
* :heart: @vdemeester
* :heart: @zhangtbj
* :heart: @zhouhaibing089

Extra shout-out for awesome release notes:
* :heart_eyes: @GregDritschler
* :heart_eyes: @HeavyWombat
* :heart_eyes: @ImJasonH
* :heart_eyes: @LinuxSuRen
* :heart_eyes: @SaschaSchwarze0
* :heart_eyes: @Tomy2e
* :heart_eyes: @a-rothwell
* :heart_eyes: @dlorenc
* :heart_eyes: @dprotaso
* :heart_eyes: @howardjohn
* :heart_eyes: @jbarrick-mesosphere
* :heart_eyes: @jerop
* :heart_eyes: @pritidesai
* :heart_eyes: @qu1queee
* :heart_eyes: @sbwsg
* :heart_eyes: @souleb
* :heart_eyes: @vdemeester
* :heart_eyes: @zhangtbj
* :heart_eyes: @zhouhaibing089
