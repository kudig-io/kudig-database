---
title: containerd v1.4 Release Notes
description: containerd v1.4 Release Notes — Kubernetes 生产运维知识库
summary: containerd v1.4 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- containerd v1.4 Release Notes 是什么
- 如何 containerd v1.4 Release Notes
trigger_keywords:
- containerd
- v1.4
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[containerd|containerd]] v1.4 Release Notes

Source: [v1.4.13](https://github.com/containerd/containerd/releases/tag/v1.4.13)

Welcome to the v1.4.13 release of containerd!

The thirteenth patch release for containerd 1.4 is a security release to address
[CVE-2022-23648](https://github.com/containerd/containerd/security/advisories/GHSA-crp2-qrr5-8pq7).

### Notable Updates

* **Use fs.RootPath when mounting volumes** ([GHSA-crp2-qrr5-8pq7](https://github.com/containerd/containerd/security/advisories/GHSA-crp2-qrr5-8pq7))

See the changelog for complete list of changes

Please try out the release binaries and report any issues at
https://github.com/containerd/containerd/issues.

### Contributors

* Derek McGowan
* Sebastiaan van Stijn
* Phil Estes
* Akihiro Suda
* David [[Porter|Porter]]
* Kazuyoshi Kato
* Michael Crosby

### Changes
<details><summary>17 commits</summary>
<p>

* Github Security Advisory [GHSA-crp2-qrr5-8pq7](https://github.com/containerd/containerd/security/advisories/GHSA-crp2-qrr5-8pq7)
  * Prepare release notes for v1.4.13
  * Use fs.RootPath when mounting volumes
* [release/1.4] backport: Wait for containerd installation in GCE scripts ([#6553](https://github.com/containerd/containerd/pull/6553))
  * Wait for containerd installation in GCE scripts
* [release/1.4] Update Go to 1.16.14 ([#6527](https://github.com/containerd/containerd/pull/6527))
  * Do not use `go get` to install executables
  * [release/1.4] update Go to 1.16.14
  * [release/1.4] Update Go to 1.16.13
* [release/1.4] vendor: github.com/opencontainers/image-spec v1.0.2 ([#6265](https://github.com/containerd/containerd/pull/6265))
  * [release/1.4] vendor: github.com/opencontainers/image-spec v1.0.2
* [release/1.4] Update Go to 1.16.12 ([#6368](https://github.com/containerd/containerd/pull/6368))
  * [release/1.4] Update Go to 1.16.12
* [release/1.4] update runc binary to v1.0.3 ([#6344](https://github.com/containerd/containerd/pull/6344))
  * update runc binary to v1.0.3
* [release/1.4] Update Go to 1.16.11 ([#6335](https://github.com/containerd/containerd/pull/6335))
  * [release/1.4] Update Go to 1.16.11
</p>
</details>

### Changes from containerd/cri
<details><summary>4 commits</summary>
<p>

* [release/1.4] Use fs.RootPath when mounting volumes ([#1655](https://github.com/containerd/cri/pull/1655))
  * Use fs.RootPath when mounting volumes
* [release/1.4] update Go 1.15.14 (to match containerd) ([#1645](https://github.com/containerd/cri/pull/1645))
  * [release/1.4] update Go 1.15.14 (to match containerd)
</p>
</details>

### Dependency Changes

* **github.com/containerd/cri**             3b02bec16031 -> 8f1a8a1fb9eb
* **github.com/opencontainers/image-spec**  v1.0.1 -> v1.0.2

Previous release can be found at [v1.4.12](https://github.com/containerd/containerd/releases/tag/v1.4.12)
