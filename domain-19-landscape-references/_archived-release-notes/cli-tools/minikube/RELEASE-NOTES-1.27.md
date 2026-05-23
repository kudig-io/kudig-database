---
title: minikube v1.27 Release Notes
description: minikube v1.27 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- minikube v1.27 Release Notes 是什么
- 如何 minikube v1.27 Release Notes
trigger_keywords:
- minikube
- v1.27
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# minikube v1.27 Release Notes

Source: [v1.27.1](https://github.com/kubernetes/minikube/releases/tag/v1.27.1)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.27.1 - 2022-10-07

Features (Experimental):
* QEMU Driver: Add support for dedicated network on macOS (socket_vmnet) [#14989](https://github.com/kubernetes/minikube/pull/14989)
* QEMU Driver: Add support minikube [[Service|service]] and tunnel on macOS [#14989](https://github.com/kubernetes/minikube/pull/14989)

Minor Imprevements:
* Check if context is invalid during update-context command [#15032](https://github.com/kubernetes/minikube/pull/15032)
* Use SSH tunnel if user specifies bindAddress [#14951](https://github.com/kubernetes/minikube/pull/14951)
* Warn QEMU users if DNS issue detected [#15073](https://github.com/kubernetes/minikube/pull/15073)

Bug Fixes:
* Fix status command taking a long time on docker driver while paused [#15077](https://github.com/kubernetes/minikube/pull/15077)
* Fix not allowing passing only an exposed port to --ports [#15085](https://github.com/kubernetes/minikube/pull/15085)
* Fix `minikube dashboard` failing on macOS [#15037](https://github.com/kubernetes/minikube/pull/15037)
* Fix incorrect command in powershell command tip [#15012](https://github.com/kubernetes/minikube/pull/15012)

Version Upgrades:
* Bump [[Kubernetes|Kubernetes]] version default: v1.25.2 and latest: v1.25.2 [#14995](https://github.com/kubernetes/minikube/pull/14995)
* Upgrade kubernetes dashboard from v2.6.0 to v2.7.0 [#15000](https://github.com/kubernetes/minikube/pull/15000)

For a more detailed changelog, including changes occurring in pre-release versions, see [CHANGELOG.md](https://github.com/kubernetes/minikube/blob/master/CHANGELOG.md).

Thank you to our contributors for this release!

- Anthony Nandaa
- Jeff MAURY
- Medya Ghazizadeh
- Rob Leland
- Steven Powell
- Yuiko Mouri
- cokia
- klaases
- ziyi-xie

Thank you to our PR reviewers for this release!

- eiffel-fl (9 comments)
- medyagh (6 comments)
- AkihiroSuda (2 comments)
- klaases (2 comments)
- t-inu (1 comments)

Thank you to our triage members for this release!

- klaases (31 comments)
- RA489 (30 comments)
- afbjorklund (17 comments)
- nikimanoledaki (7 comments)
- medyagh (3 comments)

Check out our [contributions leaderboard](https://minikube.sigs.k8s.io/docs/contrib/leaderboard/v1.27.1/) for this release!

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## Binary Checksums

darwin-amd64: `aa60ff42b4d60b1a65552a5f527d78f68efa887e1eab0af013996badfbccc3c8`
darwin-arm64: `c12ad4e16460e8cdf9f49a9cf6514878875453485570cafe4423fc4d3a69d590`
linux-s390x: `cb22c8c54c4e9b441ec6680f38dc067909bc1f793a205202274eaff2cd947e9f`
linux-amd64: `159bc79f3914dadb7c9f56b6e9d5b73a1c54acb26dca8f1ea84b99ff5da42620`
linux-arm: `f14348a7653672745e92eba934543c0a9f3fa14fbdeff3cf76da41e174769bf0`
linux-arm64: `6a3fb0eaac110c35f018948838aa6ab0898e811a08f76e74b85d8573fb08b1d0`
linux-ppc64le: `aae55c97d7fa9d3180843cffc4c6a504d6882c45a232bdef23e46ec83e191aae`
windows-amd64.exe: `d5957435f3a94a43ce0c764ecaf3b9c4f7c6f8bcafdc4ef7b2b86937ec5c311c`

## ISO Checksums

amd64: `ccc432f3f60647fa050def9da98daf720266e87fd1ed1d3b60f60380e8dd291a`  
arm64: `62f4f28634e78aa394c0f1d5aa735634b8c258c0cd40f7c30a156dca325c9a9f`