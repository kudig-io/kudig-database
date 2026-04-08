# tekton v0.52 Release Notes

Source: [v0.52.1](https://github.com/tektoncd/pipeline/releases/tag/v0.52.1)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.52.1](https://github.com/tektoncd/pipeline/tree/v0.52.1/docs)
-[Examples @ v0.52.1](https://github.com/tektoncd/pipeline/tree/v0.52.1/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.52.1/release.yaml
```

## Attestation

The Rekor UUID for this release is `24296fb24b8ad77a97c22594268cc45d986246339ada304b7587b205b59cf5d59df2650d24b14825`

Obtain the attestation:
```shell
REKOR_UUID=24296fb24b8ad77a97c22594268cc45d986246339ada304b7587b205b59cf5d59df2650d24b14825
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.52.1/release.yaml
REKOR_UUID=24296fb24b8ad77a97c22594268cc45d986246339ada304b7587b205b59cf5d59df2650d24b14825

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.52.1@sha256:" + .digest.sha256')

# Download the release file
curl "$RELEASE_FILE" > release.yaml

# For each image in the attestation, match it to the release file
for image in $REKOR_ATTESTATION_IMAGES; do
  printf $image; grep -q $image release.yaml && echo " ===> ok" || echo " ===> no match";
done
```

## Changes

### Fixes

* :bug: [release-v0.52.x] Regression: fix results with out of order tasks (#7174)

Fix regression where a different order of task definition may cause result resolution to break


## Thanks

Thanks to these contributors who contributed to v0.52.1!
* :heart: @afrittoli 
* :heart: @tekton-robot

Extra shout-out for awesome release notes:
* :heart_eyes: @afrittoli 
* :heart_eyes: @tekton-robot