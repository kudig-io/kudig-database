---
title: minikube v1.24 Release Notes
description: minikube v1.24 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- minikube v1.24 Release Notes 是什么
- 如何 minikube v1.24 Release Notes
trigger_keywords:
- minikube
- v1.24
- Release
- Notes
- release
- notes
---

# minikube v1.24 Release Notes

Source: [v1.24.0](https://github.com/kubernetes/minikube/releases/tag/v1.24.0)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.24.0 - 2021-11-04

Features:
* Add --no-kubernetes flag  to start minikube without kubernetes [#12848](https://github.com/kubernetes/minikube/pull/12848)
* `minikube addons list` shows addons if cluster does not exist [#12837](https://github.com/kubernetes/minikube/pull/12837)

Bug fixes:
* virtualbox: change default `host-only-cidr` [#12811](https://github.com/kubernetes/minikube/pull/12811)
* fix zsh completion [#12841](https://github.com/kubernetes/minikube/pull/12841)
* Fix starting on Windows with VMware driver on non `C:` drive [#12819](https://github.com/kubernetes/minikube/pull/12819)

For a more detailed changelog, including changes occuring in pre-release versions, see [CHANGELOG.md](https://github.com/kubernetes/minikube/blob/master/CHANGELOG.md).

Thank you to our contributors for this release!

- Akira Yoshiyama
- Keyhoh
- Medya Ghazizadeh
- Nicolas Busseneau
- Sharif Elgamal
- Steven Powell
- Toshiaki Inukai

Thank you to our PR reviewers for this release!

- spowelljr (11 comments)
- sharifelgamal (10 comments)
- afbjorklund (6 comments)
- atoato88 (5 comments)
- medyagh (3 comments)
- yosshy (1 comments)

Thank you to our triage members for this release!

- sharifelgamal (13 comments)
- afbjorklund (9 comments)
- spowelljr (6 comments)
- medyagh (3 comments)
- Sarathgiggso (2 comments)

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## ISO Checksum

`b4d2ab559f40c031fbebe6d967a625980c22aa6e65e3a12916b25f2105f23f56`