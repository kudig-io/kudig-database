---
title: minikube v1.5 Release Notes
description: minikube v1.5 Release Notes — Kubernetes 生产运维知识库
summary: minikube v1.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- minikube v1.5 Release Notes 是什么
- 如何 minikube v1.5 Release Notes
trigger_keywords:
- minikube
- v1.5
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




# minikube v1.5 Release Notes

Source: [v1.5.2](https://github.com/kubernetes/minikube/releases/tag/v1.5.2)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.5.2 - 2019-10-31

* [[Service|service]]: fix --url mode [#5790](https://github.com/kubernetes/minikube/pull/5790)
* Refactor command runner interface, allow stdin writes [#5530](https://github.com/kubernetes/minikube/pull/5530)
* macOS install docs: minikube is a normal Homebrew formula now [#5750](https://github.com/kubernetes/minikube/pull/5750)
* Allow CPU count check to be disabled using --force [#5803](https://github.com/kubernetes/minikube/pull/5803)
* Make network validation friendlier, especially to corp networks [#5802](https://github.com/kubernetes/minikube/pull/5802)

Thank you to our contributors for this release:

- Anders F Björklund
- Issy Long
- Medya Ghazizadeh
- Thomas Strömberg

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## ISO Checksum

`088209aa023e22fb012dbfe3841c8be0c149c36ac379ce73e28041d4141b17b9`

<!-- risk-assessed -->
