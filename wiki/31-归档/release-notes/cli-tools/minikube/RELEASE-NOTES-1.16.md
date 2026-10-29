---
title: minikube v1.16 Release Notes
description: minikube v1.16 Release Notes — Kubernetes 生产运维知识库
summary: minikube v1.16 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- ingress
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- minikube v1.16 Release Notes 是什么
- 如何 minikube v1.16 Release Notes
trigger_keywords:
- minikube
- v1.16
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# minikube v1.16 Release Notes

Source: [v1.16.0](https://github.com/kubernetes/minikube/releases/tag/v1.16.0)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.16.0 - 2020-12-17
* fix ip node retrieve for none driver [#9986](https://github.com/kubernetes/minikube/pull/9986)
* remove experimental warning for multinode [#9987](https://github.com/kubernetes/minikube/pull/9987)
* Enable [[Ingress|Ingress]] Addon for Docker Windows [#9761](https://github.com/kubernetes/minikube/pull/9761)
* Bump preload to v8 to include updated dashboard [#9984](https://github.com/kubernetes/minikube/pull/9984)
* Add ssh-host command for getting the ssh host keys [#9630](https://github.com/kubernetes/minikube/pull/9630)
* Added sub-step logging to adm init step on start [#9904](https://github.com/kubernetes/minikube/pull/9904)
* Add --node option for command `ip` and `ssh-key` [#9873](https://github.com/kubernetes/minikube/pull/9873)
* Upgrade Docker, from 20.10.0 to 20.10.1 [#9966](https://github.com/kubernetes/minikube/pull/9966)
* Upgrade [[Kubernetes|kubernetes]] dashboard to v2.1.0 for 1.20 [#9963](https://github.com/kubernetes/minikube/pull/9963)
* Upgrade buildkit from 0.8.0 to 0.8.1 [#9967](https://github.com/kubernetes/minikube/pull/9967)

Thank you to our contributors for this release!

- Anders F Björklund
- Jituri, Pranav
- Ling Samuel
- Sharif Elgamal
- Steven Powell
- Thomas Strömberg
- priyawadhwa
- wangxy518

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## ISO Checksum

`acbd805831ad3afe8935c6e28888a414e9cde952afe5b574b9f44c853bde8814`

<!-- risk-assessed -->
