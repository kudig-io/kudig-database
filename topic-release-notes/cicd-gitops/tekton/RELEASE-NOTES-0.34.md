# tekton v0.34 Release Notes

Source: [v0.34.1](https://github.com/tektoncd/pipeline/releases/tag/v0.34.1)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.34.1](https://github.com/tektoncd/pipeline/tree/v0.34.1/docs)
-[Examples @ v0.34.1](https://github.com/tektoncd/pipeline/tree/v0.34.1/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.34.1/release.yaml
```

## Attestation

The Rekor UUID for this release is `95e22c05299c9a60e2b4e8bc6a18017b0a8b4da3bc222fd945f7b407979108d1`

Obtain the attestation:
```shell
REKOR_UUID=95e22c05299c9a60e2b4e8bc6a18017b0a8b4da3bc222fd945f7b407979108d1
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | base64 --decode | jq
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.34.1/release.yaml
REKOR_UUID=95e22c05299c9a60e2b4e8bc6a18017b0a8b4da3bc222fd945f7b407979108d1

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | base64 --decode | jq -r '.subject[]|.name + ":v0.34.1@sha256:" + .digest.sha256')

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

Tekton Pipelines v0.34.0 rebuilt on golang v1.17.8

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

Thanks to these contributors who contributed to v0.34.1!


Extra shout-out for awesome release notes:


<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->