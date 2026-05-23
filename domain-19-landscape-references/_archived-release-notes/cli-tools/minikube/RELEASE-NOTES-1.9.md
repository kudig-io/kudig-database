---
title: minikube v1.9 Release Notes
description: minikube v1.9 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- ingress
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- minikube v1.9 Release Notes 是什么
- 如何 minikube v1.9 Release Notes
trigger_keywords:
- minikube
- v1.9
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# minikube v1.9 Release Notes

Source: [v1.9.2](https://github.com/kubernetes/minikube/releases/tag/v1.9.2)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.9.2 - 2020-04-04

Minor improvements:

* UX: Remove noisy debug statement [#7407](https://github.com/kubernetes/minikube/pull/7407)
* Feature: Make --wait more flexible [#7375](https://github.com/kubernetes/minikube/pull/7375)
* Docker: adjust warn if slow for ps and volume [#7410](https://github.com/kubernetes/minikube/pull/7410)
* Localization: Update Japanese translations [#7403](https://github.com/kubernetes/minikube/pull/7403)
* Performance: Parallelize updating cluster and setting up certs [#7394](https://github.com/kubernetes/minikube/pull/7394)
* Addons: allow [[Ingress|ingress]] addon for docker/podman drivers only on linux for now [#7393](https://github.com/kubernetes/minikube/pull/7393)

- Anders F Björklund
- Medya Ghazizadeh
- Prasad Katti
- Priya Wadhwa
- Thomas Strömberg
- tomocy

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## ISO Checksum

`c161e8ffdbc8d7cc92c2f57809bf846ff73996f06bc6a757bef518b3acce5b47`