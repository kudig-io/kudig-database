# tekton v0.55 Release Notes

Source: [v0.55.0](https://github.com/tektoncd/pipeline/releases/tag/v0.55.0)

# 🎉 PipelineTask.OnError and bugfixes 🎉

-[Docs @ v0.55.0](https://github.com/tektoncd/pipeline/tree/v0.55.0/docs)
-[Examples @ v0.55.0](https://github.com/tektoncd/pipeline/tree/v0.55.0/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.55.0/release.yaml
```

## Attestation

The Rekor UUID for this release is `24296fb24b8ad77acf6e7f5cf38da4c2178e88e08bc2f291dc52b756371a21d349ca985bd125ace9`

Obtain the attestation:
```shell
REKOR_UUID=24296fb24b8ad77acf6e7f5cf38da4c2178e88e08bc2f291dc52b756371a21d349ca985bd125ace9
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.55.0/release.yaml
REKOR_UUID=24296fb24b8ad77acf6e7f5cf38da4c2178e88e08bc2f291dc52b756371a21d349ca985bd125ace9

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.55.0@sha256:" + .digest.sha256')

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

* :sparkles: TEP-0142: Introduce WorkingDir in StepActions (#7461)

Introduce WorkingDir in StepActions

* :sparkles: Support overriding the SCM type and server URL (#7450)

User are now able to override the global server URL when using the git resolver to allow fetching from multiple git providers.

* :sparkles: [TEP-0050] Implement PipelineTask OnError (#7422)

Implement "Ignore Task Failure" with new "PipelineTask.OnError" API field (TEP-0050). User can now set `pipelineTask.onError: continue` to ignore failure


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

* :bug: Fix enum validation with multiple param references (#7481)

bug fix: allow task-level param references multiple pipeline-level params with enum

* :bug: Fix: do not fail TaskRun for concurrent modification errors (#7467)

fix: taskRuns will not fail for concurrent modification errors when stopping sideCars

* :bug: Fix validations for Sidecars to be consistent (#7443)

sidecars are now validated at admission webhook

* :bug: fix: resolve issue where resolutionrequest defaulted to v1alpha1 vs v1beta1 (#7438)

Resolved issue where resolutionrequest defaulted to v1alpha1 when it should be v1beta1

* :bug: Do not require  for entrypoint cancellation (#7430)

ntrypoint cancellation only requires `keep-pod-on-cancel: true` feature-flag.

* :bug: Freeze image sha for dind-sidecar example test. (#7498)
* :bug: FIX: Prevent panic on parameter evaluation (#7488)
* :bug: change ResultRef.ResultsIndex from int to *int (#7460)
* :bug: don't return validation error when final tasks failed/skipped (#7407)


### Misc


* :hammer: Tracing: Add credentialsSecret for basic authentication to remote endpoint (#7238)

Tracing config now includes an additional optional field `credentialsSecret` where users can specify the name of a secret. The username and password fields from the secret will be used to authenticate against Tracing collector endpoint.

* :hammer: [TEP-0131] Update Conformance Spec for v1 api types (#7224)

Updates the conformance api spec with v1 api types in line with TEP-131
* :hammer: Error sweep: complete user-facing error messages formats (#7474)
* :hammer: Replace PipelineRunReasonFailedValidation with more granular reasons (#7417)
* :hammer: Error sweep: Move TaskRun Reasons in pkg/pod to pkg/apis (#7406)
* :hammer: Cleanup error message for Step container status (#7405)
* :hammer: Error sweep: add more context for PipelineRunCouldntGetPipeline error message (#7403)
* :hammer: Error sweep: refactor steps termination when failing TaskRun (#7386)
* :hammer: Refactor common util functions for /test (#7369)
* :hammer: Fix showing error message when validation fail (#7509)
* :hammer: Git resolver: validate repo URL (#7482)
* :hammer: Bump github.com/spiffe/spire-api-sdk from 1.8.4 to 1.8.5 (#7463)
* :hammer: Fix some spelling in stepactions.md (#7432)
* :hammer: Remove .envrc and show an example in .envrc.sample (#7429)
* :hammer: Create scorecard.yml (#7409)
* :hammer: Add @jeromeJu as a pipelines maintainer (#7327)
* :hammer: Bump github.com/google/uuid from 1.3.1 to 1.4.0 (#7308)

### Docs


* :book: Add `stdoutConfig` and `stderrConfig` to alpha features table (#7494)
* :book: Fix step actions documentation (#7492)
* :book: [TEP-0050] Add Ignore Task Failure to alpha table (#7468)
* :book: Update Feature Flags Documentation (#7445)
* :book: Update StepActions Documentation (#7441)
* :book: Fix typos and broken links in StepActions doc (#7431)
* :book: Make git resolver label explicit (#7428)
* :book: Update release with v0.54.0 (#7427)
* :book: Update release cheat sheet (#7425)

## Thanks

Thanks to these contributors who contributed to v0.55.0!
* :heart: @AlanGreene
* :heart: @JeromeJu
* :heart: @QuanZhang-William
* :heart: @Yongxuanzhang
* :heart: @aaron-prindle
* :heart: @afrittoli
* :heart: @chitrangpatel
* :heart: @chmouel
* :heart: @dependabot[bot]
* :heart: @dibyom
* :heart: @jerop
* :heart: @joaosilva15
* :heart: @kmjayadeep
* :heart: @vdemeester

Extra shout-out for awesome release notes:
* :heart_eyes: @JeromeJu
* :heart_eyes: @QuanZhang-William
* :heart_eyes: @aaron-prindle
* :heart_eyes: @chitrangpatel
* :heart_eyes: @chmouel
* :heart_eyes: @dibyom
* :heart_eyes: @kmjayadeep
* :heart_eyes: @vdemeester

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->