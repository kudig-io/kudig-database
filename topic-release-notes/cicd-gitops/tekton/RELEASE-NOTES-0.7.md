---
title: tekton v0.7 Release Notes
description: tekton v0.7 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v0.7 Release Notes 是什么
- 如何 tekton v0.7 Release Notes
trigger_keywords:
- tekton
- v0.7
- Release
- Notes
- release
- notes
---

# tekton v0.7 Release Notes

Source: [v0.7.0](https://github.com/tektoncd/pipeline/releases/tag/v0.7.0)

# 🎉 Nightly Releases, Sidecars for Tasks, CloudEvent pipeline resource, and much much more! 🎉

-[Docs @ v0.7.0](https://github.com/tektoncd/pipeline/tree/v0.7.0/docs#tekton-pipelines)
-[Examples @ v0.7.0](https://github.com/tektoncd/pipeline/tree/v0.7.0/examples)

## Changes

### Features

* :sparkles: Add nightly release pipeline 🌙. 

Nightly build artifacts are available at gs://tekton-release-nightly and gcr.io/tekton-nightly (#1274).

* :sparkles: Add namespace to cluster resource (#1255).

* :sparkles: Verify if pipeline works after upgrading from previous release to current release. 

This introduces upgrade testing 🎉🎉 (#1162).

* :sparkles: Allow declaring and passing resources to conditions. 

Adds resource support for conditionals. Conditions have to declare the resources they can use and these can be passed in via the Pipeline and PipelineRun spec similar to TaskResources. (#1151)

* :sparkles: Add sidecars to Tasks

Tasks can now define a list of sidecar containers to run alongside their steps (#1236)

* :sparkles: Update build-gcs resource type to support .tar.gz archives (#1200)

* :sparkles: Cloud Event output resource (#837)

New output resource CloudEventPipelineResource available.
When used in a `Task`, it will generate a cloud event upon completion of any `TaskRun`
that references that `Task`. The cloud event includes the entire body of the `TaskRun`.

# Deprecation Notices
    
* 🚨 Deprecation Notice for Results Field for PipelineRuns and TaskRuns

Adding code comments to bring awareness to Results field removal in v0.8.0 (#1278).

* 🚨 Update build-gcs resource type to support .tar.gz archives

build-gcs resources will no longer support `artifactType` `Archive`, use `ZipArchive` and `TarGzArchive` instead (#1200)

# Backwards incompatible changes

In current release:

* 🚨 Support for ${} syntax removed in favor of $()

${} syntax will no longer perform variable replacement, $() must be used instead. Last release introduced this change in a backwards compatible manner, by supporting both $() and ${}; now we are making the change to remove support for the ${} syntax (#1311).

* 🚨Change the behavior of outputs that are also used as inputs.

Tasks that take input and output resources of the same type must now copy or move the resource from the input directory to the output directory manually. Tekton no longer automatically reads outputs from the input directory when the same resource is supplied in both places (#1122).

* 🚨Remove deprecated podSpec field in favor of podTemplate.

Remove deprecated podSpec (nodeSelector, affinity and tolerations) fields in favor of podTemplate (#1299).

* 🚨Update build-gcs resource type to support .tar.gz archives

build-gcs resources cannot be used as an output resource (#1200)

# Fixes

* :bug: Annotate TargetPath and OutputImageDir with omitempty (#1225)
* :bug: Accept any sidecar termination reason (#1265)
* :bug: Support Condition only resources in PipelineTask (#1270)
* :bug: Made the digest exporter report image digest if there is only one image. (#1237)
* :bug: Fix and work around timeout handler data races (#1308): 
   - Removes potential race conditions in timeout handler
   - It drops logging from timeout_handler. We'll attempt to restore logging in a thread-safe manner in the future.

# Misc

* :hammer: Remove "Building" status reason (#1226).
* :hammer: Refactor reconciler package to not reference api version (#1216)
* :hammer: Bump github.com/knative/pkg dependency and deps… (#1117)
* :hammer: Add a using resources section in resources.md (#1257)
* :hammer: Update xerrors dependency (#1277)
* :hammer: Use ubuntu images for sidecar tests (#1254)
* :hammer: Increase linter timeout to 3 minutes (#1318)
* :hammer: Only compare ImageID suffix in TestTaskRunStatus test (#1233)
* :hammer: Add YAML test for non-build-gcs GCS resource (#1223)
* :hammer: Delete gopath-test taskrun example / YAML test (#1224)
* :hammer: Rename taskrun-github-pr-yaml totaskrun-github-pr.yaml. (#1218)
* :hammer: Refactor input resource volume handling to remove a type switch statement (#1139)
* :hammer: Add GcsFetcher and GcsUploader images to release task and pipeline (#1196)
* :hammer: Fix release pipeline to handle #1122 (#1327)
* :hammer: Emit pipelinerun event when it is cancelled (#1230)
* :hammer: Add managed-by label to Pods created from TaskRuns (#1329)
* :hammer: Update Deployments to use the apps/v1 API version (#1330)

# Docs

* :book: Fix #1211 - Remove reference to yqArg in pipeline tutorial (#1212)
* :book: Fix name variables in cluster resource doc. (#1261)
* :book: Fix typos in release doc. (#1317)
* :book: Update release README numbering (#1321)
* :book: Typos and correctness fixes for creds-init CLI doc (#1297)
* :book: Enhancements for PullRequest Resource docs (#1276)
* :book: Fix Pull Request resource example url. (#1316)
* :book: Add links to 0.6.0 docs (#1203)
* :book: [Doc] Fix typos and link errors for pipelines.md (#1245)
* :book: Correct a small error in tasks.md readme (#1217)
* :book: Fix a few comments (#1207)
* :book: Update v0.6.0 docs to include fixed tutorial 🤒 (#1208)
* :book: Fix braces in tutorial 😭 (#1206)
* :book: Doc updates for typos and clearification (#1192)

## Thanks

Thanks to these contributors who contributed to v0.7.0!

* :heart: @EliZucker
* :heart: @ImJasonH
* :heart: @Letty5411
* :heart: @afrittoli
* :heart: @ahpook
* :heart: @bobcatfish
* :heart: @cappyzawa
* :heart: @chhsia0
* :heart: @chmouel
* :heart: @danielhelfand
* :heart: @dibyom
* :heart: @dlorenc
* :heart: @gavinfish
* :heart: @houshengbo
* :heart: @hrishin
* :heart: @moficodes
* :heart: @sbwsg
* :heart: @tejal29
* :heart: @vdemeester
* :heart: @vtereso

Extra shout-out for awesome release notes:
* :heart_eyes: @bobcatfish
* :heart_eyes: @dlorenc
* :heart_eyes: @vdemeester
* :heart_eyes: @danielhelfand
* :heart_eyes: @chhsia0
* :heart_eyes: @cappyzawa
* :heart_eyes: @dibyom
* :heart_eyes: @sbwsg
* :heart_eyes: @ImJasonH
* :heart_eyes: @afrittoli