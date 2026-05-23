---
title: minikube v1.26 Release Notes
description: minikube v1.26 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- cri-o
- docker
- webhook
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- minikube v1.26 Release Notes 是什么
- 如何 minikube v1.26 Release Notes
trigger_keywords:
- minikube
- v1.26
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# minikube v1.26 Release Notes

Source: [v1.26.1](https://github.com/kubernetes/minikube/releases/tag/v1.26.1)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.26.1 - 2022-08-02

Minor Improvements:
* Check for cri-dockerd & dockerd runtimes when using none-driver on [[Kubernetes|Kubernetes]] 1.24+  [#14555](https://github.com/kubernetes/minikube/pull/14555)
* Add solution message for when `cri-docker` is missing [#14483](https://github.com/kubernetes/minikube/pull/14483)
* Limit number of audit entries [#14695](https://github.com/kubernetes/minikube/pull/14695)
* Optimize audit logging [#14596](https://github.com/kubernetes/minikube/pull/14596)
* Show the [[Container Runtime|container runtime]] when running without kubernetes #13432  [#14200](https://github.com/kubernetes/minikube/pull/14200)
* Add warning when enabling thrid-party addons [#14499](https://github.com/kubernetes/minikube/pull/14499)

Bug fixes:
* Fix url index out of range error in service [#14658](https://github.com/kubernetes/minikube/pull/14658)
* Fix incorrect user and profile in audit logging [#14562](https://github.com/kubernetes/minikube/pull/14562)
* Fix overwriting err for OCI "minikube start" [#14506](https://github.com/kubernetes/minikube/pull/14506)
* Fix panic when environment variables are empty [#14415](https://github.com/kubernetes/minikube/pull/14415)

Version Upgrades:
* Bump Kubernetes version default: v1.24.3 and latest: v1.24.3 [#14606](https://github.com/kubernetes/minikube/pull/14606)
* ISO: Update Docker from 20.10.16 to 20.10.17 [#14534](https://github.com/kubernetes/minikube/pull/14534)
* ISO/Kicbase: Update cri-o from v1.22.3 to v1.24.1 [#14420](https://github.com/kubernetes/minikube/pull/14420)
* ISO: Update conmon from v2.0.24 to v2.1.2 [#14545](https://github.com/kubernetes/minikube/pull/14545)
* Update gcp-auth-webhook from v0.0.9 to v0.0.10 [#14670](https://github.com/kubernetes/minikube/pull/14670)
* ISO/Kicbase: Update base images [#14481](https://github.com/kubernetes/minikube/pull/14481)

For a more detailed changelog, including changes occurring in pre-release versions, see [CHANGELOG.md](https://github.com/kubernetes/minikube/blob/master/CHANGELOG.md).

Thank you to our contributors for this release!

- Akihiro Suda
- Akira Yoshiyama
- Bradley S
- Christoph "criztovyl" Schulz
- Gimb0
- HarshCasper
- Jeff MAURY
- Medya Ghazizadeh
- Niels de Vos
- Paul S. Schweigert
- Santhosh Nagaraj S
- Steven Powell
- Tobias Pfandzelter
- anoop142
- inifares23lab
- klaases
- peizhouyu
- zhouguowei
- 吴梓铭
- 李龙峰

Thank you to our PR reviewers for this release!

- spowelljr (50 comments)
- medyagh (9 comments)
- atoato88 (3 comments)
- klaases (2 comments)
- afbjorklund (1 comments)

Thank you to our triage members for this release!

- afbjorklund (75 comments)
- RA489 (56 comments)
- klaases (32 comments)
- spowelljr (27 comments)
- medyagh (13 comments)

Check out our [contributions leaderboard](https://minikube.sigs.k8s.io/docs/contrib/leaderboard/v1.26.0/) for this release!

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## Binary Checksums

linux-arm64: `419f65fb0dc1045a07943192f11f6fc91858ee576b4f9ddb0be5b3f637e36ab0`
linux-ppc64le: `a87b01d9776b5e2f941d3bf9a8dbe9ccde12c154eff89494eb579346bbb4343b`
linux-s390x: `b655a3ad176746260dc937974c179477487bac45c5adfa046612d50691530e99`
linux-amd64: `9acd25706661b932ee98063147e58080cb949b92fd0d97b3b96dc5f898dcad21`
linux-arm: `6aa8bdb2b0eb7a1c306907cf5941d9d1e7d3cd623471b859c1da81cea2cd189d`
windows-amd64.exe: `9c9934a396acdd164e2b2449def5d831aeb1577571b8ac9d922ffe466b0f270e`
darwin-arm64: `61b0543ee7a27b48992517df17ce7a56ad687762210f55112c7f2eb8a0e55655`
darwin-amd64: `57578517edec2fcf8425b47ef9535c56c1b7c0f383b4d676ebf3787076ac4ede`

## ISO Checksums

amd64: `b764c656066434fcb83ab7b0a9512ca03dab6faab5469074f7701f7fe6b8678d`  
arm64: `46fb2e38fdd2d56b8b7fd46851ed5e8cb53fd804973cde4b2b09988d8495e765`