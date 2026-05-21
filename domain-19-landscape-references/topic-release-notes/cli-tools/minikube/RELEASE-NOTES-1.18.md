---
title: minikube v1.18 Release Notes
description: minikube v1.18 Release Notes — Kubernetes 生产运维知识库
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
- minikube v1.18 Release Notes 是什么
- 如何 minikube v1.18 Release Notes
trigger_keywords:
- minikube
- v1.18
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# minikube v1.18 Release Notes

Source: [v1.18.1](https://github.com/kubernetes/minikube/releases/tag/v1.18.1)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.18.1 - 2021-03-04

Features:

* kvm2 driver: Add flag --kvm-numa-count" support topology-manager simulate numa  [#10471](https://github.com/kubernetes/minikube/pull/10471)

Minor Improvements:

* Spanish translations [#10687](https://github.com/kubernetes/minikube/pull/10687)
* Change podman priority to default on Linux [#10458](https://github.com/kubernetes/minikube/pull/10458)

Bug Fixes:

* Remove WSLENV empty check from IsMicrosoftWSL [#10711](https://github.com/kubernetes/minikube/pull/10711)
* Added WaitGroups to prevent stderr/stdout from being empty in error logs [#10694](https://github.com/kubernetes/minikube/pull/10694)

Version changes:

* Restore kube-cross build image and bump go to version 1.16 [#10691](https://github.com/kubernetes/minikube/pull/10691)
* Bump github.com/spf13/viper from 1.7.0 to 1.7.1 [#10658](https://github.com/kubernetes/minikube/pull/10658)

Thank you to our contributors for this release!

- Anders F Björklund
- Emanuel
- Ilya Zuyev
- Medya Ghazizadeh
- Sharif Elgamal
- Steven Powell
- phantooom

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## ISO Checksum

`1a7960b845301107cb6a0c29001c8df310d7bce586cf88ceacfc78f22b622ba5`