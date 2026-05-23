---
title: minikube v1.23 Release Notes
description: minikube v1.23 Release Notes — Kubernetes 生产运维知识库
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
- minikube v1.23 Release Notes 是什么
- 如何 minikube v1.23 Release Notes
trigger_keywords:
- minikube
- v1.23
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# minikube v1.23 Release Notes

Source: [v1.23.2](https://github.com/kubernetes/minikube/releases/tag/v1.23.2)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.23.2 - 2021-09-21

Fix crio regression:
* Roll back default crio cgroup to systemd [#12533](https://github.com/kubernetes/minikube/pull/12533)
* Fix template typo [#12532](https://github.com/kubernetes/minikube/pull/12532)

For a more detailed changelog, including changes occuring in pre-release versions, see [CHANGELOG.md](https://github.com/kubernetes/minikube/blob/master/CHANGELOG.md).

Thank you to our contributors for this release!

- Jeff MAURY
- Lakshya Gupta
- Medya Ghazizadeh
- Sharif Elgamal
- Steven Powell

Thank you to our PR reviewers for this release!

- medyagh (2 comments)
- sharifelgamal (1 comments)

Thank you to our triage members for this release!

- afbjorklund (12 comments)
- yxxhero (10 comments)
- medyagh (7 comments)
- spowelljr (4 comments)
- dilyanpalauzov (2 comments)

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## ISO Checksum

`b15d7bbdb60a095198b64f56edd7f3f2c02b89859f082f86ad9a8e3e5333b2a6`