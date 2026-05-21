---
title: tekton v0.25 Release Notes
description: tekton v0.25 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- rbac
- webhook
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v0.25 Release Notes 是什么
- 如何 tekton v0.25 Release Notes
trigger_keywords:
- tekton
- v0.25
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# tekton v0.25 Release Notes

Source: [v0.25.0](https://github.com/tektoncd/pipeline/releases/tag/v0.25.0)

# 🎉 Hermetic Execution, Embedded Custom Tasks and Graceful Termination & Timeouts 🎉

-[Docs @ v0.25.0](https://github.com/tektoncd/pipeline/tree/v0.25.0/docs)
-[Examples @ v0.25.0](https://github.com/tektoncd/pipeline/tree/v0.25.0/examples)

## Installation one-liner

```
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.25.0/release.yaml
```
## Upgrade Notices

N/A

# Features

* :sparkles: Add ConfigMap which can contain pipelines info and RBAC to access this ConfigMap (#3971)

* :sparkles: Add support for experimental hermetic execution mode to TaskRuns (#3956)

   Add support for experimental hermetic execution mode to TaskRuns

* :sparkles: Graceful Pipeline Run Termination (#3915)

   The new PipelineRun spec statuses have been added to control the way who a PipelineRun is being canceled or stopped.

   - "StoppedRunFinally" - To stop (i.e. let the tasks complete, then execute finally tasks) a PipelineRun
   - "CancelledRunFinally" - To cancel (i.e. interrupt any executing non finally tasks, then execute finally tasks)
   - "Cancelled" - replaces today's "PipelineRunCancelled" - i.e. interrupt any executing tasks without running finally tasks

   Support for existing statuses has been left unchanged. The status "PipelineRunCancelled" is deprecated and replaced by "Cancelled" (it would be removed in v1). The new states are released as alpha API features. Read more in [TEP#0058](https://github.com/tektoncd/community/blob/main/teps/0058-graceful-pipeline-run-termination.md).

* :sparkles: TEP-0061, Allow custom task to be embedded. (#3901)

   It is now possible to embed the spec of a custom task in a Run resource, whether stand-alone or embedded in a Pipeline.

   - API changes, This PR adds new APIs i.e. adds a field `Spec *EmbeddedRunSpec` to `RunSpec`
   - An embedded task will accepts new field `Spec` with type `runtime.RawExtension` in addition to
    `ApiVersion` and `Kind` fields of type string (as part of `runtime.TypeMeta`) 
   - Validation changes, in addition to adding support for `Run.RunSpec.Spec` the validations will be changed
    to support "One of `Run.RunSpec.Spec` or `Run.RunSpec.Ref` " only and not both as part of a single
    API request to kubernetes.

   action required: Developers of custom controllers (existing and new), who would like to support
 embedded spec for their custom task, need to implement the logic required to extract, validate
 and use the custom task spec from the new RunSpec.Spec field. Please review the documentation
 on upgrading, for more details and some examples.

* :sparkles: Add a Timeouts optional field to pipelinerun (#3843)
   - API changes
      - Added field Timeouts to PipelineRun spec. It is a dict with the following sub-fields
         - pipeline, to control the pipeline failure timeout
         - tasks, to control the pipeline tasks failure timeout
         - finally, to control the pipeline finally tasks failure timeout
   - Changes in behavior
      - When supplied, a timeouts field combination permits deciding which part of the pipeline runtime is allocated to tasks and finally tasks.

* :sparkles: Add variables context.pipelineTask.retries and context.task.retry-count (#3770)


# Deprecation Notices

* :rotating_light: "PipelineRunCancelled" is deprecated

   The status "PipelineRunCancelled" is deprecated and replaced by "Cancelled" (it would be removed in v1) (#3915)

# Backwards incompatible changes

N/A

# Fixes

* :bug: Fix Windows image entrypoint paths for parity with Linux (#4018)

* :bug: A custom task check could be true, even though the Provided Spec is invalid. (#4005)

* :bug: Fix Workspaces in Sidecar to be serialized as workspaces not Workspaces (#3966)

* :bug: Encode scripts as base 64 to avoid k8s mangling 40 (#3963)

   Scripts in Tasks are now written into the Task's pod using base64 to avoid kubernetes' built-in arg processing. This means they're a little larger than they were prior to this release but otherwise should continue working as expected.

* :bug: Only fetch the definitions once 🧙 (#3941)

   Only fetch the definition once, and then used the spec stored in the status as source of truth. 
   This reduce the probable race condition when a `PipelineRun` or a `TaskRun` refers to a `Pipeline` or `Task` that changes during its execution.

* :bug: Skip *heavy* validation on deletion 🙃 (#3937)

   Skip *heavy* validation on deletion in the webhook

* :bug: Validate run for both ref and spec as nil. (#3977)


# Misc

* :hammer: Revert Fix issue with 69 in Script blocks (#3938)

   Revert fix for instances of "$$" in script blocks. Kubernetes replaces "$$" with a single "$" and your scripts need to deal appropriately with these instances.

* :hammer: Cleanup integraton tests for multi-arch case (#3998)

* :hammer: Fix list of proposed kind labels for PRs (#3987)

* :hammer: Use tektoncd/results repo for git-init symbolic ref tests (#4038)

* :hammer: Add Dockerfiles for Windows entrypoint and nop images (#3996)

* :hammer: Add hermetic test running as non-root user (#3973)

* :hammer: Change the way we run e2e tests with feature gates in pipelines (#3930)

# Docs

* :book: Update api-spec.md to fix some formatting (#4030)

* :book: Add v0.24.2 to the README (#4015)

* :book: Fix minor typo in auth docs (#3992)

* :book: Document how to use the kind tool and the Tekton/plumbing convenience script (#3972)

   Documenting how to use the kind tool to setup the development environment.

* :book: Organize DEVELOPMENT.md, provide consistent examples & reflect go mod support (#3955)

   Organized `DEVELOPMENT.md` to provide more consistent examples and instructions.

* :book: Link to docs and examples for v0.24.3 (#4031)

* :book: Update docs to use Kubernetes 1.18 as the minimum version (#3986)

* :book: docs(pipelines): specify how to reference an array parameter (#3967)

* :book: Document the safe-to-evict annotation on the webhook deployment (#3961)

* :book: Add missing frontmatter to some docs (#3958)

* :book: Update pipelines.md (#3946)

* :book: Link to docs and examples for v0.24.1 (#3944)

* :book: Format code blocks with prettier (#3911)

## Thanks

Thanks to these contributors who contributed to v0.25.0!
* :heart: @R2wenD2
* :heart: @ScrapCodes
* :heart: @afrittoli
* :heart: @aiden-deloryn
* :heart: @barthy1
* :heart: @eliasnorrby
* :heart: @imjasonh
* :heart: @jeffmaury
* :heart: @jerop
* :heart: @jmlrt
* :heart: @mrutkows
* :heart: @n3wscott
* :heart: @nikhil-thomas
* :heart: @priyawadhwa
* :heart: @rafalbigaj
* :heart: @rguichard
* :heart: @sbwsg
* :heart: @souleb
* :heart: @vdemeester
* :heart: @vinamra28
* :heart: @yaoxiaoqi

Extra shout-out for awesome release notes:
* :heart_eyes: @R2wenD2
* :heart_eyes: @ScrapCodes
* :heart_eyes: @aiden-deloryn
* :heart_eyes: @jeffmaury
* :heart_eyes: @jerop
* :heart_eyes: @mrutkows
* :heart_eyes: @nikhil-thomas
* :heart_eyes: @priyawadhwa
* :heart_eyes: @rafalbigaj
* :heart_eyes: @sbwsg
* :heart_eyes: @souleb
* :heart_eyes: @vdemeester
* :heart_eyes: @vinamra28
* :heart_eyes: @yaoxiaoqi
