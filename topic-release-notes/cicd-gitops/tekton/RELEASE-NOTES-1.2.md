# tekton v1.2 Release Notes

Source: [v1.2.0](https://github.com/tektoncd/pipeline/releases/tag/v1.2.0)

# 🎉 Bug fixes and documentation enhancements 🎉

-[Docs @ v1.2.0](https://github.com/tektoncd/pipeline/tree/v1.2.0/docs)
-[Examples @ v1.2.0](https://github.com/tektoncd/pipeline/tree/v1.2.0/examples)

## Installation one-liner

```shell
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v1.2.0/release.yaml
```

## Attestation

The Rekor UUID for this release is `108e9186e8c5677a754b4d7d26d7ac445cc63785908c6df49e449f3da28b067511a0f2298767d8be`

Obtain the attestation:
```shell
REKOR_UUID=108e9186e8c5677a754b4d7d26d7ac445cc63785908c6df49e449f3da28b067511a0f2298767d8be
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://storage.googleapis.com/tekton-releases/pipeline/previous/v1.2.0/release.yaml
REKOR_UUID=108e9186e8c5677a754b4d7d26d7ac445cc63785908c6df49e449f3da28b067511a0f2298767d8be

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v1.2.0@sha256:" + .digest.sha256')

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

* :bug: Do not propagate managed-by annotation to Pods (#8846)

od created by the Pipeline controller will now always have app.kubernetes.io/managed-by set to the default configuration. Prior to this change, it would be overriden by the value of that label set on TaskRun (or PipelineRun).

* :bug: fix: Avoid errors in PVC handler if PVC is already deleted (#8811)

PVCs that have already been deleted will no longer cause errors during resource cleanup operations.




### Misc

* :hammer: refactor: replace go-multierror with errors.Join for native error aggregation (#8791)

Replaced external go-multierror dependency with Go's built-in errors.Join for error aggregation, reducing dependencies and improving code maintainability.


* :hammer: fix: Remove direct usage of testify from pkg/spire/ (#8788)

Removed direct dependency on testify in pkg/spire to simplify testing and reduce external dependencies.

* :hammer: [TEP-0056]: Extract common status helper functions from `pipelinerun_test.go`. (#8850)
* :hammer: [TEP-0056]: Refactor names and fix typos. (#8806)
* :hammer: Move `Step` step action reference test to `container_validation_test.go`. (#8779)
* :hammer: Move `Step` Validate and ValidateError tests to `container_validation_test.go`. (#8778)
* :hammer: Move `Step` `ResultRef` and `ArtifactsRef` tests to `container_validation_test.go`. (#8777)
* :hammer: Move `Step` `onError` and API version tests to `container_validation_test.go`. (#8773)
* :hammer: Move `Step` ref tests to `container_validation_test.go`. (#8772)
* :hammer: Move `Step` artifacts flag tests to `container_validation_test.go`. (#8769)
* :hammer: Fix CRD Generation Errors (#8726)
* :hammer: build(deps): bump go.opentelemetry.io/otel/exporters/otlp/otlptrace/otlptracehttp from 1.36.0 to 1.37.0 (#8848)
* :hammer: build(deps): bump the all group in /tekton with 4 updates (#8844)
* :hammer: build(deps): bump k8s.io/code-generator from 0.32.5 to 0.32.6 (#8842)
* :hammer: build(deps): bump k8s.io/apiextensions-apiserver from 0.32.5 to 0.32.6 (#8840)
* :hammer: build(deps): bump k8s.io/client-go from 0.32.5 to 0.32.6 (#8837)
* :hammer: build(deps): bump github.com/cloudevents/sdk-go/v2 from 2.16.0 to 2.16.1 (#8836)
* :hammer: build(deps): bump github.com/google/go-containerregistry from 0.20.5 to 0.20.6 (#8835)
* :hammer: build(deps): bump the all group in /tekton with 4 updates (#8834)
* :hammer: build(deps): bump step-security/harden-runner from 2.12.0 to 2.12.1 (#8833)
* :hammer: build(deps): bump github/codeql-action from 3.28.19 to 3.29.0 (#8832)
* :hammer: build(deps): bump tj-actions/changed-files from 4140eb99d2cced9bfd78375c2088371853262f79 to d52d20fa3f981cb852b861fd8f55308b5fe29637 (#8831)
* :hammer: build(deps): bump github.com/sigstore/sigstore from 1.9.4 to 1.9.5 (#8826)
* :hammer: build(deps): bump github.com/sigstore/sigstore/pkg/signature/kms/hashivault from 1.9.4 to 1.9.5 (#8823)
* :hammer: build(deps): bump github.com/sigstore/sigstore/pkg/signature/kms/aws from 1.9.4 to 1.9.5 (#8822)
* :hammer: build(deps): bump github.com/sigstore/sigstore/pkg/signature/kms/azure from 1.9.4 to 1.9.5 (#8821)
* :hammer: build(deps): bump github.com/sigstore/sigstore/pkg/signature/kms/gcp from 1.9.4 to 1.9.5 (#8820)
* :hammer: build(deps): bump github/codeql-action from 3.28.18 to 3.28.19 (#8819)
* :hammer: build(deps): bump tj-actions/changed-files from c6634ca281a9fc05b03bee224ba00910cb78ab6e to 4140eb99d2cced9bfd78375c2088371853262f79 (#8818)
* :hammer: build(deps): bump golang.org/x/sync from 0.14.0 to 0.15.0 (#8817)
* :hammer: build(deps): bump google.golang.org/grpc from 1.72.2 to 1.73.0 (#8816)
* :hammer: Fix the sed command with crane (#8812)
* :hammer: fix(lint): resolve golangci-lint errors in testing context (#8810)
* :hammer: build(deps): bump ossf/scorecard-action from 2.4.1 to 2.4.2 (#8808)
* :hammer: build(deps): bump the all group in /tekton with 4 updates (#8807)

### Docs

* :book: docs: clarify container entrypoint behavior (#8696)

docs: clarify container contract documentation to avoid ambiguity
* :book: Update releases.md for 1.1.0 (#8814)
* :book: fix(docs): remove duplicated block from artifacts.md (#8802)

## Thanks

Thanks to these contributors who contributed to v1.2.0!
* :heart: @afrittoli
* :heart: @alex-cobas
* :heart: @arthur-c
* :heart: @dependabot[bot]
* :heart: @fambelic
* :heart: @infernus01
* :heart: @l-qing
* :heart: @tricktron
* :heart: @twoGiants
* :heart: @vdemeester

Extra shout-out for awesome release notes:
* :heart_eyes: @fambelic
* :heart_eyes: @infernus01
* :heart_eyes: @l-qing
* :heart_eyes: @vdemeester

<!--
## Unsorted PR List


To Be Done: Deprecation Notices, Backward Incompatible Changes
-->