---
title: containerd v2.1 Release Notes
description: containerd v2.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- containerd v2.1 Release Notes 是什么
- 如何 containerd v2.1 Release Notes
trigger_keywords:
- containerd
- v2.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# containerd v2.1 Release Notes

Source: [v2.1.6](https://github.com/containerd/containerd/releases/tag/v2.1.6)

Welcome to the v2.1.6 release of containerd!

The sixth patch release for containerd 2.1 contains various fixes and updates.

### Highlights

#### Runtime

* **Update runc binary to v1.3.4** ([#12618](https://github.com/containerd/containerd/pull/12618))

Please try out the release binaries and report any issues at
https://github.com/containerd/containerd/issues.

### Contributors

* Akihiro Suda
* Derek McGowan
* Mike Brown
* Phil Estes
* Austin Vazquez
* Kirtana Ashok
* Andrey Noskov
* CrazyMax
* Davanum Srinivas
* Krisztian Litkey
* Maksym Pavlenko
* Michael Weibel
* Paweł Gronowski
* Sebastiaan van Stijn
* Wei Fu

### Changes
<details><summary>28 commits</summary>
<p>

* Prepare release notes for v2.1.6 ([#12653](https://github.com/containerd/containerd/pull/12653))
  * [`93f79087a`](https://github.com/containerd/containerd/commit/93f79087acf3fc3f01e80b354fbe2cea771304b6) Prepare release notes for v2.1.6
* go.mod: containerd/zfs v2.0.0 ([#12655](https://github.com/containerd/containerd/pull/12655))
  * [`7e75db3a9`](https://github.com/containerd/containerd/commit/7e75db3a929414f46a3e7c8790ea0eec3288e394) build(deps): bump github.com/containerd/zfs/v2 from 2.0.0-rc.0 to 2.0.0
* cri/nri: short-circuit nil adjustment. ([#12673](https://github.com/containerd/containerd/pull/12673))
  * [`2b8e11b12`](https://github.com/containerd/containerd/commit/2b8e11b12b97ada3d9741ebb29d0d8088ee3cbb8) cri/nri: short-circuit nil adjustment.
* go.mod: github.com/containernetworking/plugins v1.9.0 ([#12659](https://github.com/containerd/containerd/pull/12659))
  * [`69efd067c`](https://github.com/containerd/containerd/commit/69efd067caca778588edd945a5e9f2a4feb156a6) go.mod: github.com/containernetworking/plugins v1.9.0
* go.mod: golang.org/x/crypto v0.45.0 (drop support for Go 1.23) ([#12639](https://github.com/containerd/containerd/pull/12639))
  * [`e81678853`](https://github.com/containerd/containerd/commit/e816788537ca2b9484aa86da58391923e873f571) go.mod: golang.org/x/crypto v0.45.0
  * [`55a2d8c8d`](https://github.com/containerd/containerd/commit/55a2d8c8d0bf6c5a7481c8922eb5a351f82f9344) CI: drop Go 1.23
  * [`fd8e3c39b`](https://github.com/containerd/containerd/commit/fd8e3c39b952b2fb9278df64f9de4e46fa78dd36) Update Go requirements in BUILDING
* core/runtime/v2: remove uses of otelgrpc.UnaryClientInterceptor ([#12623](https://github.com/containerd/containerd/pull/12623))
  * [`a4454c49a`](https://github.com/containerd/containerd/commit/a4454c49a66b48309c0d9a1f5c386daf5d692614) core/runtime/v2: remove uses of otelgrpc.UnaryClientInterceptor
* Update runc binary to v1.3.4 ([#12618](https://github.com/containerd/containerd/pull/12618))
  * [`251f0a285`](https://github.com/containerd/containerd/commit/251f0a2854f7831ca040d7ba2dab181fd02f2240) runc: Update runc binary to v1.3.4
* ci: bump Go 1.24.11, 1.25.5 ([#12626](https://github.com/containerd/containerd/pull/12626))
  * [`c07c29bca`](https://github.com/containerd/containerd/commit/c07c29bca60eeff9454b005e9856acfb12cfd68c) ci: bump Go 1.24.11, 1.25.5
  * [`e52817652`](https://github.com/containerd/containerd/commit/e528176522bc3df790ea923cfac15c18336ae429) ci: bump Go 1.24.10, 1.25.4
  * [`04bbb66e4`](https://github.com/containerd/containerd/commit/04bbb66e408602872693282063c080bb2e9e6cf9) ci(release): set GO_VERSION in Dockerfile
* ci: update CIFuzz actions to support Ubuntu 24.04 ([#12633](https://github.com/containerd/containerd/pull/12633))
  * [`492987ccc`](https://github.com/containerd/containerd/commit/492987cccf044c4015ec35ce161606cc514de75e) ci: update CIFuzz actions to support Ubuntu 24.04
* build(deps): bump github.com/opencontainers/selinux ([#12590](https://github.com/containerd/containerd/pull/12590))
  * [`55a25ec6e`](https://github.com/containerd/containerd/commit/55a25ec6efa335cdb9e6b56207643f33277be4d4) build(deps): bump github.com/opencontainers/selinux
* Redact all query parameters in CRI error logs ([#12547](https://github.com/containerd/containerd/pull/12547))
  * [`b72d0dfe0`](https://github.com/containerd/containerd/commit/b72d0dfe0458e1b5f1e67ba70476fc4887ee5f08) fix: redact all query parameters in CRI error logs
* Update 2.1 branch to no longer build as latest ([#12487](https://github.com/containerd/containerd/pull/12487))
  * [`ecd58bd65`](https://github.com/containerd/containerd/commit/ecd58bd6507cb6c566c68b440ceea5d7d99b3260) Update 2.1 branch to no longer build as latest
</p>
</details>

### Changes from containerd/platforms
<details><summary>5 commits</summary>
<p>

* use windowsMatchComparer for OSVersion match order ([containerd/platforms#25](https://github.com/containerd/platforms/pull/25))
  * [`8c0d9f9`](https://github.com/containerd/platforms/commit/8c0d9f9835bbe848b9c6f6f4a3a23f7dc97de927) use windowsMatchComparer for OSVersion match order
* Add WS2025 to Windows matcher and code optimizations ([containerd/platforms#24](https://github.com/containerd/platforms/pull/24))
  * [`8447b0a`](https://github.com/containerd/platforms/commit/8447b0ad126eb97a40c5bde800d38370a39ba52f) Update ci.yml
  * [`4549974`](https://github.com/containerd/platforms/commit/4549974181760492ffc528fae4d7f29620a2c67c) Add WS2025 to Windows matcher and code optimizations
</p>
</details>

### Dependency Changes

* **github.com/containerd/platforms**         v1.0.0-rc.1 -> v1.0.0-rc.2
* **github.com/containerd/zfs/v2**            v2.0.0-rc.0 -> v2.0.0
* **github.com/containernetworking/plugins**  v1.7.1 -> v1.9.0
* **github.com/coreos/go-systemd/v22**        v22.5.0 -> v22.6.0
* **github.com/cyphar/filepath-securejoin**   v0.5.1 **_new_**
* **github.com/go-logr/logr**                 v1.4.2 -> v1.4.3
* **github.com/opencontainers/selinux**       v1.12.0 -> v1.13.1
* **github.com/vishvananda/netlink**          0e7078ed04c8 -> v1.3.1
* **golang.org/x/crypto**                     v0.36.0 -> v0.45.0
* **golang.org/x/mod**                        v0.24.0 -> v0.29.0
* **golang.org/x/net**                        v0.38.0 -> v0.47.0
* **golang.org/x/sync**                       v0.14.0 -> v0.18.0
* **golang.org/x/sys**                        v0.33.0 -> v0.38.0
* **golang.org/x/term**                       v0.30.0 -> v0.37.0
* **golang.org/x/text**                       v0.23.0 -> v0.31.0
* **google.golang.org/protobuf**              v1.36.6 -> v1.36.7

Previous release can be found at [v2.1.5](https://github.com/containerd/containerd/releases/tag/v2.1.5)
### Which file should I download?
* `containerd-<VERSION>-<OS>-<ARCH>.tar.gz`:         ✅Recommended. Dynamically linked with glibc 2.35 (Ubuntu 22.04).
* `containerd-static-<VERSION>-<OS>-<ARCH>.tar.gz`:  Statically linked. Expected to be used on Linux distributions that do not use glibc >= 2.35. Not position-independent.

In addition to containerd, typically you will have to install [runc](https://github.com/opencontainers/runc/releases)
and [CNI plugins](https://github.com/containernetworking/plugins/releases) from their official sites too.

See also the [Getting Started](https://github.com/containerd/containerd/blob/main/docs/getting-started.md) documentation.
