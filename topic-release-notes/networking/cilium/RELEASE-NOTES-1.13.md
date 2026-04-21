# cilium v1.13 Release Notes

Source: [v1.13.18](https://github.com/cilium/cilium/releases/tag/v1.13.18)

Summary of Changes
------------------
We are pleased to release Cilium v1.13.18 which includes stability and bug fixes. Thanks to all contributors, reviewers, testers, and users!

**Bugfixes:**
* envoy: Avoid short circuit backend filtering (Backport PR #33535, Upstream PR #33403, @sayboras)
* Fix service connection to terminating backend, when the service has no more backends available. (Backport PR #33276, Upstream PR #31840, @julianwiedmann)
* Fixes unencrypted traffic among nodes when IPsec is used with L7 egress proxy. (Backport PR #31977, Upstream PR #32683, @jschwinger233)
* ipsec: do not nil out EncryptInterface when using IPAM ENI on netlink… (Backport PR #33633, Upstream PR #33512, @jasonaliyetti)
* Report the correct drop reason when a packet is dropped by the bpf_lxc program. (Backport PR #33633, Upstream PR #33551, @julianwiedmann)
* Revert PR #32244 which caused unintended side-effects that negatively impacted network performance. (Backport PR #33376, Upstream PR #33304, @learnitall)
* Update IPsec to handle larger PSK values when using per-tunnel PSK (Backport PR #33532, Upstream PR #33472, @jasonaliyetti)

**CI Changes:**
* ci: Add IPsec leak detection for ci-ipsec-e2e (Backport PR #33080, Upstream PR #32930, @jschwinger233)
* ci: use env variable to store branch name (Backport PR #33376, Upstream PR #26779, @ferozsalam)
* gh: ipsec: clarify check for leaked proxy traffic during key rotation (Backport PR #33633, Upstream PR #33509, @julianwiedmann)
* gha: Only retrieve IPv4 CIDR from docker network (Backport PR #33112, Upstream PR #33093, @sayboras)

**Misc Changes:**
* .github: add workflow for renovate to build base images (Backport PR #33348, Upstream PR #33326, @aanm)
* .github: fix cloud workflows for renovate (Backport PR #33315, Upstream PR #33320, @aanm)
* .github: fix worfklows used by renovate (Backport PR #33315, Upstream PR #33309, @aanm)
* [v1.13] - remove tracking of backports with MLH (cilium/cilium#33126, @aanm)
* Add auto-merge for renovate for trusted dependencies (Backport PR #33315, Upstream PR #33287, @aanm)
* build(deps): bump urllib3 from 2.0.7 to 2.2.2 in /Documentation (Backport PR #33376, Upstream PR #33218, @dependabot[bot])
* build-images-base: cancel github runs based on branch name (Backport PR #33376, Upstream PR #33353, @aanm)
* build-images-base: push to branch if pull request ref doesn't exist (Backport PR #33376, Upstream PR #33368, @aanm)
* build-images: fetch artifacts with specific pattern (Backport PR #33376, Upstream PR #33216, @aanm)
* chore(deps): update all github action dependencies (v1.13) (cilium/cilium#33193, @cilium-renovate[bot])
* chore(deps): update all github action dependencies (v1.13) (cilium/cilium#33209, @cilium-renovate[bot])
* chore(deps): update all github action dependencies (v1.13) (cilium/cilium#33370, @cilium-renovate[bot])
* chore(deps): update all github action dependencies (v1.13) (cilium/cilium#33499, @cilium-renovate[bot])
* chore(deps): update docker.io/library/alpine docker tag to v3.17.8 (v1.13) (cilium/cilium#33367, @cilium-renovate[bot])
* chore(deps): update docker.io/library/golang:1.21.11 docker digest to 2eb85b8 (v1.13) (cilium/cilium#33189, @cilium-renovate[bot])
* chore(deps): update docker.io/library/golang:1.21.11 docker digest to b405b62 (v1.13) (cilium/cilium#33366, @cilium-renovate[bot])
* chore(deps): update docker/build-push-action action to v5.4.0 (v1.13) (cilium/cilium#33025, @cilium-renovate[bot])
* chore(deps): update go to v1.21.12 (v1.13) (cilium/cilium#33541, @cilium-renovate[bot])
* chore(deps): update stable lvh-images (v1.13) (patch) (cilium/cilium#33008, @cilium-renovate[bot])
* chore(deps): update stable lvh-images (v1.13) (patch) (cilium/cilium#33192, @cilium-renovate[bot])
* chore(deps): update stable lvh-images (v1.13) (patch) (cilium/cilium#33344, @cilium-renovate[bot])
* Creation of the /hello endpoint is delayed until the host datapath has been initialized. (Backport PR #33253, Upstream PR #27392, @lmb)
* daemon: Allow DNS transparent mode to be turned off with encryption (Backport PR #33532, Upstream PR #33420, @gandro)
* docs: Improve note on kube-apiserver entity limitations (Backport PR #33532, Upstream PR #33382, @gandro)
* docs: ipsec: mention dependency on transparent mode for DNS proxy (Backport PR #33100, Upstream PR #33062, @julianwiedmann)
* docs: ipsec: remove limitation for native-routing with L7 egress policy (Backport PR #31977, Upstream PR #32906, @julianwiedmann)
* Documentation: accept ORG and REPO (Backport PR #33532, Upstream PR #33514, @aanm)
* Fix renovate's concurrency group (Backport PR #33563, Upstream PR #33528, @aanm)
* install/kubernetes: update nodeinit image to latest version (Backport PR #33532, Upstream PR #33427, @marseel)
* Miscellaneous improvements to clustermesh-related troubleshooting tools (Backport PR #33376, Upstream PR #32951, @giorio94)
* pkg/endpoint: do not rely on bpf_host.o to detect host endpoint (Backport PR #33253, Upstream PR #32521, @lmb)
* pkg/endpoint: make state synchronization atomic (Backport PR #33253, Upstream PR #32439, @lmb)
* Renovate changes (Backport PR #33563, Upstream PR #33519, @aanm)
* renovate: add auto-approve bot for renovate PRs (Backport PR #33644, Upstream PR #33604, @aanm)

**Other Changes:**
* [1.13] Instruct users to manually clean stale routing rules after downgrade (cilium/cilium#33079, @jschwinger233)
* [v1.13] ci: ipsec-e2e: fine-tune L7 proxy check (cilium/cilium#33576, @julianwiedmann)
* envoy: Bump golang version to v1.22.5 (cilium/cilium#33554, @sayboras)
* envoy: Update envoy 1.28.x to v1.28.5 (cilium/cilium#33481, @sayboras)
* github: fix concurrency groups for push events (cilium/cilium#33646, @aanm)
* install: Update image digests for v1.13.17 (cilium/cilium#33017, @qmonnet)


## Docker Manifests

### cilium

`docker.io/cilium/cilium:v1.13.18@sha256:9dc74ba5321c999e498b5f05202c7e27015360dd19278f19b15a25bee79d22f1`
`quay.io/cilium/cilium:v1.13.18@sha256:9dc74ba5321c999e498b5f05202c7e27015360dd19278f19b15a25bee79d22f1`

### clustermesh-apiserver

`docker.io/cilium/clustermesh-apiserver:v1.13.18@sha256:c2a38a7fd080c4159ef6a499945f3af069333385255ddc80c2fd35328f6b512a`
`quay.io/cilium/clustermesh-apiserver:v1.13.18@sha256:c2a38a7fd080c4159ef6a499945f3af069333385255ddc80c2fd35328f6b512a`

### docker-plugin

`docker.io/cilium/docker-plugin:v1.13.18@sha256:34ec3e5ed73ccea9d38fabce7d0578d568dd4c831611e93d573bd9df860f7c65`
`quay.io/cilium/docker-plugin:v1.13.18@sha256:34ec3e5ed73ccea9d38fabce7d0578d568dd4c831611e93d573bd9df860f7c65`

### hubble-relay

`docker.io/cilium/hubble-relay:v1.13.18@sha256:220ac4b70ffb5ecf598af1024dc0997affdf86f2e4c1a12f5aa9ede490cd181d`
`quay.io/cilium/hubble-relay:v1.13.18@sha256:220ac4b70ffb5ecf598af1024dc0997affdf86f2e4c1a12f5aa9ede490cd181d`

### operator-alibabacloud

`docker.io/cilium/operator-alibabacloud:v1.13.18@sha256:27da1054d0aa105970ae150133cd0ed5a17e9696533e055f2f93902d4e4d3359`
`quay.io/cilium/operator-alibabacloud:v1.13.18@sha256:27da1054d0aa105970ae150133cd0ed5a17e9696533e055f2f93902d4e4d3359`

### operator-aws

`docker.io/cilium/operator-aws:v1.13.18@sha256:20740ff319ea3169f40593f514887769461167c64f83703c43dcd0ffe3641a95`
`quay.io/cilium/operator-aws:v1.13.18@sha256:20740ff319ea3169f40593f514887769461167c64f83703c43dcd0ffe3641a95`

### operator-azure

`docker.io/cilium/operator-azure:v1.13.18@sha256:5cc125efdfd2dbdf8d0361c714c4f27699603f47a18e5abad5223ffd7bda9b6c`
`quay.io/cilium/operator-azure:v1.13.18@sha256:5cc125efdfd2dbdf8d0361c714c4f27699603f47a18e5abad5223ffd7bda9b6c`

### operator-generic

`docker.io/cilium/operator-generic:v1.13.18@sha256:6a6332840d4df6eef48bb81ced12af8d860438aa2974b39b875cd6c234302b69`
`quay.io/cilium/operator-generic:v1.13.18@sha256:6a6332840d4df6eef48bb81ced12af8d860438aa2974b39b875cd6c234302b69`

### operator

`docker.io/cilium/operator:v1.13.18@sha256:9c45df2974f412341177144ff131be5faee34ee507310c4505d7b1161111b7b4`
`quay.io/cilium/operator:v1.13.18@sha256:9c45df2974f412341177144ff131be5faee34ee507310c4505d7b1161111b7b4`

