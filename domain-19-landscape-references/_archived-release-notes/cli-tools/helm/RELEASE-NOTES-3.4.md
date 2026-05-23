---
title: helm v3.4 Release Notes
description: helm v3.4 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- ingress
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- helm v3.4 Release Notes 是什么
- 如何 helm v3.4 Release Notes
trigger_keywords:
- helm
- v3.4
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
created: "2026-05-23"
---

# [[Helm|helm]] v3.4 Release Notes

Source: [v3.4.2](https://github.com/helm/helm/releases/tag/v3.4.2)

Helm v3.4.2 is a patch release. Users are encouraged to upgrade for the best experience.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[entities/kubernetes|Kubernetes]] Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [GitHub/helm/charts](https://github.com/helm/charts)

## Installation and Upgrading

Download Helm v3.4.2. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.4.2-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.4.2-darwin-amd64.tar.gz.sha256sum) / c33b7ee72b0006f23b33f5032b531dd609fff7b08a4324f9ba07722a4f3fec9a)
- Linux amd64](https://get.helm.sh/helm-v3.4.2-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.4.2-linux-amd64.tar.gz.sha256sum) / cacde7768420dd41111a4630e047c231afa01f67e49cc0c6429563e024da4b98)
- [Linux arm](https://get.helm.sh/helm-v3.4.2-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.4.2-linux-arm.tar.gz.sha256sum) / feafaebe64f0fa4228d5b2014defb462d1898fcddbd33a1c34531cbad24e159f)
- [Linux arm64](https://get.helm.sh/helm-v3.4.2-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.4.2-linux-arm64.tar.gz.sha256sum) / 486cad35b9ac1da88781847f2fcaaaed729e44705eb42593322e4b52d0f2c1a1)
- [Linux i386](https://get.helm.sh/helm-v3.4.2-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.4.2-linux-386.tar.gz.sha256sum) / c7a4872d7409bc2840a2c82380b2abbd94b69b4264fad08ed8bb2a4cc617118e)
- [Linux ppc64le](https://get.helm.sh/helm-v3.4.2-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.4.2-linux-ppc64le.tar.gz.sha256sum) / 52062596e5625a3238c6b967d31cf6ec1f0fd5926d2443a1179aeb91ed14d539)
- [Linux s390x](https://get.helm.sh/helm-v3.4.2-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.4.2-linux-s390x.tar.gz.sha256sum) / c33b7ee72b0006f23b33f5032b531dd609fff7b08a4324f9ba07722a4f3fec9a)
- [Windows amd64](https://get.helm.sh/helm-v3.4.2-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.4.2-windows-amd64.zip.sha256sum) / 76ff3f8c21c9af5b80abdd87ec07629ad88dbfe6206decc4d3024f26398554b9)

This release was signed with `672C 657B E06B 4B30 969C 4A57 4614 49C2 5E36 B98E ` and can be found at @mattfarina [keybase account](https://keybase.io/mattfarina). Please use the attached signatures for verifying this release using `gpg`.

The [Quickstart Guide](https://helm.sh/docs/intro/quickstart/) will get you going from there. For **upgrade instructions** or detailed installation notes, check the install guide](https://helm.sh/docs/intro/install/). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/master/scripts/get-helm-3) on any system with `bash`.

## What's Next

- 3.5.0 is the next feature release. This will be released on January 13. 2021.

## Changelog

- Updating to Kubernetes 1.19.4 package versions 23dd3af5e19a02d4f4baa5b2f242645a1a3af629 (Matt Farina)
- fix: ingress path issue 3ba833f5ad97c157a3a27b9985d6f0c660db901e (Salim Salaues)