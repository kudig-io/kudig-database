# tekton v0.63 Release Notes

Source: [v0.63.0](https://github.com/tektoncd/pipeline/releases/tag/v0.63.0)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.63.0](https://github.com/tektoncd/pipeline/tree/v0.63.0/docs)
-[Examples @ v0.63.0](https://github.com/tektoncd/pipeline/tree/v0.63.0/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.63.0/release.yaml
```

## Attestation

The Rekor UUID for this release is `108e9186e8c5677a41806e924e8c5d6a3c1e083f8c35950f0d1af7e0e6a4c0712a2eb4bf92e9538e`

Obtain the attestation:
```shell
REKOR_UUID=108e9186e8c5677a41806e924e8c5d6a3c1e083f8c35950f0d1af7e0e6a4c0712a2eb4bf92e9538e
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.63.0/release.yaml
REKOR_UUID=108e9186e8c5677a41806e924e8c5d6a3c1e083f8c35950f0d1af7e0e6a4c0712a2eb4bf92e9538e

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.63.0@sha256:" + .digest.sha256')

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

* :sparkles: cluster-reslover: add support for StepAction (#8199)

tepAction are now supported to a refered via the cluster resolver.

* :sparkles: Allow securityContext field for affinity assistant podtemplate (#8176)

Added the ability to set the pod-level `securityContext` for the AffinityAssistant StatefulSet. 
This can be configured by providing a `default-affinity-assistant-pod-template` in the `config-defaults` ConfigMap or by specifying a pod template in `TaskRun` or `PipelineRun`.

* :sparkles: Add UID label to PipelineRun and TaskRun (#8166)

TaskRun pods have tekton.dev/taskRunUID and tekton.dev/pipelineRunUID labels


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

* :bug: Fix Artifact type to a pointer (#8226)

Fix Artifact type to a pointer.

* :bug: fix task name show  in metric (#8216)

fix clusterTask name show `anonymous` in metric

* :bug: apply default-container-resource-requirements before LimitRange transformer (#8197)

[Bug fix]: `default-container-resource-requirements` will be applied to the container before `LimtRange`

* :bug: fix(pipelinerun): resolve issue where canceling active pipelinerun fails (#8173)

fix(pipelinerun): resolve issue where canceling active pipelinerun fails

* :bug: fix(taskrun): resolve issue with TaskRun not failing promptly after Pod OOM (#8171)

fix(taskrun): resolve issue with TaskRun not failing promptly after Pod OOM

* :bug: fix: If the finally timeout is set to 0s, the  calculates the next queue entry time according to the pipeline timeout. (#8056)

If the finally timeout is set to 0s, the `reconciler` calculates the next queue entry time according to the pipeline timeout.

* :bug: feat(matrix): Fix matrix param type mismatch problem for ref array result from customrun scenario (#8024)


### Misc




* :hammer: build(deps): bump tj-actions/changed-files from 44.5.7 to 45.0.0 (#8223)
* :hammer: build(deps): bump github.com/golangci/golangci-lint from 1.59.1 to 1.60.3 in /tools (#8219)
* :hammer: build(deps): bump github.com/docker/docker from 26.1.3+incompatible to 26.1.5+incompatible (#8218)
* :hammer: Bump the all group in /tekton with 4 updates (#8212)
* :hammer: Bump github/codeql-action from 3.26.0 to 3.26.3 (#8211)
* :hammer: Bump the all group in /tekton with 4 updates (#8204)
* :hammer: Bump actions/upload-artifact from 4.3.5 to 4.3.6 (#8203)
* :hammer: Bump step-security/harden-runner from 2.9.0 to 2.9.1 (#8202)
* :hammer: Bump github/codeql-action from 3.25.15 to 3.26.0 (#8201)
* :hammer: {taskrun,pipelinerun}metrics: make sure config is up-to-date (#8187)
* :hammer: Bump the all group in /tekton with 2 updates (#8180)
* :hammer: Bump actions/upload-artifact from 4.3.4 to 4.3.5 (#8179)
* :hammer: Bump tj-actions/changed-files from 44.5.5 to 44.5.7 (#8178)
* :hammer: Bump github/codeql-action from 3.25.13 to 3.25.15 (#8162)
* :hammer: Bump ossf/scorecard-action from 2.3.3 to 2.4.0 (#8161)
* :hammer: Bump the all group in /tekton with 4 updates (#8160)
* :hammer: Bump go.opentelemetry.io/otel/sdk from 1.27.0 to 1.28.0 (#8154)

### Docs


* :book: docs: fix links to Matrix examples (#7953)

## Thanks

Thanks to these contributors who contributed to v0.63.0!
* :heart: @AverageMarcus
* :heart: @chengjoey
* :heart: @chitrangpatel
* :heart: @cugykw
* :heart: @dependabot[bot]
* :heart: @hittyt
* :heart: @jkandasa
* :heart: @khrm
* :heart: @kristofferchr
* :heart: @l-qing
* :heart: @vdemeester

Extra shout-out for awesome release notes:
* :heart_eyes: @chengjoey
* :heart_eyes: @chitrangpatel
* :heart_eyes: @cugykw
* :heart_eyes: @jkandasa
* :heart_eyes: @khrm
* :heart_eyes: @kristofferchr
* :heart_eyes: @l-qing
* :heart_eyes: @vdemeester

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->