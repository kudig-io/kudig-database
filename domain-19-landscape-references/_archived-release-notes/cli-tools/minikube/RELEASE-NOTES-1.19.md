---
title: minikube v1.19 Release Notes
description: minikube v1.19 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- minikube v1.19 Release Notes 是什么
- 如何 minikube v1.19 Release Notes
trigger_keywords:
- minikube
- v1.19
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# minikube v1.19 Release Notes

Source: [v1.19.0](https://github.com/kubernetes/minikube/releases/tag/v1.19.0)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.19.0 - 2021-04-09

* allow Auto-Pause addon on VMs [#11019](https://github.com/kubernetes/minikube/pull/11019)
* Do not allow running darwin/amd64 minikube binary on darwin/arm64 systems [#11024](https://github.com/kubernetes/minikube/pull/11024)
* Respect memory being set in the minikube config [#11014](https://github.com/kubernetes/minikube/pull/11014)
* new command image ls to list images in a cluster [#11007](https://github.com/kubernetes/minikube/pull/11007)

Thank you to our contributors for this release!

- Anders F Björklund
- Cookie Wang
- Ilya Zuyev
- Medya Ghazizadeh
- Predrag Rogic
- Sharif Elgamal
- Steven Powell
- 李龙峰

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## ISO Checksum

`bbeceb7eb4d60d9faf76ecb740483260715f0d18ca284c8192968695729f4fc5`