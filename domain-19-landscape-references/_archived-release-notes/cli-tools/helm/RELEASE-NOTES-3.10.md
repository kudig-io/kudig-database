---
title: helm v3.10 Release Notes
description: helm v3.10 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- helm v3.10 Release Notes 是什么
- 如何 helm v3.10 Release Notes
trigger_keywords:
- helm
- v3.10
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
created: "2026-05-23"
---

# [[Helm|helm]] v3.10 Release Notes

Source: [v3.10.3](https://github.com/helm/helm/releases/tag/v3.10.3)

## v3.10.3

Helm v3.10.3 is a security (patch) release. Users are strongly recommended to update to this release.

While fuzz testing Helm, provided by the CNCF:

- a possible stack overflow was discovered with the _strvals_ package. Stack overflow cannot be recovered from in Go. This can potentially be used to produce a denial of [[Service|service]] (DOS) for SDK users. More details are available in [the advisory](https://github.com/helm/helm/security/advisories/GHSA-6rx9-889q-vv2r).
- a possible segmentation violation was discovered with the _repo_ package. Some segmentation violations cannot be recovered from in Go. This can potentially be used to produce a denial of service (DOS) for SDK users. More details are available in [the advisory](https://github.com/helm/helm/security/advisories/GHSA-53c4-hhmh-vw5q).
- a possible segmentation violation was discovered with the _chartutil_ package. This can potentially be used to produce a denial of service (DOS)  for SDK users. More details are available in [the advisory](https://github.com/helm/helm/security/advisories/GHSA-67fx-wx78-jx33)

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[entities/kubernetes|Kubernetes]] Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [ArtifactHub/packages](https://artifacthub.io/packages/search?kind=0)

## Installation and Upgrading

Download Helm v3.10.3. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.10.3-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.10.3-darwin-amd64.tar.gz.sha256sum) / 77a94ebd37eab4d14aceaf30a372348917830358430fcd7e09761eed69f08be5)
- [MacOS arm64](https://get.helm.sh/helm-v3.10.3-darwin-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.10.3-darwin-arm64.tar.gz.sha256sum) / 4f3490654349d6fee8d4055862efdaaf9422eca1ffd2a15393394fd948ae3377)
- Linux amd64](https://get.helm.sh/helm-v3.10.3-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.10.3-linux-amd64.tar.gz.sha256sum) / 950439759ece902157cf915b209b8d694e6f675eaab5099fb7894f30eeaee9a2)
- [Linux arm](https://get.helm.sh/helm-v3.10.3-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.10.3-linux-arm.tar.gz.sha256sum) / dca718eb68c72c51fc7157c4c2ebc8ce7ac79b95fc9355c5427ded99e913ec4c)
- [Linux arm64](https://get.helm.sh/helm-v3.10.3-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.10.3-linux-arm64.tar.gz.sha256sum) / 260cda5ff2ed5d01dd0fd6e7e09bc80126e00d8bdc55f3269d05129e32f6f99d)
- [Linux i386](https://get.helm.sh/helm-v3.10.3-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.10.3-linux-386.tar.gz.sha256sum) / 592e98a492cb782aa7cd67e9afad76e51cd68f5160367600fe542c2d96aa0ad4)
- [Linux ppc64le](https://get.helm.sh/helm-v3.10.3-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.10.3-linux-ppc64le.tar.gz.sha256sum) / 93cdf398abc68e388d1b46d49d8e1197544930ecd3e81cc58d0a87a4579d60ed)
- [Linux s390x](https://get.helm.sh/helm-v3.10.3-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.10.3-linux-s390x.tar.gz.sha256sum) / 6cfa0b9078221f980ef400dc40c95eb71be81d14fdf247ca55efedb068e1d4fa)
- [Windows amd64](https://get.helm.sh/helm-v3.10.3-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.10.3-windows-amd64.zip.sha256sum) / 5d97aa26830c1cd6c520815255882f148040587fd7cdddb61ef66e4c081566e0)

This release was signed with `F126 1BDE 9290 12C8 FF2E 501D 6EA5 D759 8529 A53E ` and can be found at @hickeyma [keybase account](https://keybase.io/hickeyma). Please use the attached signatures for verifying this release using `gpg`.

The [Quickstart Guide](https://helm.sh/docs/intro/quickstart/) will get you going from there. For **upgrade instructions** or detailed installation notes, check the [install guide](https://helm.sh/docs/intro/install/). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3) on any system with `bash`.

## What's Next

- 3.11.0 is the next feature release and will be on January 18, 2023.

## Changelog

- Fix backwards compatibility 835b7334cfe2e5e27870ab3ed4135f136eecc704 (Martin Hickey)
- Update string handling 3caf8b586b47e838e492f9ec05396bf8c5851b92 (Martin Hickey)
- Update repo handling 7c0e203529d4b9d51c5fe57c9e0bd9df1bd95ab4 (Martin Hickey)
- Update schema validation handling f4b93226c6066e009a5162d0b08debbf3d82a67f (Martin Hickey)