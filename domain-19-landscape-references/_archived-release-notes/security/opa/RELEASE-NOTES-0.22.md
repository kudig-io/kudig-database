---
title: kind v0.22 Release Notes
description: kind v0.22 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kind v0.22 Release Notes 是什么
- 如何 kind v0.22 Release Notes
trigger_keywords:
- kind
- v0.22
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# kind v0.22 Release Notes

Source: [v0.22.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.22.0)

This release is a quick follow-up to [v0.21.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.21.0) with bug fixes including not overriding the host's binfmt_misc (a regression in v0.20.0, see: https://github.com/kubernetes-sigs/kind/issues/3510).


<h1 id="breaking-changes">Breaking Changes</h1>

- The default node image is now [[Kubernetes|Kubernetes]] 1.29.2: `kindest/node:v1.29.2@sha256:51a1434a5397193442f0be2a297b488b6c919ce8a3931be0ce822606ea5ca245`

**If you haven't already, please see also [v0.21.0  release notes](https://github.com/kubernetes-sigs/kind/releases/tag/v0.21.0) given the short time between releases.**
 
<h1 id="new-features">New Features</h1>

- Remove `exclude-from-external-load-balancers` from single node clusters https://github.com/kubernetes-sigs/kind/issues/3506
- Support for building node images on hosts with proxies

Images pre-built for this release:
- v1.29.2: `kindest/node:v1.29.2@sha256:51a1434a5397193442f0be2a297b488b6c919ce8a3931be0ce822606ea5ca245`
- v1.29.1: `kindest/node:v1.29.1@sha256:0c06baa545c3bb3fbd4828eb49b8b805f6788e18ce67bff34706ffa91866558b`
- v1.28.7: `kindest/node:v1.28.7@sha256:9bc6c451a289cf96ad0bbaf33d416901de6fd632415b076ab05f5fa7e4f65c58`
- v1.28.6: `kindest/node:v1.28.6@sha256:e9e59d321795595d0eed0de48ef9fbda50388dc8bd4a9b23fb9bd869f370ec7e`
- v1.27.11: `kindest/node:v1.27.11@sha256:681253009e68069b8e01aad36a1e0fa8cf18bb0ab3e5c4069b2e65cafdd70843`
- v1.27.10: `kindest/node:v1.27.10@sha256:e6b2f72f22a4de7b957cd5541e519a8bef3bae7261dd30c6df34cd9bdd3f8476`
- v1.26.14: `kindest/node:v1.26.14@sha256:5d548739ddef37b9318c70cb977f57bf3e5015e4552be4e27e57280a8cbb8e4f`
- v1.26.13: `kindest/node:v1.26.13@sha256:8cb4239d64ff897e0c21ad19fe1d68c3422d4f3c1c1a734b7ab9ccc76c549605`
- v1.25.16: `kindest/node:v1.25.16@sha256:e8b50f8e06b44bb65a93678a65a26248fae585b3d3c2a669e5ca6c90c69dc519`
- v1.24.17: `kindest/node:v1.24.17@sha256:bad10f9b98d54586cba05a7eaa1b61c6b90bfc4ee174fdc43a7b75ca75c95e51`
- v1.23.17: `kindest/node:v1.23.17@sha256:14d0a9a892b943866d7e6be119a06871291c517d279aedb816a4b4bc0ec0a5b3`

**NOTE**: You *must* use the `@sha256` digest to guarantee an image built for this release, until such a time as we switch to a different tagging scheme. Even then we will highly encourage digest pinning for security and reproducibility reasons.

See also: 
- https://kind.sigs.k8s.io/docs/user/quick-start/#creating-a-cluster
- https://kind.sigs.k8s.io/docs/user/quick-start/#building-images

NOTE: These node images support amd64 and arm64, both of our supported platforms. **You must use the same platform as your host,** for more context see https://github.com/kubernetes-sigs/kind/issues/2718

<h1 id="fixes">Fixes</h1>

- binfmt_misc no longer set by node images (regression in v0.20.0)
- fix runc hooks when non-root / usernamespaces
- Support multiple random extraPortMappings
- Docs fixes for contour and WSL2


<h1 id="contributors">Contributors</h1>

**Thank you to everyone who contributed to this release! ❤️**

Users whose commits are in this release (alphabetically by user name)


- @BenTheElder 
- @dependabot[bot]
- @dgl 
- @howardjohn 
- @k8s-ci-robot
- @r-suke 
- @skriss  
- @wouterh-dev 
- @Zumium 

Thank you as well to everyone else not listed here who contributed in other ways like filing issues, giving feedback, testing fixes, and helping users in slack!


