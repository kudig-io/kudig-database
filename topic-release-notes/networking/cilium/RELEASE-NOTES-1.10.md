# cilium v1.10 Release Notes

Source: [v1.10.20](https://github.com/cilium/cilium/releases/tag/v1.10.20)

The Cilium core team is pleased to announce v1.10.20. These releases include a range of bugfixes and updates Envoy to v1.22.7 to pull in an updated BoringSSL library dependency to address CVE-2023-0286.

This will be the last community release for the v1.10 series.

Summary of Changes
------------------

**Minor Changes:**
* envoy: Bump envoy version to 1.22.7 (Backport PR #23639, Upstream PR #23502, @sayboras)

**Bugfixes:**
* Fix a data race in dnsproxy which could lead to DNS requests drops. (Backport PR #23422, Upstream PR #22619, @aspsk)

**Misc Changes:**
* .github/workflows: add version number in GH action (#23621, @aanm)
* .github/workflows: fix external contribution detection (Backport PR #23422, Upstream PR #23406, @aanm)
* backporting: leave `backport/author` PRs alone (Backport PR #23422, Upstream PR #22654, @bimmlerd)
* build(deps): bump actions/cache from 3.2.3 to 3.2.4 (#23452, @dependabot[bot])
* build(deps): bump actions/github-script from 6.3.3 to 6.4.0 (#23413, @dependabot[bot])
* build(deps): bump docker/build-push-action from 3.3.0 to 4.0.0 (#23490, @dependabot[bot])
* build(deps): bump docker/setup-buildx-action from 2.2.1 to 2.4.0 (#23451, @dependabot[bot])
* build(deps): bump docker/setup-buildx-action from 2.4.0 to 2.4.1 (#23592, @dependabot[bot])
* build(deps): bump github/codeql-action from 2.1.39 to 2.2.1 (#23412, @dependabot[bot])
* build(deps): bump github/codeql-action from 2.2.1 to 2.2.2 (#23609, @dependabot[bot])
* build(deps): bump KyleMayes/install-llvm-action from 1.6.1 to 1.7.0 (#23387, @dependabot[bot])
* chore(deps): update docker.io/library/ubuntu:20.04 docker digest to 4a45212 (v1.10) (#23566, @renovate[bot])
* chore(deps): update docker.io/library/ubuntu:20.04 docker digest to b872b03 (v1.10) (#23476, @renovate[bot])
* ci: increase Jenkinsfile timeout for 1.10 branch (#23544, @nbusseneau)
* docs: Improve wording for deny policies limitation (Backport PR #23422, Upstream PR #23095, @joestringer)
* docs: update committer security requirements (Backport PR #23422, Upstream PR #23134, @xmulligan)
* IPsec: Refactor `ipSecReplaceState{In,Out}` functions (Backport PR #23422, Upstream PR #23158, @pchaigno)
* Update Cilium install guide about EKS aws-node DaemonSet potential connectivity problem on uninstall (Backport PR #23422, Upstream PR #22620, @NikAleksandrov)

**Other Changes:**
* install: Update image digests for v1.10.19 (#23400, @qmonnet)


## Docker Manifests

### cilium

`docker.io/cilium/cilium:v1.10.20@sha256:c9b3af1f9c405cc8dcb163af0e0ea0a376a9f62304501c9392d26e91178d7869`
`quay.io/cilium/cilium:v1.10.20@sha256:c9b3af1f9c405cc8dcb163af0e0ea0a376a9f62304501c9392d26e91178d7869`

### clustermesh-apiserver

`docker.io/cilium/clustermesh-apiserver:v1.10.20@sha256:0c38519ec0c1462ceef9c58e008c9a36ee0633d3ad6c3fd860bfd20418987fe6`
`quay.io/cilium/clustermesh-apiserver:v1.10.20@sha256:0c38519ec0c1462ceef9c58e008c9a36ee0633d3ad6c3fd860bfd20418987fe6`

### docker-plugin

`docker.io/cilium/docker-plugin:v1.10.20@sha256:125f79097d9546bc33fb0da7490d2fff8a027c21a8dd27fbe7410f13dba63b9d`
`quay.io/cilium/docker-plugin:v1.10.20@sha256:125f79097d9546bc33fb0da7490d2fff8a027c21a8dd27fbe7410f13dba63b9d`

### hubble-relay

`docker.io/cilium/hubble-relay:v1.10.20@sha256:0c4932b94d6d7cea7045597ac93234de36031668fc7a509aa34a6a23d516135d`
`quay.io/cilium/hubble-relay:v1.10.20@sha256:0c4932b94d6d7cea7045597ac93234de36031668fc7a509aa34a6a23d516135d`

### operator-alibabacloud

`docker.io/cilium/operator-alibabacloud:v1.10.20@sha256:6174f8d45a1ee99c650ca4792e8cecd5a654e9620cc2383e546e843660092e2e`
`quay.io/cilium/operator-alibabacloud:v1.10.20@sha256:6174f8d45a1ee99c650ca4792e8cecd5a654e9620cc2383e546e843660092e2e`

### operator-aws

`docker.io/cilium/operator-aws:v1.10.20@sha256:73ed66f6ec9363441591b3c60c19834089ad0c0138e47a2de3497e5e7282b6c7`
`quay.io/cilium/operator-aws:v1.10.20@sha256:73ed66f6ec9363441591b3c60c19834089ad0c0138e47a2de3497e5e7282b6c7`

### operator-azure

`docker.io/cilium/operator-azure:v1.10.20@sha256:e870bbafecec3529961fa8e494159d365c92eabbd3597145fca582e1cc3fd2ec`
`quay.io/cilium/operator-azure:v1.10.20@sha256:e870bbafecec3529961fa8e494159d365c92eabbd3597145fca582e1cc3fd2ec`

### operator-generic

`docker.io/cilium/operator-generic:v1.10.20@sha256:f7a07e674687fc4be01168409eeed0986ed7cb19c8d966501507fd24b9f6bd98`
`quay.io/cilium/operator-generic:v1.10.20@sha256:f7a07e674687fc4be01168409eeed0986ed7cb19c8d966501507fd24b9f6bd98`

### operator

`docker.io/cilium/operator:v1.10.20@sha256:4fce3efe468e26598c1f2c9b7c47d4f97572f9ae113fc2c9cb11a6384a5b939b`
`quay.io/cilium/operator:v1.10.20@sha256:4fce3efe468e26598c1f2c9b7c47d4f97572f9ae113fc2c9cb11a6384a5b939b`

