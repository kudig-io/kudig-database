---
title: minikube v1.8 Release Notes
description: minikube v1.8 Release Notes — Kubernetes 生产运维知识库
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
- minikube v1.8 Release Notes 是什么
- 如何 minikube v1.8 Release Notes
trigger_keywords:
- minikube
- v1.8
- Release
- Notes
- release
- notes
---

# minikube v1.8 Release Notes

Source: [v1.8.2](https://github.com/kubernetes/minikube/releases/tag/v1.8.2)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.8.2 - 2020-03-13

Shiny new improvements:

* allow setting api-server port for docker/podman drivers [#6991](https://github.com/kubernetes/minikube/pull/6991)
* Update NewestKubernetesVersion to 1.18.0-beta.2 [#6988](https://github.com/kubernetes/minikube/pull/6988)
* Add warning if disk image is missing features [#6978](https://github.com/kubernetes/minikube/pull/6978)

Captivating bug fixes:

* Hyper-V: Round suggested memory alloc by 100MB for VM's [#6987](https://github.com/kubernetes/minikube/pull/6987)
* Merge repositories.json after extracting preloaded tarball so that reference store isn't lost [#6985](https://github.com/kubernetes/minikube/pull/6985)
* Fix dockerd internal port changing on restart [#7021](https://github.com/kubernetes/minikube/pull/7021)
* none: Skip driver preload and image caching [#7015](https://github.com/kubernetes/minikube/pull/7015)
* preload: fix bug for windows file separators [#6968](https://github.com/kubernetes/minikube/pull/6968)
* Block on preload download [#7003](https://github.com/kubernetes/minikube/pull/7003)
* Check if lz4 is available before trying to use it [#6941](https://github.com/kubernetes/minikube/pull/6941)
* Allow backwards compatibility with 1.6 and earlier configs [#6969](https://github.com/kubernetes/minikube/pull/6969)

Huge thank you for this release towards our contributors: 
- Anders F Björklund
- Ian Molee
- Kenta Iso
- Medya Ghazizadeh
- Priya Wadhwa
- Sharif Elgamal
- Thomas Strömberg

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## ISO Checksum

`ba2acd69eeb01ca6568e31a87b6482f8ca8eff4261d9ce1a86dc8da4d3a9ce99`