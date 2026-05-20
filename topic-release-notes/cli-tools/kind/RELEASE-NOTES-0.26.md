---
title: kind v0.26 Release Notes
description: kind v0.26 Release Notes — Kubernetes 生产运维知识库
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
- kind v0.26 Release Notes 是什么
- 如何 kind v0.26 Release Notes
trigger_keywords:
- kind
- v0.26
- Release
- Notes
- release
- notes
---

# kind v0.26 Release Notes

Source: [v0.26.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.26.0)

This release brings Kubernetes v1.32.0 to kind by default and fixes `kind build node-image` for building for Kubernetes releases <v1.31.0 when not building Kubernetes from source (e.g. `kind build node-image v1.30.0`).

<h1 id="breaking-changes">Breaking Changes</h1>

- Removed two deprecated flags that had been printing usage warnings for many releases.
  - Removed long-deprecated `--kube-root` flag from `kind build node-image`, use `--type=source` and pass the path as an argument instead.
    - Docs: https://kind.sigs.k8s.io/docs/user/quick-start/#building-images
  - Removed long-deprecated `--loglevel` flag, use `-v` / `--verbosity` instead (see `kind help`)
- The default node image is now Kubernetes v1.32.0: `kindest/node:v1.32.0@sha256:c48c62eac5da28cdadcf560d1d8616cfa6783b58f0d94cf63ad1bf49600cb027`

<h1 id="new-features">New Features</h1>

- Updated to go 1.23.4
- Updated node image dependencies to latest
  - NOTE: you should only be depending on node images to allow kind to create Kubernetes nodes at a given Kubernetes version. The contents of these images are subject to change. However, we have brought everything up to the latest releases which may contain bug fixes.
- Updated local-path-provisioner to v0.30.0
- Refreshed Ingres docs to use https://sigs.k8s.io/cloud-provider-kind

Images pre-built for this release:

- v1.32.0: `kindest/node:v1.32.0@sha256:c48c62eac5da28cdadcf560d1d8616cfa6783b58f0d94cf63ad1bf49600cb027`
- v1.31.4: `kindest/node:v1.31.4@sha256:2cb39f7295fe7eafee0842b1052a599a4fb0f8bcf3f83d96c7f4864c357c6c30`
- v1.30.8: `kindest/node:v1.30.8@sha256:17cd608b3971338d9180b00776cb766c50d0a0b6b904ab4ff52fd3fc5c6369bf`
- v1.29.12: `kindest/node:v1.29.12@sha256:62c0672ba99a4afd7396512848d6fc382906b8f33349ae68fb1dbfe549f70dec`

**NOTE**: You *must* use the `@sha256` digest to guarantee an image built for this release, until such a time as we switch to a different tagging scheme. Even then we will highly encourage digest pinning for security and reproducibility reasons.

See also: 
- https://kind.sigs.k8s.io/docs/user/quick-start/#creating-a-cluster
- https://kind.sigs.k8s.io/docs/user/quick-start/#building-images

NOTE: These node images support amd64 and arm64, both of our supported platforms. **You must use the same platform as your host,** for more context see https://github.com/kubernetes-sigs/kind/issues/2718

<h1 id="fixes">Fixes</h1>

- Fixed `kind build node-image --type=url` and `kind build node-image --type=release` where Kubernetes version is less than `v1.31.0`
  - E.G. `kind build node-image v1.30.0` and `kind build node-image https://dl.k8s.io/v1.30.0/kubernetes-server-linux-arm64.tar.gz`
  - This fix will only work for standard release tags (and not pre-releases etc), for those continue to use `--type=source`.
  - v1.31.0+ will work with all build types for all versions

<h1 id="contributors">Contributors</h1>

**Thank you to everyone who contributed to this kind over the years!**

Commiters for this release:

- @aojea 
- @BenTheElder
- @dependabot[bot]
- @k8s-ci-robot 
- @stmcginnis 