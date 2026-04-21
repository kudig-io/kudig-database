# tekton v1.7 Release Notes

Source: [v1.7.0](https://github.com/tektoncd/pipeline/releases/tag/v1.7.0)

# 🎉 Bug fixes, stability improvements and dependency updates 🎉

-[Docs @ v1.7.0](https://github.com/tektoncd/pipeline/tree/v1.7.0/docs)
-[Examples @ v1.7.0](https://github.com/tektoncd/pipeline/tree/v1.7.0/examples)

## Installation one-liner

```shell
kubectl apply -f https://infra.tekton.dev/tekton-releases/pipeline/previous/v1.7.0/release.yaml
```

## Attestation

The Rekor UUID for this release is `108e9186e8c5677a0af3ff47db2d68605b227b75af0aa40d87262257e2b9295f35454fe3d050ed38`

Obtain the attestation:
```shell
REKOR_UUID=108e9186e8c5677a0af3ff47db2d68605b227b75af0aa40d87262257e2b9295f35454fe3d050ed38
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://infra.tekton.dev/tekton-releases/pipeline/previous/v1.7.0/release.yaml
REKOR_UUID=108e9186e8c5677a0af3ff47db2d68605b227b75af0aa40d87262257e2b9295f35454fe3d050ed38

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v1.7.0@sha256:" + .digest.sha256')

# Download the release file
curl -L "$RELEASE_FILE" > release.yaml

# For each image in the attestation, match it to the release file
for image in $REKOR_ATTESTATION_IMAGES; do
  printf $image; grep -q $image release.yaml && echo " ===> ok" || echo " ===> no match";
done
```

## Changes

### Fixes

* :bug: fix: Populate step statuses before TaskRun timeout handling (#9184)

Fix a race condition on timeout that would result in a TaskRun status without steps statuses.

* :bug: fix: panic in v1beta1 matrix validation for invalid result refs (#9135)

Resolved an issue where Pipelines with invalid result references in matrix parameters would cause a panic during validation (v1beta1 API)

* :bug: Use patch instead of update to replace sidecars with nop image (#9128)

Fixed race condition causing TaskRuns to fail with 409 conflict error when stopping sidecars. 
StopSidecars now uses Patch instead of Update to avoid conflicts with concurrent kubelet pod status updates.


* :bug: fix: Add missing comma in slash commands workflow (#9157)
* :bug: Fix tekton/publish sed for combined-based-image digest replacement (#9119)
* :bug: examples: reduce the size of the matrix to reduce flakiness (#9187)

### Misc

* :hammer: Migrate tests images out of dockerhub. (#9158)
* :hammer: refactor: add clock injection to cache for testing (#9142)
* :hammer: Remove deprecated `// +build` directive from most files (#9118)
* :hammer: build(deps): bump tj-actions/changed-files from 6da3c88b60ebf09464ada9b06fba5b6f2d34bb94 to abdd2f68ea150cee8f236d4a9fb4e0f2491abf1b (#9196)
* :hammer: chore(release-pipeline): update references to oci bucket (#9189)
* :hammer: .github/workflows: fix e2e-matrix-extras (#9185)
* :hammer: build(deps): bump golang.org/x/crypto from 0.43.0 to 0.45.0 (#9181)
* :hammer: build(deps): bump actions/checkout from 5.0.0 to 6.0.0 (#9180)
* :hammer: build(deps): bump golangci/golangci-lint-action from 9.0.0 to 9.1.0 (#9179)
* :hammer: .github: add a dependabot configuration to monitor .ko.yaml (#9173)
* :hammer: feat: Add GitHub Actions cherry-pick slash command (#9172)
* :hammer: build(deps): bump the all group in /tekton with 4 updates (#9170)
* :hammer: build(deps): bump actions/dependency-review-action from 4.8.1 to 4.8.2 (#9169)
* :hammer: build(deps): bump actions/setup-go from 6.0.0 to 6.1.0 (#9168)
* :hammer: build(deps): bump tj-actions/changed-files from 70069877f29101175ed2b055d210fe8b1d54d7d7 to 6da3c88b60ebf09464ada9b06fba5b6f2d34bb94 (#9167)
* :hammer: build(deps): bump chainguard-dev/actions from 1.5.3 to 1.5.10 (#9166)
* :hammer: build(deps): bump github/codeql-action from 4.31.0 to 4.31.5 (#9165)
* :hammer: Fix commit SHA of actions/github-script in e2e-extras workflow (#9161)
* :hammer: Fix the e2e-extras slash command (#9160)
* :hammer: examples: make sure we use the same image for sidecar and step (#9139)
* :hammer: fix(ci): correct grep patterns in detect job (#9137)
* :hammer: build(deps): bump the all group in /tekton with 4 updates (#9134)
* :hammer: build(deps): bump chainguard-dev/actions from 1.5.7 to 1.5.8 (#9133)
* :hammer: build(deps): bump tj-actions/changed-files from 0ff001de0805038ff3f118de4875002200057732 to 70069877f29101175ed2b055d210fe8b1d54d7d7 (#9132)
* :hammer: build(deps): bump step-security/harden-runner from 2.13.1 to 2.13.2 (#9131)
* :hammer: build(deps): bump golangci/golangci-lint-action from 8.0.0 to 9.0.0 (#9130)
* :hammer: fix: label checker action reference (#9129)
* :hammer: Update releases.md after 1.6.0 release (#9127)
* :hammer: build(deps): bump the all group in /tekton with 4 updates (#9124)
* :hammer: build(deps): bump tj-actions/changed-files from dbf178ceecb9304128c8e0648591d71208c6e2c9 to 0ff001de0805038ff3f118de4875002200057732 (#9122)
* :hammer: feat: upload release manifests to oracle cloud (#9121)
* :hammer: test: reduce the number of examples tests running in parallel (#9114)
* :hammer: Run less e2e matrix by default (#9109)
* :hammer: ci: skip running builds and tests if no code changed (#8768)
* :hammer: fix: update tekton setup action (#9126)
* :hammer: build(deps): bump github.com/docker/docker from 26.1.5+incompatible to 28.0.0+incompatible in /test/resolver-with-timeout (#9182)

## Thanks

Thanks to these contributors who contributed to v1.7.0!
* :heart: @AlanGreene
* :heart: @aThorp96
* :heart: @anithapriyanatarajan
* :heart: @dependabot[bot]
* :heart: @divyansh42
* :heart: @mathur07
* :heart: @prad9192
* :heart: @twoGiants
* :heart: @vdemeester

Extra shout-out for awesome release notes:
* :heart_eyes: @mathur07
* :heart_eyes: @prad9192
* :heart_eyes: @vdemeester
