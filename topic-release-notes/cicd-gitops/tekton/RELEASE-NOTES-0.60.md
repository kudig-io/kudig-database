# tekton v0.60 Release Notes

Source: [v0.60.2](https://github.com/tektoncd/pipeline/releases/tag/v0.60.2)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.60.2](https://github.com/tektoncd/pipeline/tree/v0.60.2/docs)
-[Examples @ v0.60.2](https://github.com/tektoncd/pipeline/tree/v0.60.2/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.60.2/release.yaml
```

## Attestation

The Rekor UUID for this release is `24296fb24b8ad77a1b1da05e47cee68581daf1cd5823facc5b59b76edaf9ce986efe5c68bd1a4cbe`

Obtain the attestation:
```shell
REKOR_UUID=24296fb24b8ad77a1b1da05e47cee68581daf1cd5823facc5b59b76edaf9ce986efe5c68bd1a4cbe
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.60.2/release.yaml
REKOR_UUID=24296fb24b8ad77a1b1da05e47cee68581daf1cd5823facc5b59b76edaf9ce986efe5c68bd1a4cbe

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.60.2@sha256:" + .digest.sha256')

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

- 🐛 [release-v0.60.x] Fix: Identify workspace usage in a Task (#8021)



### Misc






### Docs




## Thanks

Thanks to these contributors who contributed to v0.60.2!
* :heart: @tekton-robot, @chitrangpatel 

Extra shout-out for awesome release notes:
* :heart_eyes: @tekton-robot

<!--
## Unsorted PR List
- [release-v0.60.x] Fix: Identify workspace usage in a Task (#8021)

To Be Done: Deprecation Notices, Backward Incompatible Changes
-->