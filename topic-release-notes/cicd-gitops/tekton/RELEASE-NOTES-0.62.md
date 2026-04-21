# tekton v0.62 Release Notes

Source: [v0.62.9](https://github.com/tektoncd/pipeline/releases/tag/v0.62.9)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.62.9](https://github.com/tektoncd/pipeline/tree/v0.62.9/docs)
-[Examples @ v0.62.9](https://github.com/tektoncd/pipeline/tree/v0.62.9/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.62.9/release.yaml
```

## Attestation

The Rekor UUID for this release is `108e9186e8c5677a7ff10d12220d6155b84235df4518ed42400668179ccaaacc93e9631f44868e03`

Obtain the attestation:
```shell
REKOR_UUID=108e9186e8c5677a7ff10d12220d6155b84235df4518ed42400668179ccaaacc93e9631f44868e03
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.62.9/release.yaml
REKOR_UUID=108e9186e8c5677a7ff10d12220d6155b84235df4518ed42400668179ccaaacc93e9631f44868e03

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.62.9@sha256:" + .digest.sha256')

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

* :bug: [release-v0.62.x] fix: Fix remote task params default-value substitution (#8649)

ask Param defaults will now be correctly substituted in Steps when the Task is referenced by a TaskRun




### Misc




* :hammer: [release-v.62.x] .github/workflows: add a build and test workflows (#8581)

### Docs




## Thanks

Thanks to these contributors who contributed to v0.62.9!
* :heart: @tekton-robot
* :heart: @vdemeester

Extra shout-out for awesome release notes:
* :heart_eyes: @tekton-robot

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->