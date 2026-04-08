# tekton v1.6 Release Notes

Source: [v1.6.1](https://github.com/tektoncd/pipeline/releases/tag/v1.6.1)

<!-- For major releases, add a tag line
# 🎉 [Tag Line - to be done] 🎉
-->

-[Docs @ v1.6.1](https://github.com/tektoncd/pipeline/tree/v1.6.1/docs)
-[Examples @ v1.6.1](https://github.com/tektoncd/pipeline/tree/v1.6.1/examples)

## Installation one-liner

```shell
kubectl apply -f https://infra.tekton.dev/tekton-releases/pipeline/previous/v1.6.1/release.yaml
```

## Attestation

The Rekor UUID for this release is `108e9186e8c5677a4ba876bd39916b4123385435497b76a0e5cfee59ac292230166e5ded5b9d4596`

Obtain the attestation:
```shell
REKOR_UUID=108e9186e8c5677a4ba876bd39916b4123385435497b76a0e5cfee59ac292230166e5ded5b9d4596
rekor-cli get --uuid $REKOR_UUID --format json | jq -r .Attestation | jq .
```

Verify that all container images in the attestation are in the release file:
```shell
RELEASE_FILE=https://infra.tekton.dev/tekton-releases/pipeline/previous/v1.6.1/release.yaml
REKOR_UUID=108e9186e8c5677a4ba876bd39916b4123385435497b76a0e5cfee59ac292230166e5ded5b9d4596

# Obtains the list of images with sha from the attestation
REKOR_ATTESTATION_IMAGES=$(rekor-cli get --uuid "$REKOR_UUID" --format json | jq -r .Attestation | jq -r '.subject[]|.name + ":v1.6.1@sha256:" + .digest.sha256')

# Download the release file
curl -L "$RELEASE_FILE" > release.yaml

# For each image in the attestation, match it to the release file
for image in $REKOR_ATTESTATION_IMAGES; do
  printf $image; grep -q $image release.yaml && echo " ===> ok" || echo " ===> no match";
done
```

<!-- Any special upgrade notice
## Upgrade Notices
-->

## Changes

### :warning: Security Fixes

- **[GHSA-j5q5-j9gm-2w5c](https://github.com/tektoncd/pipeline/security/advisories/GHSA-j5q5-j9gm-2w5c)** (Critical): Path traversal in git resolver allows reading arbitrary files from the resolver pod. Fixed by validating the `pathInRepo` parameter to prevent directory traversal.

- **[GHSA-cv4x-93xx-wgfj](https://github.com/tektoncd/pipeline/security/advisories/GHSA-cv4x-93xx-wgfj)** / CVE-2026-33022 (Medium): Controller panic via long resolver name in TaskRun/PipelineRun. A user with permission to create TaskRuns or PipelineRuns could crash the controller into a restart loop by setting a resolver name of 31+ characters, causing denial of service cluster-wide. Thanks to @1seal for reporting this vulnerability.

### Features

* :sparkles: Add support for `hostUsers` field in PodTemplate to control user namespace isolation (#9324)

### Fixes

* :bug: fix: Prevent excessive reconciliation when timeout disabled (#9303) — Fix an issue where there was excessive reconciliation in case of no timeout on TaskRun or PipelineRun.
* :bug: fix: panic in v1beta1 matrix validation for invalid result refs (#9257) — Resolved an issue where Pipelines with invalid result references in matrix parameters would cause a panic during validation (v1beta1 API)
* :bug: fix(pipelinerun): fix the issue of massive invalid status updates caused by unordered arrays, which will greatly impact the resource load and stability of the apiserver (#9314)

### Misc

* :hammer: ci: add CI summary fan-in job for branch protection (#9408)
* :hammer: tekton: update plumbing ref to latest commit (#9412)
* :hammer: tekton: update plumbing ref to include full image references fix (#9402)
* :hammer: Backported test reliability fixes including reduced test parallelism, image migration from DockerHub to mirror.gcr.io, and improved dind-sidecar probe configuration (#9251)

## Thanks

Thanks to these contributors who contributed to v1.6.1!

* :heart: @1seal
* :heart: @tekton-robot
* :heart: @vdemeester

