# tekton v0.70 Release Notes

Source: [v0.70.0](https://github.com/tektoncd/pipeline/releases/tag/v0.70.0)

# 🎉 OpenAPI schema to Tekton CRDs 🎉

-[Docs @ v0.70.0](https://github.com/tektoncd/pipeline/tree/v0.70.0/docs)
-[Examples @ v0.70.0](https://github.com/tektoncd/pipeline/tree/v0.70.0/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.70.0/release.yaml
```

## Attestation

The Rekor UUID for this release is `108e9186e8c5677a9b98c9f620b1202d23cdf7b6bc38da3acecc1a9cb6f206d98fefed3ce02b0e09`

Obtain the attestation:
```shell
REKOR_UUID=108e9186e8c5677a9b98c9f620b1202d23cdf7b6bc38da3acecc1a9cb6f206d98fefed3ce02b0e09
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.70.0/release.yaml
REKOR_UUID=108e9186e8c5677a9b98c9f620b1202d23cdf7b6bc38da3acecc1a9cb6f206d98fefed3ce02b0e09

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v0.70.0@sha256:" + .digest.sha256')

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


* Add structural OpenAPI schema to Tekton CRDs (#8490)

action required: The structural OpenAPI schema to Tekton CRDs are added enabling API server schema validation and supporting `kubectl explain` to describe fields and structure of Tekton CRDs. Due to the API server schema validation, users should make sure Tekton CRs have a valid schema when creating or updating CRs.

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

* :bug: fix: Fix remote task params default-value substitution (#8641)

Task Param defaults will now be correctly substituted in Steps when the Task is referenced by a TaskRun

* :bug: fix: configure StepAction to use conversion webhook (#8644)


### Misc

* :hammer: cleanup: breakup the pkg/credentials into writer and matcher + ensure non corev1 usage in entrypoint for FIPs compliance (#8542)

import only the writer part of the credentials package in the entrypoint so that we do not pull core v1 API indirectly into the package

* :hammer: FIPS Compliance: Refactor Entrypoint, Remove zap Dependency & Update Build Checks (#8544)
* :hammer: build(deps): bump github.com/golang-jwt/jwt/v4 from 4.5.1 to 4.5.2 (#8670)
* :hammer: build(deps): bump the all group in /tekton with 2 updates (#8668)
* :hammer: build(deps): bump actions/setup-go from 5.3.0 to 5.4.0 (#8667)
* :hammer: build(deps): bump actions/cache from 4.2.2 to 4.2.3 (#8666)
* :hammer: build(deps): bump github/codeql-action from 3.28.11 to 3.28.13 (#8665)
* :hammer: build(deps): bump tj-actions/changed-files from dcc7a0cba800f454d79fff4b993e8c3555bcc0a8 to 27ae6b33eaed7bf87272fdeb9f1c54f9facc9d99 (#8664)
* :hammer: build(deps): bump golangci/golangci-lint-action from 6.5.0 to 6.5.1 (#8654)
* :hammer: build(deps): bump the all group in /tekton with 2 updates (#8653)
* :hammer: build(deps): bump github/codeql-action from 3.28.10 to 3.28.11 (#8633)
* :hammer: build(deps): bump the all group in /tekton with 2 updates (#8632)
* :hammer: build(deps): bump github.com/google/cel-go from 0.23.2 to 0.24.1 (#8614)
* :hammer: build(deps): bump ossf/scorecard-action from 2.4.0 to 2.4.1 (#8608)
* :hammer: Refactor pipelinerun metrics tests (#8340)

### Docs

* :book: Document `ko` settings for kind clusters with and without a local registry. (#8662)
* :book: Fix wrong entry in development documentation and other minor documentation corrections. (#8661)
* :book: Add release 0.69 to releases.md (#8630)

## Thanks

Thanks to these contributors who contributed to v0.70.0!
* :heart: @PuneetPunamiya
* :heart: @aThorp96
* :heart: @afrittoli
* :heart: @burigolucas
* :heart: @dependabot[bot]
* :heart: @devholic
* :heart: @twoGiants
* :heart: @waveywaves

Extra shout-out for awesome release notes:
* :heart_eyes: @aThorp96
* :heart_eyes: @burigolucas
* :heart_eyes: @waveywaves

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->