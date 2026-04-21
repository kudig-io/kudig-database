# tekton v0.4 Release Notes

Source: [v0.4.0](https://github.com/tektoncd/pipeline/releases/tag/v0.4.0)

* [Docs @ v0.4.0](https://github.com/tektoncd/pipeline/tree/v0.4.0/docs#tekton-pipelines)
* [Examples @ v0.4.0](https://github.com/tektoncd/pipeline/tree/v0.4.0/examples)

This is the third dogfood released version of Tekton Pipelines, where the images were built, pushed and tagged using a Task!

## Changes

### Breaking Changes
- Remove the Trigger field from TaskRun/PipelineRun [#857](https://github.com/tektoncd/pipeline/pull/857)

### Features
- Propagate annotations from Pipeline/Task to PipelineRun/TaskRun [#894](https://github.com/tektoncd/pipeline/pull/894)
- Generalize messages when TaskRun errs during pod creation [#891](https://github.com/tektoncd/pipeline/pull/891)
- Allow configuration of PVC size from ConfigMap [#866](https://github.com/tektoncd/pipeline/pull/866)
- Add support for taskRun to expose digest of built images [#721](https://github.com/tektoncd/pipeline/pull/721)

### Fixes

- Correct setting for kaniko in tasks [#883](https://github.com/tektoncd/pipeline/pull/883)
- Fix documentation section about installing in custom namespace [#881](https://github.com/tektoncd/pipeline/pull/881)
- Surface resource constraint problems in TaskRun Status [#876](https://github.com/tektoncd/pipeline/pull/876)
- Enforce Default TaskRun Timeout [#871](https://github.com/tektoncd/pipeline/pull/871)
- Update doc to remove taskSpec as an alternative to pipelineRef in a pipelineRun [#870](https://github.com/tektoncd/pipeline/pull/870)

### Misc

- Add Type-Level godocs for API types [#921](https://github.com/tektoncd/pipeline/pull/921)
- Add godoc for missing Pipeline fields [#920](https://github.com/tektoncd/pipeline/pull/920)
- Use tektoncd build-base base image for the release [#919](https://github.com/tektoncd/pipeline/pull/919)
- Cleanup of initializing working dirs [#843](https://github.com/tektoncd/pipeline/pull/843)

## Thanks

Thanks to these contributors who contributed to v0.4.0!

- @pmorie
- @mattmoor
- @bobcatfish
- @vdemeester
- @houshengbo
- @vincent-pli
- @carlosgg
- @willbeason
- @nader-ziada
- @abayer
- @sbwsg
- @steveodonovan
- @zouyee
- @sthana
- @josephlewis42
- @abergmeier
- @dicarlo2
- @xtreme-sameer-vohra
- @richardmarshall
- @joseblas
- @ImJasonH
- @paassdc
- @Gl4di4torRr 
