---
title: minikube v1.6 Release Notes
description: minikube v1.6 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- ingress
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- minikube v1.6 Release Notes 是什么
- 如何 minikube v1.6 Release Notes
trigger_keywords:
- minikube
- v1.6
- Release
- Notes
- release
- notes
---

# minikube v1.6 Release Notes

Source: [v1.6.2](https://github.com/kubernetes/minikube/releases/tag/v1.6.2)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

* Offline: always transfer image if lookup fails, always download drivers [#6111](https://github.com/kubernetes/minikube/pull/6111)
* Update ingress-dns addon [#6091](https://github.com/kubernetes/minikube/pull/6091)
* Fix update-context to use KUBECONFIG when the env is set [#6090](https://github.com/kubernetes/minikube/pull/6090)
* Fixed IPv6 format for SSH [#6110](https://github.com/kubernetes/minikube/pull/6110)
* Add hyperkit version check whether user's hyperkit is newer [#5833](https://github.com/kubernetes/minikube/pull/5833)
* start: Remove create/delete retry loop [#6129](https://github.com/kubernetes/minikube/pull/6129)
* Change error text to encourage better issue reports [#6121](https://github.com/kubernetes/minikube/pull/6121)

Huge thank you for this release towards our contributors: 
- Anukul Sangwan
- Aresforchina
- Curtis Carter
- Kenta Iso
- Medya Ghazizadeh
- Priya Wadhwa
- Sharif Elgamal
- Thomas Strömberg
- Zhou Hao


## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## ISO Checksum

`a24153a2e49f082d5f4a36ea5d1608cba2482d563e8642a8dffd6560c40f3ed2`