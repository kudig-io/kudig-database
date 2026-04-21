# tekton v0.33 Release Notes

Source: [v0.33.4](https://github.com/tektoncd/pipeline/releases/tag/v0.33.4)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.33.4](https://github.com/tektoncd/pipeline/tree/v0.33.4/docs)
-[Examples @ v0.33.4](https://github.com/tektoncd/pipeline/tree/v0.33.4/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.33.4/release.yaml
```

## Attestation

The Rekor UUID for this release is `d77c1b4c638f50249c5dcba385b4600d0f2759a50b7af5f9374101207d4f6797`

Obtain the attestation:
```shell
REKOR_UUID=d77c1b4c638f50249c5dcba385b4600d0f2759a50b7af5f9374101207d4f6797
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | base64 --decode | jq
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.33.4/release.yaml
REKOR_UUID=d77c1b4c638f50249c5dcba385b4600d0f2759a50b7af5f9374101207d4f6797

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | base64 --decode | jq -r '.subject[]|.name + ":v0.33.4@sha256:" + .digest.sha256')

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

Tekton Pipeline release v0.33.3 rebuilt on golang v1.17.8

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

Thanks to these contributors who contributed to v0.33.4!


Extra shout-out for awesome release notes:


<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->