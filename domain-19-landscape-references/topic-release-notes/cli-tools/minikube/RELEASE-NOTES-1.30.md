---
title: minikube v1.30 Release Notes
description: minikube v1.30 Release Notes — Kubernetes 生产运维知识库
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
- minikube v1.30 Release Notes 是什么
- 如何 minikube v1.30 Release Notes
trigger_keywords:
- minikube
- v1.30
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# minikube v1.30 Release Notes

Source: [v1.30.1](https://github.com/kubernetes/minikube/releases/tag/v1.30.1)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.30.1 - 2023-04-04

* Docker driver: Fix incorrectly stating `Image was not built for the current minikube` [#16226](https://github.com/kubernetes/minikube/pull/16226)
* Mark VMware driver as unsupported  [#16233](https://github.com/kubernetes/minikube/pull/16233)

For a more detailed changelog, see [CHANGELOG.md](https://github.com/kubernetes/minikube/blob/master/CHANGELOG.md).

Thank you to our contributors for this release!

- Juan Martin Loyola
- Medya Ghazizadeh
- Steven Powell

Thank you to our PR reviewers for this release!

- medyagh (1 comments)

Thank you to our triage members for this release!

- afbjorklund (8 comments)
- spowelljr (6 comments)
- kundan2707 (2 comments)
- medyagh (1 comments)
- rafariossaa (1 comments)

Check out our [contributions leaderboard](https://minikube.sigs.k8s.io/docs/contrib/leaderboard/v1.30.1/) for this release!

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## Binary Checksums

linux-amd64: `e53d9e8c31f4c5f683182f5323d3527aa0725f713945c6d081cf71aa548ab388`
linux-arm: `cd80af213fa394127311096a60f69c216c3d8c242765833fd5fcd54be9e7a7ad`
linux-arm64: `3addf91be8cf1cb460b856171d3621b8b2f4bc96254246c912aeb30671ac37b7`
linux-ppc64le: `65cd4e46dd3a8c14c61c271ef67bb17c4c5498870d7eada548cf06eafcf73c40`
linux-s390x: `79f9bb083940563c65c2a01d48ddff3c515e7e67c1fc65e70c7b42b507d4a5cb`
windows-amd64.exe: `cb3cf94860bd7a6ccb514fb1fed6641c51bcd9de1ea1a823cf862e632852af4a`
darwin-amd64: `b5938a8772c5565b5d0b795938c367c5190bf65bb51fc55fb2417cb4e1d04ef1`
darwin-arm64: `3aa935f0657b25634944510bb2e1111f49d19d0cba32dcd594721c6673ba0a01`

## ISO Checksums

amd64: `ca0c9143797c66cf89d69e4398de538a4867952cc9cc7c91d89bc5cc027788b0`  
arm64: `c0be28dfafde2d69a4b20f9f8238df74f910151faf2fa2562e88c8094dfd1f25`