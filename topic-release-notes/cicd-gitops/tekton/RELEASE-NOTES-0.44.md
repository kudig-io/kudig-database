# tekton v0.44 Release Notes

Source: [v0.44.5](https://github.com/tektoncd/pipeline/releases/tag/v0.44.5)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v0.44.5](https://github.com/tektoncd/pipeline/tree/v0.44.5/docs)
-[Examples @ v0.44.5](https://github.com/tektoncd/pipeline/tree/v0.44.5/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.44.5/release.yaml
```

## Attestation

The Rekor UUID for this release is `24296fb24b8ad77ae6d4a97d973af478bc9cefd6f575761773249d2706bf3d35bc7b81a7cc481fcf`

Obtain the attestation:
```shell
REKOR_UUID=24296fb24b8ad77ae6d4a97d973af478bc9cefd6f575761773249d2706bf3d35bc7b81a7cc481fcf
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.44.5/release.yaml
REKOR_UUID=24296fb24b8ad77ae6d4a97d973af478bc9cefd6f575761773249d2706bf3d35bc7b81a7cc481fcf

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.44.5@sha256:" + .digest.sha256')

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

* :bug: [cherry-pick-v0.44.x] Fix PipelineRun reconciler panic for computed timeouts (#6999)

bug fix: Avoid controller panics for computed timeouts


- [v0.44.x] Bump golang.org/x/net to 0.17.0 (#7222)


### Misc






### Docs




## Thanks

Thanks to these contributors who contributed to v0.44.5!
* :heart: @khrm
* :heart: @lbernick

Extra shout-out for awesome release notes:
* :heart_eyes: @lbernick

<!--
## Unsorted PR List

To Be Done: Deprecation Notices, Backward Incompatible Changes
-->