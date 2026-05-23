---
title: kind v0.21 Release Notes
description: kind v0.21 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
- docker
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kind v0.21 Release Notes 是什么
- 如何 kind v0.21 Release Notes
trigger_keywords:
- kind
- v0.21
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# kind v0.21 Release Notes

Source: [v0.21.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.21.0)

This release patches the recent [runc CVEs](https://github.com/opencontainers/runc/security/advisories/GHSA-xr7r-f8xq-vfvv), as well as an issue with `kind build node-image` and docker v25.0.0+


<h1 id="breaking-changes">Breaking Changes</h1>

- The default node image is a [[Kubernetes|Kubernetes]] `v1.29.1` image: `kindest/node:v1.29.1@sha256:a0cc28af37cf39b019e2b448c54d1a3f789de32536cb5a5db61a49623e527144`

**If you haven't already, please see also [v0.20.0  release notes](https://github.com/kubernetes-sigs/kind/releases/tag/v0.20.0) which had important announcements that still apply going forward**.

<h1 id="new-features">New Features</h1>

- Upgraded go to 1.20.13
- Upgraded crictl to 1.28
- Upgraded [[containerd|containerd]] fuse overlayfs to 1.0.6
- Began marking some core images pinned in containerd, which may eventually make enabling imageGC safer
- kindnetd will ignore nodes with empty podCIDR, enabling some niche use-cases

Images pre-built for this release:
- v1.29.1: `kindest/node:v1.29.1@sha256:a0cc28af37cf39b019e2b448c54d1a3f789de32536cb5a5db61a49623e527144`
- v1.28.6: `kindest/node:v1.28.6@sha256:b7e1cf6b2b729f604133c667a6be8aab6f4dde5bb042c1891ae248d9154f665b`
- v1.27.10: `kindest/node:v1.27.10@sha256:3700c811144e24a6c6181065265f69b9bf0b437c45741017182d7c82b908918f`
- v1.26.13: `kindest/node:v1.26.13@sha256:15ae92d507b7d4aec6e8920d358fc63d3b980493db191d7327541fbaaed1f789`
- v1.25.16: `kindest/node:v1.25.16@sha256:9d0a62b55d4fe1e262953be8d406689b947668626a357b5f9d0cfbddbebbc727`
- v1.24.17: `kindest/node:v1.24.17@sha256:ea292d57ec5dd0e2f3f5a2d77efa246ac883c051ff80e887109fabefbd3125c7`
- v1.23.17: `kindest/node:v1.23.17@sha256:fbb92ac580fce498473762419df27fa8664dbaa1c5a361b5957e123b4035bdcf`

**NOTE**: You *must* use the `@sha256` digest to guarantee an image built for this release, until such a time as we switch to a different tagging scheme. Even then we will highly encourage digest pinning for security and reproducibility reasons.

See also: 
- https://kind.sigs.k8s.io/docs/user/quick-start/#creating-a-cluster
- https://kind.sigs.k8s.io/docs/user/quick-start/#building-images

NOTE: These node images support amd64 and arm64, both of our supported platforms. **You must use the same platform as your host,** for more context see https://github.com/kubernetes-sigs/kind/issues/2718

<h1 id="fixes">Fixes</h1>

- Updated runc to v1.1.12, containerd to v1.7.13 including the fix for https://github.com/opencontainers/runc/security/advisories/GHSA-xr7r-f8xq-vfvv
- Fixed `kind build node-image` with docker v25.0.0+
  - **NOTE**: `kind load docker-image` is still broken with Docker v25.0.0 due to a docker bug, which has a fix merged that should be included in Docker v25.0.1+
- Assorted docs fixes


<h1 id="contributors">Contributors</h1>

**Thank you to everyone who contributed to this release! ❤️**

Users whose commits are in this release (alphabetically by user name)

- @adelton 
- @ameukam 
- @AkihiroSuda 
- @AryanSharma9917
- @BenTheElder
- @bpfoster  
- @corneliusroemer 
- @dependabot[bot]
- @k8s-ci-robot 
- @kir4h 
- @liangyuanpeng
- @lixin963 
- @matzew 
- @mausearce 
- @ronaldpetty
- @roman-kiselenko 
- @saschagrunert 

Thank you as well to everyone else not listed here who contributed in other ways like filing issues, giving feedback, testing fixes, and helping users in slack!


