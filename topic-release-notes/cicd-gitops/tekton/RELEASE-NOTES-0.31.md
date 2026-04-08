# tekton v0.31 Release Notes

Source: [v0.31.4](https://github.com/tektoncd/pipeline/releases/tag/v0.31.4)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.31.4](https://github.com/tektoncd/pipeline/tree/v0.31.4/docs)
-[Examples @ v0.31.4](https://github.com/tektoncd/pipeline/tree/v0.31.4/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.31.4/release.yaml
```

## Attestation

The Rekor UUID for this release is `34c51da902ac3809cabe793c88a66863eff038a74275ee9f51c83e47d6f0b9b1`

Obtain the attestation:
```shell
REKOR_UUID=34c51da902ac3809cabe793c88a66863eff038a74275ee9f51c83e47d6f0b9b1
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | base64 --decode | jq
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.31.4/release.yaml
REKOR_UUID=34c51da902ac3809cabe793c88a66863eff038a74275ee9f51c83e47d6f0b9b1

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | base64 --decode | jq -r '.subject[]|.name + ":v0.31.4@sha256:" + .digest.sha256')

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

Tekton Pipeline v0.32.4 rebuilt with golang 1.17.8

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






### Misc






### Docs




## Thanks

Thanks to these contributors who contributed to v0.31.4!


Extra shout-out for awesome release notes:


<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->