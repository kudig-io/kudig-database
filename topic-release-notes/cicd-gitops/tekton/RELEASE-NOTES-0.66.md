# tekton v0.66 Release Notes

Source: [v0.66.0](https://github.com/tektoncd/pipeline/releases/tag/v0.66.0)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.66.0](https://github.com/tektoncd/pipeline/tree/v0.66.0/docs)
-[Examples @ v0.66.0](https://github.com/tektoncd/pipeline/tree/v0.66.0/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.66.0/release.yaml
```

## Attestation

The Rekor UUID for this release is `108e9186e8c5677aaef800dc5c82c7e8a7dc72d7ed947dc0e166c29c7bfd9f2b6edca989022cb90c`

Obtain the attestation:
```shell
REKOR_UUID=108e9186e8c5677aaef800dc5c82c7e8a7dc72d7ed947dc0e166c29c7bfd9f2b6edca989022cb90c
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.66.0/release.yaml
REKOR_UUID=108e9186e8c5677aaef800dc5c82c7e8a7dc72d7ed947dc0e166c29c7bfd9f2b6edca989022cb90c

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.66.0@sha256:" + .digest.sha256')

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

* :sparkles: Fix StepAction support in Cluster resolver (#8382)

Fix StepAction support in Cluster resolver

* :sparkles: Expose Resolvers Controller performance tuning configurations (#8344)

We can specify custom performance tuning values in the watcher's deployment - controller container via threads-per-controller, kube-api-qps and kube-api-burst flags.



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

* :bug: fix: add missing stepaction RBAC permission for resolver (#8397)

fix: include missing RBAC permission to allow cluster resolver to get and list StepActions

* :bug: Use io.ReadFull to read the bundle content (#8389)

ix an issue on `bundle list` command with relatively big bundles that couldn't be parsed (truncated data)

* :bug: Fix StepAction support in Cluster resolver (#8382)

Fix StepAction support in Cluster resolver

* :bug: Fixes git-resolver configuration for serverUrl and scmType (#8401)
* :bug: Add `Failed Validation` group in message for the status message in PipelineRun (#8356)
* :bug: Run finally pipeline even if task is failed at the validation (#8314)


### Misc




* :hammer: build(deps): bump the all group in /tekton with 2 updates (#8408)
* :hammer: build(deps): bump the all group in /tekton with 2 updates (#8406)
* :hammer: build(deps): bump github.com/golangci/golangci-lint from 1.62.0 to 1.62.2 in /tools (#8405)
* :hammer: build(deps): bump actions/dependency-review-action from 4.4.0 to 4.5.0 (#8404)
* :hammer: build(deps): bump github/codeql-action from 3.27.4 to 3.27.5 (#8403)
* :hammer: build(deps): bump step-security/harden-runner from 2.10.1 to 2.10.2 (#8402)
* :hammer: build(deps): bump the all group in /tekton with 2 updates (#8395)
* :hammer: build(deps): bump github/codeql-action from 3.27.1 to 3.27.4 (#8394)
* :hammer: build(deps): bump github.com/golangci/golangci-lint from 1.61.0 to 1.62.0 in /tools (#8386)
* :hammer: build(deps): bump github/codeql-action from 3.27.0 to 3.27.1 (#8385)
* :hammer: build(deps): bump the all group in /tekton with 3 updates (#8384)
* :hammer: build(deps): bump tj-actions/changed-files from 45.0.3 to 45.0.4 (#8383)
* :hammer: build(deps): bump the all group in /tekton with 4 updates (#8363)
* :hammer: build(deps): bump the all group in /tekton with 4 updates (#8354)
* :hammer: build(deps): bump actions/dependency-review-action from 4.3.4 to 4.4.0 (#8353)
* :hammer: build(deps): bump actions/setup-go from 5.0.2 to 5.1.0 (#8351)
* :hammer: build(deps): bump actions/checkout from 4.2.1 to 4.2.2 (#8350)
* :hammer: build(deps): bump github/codeql-action from 3.26.13 to 3.27.0 (#8349)

### Docs


* :book: Update release.md with v0.65.0 (#8355)

## Thanks

Thanks to these contributors who contributed to v0.66.0!
* :heart: @AverageMarcus
* :heart: @PuneetPunamiya
* :heart: @dependabot[bot]
* :heart: @divyansh42
* :heart: @khrm
* :heart: @vdemeester

Extra shout-out for awesome release notes:
* :heart_eyes: @AverageMarcus
* :heart_eyes: @khrm
* :heart_eyes: @vdemeester

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->