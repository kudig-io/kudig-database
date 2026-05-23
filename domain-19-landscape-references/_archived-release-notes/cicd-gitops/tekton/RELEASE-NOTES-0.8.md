---
title: tekton v0.8 Release Notes
description: tekton v0.8 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- opa
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tekton v0.8 Release Notes 是什么
- 如何 tekton v0.8 Release Notes
trigger_keywords:
- tekton
- v0.8
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
created: "2026-05-23"
---

# tekton v0.8 Release Notes

Source: [v0.8.0](https://github.com/tektoncd/pipeline/releases/tag/v0.8.0)

# 🎉 Embedded Resource and Pipeline Specs, S3 Artifact Support, Pipeline Metrics & More! 🎉

-[Docs @ v0.8.0](https://github.com/tektoncd/pipeline/tree/v0.8.0/docs#tekton-pipelines)
-[Examples @ v0.8.0](https://github.com/tektoncd/pipeline/tree/v0.8.0/examples)

## Changes

### Features

* :sparkles: Allow PipelineResource implementations to modify the entire Pod spec.

This change simplifies the interface by removing the GetUpload/Download container and volume methods and replaces it with a more generic "modifier" system. (#1345)


* :sparkles: Adding support to enable resourceSpec.

Its now possible to embed resourceSpec into PipelineRun. (#1324)


* :sparkles: Add support for specifiying "0" as no-timeout for PipelineRuns.

This was already done in #1040 for TaskRuns, but PipelineRuns seem to have been missed. (#1365)


* :sparkles: Add checking insecure flag when creating pipeline resources.

If insecure flag is true, user can create ClusterResource without cadata. (#1354)


* :sparkles: Resolve all PipelineResources first before continuing

For folks importing the client libraries, when ResourceBindings are instantiated for TaskRuns, they now need to explicitly declare the newly embedded struct PipelineResourceBinding. (#1353)


* :sparkles: Adding support to enable pipelineSpec

Its now possible to embed the whole pipeline specification into Pipeline Run using pipelineSpec (#1333)


* :sparkles: Provide a way to specify default [[Service|service]]service accounts]]

If pipeline controllers are deployed with a config-map that has `default-service-account` key set to a non-empty string, pipeline-runs that do not specify a ServiceAccount will be modified (mutated) to  the value of the `default-service-account`. (#1227)


* :sparkles: Support runtimeClassName in pod templates

This change adds support for the [[Kubernetes|Kubernetes]] 1.12+ runtime class feature by adding the runtimeClassName field to pod templates and propagating that to the underlying pod spec. (#1363)


* :sparkles: Add support for using S3-compatible APIs instead of GCS by passing through a boto configuration file.

It is now possible to use S3-compatible APIs instead of GCS for GCS storage resources. (#1361)


* :sparkles: Update TaskRunStatus.ResourcesResult to be more generic.

    - The Name and Digest fields on TaskRunStatus.ResourcesResult are deprecated and are replaced by the new Key and ResourceRef fields.
    - Both sets of fields are present in this release, but a future release will remove the legacy fields.


* :sparkles: Add pipeline metrics 🔭 (#1387)

Following Pipelines metrics are available at `tekton-pipelines-controller:9090/metrics` endpoint
1. Taskrun/Pipeline execution duration time
2. Pipelinerun/Taskrun `success` and `failure` duration rates
3. Number of Taskruns and Pipelineruns are executing currently
4. Pod scheduling latency for Taskruns 


* :sparkles: Refactor Resource result output, and add support for Git resources.

The 'Git' PipelineResource now populates the taskRun.status.resourcesResult field with the commit used. (#1424)


* :sparkles: Support multiple SSH keys for the same host

Allow multiple SSH-auth secrets annotated for the same host (#1433)


# Deprecation Notices

* 🚨 The "Name" and "Digest" fields on TaskRunStatus.ResourcesResult are deprecated

The Name and Digest fields on TaskRunStatus.ResourcesResult are deprecated and are replaced by the new Key and ResourceRef fields.

* 🚨 ServiceAccountName(s) replaces ServiceAccount(s)

The `serviceAccount` field is deprecated.  Use `serviceAccountName` instead.


# Fixes

* :bug: Correct pod watching in Taskrun controller (#1269)
* :bug: Clean up YAML tests (#1351)
* :bug: Some of the task and pipeline names had capital letters that were invalid (#1381)
* :bug: Remove the gitlab example taskrun. (#1403)
* :bug: Add support for comment and label manifests. (#1408)
    - Fixes bug where PullRequestResource could accidentally delete newly created upsteam resources in certain race conditions.


# Misc

* :hammer: Include vendored source in release-built images (#1338)
* :hammer: Fix line breaks in PR template (#1337)
* :hammer: Actually fix PR template line breaks (#1346)
* :hammer: Tekton 0.3.1 does not support $() syntax (#1339)
* :hammer: Fix some style issues noticed after #1345 was merged. (#1356)
* :hammer: Use Tekton's nightly-built build-base image (#1352)
* :hammer: Enable the "gosec" linter for CI, and fix the one issue in our code. (#1359)
* :hammer: Avoid cases when comparing in TestGitPipelineRun. (#1362)
* :hammer: Enable "gocritic" in CI, and fix associated errors. (#1360)
* :hammer: Move nopImage and entrypointImage from pkg/… package to cmd/controller (#1348)
* :hammer: Inline the `ResourceDeclaration` struct in `TaskResource` (#1366)
* :hammer: Update PipelineSpec Task name and TaskRef name validation to prevent errors at runtime (#1358)
* :hammer: Helpful error message when multiple volumes share name (#1404)
* :hammer: Enable the unparam linter, and fix outstanding issues. (#1388)
* :hammer: Add vendored-source logic to full release pipeline. (#1340)
* :hammer: Add logging to TimeoutHandler (#1335)
* :hammer: Use a local registry in build-push-kaniko (#1415)
* :hammer: Only mount artifact bucket volume once, even with multiple inputs. (#1370)
* :hammer: Use kubectl create instead of apply (#1398)
* :hammer: Use a subfolder in the release bucket (#1391)
* :hammer: Adapt the release pipeline to Tekton v0.7.0+ (#1421)
* :hammer: Allow entrypoint binary to wait for multiple files (#1430)
* :hammer: Remove unused results field from pr and tr specs (#1425)
* :hammer: Use correct version number for release link (#1428)
* :hammer: Fail test if resource creation fails (#1399)
* :hammer: Set defaults for Tasks embedded in TaskRuns (#1431)
* :hammer: upgrade executor version (#1435)
* :hammer: Use the pre-release check task from plumbing (#1434)


# Docs

* :book: Small fixes to the release guide (#1322)
* :book: Add docker for desktop and minikube instructions (#1326)
* :book: Fix export comments (#1342)
* :book: Remove Docker Edge requirement from tutorial (#1385)
* :book: kubectl apply not work for examples with the genereateName (#1382)
* :book: Fix a tiny typo (#1390)
* :book: Added URL for permission (#1436)
* :book: Fix Typo in docs (#1442)


## Thanks

# Thanks to these contributors who contributed to v0.8.0!

* :heart: @16yuki0702
* :heart: @afrittoli
* :heart: @akihikokuroda
* :heart: @bobcatfish
* :heart: @cappyzawa
* :heart: @chandanikumari
* :heart: @chmouel
* :heart: @danielhelfand
* :heart: @dibyom
* :heart: @dlorenc
* :heart: @fraenkel
* :heart: @hrishin
* :heart: @ImJasonH
* :heart: @impl
* :heart: @jbarrick-mesosphere
* :heart: @mnuttall
* :heart: @moredhel
* :heart: @nlewo
* :heart: @pritidesai
* :heart: @pwplusnick
* :heart: @sbwsg
* :heart: @vdemeester
* :heart: @vincent-pli
* :heart: @vtereso
* :heart: @withlin
* :heart: @wlynch
