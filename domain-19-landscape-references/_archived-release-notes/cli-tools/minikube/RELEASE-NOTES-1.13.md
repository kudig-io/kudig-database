---
title: minikube v1.13 Release Notes
description: minikube v1.13 Release Notes — Kubernetes 生产运维知识库
summary: minikube v1.13 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- webhook
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- minikube v1.13 Release Notes 是什么
- 如何 minikube v1.13 Release Notes
trigger_keywords:
- minikube
- v1.13
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




# minikube v1.13 Release Notes

Source: [v1.13.1](https://github.com/kubernetes/minikube/releases/tag/v1.13.1)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.13.1 - 2020-09-18
* Update Default [[Kubernetes|Kubernetes]] Version to v1.19.2 [#9265](https://github.com/kubernetes/minikube/pull/9265)
* fix mounting for docker driver in windows [#9263](https://github.com/kubernetes/minikube/pull/9263)
* CSI Hostpath Driver & VolumeSnapshots addons [#8461](https://github.com/kubernetes/minikube/pull/8461)
* docker/podman drivers: Make sure CFS_BANDWIDTH is available for --cpus [#9255](https://github.com/kubernetes/minikube/pull/9255)
* Fix ForwardedPort for podman version 2.0.1 and up [#9237](https://github.com/kubernetes/minikube/pull/9237)
* Avoid setting time for memory assets [#9256](https://github.com/kubernetes/minikube/pull/9256)
* point to newest gcp-auth-webhook version [#9199](https://github.com/kubernetes/minikube/pull/9199)
* Set preload=false if not using overlay2 as docker storage driver [#8831](https://github.com/kubernetes/minikube/pull/8831)
* Upgrade crio to 1.17.3 [#8922](https://github.com/kubernetes/minikube/pull/8922)
* Add Docker Desktop instructions if memory is greater than minimum but less than recommended [#9181](https://github.com/kubernetes/minikube/pull/9181)
* Update minimum memory constants to use MiB instead of MB [#9180](https://github.com/kubernetes/minikube/pull/9180)

Thank you to our contributors for this release! 
- Anders F Björklund
- Dean Coakley
- Julien Breux
- Li Zhijian
- Medya Ghazizadeh
- Priya Wadhwa
- Sharif Elgamal
- Thomas Strömberg
- Zadjad Rezai
- jjanik

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## ISO Checksum

`10e9c0042339b95a840e920dbdc564b6ed64f4a7421baf0e77517439096f3c64`

<!-- risk-assessed -->
