---
title: containerd v1.1 Release Notes
description: containerd v1.1 Release Notes — Kubernetes 生产运维知识库
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
estimated_read_time: 5min
intent_queries:
- containerd v1.1 Release Notes 是什么
- 如何 containerd v1.1 Release Notes
trigger_keywords:
- containerd
- v1.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# [[containerd|containerd]] v1.1 Release Notes

Source: [v1.1.8](https://github.com/containerd/containerd/releases/tag/v1.1.8)

Welcome to the v1.1.8 release of containerd!

This is the eighth patch release for the `containerd` 1.1 release. This is the
first 1.1 release during the extended security release period. Included is
a runc fix for [CVE-2019-16884](https://nvd.nist.gov/vuln/detail/CVE-2019-16884)
as well as fixes to close inherited sockets and cleanup on context cancellation
in CRI.

Go version has also been updated to 1.12 in this release.

Please see the changelog for full details.

Please try out the release binaries and report any issues at
https://github.com/containerd/containerd/issues.

### Contributors

* Lantao Liu
* Derek McGowan
* Sebastiaan van Stijn
* Michael Crosby
* Mike Brown
* Phil Estes
* Shukui Yang
* Wei Fu

### Changes

* [`2a82a9d2f4`](https://github.com/containerd/containerd/commit/2a82a9d2f4853df7a4820781a639cc81110a50e6) Merge pull request  [#3699](https://github.com/containerd/containerd/pull/3699) from dmcgowan/fix-release-notes-1.1.8
* [`c828c5d082`](https://github.com/containerd/containerd/commit/c828c5d082810c2b4769c9d693232eac1c879ec9) Fix typo in release notes
* [`28ec992407`](https://github.com/containerd/containerd/commit/28ec992407aa8ff997f0655bb5b013e186bae704) Merge pull request  [#3698](https://github.com/containerd/containerd/pull/3698) from dmcgowan/release-1.1.8
* [`07741b3e41`](https://github.com/containerd/containerd/commit/07741b3e41099fb07e52ed12d6bf1198243aade0) Add 1.1.8 release notes
* [`b852a4d01e`](https://github.com/containerd/containerd/commit/b852a4d01eb239f8093fbea3e6b24c0e77cba4cb) Update go to 1.12 in Travis
* [`05a30d22dd`](https://github.com/containerd/containerd/commit/05a30d22dd791189c5e15286ee25605d32bb8a39) Merge pull request  [#3693](https://github.com/containerd/containerd/pull/3693) from Random-Liu/update-cri-release-1.1
* [`36ebf628f3`](https://github.com/containerd/containerd/commit/36ebf628f3818a89718bb49cfdba30242164d87f) Update cri to 6cdb2faa1f30b5203c015dfc7abad44515dd8ccd.
* [`5b7f3ab2f4`](https://github.com/containerd/containerd/commit/5b7f3ab2f46240fc2505c7e5c35bdc5396d4d67d) Merge pull request  [#3691](https://github.com/containerd/containerd/pull/3691) from crosbymichael/waitgroup
* [`d74e8a9081`](https://github.com/containerd/containerd/commit/d74e8a9081a06f36e74c9b3115a8c80388f9bfb1) Add timeout for I/O waitgroups
* [`ed1b4ef982`](https://github.com/containerd/containerd/commit/ed1b4ef9828e0431268afb12fc020678d93d13c3) Merge pull request  [#3688](https://github.com/containerd/containerd/pull/3688) from crosbymichael/runc-cve
* [`735cdbf454`](https://github.com/containerd/containerd/commit/735cdbf45457b08698754c658c3aa3d2d47694b5) Update runc for CVE-2019-16884
* [`e9e200bf17`](https://github.com/containerd/containerd/commit/e9e200bf17e312c8aa452744296ee5bad0eccfec) Merge pull request  [#3539](https://github.com/containerd/containerd/pull/3539) from thaJeztah/1.1_revert_bump_libseccomp
* [`a17c0d2b6a`](https://github.com/containerd/containerd/commit/a17c0d2b6a2a4a9d54278543090c584e4d6e9ad0) Revert "bump libseccomp-golang v0.9.1"
* [`43278be18c`](https://github.com/containerd/containerd/commit/43278be18c37eabd94416b587bcc45562fb649a7) Merge pull request  [#3375](https://github.com/containerd/containerd/pull/3375) from thaJeztah/1.1_backport_bump_libseccomp
* [`f2d1981758`](https://github.com/containerd/containerd/commit/f2d1981758b0b3a8e39da9c367c3c733d97d6999) bump libseccomp-golang v0.9.1
* [`8c64d394b0`](https://github.com/containerd/containerd/commit/8c64d394b071f6b1ff4ee6378a1c71d65f1dc213) Merge pull request  [#3363](https://github.com/containerd/containerd/pull/3363) from keloyang/close-socket-fd-1.1
* [`37828134dd`](https://github.com/containerd/containerd/commit/37828134dd0edca5cae33781ba4bd99da9f1c41f) Close the inherited socket fd
* [`dde4c10719`](https://github.com/containerd/containerd/commit/dde4c10719d9ac448e1cdaf439e29419ccbf2946) Merge pull request  [#3265](https://github.com/containerd/containerd/pull/3265) from Random-Liu/cherry-pick-#3244-#3263-release-1.1
* [`74176450ef`](https://github.com/containerd/containerd/commit/74176450ef323b1b24af1f45096e42d6f4c60e00) Return NotFound error for kill and delete in deleted state.
* [`b72eee4904`](https://github.com/containerd/containerd/commit/b72eee4904477f5ddd20ba88b7ac390a4f38f3b8) Merge pull request  [#3248](https://github.com/containerd/containerd/pull/3248) from thaJeztah/1.1_backport_bump_runc_1.0.0rc8
* [`5994218757`](https://github.com/containerd/containerd/commit/59942187573e893647230bced0e568b997b38dfc) bump opencontainers/selinux v1.2.2
* [`61a64309c3`](https://github.com/containerd/containerd/commit/61a64309c330c6f9e6bbf59fd523d7a489be5087) bump runc v1.0.0-rc8

### Changes from containerd/cri

* [`f213a05f`](https://github.com/containerd/cri/commit/f213a05fa6700e49f77468fcfd5af10788565eee) Merge pull request  [#1303](https://github.com/containerd/cri/pull/1303) from Random-Liu/sync-vendor-release-1.0
* [`2162941f`](https://github.com/containerd/cri/commit/2162941fe9744c7ef4f21a54e867b885b20b3660) Sync vendors with containerd before release.
* [`6cdb2faa`](https://github.com/containerd/cri/commit/6cdb2faa1f30b5203c015dfc7abad44515dd8ccd) Merge pull request  [#1301](https://github.com/containerd/cri/pull/1301) from Random-Liu/cherrypick-#1156-release-1.1
* [`d76dcfd3`](https://github.com/containerd/cri/commit/d76dcfd3fad338e2ad483dc53b57471cfafa2411) Make sure exec process is killed when context is canceled.
* [`87beada3`](https://github.com/containerd/cri/commit/87beada3b71fdccb65e1cd3bd47b656c39461e47) Merge pull request  [#1192](https://github.com/containerd/cri/pull/1192) from thaJeztah/1.0_backport_bump_libseccomp
* [`736fed18`](https://github.com/containerd/cri/commit/736fed183f6999d3a9866f1eab67e75b783eb9b2) bump libseccomp-golang v0.9.1
* [`3ef3d896`](https://github.com/containerd/cri/commit/3ef3d8969c0a406d5e732f2b6a826c02f895fc8c) Merge pull request  [#1122](https://github.com/containerd/cri/pull/1122) from Random-Liu/update-containerd-release-1.0
* [`b09cad68`](https://github.com/containerd/cri/commit/b09cad681d5bf1377e487991b9ffcc4071b3f7d4) Update containerd to v1.1.7.
* [`5b93010f`](https://github.com/containerd/cri/commit/5b93010fdbce345c9aabf97cc6c8647def459a30) Merge pull request  [#1119](https://github.com/containerd/cri/pull/1119) from Random-Liu/cherrypick-#1118-release-1.0
* [`92c1be72`](https://github.com/containerd/cri/commit/92c1be7258862e1712c8692204c91731023bcba4) Cherrypick #1118 to release/1.0

### Dependency Changes

Previous release can be found at [v1.1.7](https://github.com/containerd/containerd/releases/tag/v1.1.7)

* **github.com/containerd/cri**          f8171b4530bed8992973cc4a2f24efe53b821d53 -> f213a05fa6700e49f77468fcfd5af10788565eee
* **github.com/opencontainers/runc**     029124da7af7360afa781a0234d1b083550f797c -> 3e425f80a8c931f88e6d94a8c831b9d5aa481657
* **github.com/opencontainers/selinux**  v1.2.1 -> v1.2.2
