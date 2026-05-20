---
title: containerd v1.2 Release Notes
description: containerd v1.2 Release Notes — Kubernetes 生产运维知识库
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
- containerd v1.2 Release Notes 是什么
- 如何 containerd v1.2 Release Notes
trigger_keywords:
- containerd
- v1.2
- Release
- Notes
- release
- notes
---


# containerd v1.2 Release Notes

Source: [v1.2.14](https://github.com/containerd/containerd/releases/tag/v1.2.14)

Welcome to the v1.2.14 release of containerd!

The fourteenth patch release for `containerd` 1.2 is a security release to fix CVE-2020-15157.

### Security Fixes

* Fix bug which allowed manifests to coerce containerd clients into leaking registry credentials [GHSA-742w-89gc-8m9c](https://github.com/containerd/containerd/security/advisories/GHSA-742w-89gc-8m9c)

### Included Changes

* Fix regression pushing manifests as octet stream [#4268](https://github.com/containerd/containerd/pull/4268)
* Update golang version

Please try out the release binaries and report any issues at
https://github.com/containerd/containerd/issues.

### Contributors

* Sebastiaan van Stijn
* Michael Crosby
* Phil Estes
* Maksym Pavlenko
* Akihiro Suda
* Derek McGowan
* Wei Fu
* Chris C
* Davanum Srinivas
* Erik Sipsma
* Sergey Kanzhelev
* Ted Yu
* Tobias Klauser
* Ulysses Souza
* Zhiyu Li

### Changes
<details><summary>48 commits</summary>
<p>

* [`f8777f130`](https://github.com/containerd/containerd/commit/f8777f13022dd16c2a339d621bb55465fe603b19) Add release notes for v1.2.14
* [`abbb17959`](https://github.com/containerd/containerd/commit/abbb17959f55bbb9b7eb37f965d7dad2f4ea8744) Add comment clarifying fix for security issue
* [`1ead8d9de`](https://github.com/containerd/containerd/commit/1ead8d9deb3b175bf40413b8c47b3d19c2262726) treat manifest provided URLs differently
* [`7f1f9b1cb`](https://github.com/containerd/containerd/commit/7f1f9b1cbcc4c3081581a1c89cf163f909cb5446) Merge pull request  [#4464](https://github.com/containerd/containerd/pull/4464) from thaJeztah/1.2_backport_bump_golang_1.13.15
* [`f52fbb8a9`](https://github.com/containerd/containerd/commit/f52fbb8a9ecd6afd9be112e016e3e71593779735) Bump Golang 1.13.15
* [`0732aa7a6`](https://github.com/containerd/containerd/commit/0732aa7a6611dffeac0ae41f8a4bf3f18923d5cd) Bump Golang 1.13.14
* [`f6b342959`](https://github.com/containerd/containerd/commit/f6b34295960d701490ddbbd61d02a99c4e36a6ad) Bump Go 1.13.13
* [`0a454c2f7`](https://github.com/containerd/containerd/commit/0a454c2f7b55a3eb75b095fb14b35c9c24c39cf9) Merge pull request  [#4339](https://github.com/containerd/containerd/pull/4339) from AkihiroSuda/golang-1.13.12-containerd1.2
* [`2f4dfde54`](https://github.com/containerd/containerd/commit/2f4dfde54fbf21fa148962acf2b54c5a94645ea0) Bump Golang 1.13.12
* [`48cc59890`](https://github.com/containerd/containerd/commit/48cc59890abbd0f0f88eb2014569f4ee7434582b) Merge pull request  [#4319](https://github.com/containerd/containerd/pull/4319) from hakman/runc-selinux-1.2
* [`cbdfca815`](https://github.com/containerd/containerd/commit/cbdfca8157f00c278105e4391e144c86e332520b) Build runc with selinux support
* [`3b72766af`](https://github.com/containerd/containerd/commit/3b72766af2f4404b9a09e3bd9608692949ad4e25) Merge pull request  [#4268](https://github.com/containerd/containerd/pull/4268) from dmcgowan/1.2-fix-bad-backport-push-octet-stream
* [`f8ae16778`](https://github.com/containerd/containerd/commit/f8ae167780e2f00df6d1f58f0833e11ff487c57c) Fix incorrect backport of setting octet-stream
* [`d4242f0d3`](https://github.com/containerd/containerd/commit/d4242f0d3c09b47c5a483807291ec3a2564bbc19) Merge pull request  [#4270](https://github.com/containerd/containerd/pull/4270) from estesp/travis-ci-fixes
* [`17a506c94`](https://github.com/containerd/containerd/commit/17a506c94f453ca678fc4bb844fa918a9a29481a) golangci-lint update and fix
* [`05bf3d63a`](https://github.com/containerd/containerd/commit/05bf3d63a66eb202ffe949d2cdfbc2c71e41d698) Merge pull request  [#4173](https://github.com/containerd/containerd/pull/4173) from thaJeztah/1.2_backport_bump_golang_1.13
* [`4f6dc01a8`](https://github.com/containerd/containerd/commit/4f6dc01a864d41659ea7dad0532c914bd6555b4f) Bump Golang 1.13.10
* [`493665bd5`](https://github.com/containerd/containerd/commit/493665bd53e35fda1ca183f666bad3ed8c49e4ff) Bump Golang 1.13.9
* [`edc830f98`](https://github.com/containerd/containerd/commit/edc830f984f9905194fd1427f5a20833bf558d39) Merge pull request  [#4149](https://github.com/containerd/containerd/pull/4149) from thaJeztah/1.2_backport_bump_console
* [`053f4d6fd`](https://github.com/containerd/containerd/commit/053f4d6fd856727e48fd9a537753e3146bcdfcb5) Update containerd/console vendor for fix
* [`e72c2b5b1`](https://github.com/containerd/containerd/commit/e72c2b5b16a1e2cc003d7335472dbc74cbe4ca52) Bump containerd console for os.File changes
* [`8810a1387`](https://github.com/containerd/containerd/commit/8810a1387d3ab4ae681f40f1deae3b7f5199b343) bump containerd/console 0650fd9eeb50bab4fc99dceb9f2e14cf58f36e7f
* [`b3b1ef317`](https://github.com/containerd/containerd/commit/b3b1ef317266073e0d694db10a6e06e7400379b3) Merge pull request  [#4121](https://github.com/containerd/containerd/pull/4121) from payall4u/hotfix-delete-container-error
* [`f8be3cf7f`](https://github.com/containerd/containerd/commit/f8be3cf7fb73159f9a38785cc29b5702acf0b4d8) when kill container, check if container has been deleted
* [`8403abc6f`](https://github.com/containerd/containerd/commit/8403abc6f86d4603413206daaa065fc08d7bc3fc) Merge pull request  [#4060](https://github.com/containerd/containerd/pull/4060) from thaJeztah/1.2_backport_bump_golang_1.13
* [`35a174382`](https://github.com/containerd/containerd/commit/35a1743821f3fe3e91ae44c12926edd349907de6) Update Golang 1.13.8
* [`305703670`](https://github.com/containerd/containerd/commit/30570367033c0d7b6c3faa273fbdd0eda32bb65a) Update Golang 1.13.7 (CVE-2020-0601, CVE-2020-7919)
* [`1591eb809`](https://github.com/containerd/containerd/commit/1591eb809686d572256c15eab30e70f780a52ac0) Update Golang 1.13.6
* [`fc95ae8ed`](https://github.com/containerd/containerd/commit/fc95ae8ed46eda3db2cee4f5b93b92109a0cbc54) Update Golang 1.13.5
* [`77499e24e`](https://github.com/containerd/containerd/commit/77499e24eed68fede7c41e32787bf62802a4de45) Update to Golang 1.13.4
* [`2adf308a2`](https://github.com/containerd/containerd/commit/2adf308a249fd33760189fc9adcff8c0cff76d7c) Revert "Update Golang 1.12.14"
* [`9d53ba930`](https://github.com/containerd/containerd/commit/9d53ba9301b84c5c898fffd2684dd0b94189b561) Revert "Update Golang 1.12.15"
* [`c5843f944`](https://github.com/containerd/containerd/commit/c5843f944c5ab73dfe223fd3f265e4166da6dd92) Revert "Update Golang 1.12.16 (CVE-2020-0601, CVE-2020-7919)"
* [`012c4c0af`](https://github.com/containerd/containerd/commit/012c4c0afc0ec9eb7ff43a2cfab048a3aebb4399) Revert "Update Golang 1.12.17"
* [`30267a8da`](https://github.com/containerd/containerd/commit/30267a8da09e59cc3feb9da9430167d461bacc8a) platforms: update known OS and arch values
* [`591f6f491`](https://github.com/containerd/containerd/commit/591f6f491442dbd05356e60fb23972eac1f5284f) Move flag.Parse in tests to TestMain
* [`e7583ca96`](https://github.com/containerd/containerd/commit/e7583ca96e82f48f7fd61df156dacf35a9cc37f5) Merge pull request  [#4064](https://github.com/containerd/containerd/pull/4064) from thaJeztah/1.2_backport_namespace_path
* [`80914476e`](https://github.com/containerd/containerd/commit/80914476e2cb9518a170cbb33a9d41b3fa68253b) Merge pull request  [#4061](https://github.com/containerd/containerd/pull/4061) from thaJeztah/1.2_backport_golang_ci_lint
* [`469320d92`](https://github.com/containerd/containerd/commit/469320d9281cd38e01b333f15270a98890ade459) Merge pull request  [#4067](https://github.com/containerd/containerd/pull/4067) from thaJeztah/1.2_backport_content_close
* [`598f7a7b5`](https://github.com/containerd/containerd/commit/598f7a7b5757d279bd6975985af0174898b2d942) Try set GOGC for golint
* [`dfff5b146`](https://github.com/containerd/containerd/commit/dfff5b146ed81e0af87b0c0a8c9beec814ec5200) Switch to golangci-lint
* [`a18c08347`](https://github.com/containerd/containerd/commit/a18c083471e070e601289852c807cdb35e7a80ef) fix additional linting failures
* [`c1ceae579`](https://github.com/containerd/containerd/commit/c1ceae5793f7d6e45002c512363df413ba7a328a) Update timestamp atomic write
* [`82ddedea2`](https://github.com/containerd/containerd/commit/82ddedea200a9d1964010421e0a0dbde761bff0d) Ensure close in content test
* [`961c23a57`](https://github.com/containerd/containerd/commit/961c23a5700b194455a64172217e4d837afdd6d7) fix killall when use pidnamespace
* [`a386eb648`](https://github.com/containerd/containerd/commit/a386eb648eb099d087ea50ea999713a0e8f61575) Fix linter errors
* [`4fcbc810e`](https://github.com/containerd/containerd/commit/4fcbc810e9415070215f7ef3c73cff87ee2fd999) Merge pull request  [#4055](https://github.com/containerd/containerd/pull/4055) from fuweid/cp12-4048
* [`971ad613c`](https://github.com/containerd/containerd/commit/971ad613c5b75c8f9386dadc5eeffb34346ee408) bugfix: cleanup dangling shim by brand new context
</p>
</details>

### Changes from containerd/console
<details><summary>10 commits</summary>
<p>

* [`8375c34`](https://github.com/containerd/console/commit/8375c3424e4d7b114e8a90a4a40c8e1b40d1d4e6) Merge pull request  [#34](https://github.com/containerd/console/pull/34) from sipsma/close-once
* [`38c5469`](https://github.com/containerd/console/commit/38c5469e7522db0c9435a5c33e0e0629113c4952) Only close epoller FD at most once.
* [`02ecf6a`](https://github.com/containerd/console/commit/02ecf6a7291e65f4a361525245c2bea023dc2e0b) Merge pull request  [#33](https://github.com/containerd/console/pull/33) from ulyssessouza/add-file-interface
* [`f652dc3`](https://github.com/containerd/console/commit/f652dc3e99a9f4aa760deb9b4b28edb7c4e5001a) Add File interface instead of using os.File
* [`53a0f1d`](https://github.com/containerd/console/commit/53a0f1deb970a40f08acc1e740a7293bedb8b6b9) Merge pull request  [#32](https://github.com/containerd/console/pull/32) from estesp/check-vendor
* [`6214f20`](https://github.com/containerd/console/commit/6214f2040a2c667ff1458db9485d42299b1d8220) Add vendor check now that content is vendored
* [`4b1ac2b`](https://github.com/containerd/console/commit/4b1ac2bbbdd500f0887e0195f283702be93d5734) Merge pull request  [#31](https://github.com/containerd/console/pull/31) from TwinProduction/master
* [`55928bd`](https://github.com/containerd/console/commit/55928bd5a38487e9b1676c55461ed3b47e796652) Enable vendoring
* [`0650fd9`](https://github.com/containerd/console/commit/0650fd9eeb50bab4fc99dceb9f2e14cf58f36e7f) Merge pull request  [#30](https://github.com/containerd/console/pull/30) from estesp/common-project-content
* [`0b9f189`](https://github.com/containerd/console/commit/0b9f18993a29f711821e7394d1bb9e1da79d8820) Add common project repo checks/README references
</p>
</details>

### Dependency Changes

* **github.com/containerd/console**  c12b1e7919c1 -> 8375c3424e4d

Previous release can be found at [v1.2.13](https://github.com/containerd/containerd/releases/tag/v1.2.13)
