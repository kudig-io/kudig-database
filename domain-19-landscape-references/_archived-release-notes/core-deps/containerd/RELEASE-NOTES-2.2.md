---
title: containerd v2.2 Release Notes
description: containerd v2.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- containerd v2.2 Release Notes 是什么
- 如何 containerd v2.2 Release Notes
trigger_keywords:
- containerd
- v2.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
created: "2026-05-23"
---

# [[containerd|containerd]] v2.2 Release Notes

Source: [v2.2.2](https://github.com/containerd/containerd/releases/tag/v2.2.2)

Welcome to the v2.2.2 release of containerd!

The second patch release for containerd 2.2 contains various fixes and improvements.

### Highlights

#### [[Container Runtime|Container Runtime]] Interface (CRI)

* Fix migrated CRI image config when using legacy registry mirrors ([#12987](https://github.com/containerd/containerd/pull/12987))
* Unpack images with per-layer labels for runtime-specific snapshotters ([#12936](https://github.com/containerd/containerd/pull/12936))
* Fix CNI issue where DEL is never executed after a restart ([#12926](https://github.com/containerd/containerd/pull/12926))
* Harden error handling to strip potentially-sensitive registry parameters ([#12804](https://github.com/containerd/containerd/pull/12804))
* Fix nil pointer dereference in container spec memory metrics when memory constraints are not fully configured ([#12731](https://github.com/containerd/containerd/pull/12731))
* Use the specified runtime handler when pulling images ([#12721](https://github.com/containerd/containerd/pull/12721))
* Reduce noisy CDI logs ([#12717](https://github.com/containerd/containerd/pull/12717))
* Fix regression for pulling encrypted images ([#12712](https://github.com/containerd/containerd/pull/12712))

#### Runtime

* Fix unintended dropping of mount flags for read-only bind-mounts in user namespaces ([#12944](https://github.com/containerd/containerd/pull/12944))
* Fix AppArmor bug disallowing unix domain sockets on newer kernels ([#12897](https://github.com/containerd/containerd/pull/12897))

#### ctr development tool

* Fix `ctr image mount` failing with "no such device" ([#12831](https://github.com/containerd/containerd/pull/12831))

Please try out the release binaries and report any issues at
https://github.com/containerd/containerd/issues.

### Contributors

* Maksym Pavlenko
* Akhil Mohan
* Samuel Karp
* Wei Fu
* Michael Zappa
* Phil Estes
* Fabiano Fidêncio
* Jérôme Poulin
* Luke Hinds
* Aadhar Agarwal
* Akihiro Suda
* Alex Chernyakhovsky
* Chris Adeniyi-Jones
* Kazuyoshi Kato
* Rodrigo Campos
* Sebastiaan van Stijn
* You Binhao
* ningmingxiao
* qiuxue

### Changes
<details><summary>48 commits</summary>
<p>

* Prepare release notes for v2.2.2 ([#12998](https://github.com/containerd/containerd/pull/12998))
  * [`7e6ecf434`](https://github.com/containerd/containerd/commit/7e6ecf43421f9cfa64cd7043f86ae224dc7dc0a4) Prepare release notes for v2.2.2
* Fix migrated CRI image config when using legacy registry mirrors ([#12987](https://github.com/containerd/containerd/pull/12987))
  * [`a20dead7c`](https://github.com/containerd/containerd/commit/a20dead7cc644291433b2da4b1efa2f70c8a144f) set default config_path in plugin init
* Unpack images with per-layer labels for runtime-specific snapshotters ([#12936](https://github.com/containerd/containerd/pull/12936))
  * [`a5f83d8c2`](https://github.com/containerd/containerd/commit/a5f83d8c2b419a3f882182d5beca60725387f499) cri: unpack images with per-layer labels for runtime-specific snapshotters
* ci: modprobe xt_comment on almalinux ([#12957](https://github.com/containerd/containerd/pull/12957))
  * [`68855cb0b`](https://github.com/containerd/containerd/commit/68855cb0be5d372fd53c450e91cc3224157abb4b) ci: modprobe xt_comment on almalinux
* Fix unintended dropping of mount flags for read-only bind-mounts in user namespaces ([#12944](https://github.com/containerd/containerd/pull/12944))
  * [`ef7a8beb3`](https://github.com/containerd/containerd/commit/ef7a8beb375c8322b9a09666f50150717b9ae335) core/mount: add test for getUnprivilegedMountFlags
  * [`07b2cc07e`](https://github.com/containerd/containerd/commit/07b2cc07e4f3d553c5ca801c9f0800b55ba7eac2) core/mount: fix getUnprivilegedMountFlags iterating over indices instead of values
* Fix CNI issue where DEL is never executed after a restart ([#12926](https://github.com/containerd/containerd/pull/12926))
  * [`54101116f`](https://github.com/containerd/containerd/commit/54101116fcdf18e21c8d202f86ed93c34a5932af) add integration test for cni result nil
  * [`d44c4384e`](https://github.com/containerd/containerd/commit/d44c4384ec9f7adef9a4598e05f12e0850338fd8) address comment
  * [`f1835270b`](https://github.com/containerd/containerd/commit/f1835270b0b800e4c1ba13391cd4a75617810615) fix issue where cni del is never executed
* Fix AppArmor bug disallowing unix domain sockets on newer kernels ([#12897](https://github.com/containerd/containerd/pull/12897))
  * [`6c05047b4`](https://github.com/containerd/containerd/commit/6c05047b4ba86d2fb857429c6272bb66679e7dee) apparmor: explicitly set abi/3.0
* ci: add build/test go1.26.0, drop go1.24 ([#12917](https://github.com/containerd/containerd/pull/12917))
  * [`5dbf1b915`](https://github.com/containerd/containerd/commit/5dbf1b91596e35247f5928ad202da2a378859703) update golangci-lint to v2.9.0 with go1.26 support
  * [`8ec695ebe`](https://github.com/containerd/containerd/commit/8ec695ebe8b6f8ec4fbd4ebbe658a2aaa35ac857) remove windows/arm from cross build
  * [`b9c22a6e3`](https://github.com/containerd/containerd/commit/b9c22a6e39a937e86723bac0b63e30587cd8e936) ci: build/test go1.26.0
* integration: Fix TestImageLoad() failure on CI ([#12906](https://github.com/containerd/containerd/pull/12906))
  * [`09b876a81`](https://github.com/containerd/containerd/commit/09b876a8198818ab7d59e9037e6592889faea861) integration: Fix TestImageLoad() failure on CI
* cri: Fix image volumes with user namespaces ([#12885](https://github.com/containerd/containerd/pull/12885))
  * [`172ba65b6`](https://github.com/containerd/containerd/commit/172ba65b6a89479865832a7101f10e1b3a323d78) cri: Fix image volumes with user namespaces
* update to go1.24.13, go1.25.7 ([#12871](https://github.com/containerd/containerd/pull/12871))
  * [`b4240ef87`](https://github.com/containerd/containerd/commit/b4240ef8782d274b97554881cec65aa8b1da0d2c) update to go1.24.13, go1.25.7
  * [`94dbfaea7`](https://github.com/containerd/containerd/commit/94dbfaea7295d65c11f36510abc558e6e01c9205) ci: bump go 1.24.12, 1.25.6
* ci: set fetch-depth for containerd to 0 for version parsing ([#12875](https://github.com/containerd/containerd/pull/12875))
  * [`e46a7a286`](https://github.com/containerd/containerd/commit/e46a7a28682e79b9d851ea4de1840eb0dcf555b5) set fetch-depth for containerd to 0 for version parsing
* Fix `ctr image mount` failing with "no such device" ([#12831](https://github.com/containerd/containerd/pull/12831))
  * [`1d7908273`](https://github.com/containerd/containerd/commit/1d79082735d46fe24ded00a55ea6e3a33954593e) core/mount/manager: fix bind mount missing rbind option
  * [`3d509bcd3`](https://github.com/containerd/containerd/commit/3d509bcd335b15cece69ebfa117681d2715df930) core/mount/manager: add tests for WithTemporary option
* Harden error handling to strip potentially-sensitive registry parameters ([#12804](https://github.com/containerd/containerd/pull/12804))
  * [`cb3ae2119`](https://github.com/containerd/containerd/commit/cb3ae211952909a5c4d9fcb274e029286057fc34) fix: sanitize error before gRPC return to prevent credential leak in pod events
* bump google.golang.org/grpc from 1.76.0 to 1.78.0 ([#12739](https://github.com/containerd/containerd/pull/12739))
  * [`533a2552e`](https://github.com/containerd/containerd/commit/533a2552e9e1ff1896868986240f493e9f488920) build(deps): bump google.golang.org/grpc from 1.77.0 to 1.78.0
  * [`b120237fb`](https://github.com/containerd/containerd/commit/b120237fb6af3b65117ba83af204cf92790acff3) build(deps): bump google.golang.org/grpc from 1.76.0 to 1.77.0
* Fix nil pointer dereference in container spec memory metrics when memory constraints are not fully configured ([#12731](https://github.com/containerd/containerd/pull/12731))
  * [`4be4e5156`](https://github.com/containerd/containerd/commit/4be4e5156c1bfdd84f12bb43424261e3b5578208) Fix nil pointer dereference in container spec memory metrics
* cri: emit warning for concurrent CreateContainer ([#12735](https://github.com/containerd/containerd/pull/12735))
  * [`a76eb698a`](https://github.com/containerd/containerd/commit/a76eb698a52f1eb3018fe6126587dcf36fad4e7b) cri: emit warning for concurrent CreateContainer
* Use the specified runtime handler when pulling images ([#12721](https://github.com/containerd/containerd/pull/12721))
  * [`3d2e188b1`](https://github.com/containerd/containerd/commit/3d2e188b15d7db18f87251eaf134da463f36a8c8) cri: Use the runtimeHandler parameter in PullImage
* Reduce noisy CDI logs ([#12717](https://github.com/containerd/containerd/pull/12717))
  * [`633057382`](https://github.com/containerd/containerd/commit/633057382e7bfd16523865928549b38e0aa0b7e2) cri: move noisy CDI logs to debug level
* Fix regression for pulling encrypted images ([#12712](https://github.com/containerd/containerd/pull/12712))
  * [`8a7409e2e`](https://github.com/containerd/containerd/commit/8a7409e2e71fd9486db3504ab804d4419e45af41) Reinstate image decryption
</p>
</details>

### Dependency Changes

* **github.com/go-jose/go-jose/v4**              v4.1.2 -> v4.1.3
* **go.opentelemetry.io/auto/sdk**               v1.1.0 -> v1.2.1
* **go.opentelemetry.io/otel**                   v1.37.0 -> v1.38.0
* **go.opentelemetry.io/otel/metric**            v1.37.0 -> v1.38.0
* **go.opentelemetry.io/otel/sdk**               v1.37.0 -> v1.38.0
* **go.opentelemetry.io/otel/trace**             v1.37.0 -> v1.38.0
* **golang.org/x/oauth2**                        v0.30.0 -> v0.32.0
* **google.golang.org/genproto/googleapis/api**  a7a43d27e69b -> ab9386a59fda
* **google.golang.org/genproto/googleapis/rpc**  a7a43d27e69b -> ab9386a59fda
* **google.golang.org/grpc**                     v1.76.0 -> v1.78.0

Previous release can be found at [v2.2.1](https://github.com/containerd/containerd/releases/tag/v2.2.1)
### Which file should I download?
* `containerd-<VERSION>-<OS>-<ARCH>.tar.gz`:         ✅Recommended. Dynamically linked with glibc 2.35 (Ubuntu 22.04).
* `containerd-static-<VERSION>-<OS>-<ARCH>.tar.gz`:  Statically linked. Expected to be used on Linux distributions that do not use glibc >= 2.35. Not position-independent.

In addition to containerd, typically you will have to install [runc](https://github.com/opencontainers/runc/releases)
and [CNI plugins](https://github.com/containernetworking/plugins/releases) from their official sites too.

See also the [Getting Started](https://github.com/containerd/containerd/blob/main/docs/getting-started.md) documentation.
