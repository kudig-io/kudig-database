---
title: tekton v0.6 Release Notes
description: tekton v0.6 Release Notes — Kubernetes 生产运维知识库
summary: tekton v0.6 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- opa
- crd
- webhook
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v0.6 Release Notes 是什么
- 如何 tekton v0.6 Release Notes
trigger_keywords:
- tekton
- v0.6
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---



# tekton v0.6 Release Notes

Source: [v0.6.0](https://github.com/tektoncd/pipeline/releases/tag/v0.6.0)

# 🎉 Conditional Execution, Array Params, StepTemplates, and much much more! 🎉

* [Docs @ v0.6.0](https://github.com/tektoncd/pipeline/tree/release-v0.6.x/docs#tekton-pipelines)
* [Examples @ v0.6.0](https://github.com/tektoncd/pipeline/tree/v0.6.0/examples)

## Changes

### Features

* Pipeline tasks can now be conditionally executed based on a user defined condition. See the docs on how to define [Conditions](https://github.com/tektoncd/pipeline/blob/v0.6.0/docs/conditions.md) and use then in [Pipelines](https://github.com/tektoncd/pipeline/blob/v0.6.0/docs/pipelines.md#conditions)  (#1031, #1093, #1143, #1178 ). 
Note: There is more work underway with Conditionals that will be in the next release (tracked in #1137)

* Parameters now support the Array type. See the docs [here](https://github.com/tektoncd/pipeline/blob/v0.6.0/docs/pipelines.md#parameters) (#1080).

* `stepTemplate` now supports variable substitution (#1061)

* The default timeout of TaskRun and PipelineRun is  now configurable using`default-timeout-minutes` in `config/config-defaults.yaml` (default: 60 mins). The default value can be overridden in a PipelineRun or TaskRun using the `timeout` field. Setting the timeout to 0 (either in the runs or in the config defaults) means that there will be no timeout for the TaskRun/PipelineRun (#1040). 

* `TaskRun.Status.Steps.Container` now has a  `ContainerName` field (#1100)

* Allow the definition of a storage class for the artifact pvc using the ConfigMap `config-artifact-pvc` (#1148)

* Tekton now automatically creates directories for each Output resource during a Taskrun (#1156)

* Add a mechanisms to update CRD objects from one version to another (#1083)

* Adds a field called `ImageID` to `TaskRun.Spec.Status.Steps`, which contains the full image ID and digest used to run each step (#1026)

*  The pullRequest resource type now outputs and expects files in a slightly different format. The new format can be found [here](https://github.com/tektoncd/pipeline/blob/v0.6.0/docs/pipelines.md#parameters) (#1181)


### Backwards incompatible changes

#### In current release
* 🚨 Removes `containerTemplate` which was renamed to `stepTemplate` 🚨

Last release introduced this change in a backwards compatible manner, now we are making the change to remove `containerTemplate`(#1174)

* 🚨 Tekton's webhook no longer allows unknown fields in resources 🚨

If you have resources that use fields that were never part of the spec or that have been removed (example: the "trigger" field removed in 0.4.0) then those resources will now fail validation when applied.

#### Warnings for next release
* 🚨 Please migrate to using `podTemplate` instead of using `Affinity`, `Tolerations`, and `NodeSelector` 🚨
TaskRuns and PipelineRuns now contain a `PodTemplate` field and the `Affinity`, `Tolerations`, and `NodeSelector` fields are deprecated and will be removed in the next release (#1004, #1070)

* 🚨First step in changing ${} syntax to $() 🚨
Adds support for $() syntax in addition to ${}; in #1170 we will remove support for ${}. Please migrate to $()! (#1172)

*  🚨 Outputs must be placed into the /workspace/outputs directory and Tekton will no longer copy them from the input directory automatically.🚨 
 We now warn when the same resource is used as an input and an output of the same Task. This will still be a supported case, but the behavior on where files are expected to be will change in a future release (#1119)  

### Fixes
* Fix flaky unit tests (#1072)
* Fix Pipeline Spec validation by actually calling the Validate function (#1074)
* `ClusterTask`s are not validated by the webhook just like any other resource (#1082) 
* Unknown fields in resource YAML/JSON will now be rejected when running "kubectl apply" (#1081)
* Fix controller panic on when a TaskRun's Timeout is nil (#1085)
* Validate `TaskSpec`'s that are embedded in a `TaskRun` (#1084)
* Faster integration tests by switching to `Kaniko` Dockerfile for image digest exporter tests (#1141)
* Fixes a bug that allowed Tekton Tasks to run steps with a stale entrypoint (#1158)
* PR filenames are URL-escaped before being written to disk (#1195).


### Misc
* Reduce the number of imageDigestExporter step to one per Task instead of one for each steps in a Task (#1126)
* Propagate Pipeline labels to Pipeline as soon as possible, so that they are applied even if there is early validation error during reconcile (#1168)
* Changes the type of the Steps field to a type that embeds Container, instead of using the Container type directly (#1060)


## Thanks

Thanks to these contributors who contributed to v0.6.0!

* @afrittoli 
* @cappyzawa 
* @bobcatfish 
* @dlorenc 
* @dibyom 
* @EliZucker 
* @Fabian-K 
* @hongchaodeng 
* @hrishin 
* @ImJasonH 
* @vincent-pli 
* @sbwsg 
* @shuheiktgw 
* @vdemeester 
* @houshengbo 

Extra shout-out for awesome release notes:

* @vdemeester 
* @Fabian-K 
* @dlorenc 
* @EliZucker 
* @houshengbo 
* @ImJasonH 
* @bobcatfish 