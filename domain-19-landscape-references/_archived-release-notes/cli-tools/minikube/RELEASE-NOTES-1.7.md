---
title: minikube v1.7 Release Notes
description: minikube v1.7 Release Notes — Kubernetes 生产运维知识库
summary: minikube v1.7 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- calico
- helm
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- minikube v1.7 Release Notes 是什么
- 如何 minikube v1.7 Release Notes
trigger_keywords:
- minikube
- v1.7
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- cni-basics
---



# minikube v1.7 Release Notes

Source: [v1.7.3](https://github.com/kubernetes/minikube/releases/tag/v1.7.3)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.7.3 - 2020-02-20

* Add podman driver [#6515](https://github.com/kubernetes/minikube/pull/6515)
* Create Hyper-V External Switch [#6264](https://github.com/kubernetes/minikube/pull/6264)
* Don't allow creating profile by profile command [#6672](https://github.com/kubernetes/minikube/pull/6672)
* Create the Node subcommands for multi-node refactor [#6556](https://github.com/kubernetes/minikube/pull/6556)
* Improve docker volume clean up [#6695](https://github.com/kubernetes/minikube/pull/6695)
* Add podman-env for connecting with podman-remote [#6351](https://github.com/kubernetes/minikube/pull/6351)
* Update gvisor addon to latest runsc version [#6573](https://github.com/kubernetes/minikube/pull/6573)
* Fix inverted start resource logic [#6700](https://github.com/kubernetes/minikube/pull/6700)
* Fix bug in --install-addons flag [#6696](https://github.com/kubernetes/minikube/pull/6696)
* Fix bug in docker-env and add tests for docker-env command [#6604](https://github.com/kubernetes/minikube/pull/6604)
* Fix kubeConfigPath  [#6568](https://github.com/kubernetes/minikube/pull/6568)
* Fix `minikube start` in order to be able to start VM even if machine does not exist [#5730](https://github.com/kubernetes/minikube/pull/5730)
* Fail fast if waiting for SSH to be available [#6625](https://github.com/kubernetes/minikube/pull/6625)
* Add RPFilter to ISO kernel - required for modern Calico releases [#6690](https://github.com/kubernetes/minikube/pull/6690)
* Update [[Kubernetes|Kubernetes]] default version to v1.17.3 [#6602](https://github.com/kubernetes/minikube/pull/6602)
* Update crictl to v1.17.0 [#6667](https://github.com/kubernetes/minikube/pull/6667)
* Add conntrack-tools, needed for kubernetes 1.18 [#6626](https://github.com/kubernetes/minikube/pull/6626)
* Stopped and running machines should count as existing [#6629](https://github.com/kubernetes/minikube/pull/6629)
* Upgrade Docker to 19.03.6 [#6618](https://github.com/kubernetes/minikube/pull/6618)
* Upgrade conmon version for podman [#6622](https://github.com/kubernetes/minikube/pull/6622)
* Upgrade podman to 1.6.5 [#6623](https://github.com/kubernetes/minikube/pull/6623)
* Update helm-tiller addon image v2.14.3 → v2.16.1 [#6575](https://github.com/kubernetes/minikube/pull/6575)

Thank you to our wonderful and amazing contributors who contributed to this bug-fix release:

- Anders F Björklund
- Nguyen Hai Truong
- Martynas Pumputis
- Thomas Strömberg
- Medya Ghazizadeh
- Wietse Muizelaar
- Zhongcheng Lao
- Sharif Elgamal
- Priya Wadhwa
- Rohan Maity
- anencore94
- aallbright
- Tam Mach
- edge0701
- go_vargo
- sayboras

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## ISO Checksum

`e6fc4fa646bd0fa5f90a60bc9a0d6a1b1efd2e406951f65d1e5ad250b04e660b`