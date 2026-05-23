---
title: helm v3.9 Release Notes
description: helm v3.9 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- helm v3.9 Release Notes 是什么
- 如何 helm v3.9 Release Notes
trigger_keywords:
- helm
- v3.9
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

# [[Helm|helm]] v3.9 Release Notes

Source: [v3.9.4](https://github.com/helm/helm/releases/tag/v3.9.4)

Helm v3.9.4 is a security (patch) release. Users are strongly recommended to update to this release.

While fuzz testing Helm, provided by the CNCF, a possible out of memory panic was discovered with the _strvals_ package. Out of memory panics cannot be recovered from in Go. This can potentially be used to produce a denial of [[Service|service]] (DOS). More details are available in [the advisory](https://github.com/helm/helm/security/advisories/GHSA-7hfp-qfw3-5jxh).

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[entities/kubernetes|Kubernetes]] Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [ArtifactHub/packages](https://artifacthub.io/packages/search?kind=0)

## Installation and Upgrading

Download Helm v3.9.4. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.9.4-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.9.4-darwin-amd64.tar.gz.sha256sum) / fe5930feca6fd1bd2c57df01c1f381c6444d1c3d2b857526bf6cbfbd6bf906b4)
- [MacOS arm64](https://get.helm.sh/helm-v3.9.4-darwin-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.9.4-darwin-arm64.tar.gz.sha256sum) / a73d91751153169781b3ab5b4702ba1a2631fc8242eba33828b5905870059312)
- [Linux amd64](https://get.helm.sh/helm-v3.9.4-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.9.4-linux-amd64.tar.gz.sha256sum) / 31960ff2f76a7379d9bac526ddf889fb79241191f1dbe2a24f7864ddcb3f6560)
- [Linux arm](https://get.helm.sh/helm-v3.9.4-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.9.4-linux-arm.tar.gz.sha256sum) / 18ce0f79dcd927fea5b714ca03299929dad05266192d4cde3de6b4c4d4544249)
- [Linux arm64](https://get.helm.sh/helm-v3.9.4-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.9.4-linux-arm64.tar.gz.sha256sum) / d24163e466f7884c55079d1050968e80a05b633830047116cdfd8ae28d35b0c0)
- [Linux i386](https://get.helm.sh/helm-v3.9.4-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.9.4-linux-386.tar.gz.sha256sum) / a37b0070e2f072050fdf4bd7430ffbe55390fee410eb0781cd01a0fe206eb963)
- [Linux ppc64le](https://get.helm.sh/helm-v3.9.4-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.9.4-linux-ppc64le.tar.gz.sha256sum) / c63a951415c192397fda07c2f52aa60639b280920381c48d58be6803eb0c22f9)
- [Linux s390x](https://get.helm.sh/helm-v3.9.4-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.9.4-linux-s390x.tar.gz.sha256sum) / 7fec97fa800d9bd981e2f42fb0908175db1f35da2d373a971ec7376fe4cb5451)
- [Windows amd64](https://get.helm.sh/helm-v3.9.4-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.9.4-windows-amd64.zip.sha256sum) / 7cdc1342bc1863b6d5ce695fbef4d3b0d65c7c5bcef6ec6adf8fc9aa53821262)

This release was signed with `672C 657B E06B 4B30 969C 4A57 4614 49C2 5E36 B98E ` and can be found at @mattfarina [keybase account](https://keybase.io/mattfarina). Please use the attached signatures for verifying this release using `gpg`.

The [Quickstart Guide](https://helm.sh/docs/intro/quickstart/) will get you going from there. For **upgrade instructions** or detailed installation notes, check the [install guide](https://helm.sh/docs/intro/install/). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3) on any system with `bash`.

## What's Next

- 3.10.0 is the next feature release and will be on September 14, 2022