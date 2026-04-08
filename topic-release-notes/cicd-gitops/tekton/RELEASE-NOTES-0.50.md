# tekton v0.50 Release Notes

Source: [v0.50.6](https://github.com/tektoncd/pipeline/releases/tag/v0.50.6)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.50.6](https://github.com/tektoncd/pipeline/tree/v0.50.6/docs)
-[Examples @ v0.50.6](https://github.com/tektoncd/pipeline/tree/v0.50.6/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.50.6/release.yaml
```

## Attestation

The Rekor UUID for this release is `24296fb24b8ad77ab39f94a9c6627f1ae85c428863d0dbdbea4c9481976f30c00d5f9f712a117720`

Obtain the attestation:
```shell
REKOR_UUID=24296fb24b8ad77ab39f94a9c6627f1ae85c428863d0dbdbea4c9481976f30c00d5f9f712a117720
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.50.6/release.yaml
REKOR_UUID=24296fb24b8ad77ab39f94a9c6627f1ae85c428863d0dbdbea4c9481976f30c00d5f9f712a117720

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.50.6@sha256:" + .digest.sha256')

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

* :bug: [release-v0.50.x] Remove conversion configuration for  (#7798)

emove conversion webhook configuration from the ClusterTask CRD, it doesn't need it.

* :bug: [release-v0.50.x] fix: ensure clustertask annotations are synced to taskrun (#7656)

ix: ensure `ClusterTask` annotations and labels are synced to `TaskRun`

* :bug: [release-v0.50.x] Fix validations for Sidecars to be consistent (#7451)

idecars are now validated at admission webhook

* :bug: [release-v0.50.x] don't return validation error when final tasks failed/skipped (#7485)

- [release-v0.50.x] chore(deps): Migrate to github.com/go-jose/go-jose/v3 (#7858)
- [release-v0.50.x] Update go-git/v5 for CVE-2023-49569 (#7839)

### Misc






### Docs




## Thanks

Thanks to these contributors who contributed to v0.50.6!
* :heart: @tekton-robot
* :heart: @vdemeester

Extra shout-out for awesome release notes:
* :heart_eyes: @tekton-robot
* :heart_eyes: @vdemeester

<!--
## Unsorted PR List

To Be Done: Deprecation Notices, Backward Incompatible Changes
-->