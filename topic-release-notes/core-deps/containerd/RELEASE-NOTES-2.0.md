---
title: containerd v2.0 Release Notes
description: containerd v2.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
- docker
- job
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- containerd v2.0 Release Notes 是什么
- 如何 containerd v2.0 Release Notes
trigger_keywords:
- containerd
- v2.0
- Release
- Notes
- release
- notes
---


# containerd v2.0 Release Notes

Source: [v2.0.7](https://github.com/containerd/containerd/releases/tag/v2.0.7)

Welcome to the v2.0.7 release of containerd!

The seventh patch release for containerd 2.0 includes various bug fixes and updates.

### Security Updates

* **containerd**
  * [**GHSA-pwhc-rpq9-4c8w**](https://github.com/containerd/containerd/security/advisories/GHSA-pwhc-rpq9-4c8w)
  * [**GHSA-m6hq-p25p-ffr2**](https://github.com/containerd/containerd/security/advisories/GHSA-m6hq-p25p-ffr2)

* **runc**
  * [**GHSA-qw9x-cqr3-wc7r**](https://github.com/opencontainers/runc/security/advisories/GHSA-qw9x-cqr3-wc7r)
  * [**GHSA-cgrx-mc8f-2prm**](https://github.com/opencontainers/runc/security/advisories/GHSA-cgrx-mc8f-2prm)
  * [**GHSA-9493-h29p-rfm2**](https://github.com/opencontainers/runc/security/advisories/GHSA-9493-h29p-rfm2)

### Highlights

#### Container Runtime Interface (CRI)

* **Disable event subscriber during task cleanup** ([#12406](https://github.com/containerd/containerd/pull/12406))
* **Add SystemdCgroup to default runtime options** ([#12254](https://github.com/containerd/containerd/pull/12254))
* **Fix userns with container image VOLUME mounts that need copy** ([#12241](https://github.com/containerd/containerd/pull/12241))

#### Image Distribution

* **Add dial timeout field to hosts toml configuration** ([#12136](https://github.com/containerd/containerd/pull/12136))

#### Runtime

* **Update runc binary to v1.3.3** ([#12479](https://github.com/containerd/containerd/pull/12479))
* **Fix lost container logs from quickly closing io** ([#12376](https://github.com/containerd/containerd/pull/12376))
* **Create bootstrap.json with 0644 permission** ([#12184](https://github.com/containerd/containerd/pull/12184))
* **Fix pidfd leak in UnshareAfterEnterUserns** ([#12178](https://github.com/containerd/containerd/pull/12178))

Please try out the release binaries and report any issues at
https://github.com/containerd/containerd/issues.

### Contributors

* Austin Vazquez
* Phil Estes
* Rodrigo Campos
* Wei Fu
* Akihiro Suda
* Derek McGowan
* Maksym Pavlenko
* ningmingxiao
* Kirtana Ashok
* Akhil Mohan
* Andrew Halaney
* Jin Dong
* Jose Fernandez
* Mike Baynton
* Philip Laine
* Swagat Bora
* wheat2018

### Changes
<details><summary>56 commits</summary>
<p>

* Prepare release notes for v2.0.7 ([#12482](https://github.com/containerd/containerd/pull/12482))
  * [`4931e24f1`](https://github.com/containerd/containerd/commit/4931e24f169091cb4e425b7bfdd4fb0d3c20543b) Prepare release notes for v2.0.7
  * [`205bc4f2d`](https://github.com/containerd/containerd/commit/205bc4f2dbce3df32d2d5140a3d039332b02dbe6) Update mailmap
  * [`5f708b76a`](https://github.com/containerd/containerd/commit/5f708b76a41a1cf56e167971e271c7581cb2f8cb) Merge commit from fork
  * [`8cd112d82`](https://github.com/containerd/containerd/commit/8cd112d8295bafcf4a992816ff9e07f5a78ff71b) Fix directory permissions
  * [`05290b5bc`](https://github.com/containerd/containerd/commit/05290b5bc8fd938c8f77856927a280a1d5eec7b6) Merge commit from fork
  * [`4d1edf4ad`](https://github.com/containerd/containerd/commit/4d1edf4addf8c31b096680f04fee499cabc75439) fix goroutine leak of container Attach
* Update runc binary to v1.3.3 ([#12479](https://github.com/containerd/containerd/pull/12479))
  * [`b46dc6a67`](https://github.com/containerd/containerd/commit/b46dc6a67cc575a83db083f71dcdbc722605c841) runc: Update runc binary to v1.3.3
* ci: bump Go 1.24.9; 1.25.3 ([#12361](https://github.com/containerd/containerd/pull/12361))
  * [`5e9c82178`](https://github.com/containerd/containerd/commit/5e9c821780ff705c47406bf7a72d476da398135c) Update GHA runners to use latest images for basic binaries build
  * [`7f59248dc`](https://github.com/containerd/containerd/commit/7f59248dcd66cb0c418669a880e0c1d7e48e0dfa) Update GHA runners to use latest image for most jobs
  * [`e1373e8a8`](https://github.com/containerd/containerd/commit/e1373e8a8abf94b28507972694f8aea17f1b10c5) ci: bump Go 1.24.9, 1.25.3
  * [`e1a910a6a`](https://github.com/containerd/containerd/commit/e1a910a6a989b2cb0ed1ae4fda42eacbb6361e4b) ci: bump Go 1.24.8; 1.25.2
  * [`fd04b7f17`](https://github.com/containerd/containerd/commit/fd04b7f176ec52d17e0014d07d045d6fa79bd620) move exclude-dirs to issues.exclude-dirs
  * [`b49377975`](https://github.com/containerd/containerd/commit/b493779751d070255648e6b1e75dc9af8516c347) update golangci-lint to v1.64.2
  * [`6e45022a1`](https://github.com/containerd/containerd/commit/6e45022a1e01fd1c78217df3b271d7735b8c4440) build(deps): bump golangci/golangci-lint-action from 6.3.2 to 6.5.0
  * [`09ce0f2a1`](https://github.com/containerd/containerd/commit/09ce0f2a1ee5b1524c95c3831ed124e5d4fd0a2b) build(deps): bump golangci/golangci-lint-action from 6.2.0 to 6.3.2
  * [`de63a740b`](https://github.com/containerd/containerd/commit/de63a740b8108c62bf018cf1f508bb9e5842bfd2) build(deps): bump golangci/golangci-lint-action from 6.1.1 to 6.2.0
* Fix lost container logs from quickly closing io ([#12376](https://github.com/containerd/containerd/pull/12376))
  * [`f953ee8a3`](https://github.com/containerd/containerd/commit/f953ee8a3c1feeaa60a3c9d386afa424040d56de) bugfix:fix container logs lost because io close too quickly
* CI: update Fedora to 43 ([#12448](https://github.com/containerd/containerd/pull/12448))
  * [`f6f15f513`](https://github.com/containerd/containerd/commit/f6f15f5135d313309a76fc6545e7cf86653d2f6e) CI: update Fedora to 43
* Disable event subscriber during task cleanup ([#12406](https://github.com/containerd/containerd/pull/12406))
  * [`2a2329cbd`](https://github.com/containerd/containerd/commit/2a2329cbd02dc5e1a3010730fab01c618dad768c) cri/server/podsandbox: disable event subscriber
* CI: skip ubuntu-24.04-arm on private repos ([#12428](https://github.com/containerd/containerd/pull/12428))
  * [`dfb954743`](https://github.com/containerd/containerd/commit/dfb95474370ef22c4555178a3d7cc34df2a3f5bc) CI: skip ubuntu-24.04-arm on private repos
* Remove additional fuzzers from instrumentation repo ([#12420](https://github.com/containerd/containerd/pull/12420))
  * [`f6b02f6bb`](https://github.com/containerd/containerd/commit/f6b02f6bb81dc079f60d421347c931c73d4227e7) Remove additional fuzzers from CI
* runc:Update runc binary to v1.3.1 ([#12275](https://github.com/containerd/containerd/pull/12275))
  * [`75c13ee3f`](https://github.com/containerd/containerd/commit/75c13ee3fc3657ee419395e20820d1cbd4bb2f88) runc:Update runc binary to v1.3.1
* Add SystemdCgroup to default runtime options ([#12254](https://github.com/containerd/containerd/pull/12254))
  * [`427cdd06c`](https://github.com/containerd/containerd/commit/427cdd06c9d093ede03384c550a440b0522e44ba) add SystemdCgroup to default runtime options
* install-runhcs-shim: fetch target commit instead of tags ([#12255](https://github.com/containerd/containerd/pull/12255))
  * [`0b35e19fb`](https://github.com/containerd/containerd/commit/0b35e19fb118b5144a75397522e476d0571ae9ef) install-runhcs-shim: fetch target commit instead of tags
* Fix userns with container image VOLUME mounts that need copy ([#12241](https://github.com/containerd/containerd/pull/12241))
  * [`3212afc2f`](https://github.com/containerd/containerd/commit/3212afc2f2d464157bcb24663360ee7dfa7207e6) integration: Add test for directives with userns
  * [`b855c6e10`](https://github.com/containerd/containerd/commit/b855c6e10372eb43d51186ab156cdce3d9eefb04) cri: Fix userns with Dockerfile VOLUME mounts that need copy
* Fix overlayfs issues related to user namespace ([#12223](https://github.com/containerd/containerd/pull/12223))
  * [`05c0c99f4`](https://github.com/containerd/containerd/commit/05c0c99f432b341152b54ce49d9b43c5cf3d131f) core/mount: Retry unmounting idmapped directories
  * [`afdede4ce`](https://github.com/containerd/containerd/commit/afdede4ced8c848191062b31dfcff1352161a844) core/mount: Test cleanup of DoPrepareIDMappedOverlay()
  * [`47205f814`](https://github.com/containerd/containerd/commit/47205f814d552a4eea9935375dd2f0874e107e5b) core/mount: Properly cleanup on doPrepareIDMappedOverlay errors
  * [`6f4abd970`](https://github.com/containerd/containerd/commit/6f4abd970aeea241f07edc1e0fd74f69a9a05979) core/mount: Don't call nil function on errors
  * [`a2f0d65d7`](https://github.com/containerd/containerd/commit/a2f0d65d78871832da6d2aa452aeeb180cd6d8f5) core/mount: Only idmap once per overlayfs, not per layer
  * [`1c32accd7`](https://github.com/containerd/containerd/commit/1c32accd71d34e3cb5798214adf26911609d11f1) Make ovl idmap mounts read-only
* ci: bump Go 1.23.12, 1.24.6 ([#12187](https://github.com/containerd/containerd/pull/12187))
  * [`9e72e91e6`](https://github.com/containerd/containerd/commit/9e72e91e63a75147f2a082565fc580babee8af06) ci: bump Go 1.23.12, 1.24.6
* Create bootstrap.json with 0644 permission ([#12184](https://github.com/containerd/containerd/pull/12184))
  * [`009622e04`](https://github.com/containerd/containerd/commit/009622e0424fa4234d67272339fb7e282c302190) fix: create bootstrap.json with 0644 permission
* Fix pidfd leak in UnshareAfterEnterUserns ([#12178](https://github.com/containerd/containerd/pull/12178))
  * [`5bec0a332`](https://github.com/containerd/containerd/commit/5bec0a33297ad485f96116efb333ea750a27c926) sys: fix pidfd leak in UnshareAfterEnterUserns
* Fix windows test failures ([#12120](https://github.com/containerd/containerd/pull/12120))
  * [`2a2488131`](https://github.com/containerd/containerd/commit/2a2488131e3602bbbecf4afa11d0f3e4135f01a4) Fix intermittent test failures on Windows CIs
  * [`018470948`](https://github.com/containerd/containerd/commit/018470948db89512760e9c25d4c5da9c7bef5321) Remove WS2025 from CIs due to regression
* Add dial timeout field to hosts toml configuration ([#12136](https://github.com/containerd/containerd/pull/12136))
  * [`b50cbbc98`](https://github.com/containerd/containerd/commit/b50cbbc98550580b2baf5565ec5f1a3ded422b0e) Add dial timeout field to hosts toml configuration
</p>
</details>

### Dependency Changes

This release has no dependency changes

Previous release can be found at [v2.0.6](https://github.com/containerd/containerd/releases/tag/v2.0.6)
### Which file should I download?
* `containerd-<VERSION>-<OS>-<ARCH>.tar.gz`:         ✅Recommended. Dynamically linked with glibc 2.31 (Ubuntu 20.04).
* `containerd-static-<VERSION>-<OS>-<ARCH>.tar.gz`:  Statically linked. Expected to be used on non-glibc Linux distributions. Not position-independent.

In addition to containerd, typically you will have to install [runc](https://github.com/opencontainers/runc/releases)
and [CNI plugins](https://github.com/containernetworking/plugins/releases) from their official sites too.

See also the [Getting Started](https://github.com/containerd/containerd/blob/main/docs/getting-started.md) documentation.
