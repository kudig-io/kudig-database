---
title: helm v3.17 Release Notes
description: helm v3.17 Release Notes — Kubernetes 生产运维知识库
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
- helm v3.17 Release Notes 是什么
- 如何 helm v3.17 Release Notes
trigger_keywords:
- helm
- v3.17
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

# [[Helm|helm]] v3.17 Release Notes

Source: [v3.17.4](https://github.com/helm/helm/releases/tag/v3.17.4)

Helm v3.17.4 is a patch release, this bring is the security release noted below. This is intended for Helm SDK users. CLI users are recommended to use the latest version of Helm.

## Security Advisories

[GHSA-557j-xg8c-q2mm](https://github.com/helm/helm/security/advisories/GHSA-557j-xg8c-q2mm): Chart Dependency Updating With Malicious Chart.yaml Content And Symlink

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[entities/kubernetes.md|Kubernetes]] Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [ArtifactHub/packages](https://artifacthub.io/packages/search?kind=0)

## Installation and Upgrading

Download Helm v3.17.4. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.17.4-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.17.4-darwin-amd64.tar.gz.sha256sum) / 6dfce5d8b09442205393068f5c911b8f20958edd5b67cc26f7eb3330f93225f1)
- [MacOS arm64](https://get.helm.sh/helm-v3.17.4-darwin-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.17.4-darwin-arm64.tar.gz.sha256sum) / f4732719827a76452035641629aa92091adc4a9f1ccbc8d53a729b53e0add869)
- Linux amd64](https://get.helm.sh/helm-v3.17.4-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.17.4-linux-amd64.tar.gz.sha256sum) / c91e3d7293849eff3b4dc4ea7994c338bcc92f914864d38b5789bab18a1d775d)
- [Linux arm](https://get.helm.sh/helm-v3.17.4-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.17.4-linux-arm.tar.gz.sha256sum) / 02c25aa6d656aa3718f2e7d243f28603ce76d00652b875508e98ffb0a1a8e7bb)
- [Linux arm64](https://get.helm.sh/helm-v3.17.4-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.17.4-linux-arm64.tar.gz.sha256sum) / 460a31d1511abb5ad776a26a2a3f0f1382a241b2df3c6d725b0f63c9058ba15a)
- [Linux i386](https://get.helm.sh/helm-v3.17.4-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.17.4-linux-386.tar.gz.sha256sum) / f9e60b44fb5efd08b78b4ec1a9dec93407a084a36710fb0d0402575b4b63c1b4)
- [Linux ppc64le](https://get.helm.sh/helm-v3.17.4-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.17.4-linux-ppc64le.tar.gz.sha256sum) / a181fdb739d59b7aee1951819ab532f38fe2181927f9b601f77aadf5f526f164)
- [Linux s390x](https://get.helm.sh/helm-v3.17.4-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.17.4-linux-s390x.tar.gz.sha256sum) / 392a190f3fc2195fa93828feaa108fe2709d2670c59d23c0645a7f9f857074f0)
- [Linux riscv64](https://get.helm.sh/helm-v3.17.4-linux-riscv64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.17.4-linux-riscv64.tar.gz.sha256sum) / ee1f02a2f69c2762fd6d0ca1b803a79aca9497b2ce7d1cc0ae55c699727777c5)
- [Windows amd64](https://get.helm.sh/helm-v3.17.4-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.17.4-windows-amd64.zip.sha256sum) / 8ffe44ee2aca8cf32156b42efd50e5dabda8da65ab1f10707d9666dea6fcc7e9)
- [Windows arm64](https://get.helm.sh/helm-v3.17.4-windows-arm64.zip) ([checksum](https://get.helm.sh/helm-v3.17.4-windows-arm64.zip.sha256sum) / ef36602d30ce0945b57cc0ceeff674128adf8918da9af6e4fff496930549ac61)

The [Quickstart Guide](https://helm.sh/docs/intro/quickstart/) will get you going from there. For **upgrade instructions** or detailed installation notes, check the install guide](https://helm.sh/docs/intro/install/). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3) on any system with `bash`.

## What's Next

 - 3.18.5 is the next patch release and will be on August 13, 2025
 - 3.19.0 is the next minor release and will be on September 11, 2025

## Changelog

- fixup! Updating link handling 0e59b9e5b951d34584bed39a28786893bbb0fbe2 (Luis Rascao)
- Updating link handling 36635988fdd9c173d7bf75ab65e115763d68a3c4 (Robert Sirchia)
