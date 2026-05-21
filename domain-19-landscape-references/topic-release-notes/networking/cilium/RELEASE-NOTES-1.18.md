---
title: cilium v1.18 Release Notes
description: cilium v1.18 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
- apiserver
- envoy
- cilium
- docker
- opa
- ingress
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- cilium v1.18 Release Notes 是什么
- 如何 cilium v1.18 Release Notes
trigger_keywords:
- cilium
- v1.18
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- ebpf-basics
- cilium-basics
- etcd-basics
- tls-basics
- policy-basics
---

# cilium v1.18 Release Notes

Source: [v1.18.8](https://github.com/cilium/cilium/releases/tag/v1.18.8)

Known issues
------------------

* Users who deploy Cilium on GKE should skip this version or upgrade to 1.19.2 due to a [known regression](https://github.com/cilium/cilium/pull/44499#issuecomment-4080979129).

Summary of Changes
------------------

**Minor Changes:**
* Allow to attach Cilium's XDP program on network interfaces that have jumbo MTU configured and support xdp.frags program type. (Backport PR cilium/cilium#44499, Upstream PR cilium/cilium#41967, @viktor-kurchenko)

**Bugfixes:**
* bpf: nodeport: use hairpin redirect for L7 LB on bridge devices (Backport PR cilium/cilium#44758, Upstream PR cilium/cilium#44658, @smagnani96)
* cilium-dbg: fix seg-fault `ip get -l reserved:host` (Backport PR cilium/cilium#44519, Upstream PR cilium/cilium#44443, @aanm)
* Enable Cilium upgrade and downgrade when existing XDP attach types differ from new XDP programs (Backport PR cilium/cilium#44499, Upstream PR cilium/cilium#44209, @dylandreimerink)
* Fix a bug where node IPv6 updates and deletes were not correctly propagated to the Linux kernel neighbor subsystem. (Backport PR cilium/cilium#44592, Upstream PR cilium/cilium#44540, @tklauser)
* Fix a bug where removed addresses from EndpointSlices might be missed if multiple EndpointSlices share the same name (Backport PR cilium/cilium#44021, Upstream PR cilium/cilium#43999, @EmilyShepherd)
* Fix envoy admin socket being created as world-accessible (Backport PR cilium/cilium#44592, Upstream PR cilium/cilium#44512, @0xch4z)
* Fixed an issue where wildcard FQDN network policy identities were not correctly pushed to Envoy when using SNI-based policies. (Backport PR cilium/cilium#44519, Upstream PR cilium/cilium#44462, @liyihuang)
* Fixed VTEP ARP responses returning 00:00:00:00:00:00 MAC due to interface MAC missing from eBPF Overlay configuration. (Backport PR cilium/cilium#44700, Upstream PR cilium/cilium#44513, @akos011221)
* gateway-api: Fix hostname intersection bug that was preventing cert-manager challenges from working correctly. (Backport PR cilium/cilium#44519, Upstream PR cilium/cilium#44492, @youngnick)
* l7lb: fix bypassing ingress policies for local backends (Backport PR cilium/cilium#44804, Upstream PR cilium/cilium#44693, @smagnani96)
* loadbalancer/healthserver: refresh ProxyRedirect per request (Backport PR cilium/cilium#44399, Upstream PR cilium/cilium#44286, @mhofstetter)

**CI Changes:**
* gh: e2e-upgrade: don't hardcode IPsec encryption algorithm (Backport PR cilium/cilium#44519, Upstream PR cilium/cilium#44381, @julianwiedmann)

**Misc Changes:**
* chore(deps): update all github action dependencies (v1.18) (cilium/cilium#44372, @cilium-renovate[bot])
* chore(deps): update all github action dependencies (v1.18) (cilium/cilium#44480, @cilium-renovate[bot])
* chore(deps): update all github action dependencies (v1.18) (cilium/cilium#44579, @cilium-renovate[bot])
* chore(deps): update all github action dependencies (v1.18) (cilium/cilium#44681, @cilium-renovate[bot])
* chore(deps): update all github action dependencies (v1.18) (cilium/cilium#44791, @cilium-renovate[bot])
* chore(deps): update all-dependencies (v1.18) (cilium/cilium#44369, @cilium-renovate[bot])
* chore(deps): update base-images (v1.18) (cilium/cilium#44580, @cilium-renovate[bot])
* chore(deps): update base-images (v1.18) (cilium/cilium#44678, @cilium-renovate[bot])
* chore(deps): update base-images to v1.25.8 (v1.18) (cilium/cilium#44810, @cilium-renovate[bot])
* chore(deps): update cilium/cilium-cli action to v0.19.1 (v1.18) (cilium/cilium#44344, @cilium-renovate[bot])
* chore(deps): update dependency cilium/cilium-cli to v0.19.1 (v1.18) (cilium/cilium#44401, @cilium-renovate[bot])
* chore(deps): update dependency cilium/cilium-cli to v0.19.2 (v1.18) (cilium/cilium#44577, @cilium-renovate[bot])
* chore(deps): update dependency sphinx-tabs to v3.5.0 (v1.18) (cilium/cilium#44679, @cilium-renovate[bot])
* chore(deps): update docker.io/library/ubuntu:24.04 docker digest to d1e2e92 (v1.18) (cilium/cilium#44476, @cilium-renovate[bot])
* chore(deps): update gcr.io/distroless/static:nonroot docker digest to e3f9456 (v1.18) (cilium/cilium#44797, @cilium-renovate[bot])
* chore(deps): update gcr.io/distroless/static:nonroot docker digest to f512d81 (v1.18) (cilium/cilium#44575, @cilium-renovate[bot])
* chore(deps): update gcr.io/etcd-development/etcd docker tag to v3.6.8 (v1.18) (cilium/cilium#44370, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/certgen docker tag to v0.4.1 (v1.18) (cilium/cilium#44680, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/cilium-envoy docker tag to v1.35.9-1770979049-232ed4a26881e4ab4f766f251f258ed424fff663 (v1.18) (cilium/cilium#44371, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/cilium-envoy docker tag to v1.35.9-1771585526-532310e626e42c7086de4ef3ea913736125bbd31 (v1.18) (cilium/cilium#44478, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/cilium-envoy docker tag to v1.35.9-1773220507-ffc0948a7ec4868e6b552a71cf4d3860e78b53cc (v1.18) (cilium/cilium#44676, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/cilium-envoy docker tag to v1.35.9-1773405792-4046425704636ea5b770460c20c065069cf572dc (v1.18) (cilium/cilium#44789, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/cilium-envoy docker tag to v1.35.9-1773656288-7b052e66eb2cfc5ac130ce0a5be66202a10d83be (v1.18) (cilium/cilium#44807, @cilium-renovate[bot])
* chore(deps): update stable lvh-images (v1.18) (patch) (cilium/cilium#44252, @cilium-renovate[bot])
* chore(deps): update stable lvh-images (v1.18) (patch) (cilium/cilium#44479, @cilium-renovate[bot])
* chore(deps): update stable lvh-images (v1.18) (patch) (cilium/cilium#44677, @cilium-renovate[bot])
* chore(deps): update stable lvh-images (v1.18) (patch) (cilium/cilium#44790, @cilium-renovate[bot])
* Docs: improve docs around ipsec upgrade in 1.18 (Backport PR cilium/cilium#44399, Upstream PR cilium/cilium#44302, @darox)
* fix(deps): update k8s.io patch updates stable (v1.18) (cilium/cilium#44477, @cilium-renovate[bot])
* fix(deps): update k8s.io patch updates stable to v0.33.9 (v1.18) (patch) (cilium/cilium#44578, @cilium-renovate[bot])
* fix(deps): update sigs.k8s.io/mcs-api/controllers digest to 0f775a3 (v1.18) (cilium/cilium#44576, @cilium-renovate[bot])
* fix(deps): update sigs.k8s.io/mcs-api/controllers digest to 15301c2 (v1.18) (cilium/cilium#44675, @cilium-renovate[bot])
* loadbalancer/healthserver: stabilize proxy-redirect test (Backport PR cilium/cilium#44519, Upstream PR cilium/cilium#44323, @mhofstetter)

**Other Changes:**
* [1.18] gha: Use eks 1.32 from us-west-2 (cilium/cilium#44753, @sayboras)
* [v1.18] endpoint/bpf: remove change empty condition for updateEnvoy (cilium/cilium#44616, @liyihuang)
* [v1.18] gh: verifier: disable RHEL8 (cilium/cilium#44317, @julianwiedmann)
* [v1.18] loadbalancer: Fix flake in hybrid-dsr.txtar (cilium/cilium#44756, @julianwiedmann)
* install: Update image digests for v1.18.7 (cilium/cilium#44326, @cilium-release-bot[bot])


## Docker Manifests

### cilium

`quay.io/cilium/cilium:v1.18.8@sha256:070a63cc414869cf6c53202cb50929a87adb7d5b25de0f2f40ab39eb6434b706`

### clustermesh-apiserver

`quay.io/cilium/clustermesh-apiserver:v1.18.8@sha256:5cb08daad7397f52ce5c36fcbfe83c56494f340d9b8f10f8bc7a3f2a812c33d5`

### docker-plugin

`quay.io/cilium/docker-plugin:v1.18.8@sha256:8e1c89bc4ef3bbc55a10edc96a9f2915af45181e46ff189c00f3d8fb7825a0b7`

### hubble-relay

`quay.io/cilium/hubble-relay:v1.18.8@sha256:dcf324aa35ab59c8fe6d002e3df6a63fff18280da464d09e4a97d58c085bb015`

### operator-alibabacloud

`quay.io/cilium/operator-alibabacloud:v1.18.8@sha256:36c1702c8afd0b0221e3d88ca08537100caef509de6a6bb7244d5fa4643a7252`

### operator-aws

`quay.io/cilium/operator-aws:v1.18.8@sha256:7ab154b269eae378456d63cc9085d96c4f472e11a1496ca4c62af68ff4b31da3`

### operator-azure

`quay.io/cilium/operator-azure:v1.18.8@sha256:a4027d349e817bda9168af1e27231be491a3026c748128a79026e366321f6332`

### operator-generic

`quay.io/cilium/operator-generic:v1.18.8@sha256:f9d1715932751b1454d0f59b492497cb1636dea6335beab0f9026fa8b5a6f62f`

### operator

`quay.io/cilium/operator:v1.18.8@sha256:cc3f7bdf9e443b807d3cb9b0bd30eddac5591c3f4b1e6fa053bfaa8697a7ee58`

