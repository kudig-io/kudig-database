---
title: minikube v1.25 Release Notes
description: minikube v1.25 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- istio
- docker
- ingress
- operator
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- minikube v1.25 Release Notes 是什么
- 如何 minikube v1.25 Release Notes
trigger_keywords:
- minikube
- v1.25
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- service-mesh-basics
---

# minikube v1.25 Release Notes

Source: [v1.25.2](https://github.com/kubernetes/minikube/releases/tag/v1.25.2)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.25.2 - 2022-02-23

Features:
* [Addon] Kong Ingress Controller [#13326](https://github.com/kubernetes/minikube/pull/13326)
* add arch to binary and image cache paths [#13539](https://github.com/kubernetes/minikube/pull/13539)
* Adds 'minikube service --all' feature to allow forwarding all services in a namespace [#13367](https://github.com/kubernetes/minikube/pull/13367)
* Make the default container runtime dynamic [#13251](https://github.com/kubernetes/minikube/pull/13251)
* Add `--disable-optimizations` flag [#13340](https://github.com/kubernetes/minikube/pull/13340)

Bug Fixes:
* Fix losing cluster on restart [#13506](https://github.com/kubernetes/minikube/pull/13506)
* Using Get-CmiInstance to detect Hyper-V availability [#13596](https://github.com/kubernetes/minikube/pull/13596)
* Fixes validation on image repository URL when it contains port but no scheme [#13053](https://github.com/kubernetes/minikube/pull/13053)
* Fixed SIGSEGV in kubectl when k8s not running [#13631](https://github.com/kubernetes/minikube/pull/13631)
* Fix hard coded docker driver in minikube service command [#13514](https://github.com/kubernetes/minikube/pull/13514)
* Fix hard coded docker driver in minikube tunnel command [#13444](https://github.com/kubernetes/minikube/pull/13444)
* Fix IstioOperator CustomResourceDefinition for istio-provisioner addon [#13024](https://github.com/kubernetes/minikube/pull/13024)
* fix ingress (also for multinode clusters) [#13439](https://github.com/kubernetes/minikube/pull/13439)
* Add exit message for too new Kubernetes version [#13354](https://github.com/kubernetes/minikube/pull/13354)
* drivers/kvm: Use ARP for retrieving interface ip addresses [#13482](https://github.com/kubernetes/minikube/pull/13482)
* kubeadm: allow skipping kube-proxy addon on restart [#13538](https://github.com/kubernetes/minikube/pull/13538)
* configure container runtimes for clusters without Kubernetes too [#13442](https://github.com/kubernetes/minikube/pull/13442)

Version Upgrades:
* KIC: Upgrade cri-dockerd [#13302](https://github.com/kubernetes/minikube/pull/13302)
* upgrade libvirt to "8th gen" [#13440](https://github.com/kubernetes/minikube/pull/13440)
* Upgrade cri-dockerd to fix the socket path [#13563](https://github.com/kubernetes/minikube/pull/13563)
* ISO: Add packaging for cri-dockerd [#13191](https://github.com/kubernetes/minikube/pull/13191)


For a more detailed changelog, including changes occuring in pre-release versions, see [CHANGELOG.md](https://github.com/kubernetes/minikube/blob/master/CHANGELOG.md).

Thank you to our contributors for this release!

- Akira Yoshiyama
- Anders F Björklund
- Anoop C S
- Balakumaran GR
- Carlos Santana
- Chris Tomkins
- Daniel Helfand
- Jeff MAURY
- Jin Zhang
- Medya Ghazizadeh
- Nikhil Sharma
- Olivier Bouchoms
- Piotr Resztak
- Predrag Rogic
- Sahan Serasinghe
- Sharif Elgamal
- Steven Powell
- Tiago Alves
- Todd MacIntyre
- Viktor Gamov
- ckannon
- klaases
- nishipy
- pedrothome1

Thank you to our PR reviewers for this release!

- medyagh (23 comments)
- afbjorklund (10 comments)
- sharifelgamal (10 comments)
- spowelljr (10 comments)
- t-inu (6 comments)
- klaases (4 comments)
- s-kawamura-w664 (3 comments)
- atoato88 (2 comments)
- alexbaeza (1 comments)
- csantanapr (1 comments)
- totollygeek (1 comments)

Thank you to our triage members for this release!

- RA489 (31 comments)
- afbjorklund (27 comments)
- spowelljr (25 comments)
- klaases (13 comments)
- sharifelgamal (12 comments)

Check out our [contributions leaderboard](https://minikube.sigs.k8s.io/docs/contrib/leaderboard/v1.25.1/) for this release!

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## ISO Checksum

`ae3fde59ff5034348c19c77124d3719e634a40a33faf1b66d21c1f5fc55a12fc`