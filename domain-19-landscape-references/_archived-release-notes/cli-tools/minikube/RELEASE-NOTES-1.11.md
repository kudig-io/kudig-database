---
title: minikube v1.11 Release Notes
description: minikube v1.11 Release Notes — Kubernetes 生产运维知识库
summary: minikube v1.11 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- apiserver
- helm
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
- minikube v1.11 Release Notes 是什么
- 如何 minikube v1.11 Release Notes
trigger_keywords:
- minikube
- v1.11
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# minikube v1.11 Release Notes

Source: [v1.11.0](https://github.com/kubernetes/minikube/releases/tag/v1.11.0)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.11.0 - 2020-05-29

Features:

* add 'defaults' sub-command to `minikube config` [#8143](https://github.com/kubernetes/minikube/pull/8143)
* addons: add OLM addon [#8129](https://github.com/kubernetes/minikube/pull/8129)
* addons:: Add Ambassador [[Ingress|Ingress]] controller addon [#8161](https://github.com/kubernetes/minikube/pull/8161)
* bump oldest k8s version supported to 1.13 [#8154](https://github.com/kubernetes/minikube/pull/8154)
* bump default [[Kubernetes|kubernetes]] version to 1.18.3 [#8307](https://github.com/kubernetes/minikube/pull/8307)
* Bump helm-tiller 2.16.7 and promote tiller ClusterRoleBinding to v1 [#8174](https://github.com/kubernetes/minikube/pull/8174)

Minor Improvements:

* docker/podman drivers: add fall back image in docker hub [#8320](https://github.com/kubernetes/minikube/pull/8320)
* docker/podman drivers: exit with usage when need login to registry [#8225](https://github.com/kubernetes/minikube/pull/8225)
* multinode: copy apiserver certs only to control plane [#8092](https://github.com/kubernetes/minikube/pull/8092)
* docker-env: restart dockerd inside minikube on failure [#8239](https://github.com/kubernetes/minikube/pull/8239)
* wait for [[entities/kubernetes.md|kubernetes components]] on soft start [#8199](https://github.com/kubernetes/minikube/pull/8199)
* improve minikube status display for one node [#8238](https://github.com/kubernetes/minikube/pull/8238)
* improve solution message for wrong kuberentes-version format [#8118](https://github.com/kubernetes/minikube/pull/8118)

Bug fixes:

* fix HTTP_PROXY env not being passed to docker engine [#8198](https://github.com/kubernetes/minikube/pull/8198)
* honor --image-repository even if --image-mirror-country is set [#8249](https://github.com/kubernetes/minikube/pull/8249)
* parallels driver: fix HostIP implementation [#8259](https://github.com/kubernetes/minikube/pull/8259)
* addon registry: avoid getting stuck on registry port 443 [#8208](https://github.com/kubernetes/minikube/pull/8208)
* respect native-ssh param properly [#8290](https://github.com/kubernetes/minikube/pull/8290)
* fixed parsing kubernetes version for keywords "latest" or "stable" [#8230](https://github.com/kubernetes/minikube/pull/8230)
* multinode: make sure multinode clusters survive restarts [#7973](https://github.com/kubernetes/minikube/pull/7973)
* multinode: delete docker volumes when deleting a  node [#8224](https://github.com/kubernetes/minikube/pull/8224)
* multinode: delete worker volumes for docker driver [#8216](https://github.com/kubernetes/minikube/pull/8216)
* multinode: recreate existing control plane node correctly [#8095](https://github.com/kubernetes/minikube/pull/8095)

Huge thank you for this release towards our contributors:

- Anders F Björklund
- Kenta Iso
- Medya Ghazizadeh
- Mikhail Zholobov
- Natale Vinto
- Nicola Ferraro
- Priya Wadhwa
- RA489
- Sharif Elgamal
- Shubham
- kadern0

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## ISO Checksum

`af64ce117799143e647a406f771cf0714761b6a25925b5900778168d41496ca2`

<!-- risk-assessed -->
