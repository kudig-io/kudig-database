---
title: minikube v1.15 Release Notes
description: minikube v1.15 Release Notes — Kubernetes 生产运维知识库
summary: minikube v1.15 Release Notes — Kubernetes 生产运维知识库
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
- minikube v1.15 Release Notes 是什么
- 如何 minikube v1.15 Release Notes
trigger_keywords:
- minikube
- v1.15
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




# minikube v1.15 Release Notes

Source: [v1.15.1](https://github.com/kubernetes/minikube/releases/tag/v1.15.1)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.15.1 - 2020-11-16

Feature:
* Add Support for driver name alias [#9672](https://github.com/kubernetes/minikube/pull/9672)

Bug fix:
* less verbose language selector [#9715](https://github.com/kubernetes/minikube/pull/9715)

Thank you to our contributors for this release!
- Ben Leggett
- Medya Ghazizadeh
- Priya Wadhwa
- Sadlil
- Sharif Elgamal
- Vasilyev, Viacheslav

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## ISO Checksum

`c3b5682d484e0e507ccc39da7e12ac6a868a3e8e0ed7e3cf91836d9565e474ec`

<!-- risk-assessed -->
