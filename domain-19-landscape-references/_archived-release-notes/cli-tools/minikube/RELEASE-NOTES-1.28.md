---
title: minikube v1.28 Release Notes
description: minikube v1.28 Release Notes — Kubernetes 生产运维知识库
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
- minikube v1.28 Release Notes 是什么
- 如何 minikube v1.28 Release Notes
trigger_keywords:
- minikube
- v1.28
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# minikube v1.28 Release Notes

Source: [v1.28.0](https://github.com/kubernetes/minikube/releases/tag/v1.28.0)

📣😀 **Please fill out our [fast 5-question survey](https://forms.gle/Gg3hG5ZySw8c1C24A)** so that we can learn how & why you use minikube, and what improvements we should make. Thank you! 💃🎉

## Release Notes

## Version 1.28.0 - 2022-11-04

**SECURITY WARNING:** Log4j CVEs were detected in an image the `efk` addon uses, if you don't use the `efk` addon no action is required. If you use the addon we recommend running `minikube addons disable efk` to terminate the vulnerable [[Pods|pods]].
See [#15280](https://github.com/kubernetes/minikube/issues/15280) for more details.

Security:
* Prevent enabling `efk` addon due to containing Log4j CVE [#15281](https://github.com/kubernetes/minikube/pull/15281)

Features:
* Auto select network on QEMU [#15266](https://github.com/kubernetes/minikube/pull/15266)
* Implement mounting on QEMU with socket_vmnet [#15108](https://github.com/kubernetes/minikube/pull/15108)
* Added cloud-spanner emulator addon [#15160](https://github.com/kubernetes/minikube/pull/15160)
* Add `minikube license` command [#15158](https://github.com/kubernetes/minikube/pull/15158)

Minor Improvements:
* Allow port forwarding on Linux with Docker Desktop [#15126](https://github.com/kubernetes/minikube/pull/15126)
* Add back [[Service|service]] to mount VirtualBox host directory into the guest. [#14784](https://github.com/kubernetes/minikube/pull/14784)
* ISO: Add FANOTIFY_ACCESS_PERMISSIONS to kernel configs [#15232](https://github.com/kubernetes/minikube/pull/15232)
* When enabling addon warn if addon has no associated Github username [#15081](https://github.com/kubernetes/minikube/pull/15081)

Bug Fixes:
* Fix detecting preload cache of size 0 as valid [#15256](https://github.com/kubernetes/minikube/pull/15256)
* Fix always writing to daemon by trimming `docker.io` from image name [#14956](https://github.com/kubernetes/minikube/pull/14956)
* Fix minikube tunnel repeated printout of status [#14933](https://github.com/kubernetes/minikube/pull/14933)

Version Upgrades:
* Upgrade Portainer addon to 2.15.1 & HTTPS access enabled [#15172](https://github.com/kubernetes/minikube/pull/15172)
* Upgrade [[Headlamp|Headlamp]] addon to 0.13.0 [#15186](https://github.com/kubernetes/minikube/pull/15186)
* ISO: Upgrade Docker from 20.10.18 to 20.10.20 [#15159](https://github.com/kubernetes/minikube/pull/15159)
* KIC: Upgrade base image from ubuntu:focal-20220826 to ubuntu:focal-20220922 [#15075](https://github.com/kubernetes/minikube/pull/15075)
* KCI: Upgrade base image from ubuntu:focal-20220922 to ubuntu:focal-20221019 [#15219](https://github.com/kubernetes/minikube/pull/15219)

For a more detailed changelog, including changes occurring in pre-release versions, see [CHANGELOG.md](https://github.com/kubernetes/minikube/blob/master/CHANGELOG.md).

Thank you to our contributors for this release!

- Chris Kannon
- Francis Laniel
- Jeff MAURY
- Jevon Tane
- Medya Ghazizadeh
- Nitin Agarwal
- Oldřich Jedlička
- Rahil Patel
- Steven Powell
- Tian
- Yue Yang
- joaquimrocha
- klaases
- shixiuguo

Thank you to our PR reviewers for this release!

- spowelljr (25 comments)
- medyagh (14 comments)

Thank you to our triage members for this release!

- RA489 (64 comments)
- klaases (39 comments)
- afbjorklund (23 comments)
- spowelljr (22 comments)
- medyagh (4 comments)

## Installation

See [Getting Started](https://minikube.sigs.k8s.io/docs/start/)

## Binary Checksums

darwin-amd64: `667ab2baeae7829392533a9ef733e94ea88c87a8edf38ee088d349077152cc95`
darwin-arm64: `8dc1b6018ce7ba344394e26dac62a7db477dbfcba63d9cdf55ea8e0c2fc755f4`
linux-arm: `802af1d58e5707f2c1e2253f88dadf79bbde25bd803ad726d68ef21eb2b2b75a`
linux-arm64: `648d09c00ceee3b53d60ea92796c772abcb42bd9bb7e871fe0b233e39a4675ce`
linux-ppc64le: `27b4d08b98711ab89306e92085d7dd129339c170483d1c54343a71dfa75b4cc7`
linux-s390x: `3067c320e521b6ad1642b50eb955305ec7cefca54b53f73713494a857b1431a0`
linux-amd64: `58cead5ece9815a61d4be253a07b2385f8cf373d9e1eba5c9783444e4e9e2d8e`
windows-amd64.exe: `65304e5c5c98a688fb0abc2d76d77b3b441f9501d6dfa71d96a0532f4114d34c`

## ISO Checksums

amd64: `fbf80209b11a5821c31dadba76982a31a1e09396e8353ce81e92a7d85d921749`  
arm64: `b5e2f1b912d42ed94b6cb837618c1b7fbafbedc5082c736fccd70be3dec6d833`