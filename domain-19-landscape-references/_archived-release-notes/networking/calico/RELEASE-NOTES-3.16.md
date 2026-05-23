---
title: helm v3.16 Release Notes
description: helm v3.16 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- helm v3.16 Release Notes 是什么
- 如何 helm v3.16 Release Notes
trigger_keywords:
- helm
- v3.16
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

# [[Helm|helm]] v3.16 Release Notes

Source: [v3.16.4](https://github.com/helm/helm/releases/tag/v3.16.4)

Helm v3.16.4 is a patch release. Users are encouraged to upgrade for the best experience. Users are encouraged to upgrade for the best experience.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[entities/kubernetes|Kubernetes]] Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [ArtifactHub/packages](https://artifacthub.io/packages/search?kind=0)

## Installation and Upgrading

Download Helm v3.16.4. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.16.4-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.16.4-darwin-amd64.tar.gz.sha256sum) / 8dc25671120a4af197afe7ad9041fb8e1dd71bc01e5ef73dba1139cbc9e9f44b)
- [MacOS arm64](https://get.helm.sh/helm-v3.16.4-darwin-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.16.4-darwin-arm64.tar.gz.sha256sum) / e2442d8f05d53d84c39b869bc5fe5affad247ee2f4c706a040919c146edb1f94)
- Linux amd64](https://get.helm.sh/helm-v3.16.4-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.16.4-linux-amd64.tar.gz.sha256sum) / fc307327959aa38ed8f9f7e66d45492bb022a66c3e5da6063958254b9767d179)
- [Linux arm](https://get.helm.sh/helm-v3.16.4-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.16.4-linux-arm.tar.gz.sha256sum) / 432e774d1087d3773737888d384c62477b399227662b42cbf0c32e95e6e72556)
- [Linux arm64](https://get.helm.sh/helm-v3.16.4-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.16.4-linux-arm64.tar.gz.sha256sum) / d3f8f15b3d9ec8c8678fbf3280c3e5902efabe5912e2f9fcf29107efbc8ead69)
- [Linux i386](https://get.helm.sh/helm-v3.16.4-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.16.4-linux-386.tar.gz.sha256sum) / 6022b34e165a5196827043c77bcd1fa28ef48109b3a4af46e8660ddb0a6c44e0)
- [Linux ppc64le](https://get.helm.sh/helm-v3.16.4-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.16.4-linux-ppc64le.tar.gz.sha256sum) / 0ba4375a6dcf6117a8e7729fbed36d9220f8ad98dbc7aabc16186f22917caead)
- [Linux s390x](https://get.helm.sh/helm-v3.16.4-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.16.4-linux-s390x.tar.gz.sha256sum) / 10b88dc2afd97267feba8283d58dcb1c937defb7df8de152c86b60aaece47d59)
- [Linux riscv64](https://get.helm.sh/helm-v3.16.4-linux-riscv64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.16.4-linux-riscv64.tar.gz.sha256sum) / e733c727de7644cfeb25b094e1f0a56c53a7cb437c93c992aa92e6c7b9f6af93)
- [Windows amd64](https://get.helm.sh/helm-v3.16.4-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.16.4-windows-amd64.zip.sha256sum) / 9bb114c12e530a7129fd3d78fc1784e813dd7a6e29e5f9b5af8bc83fd1066d36)
- [Windows arm64](https://get.helm.sh/helm-v3.16.4-windows-arm64.zip) ([checksum](https://get.helm.sh/helm-v3.16.4-windows-arm64.zip.sha256sum) / a4afeee93031920ec08b7e880c859a159bcd222837a81196cadaeb42408ad0b3)

This release was signed with `672C 657B E06B 4B30 969C 4A57 4614 49C2 5E36 B98E ` and can be found at @mattfarina [keybase account](https://keybase.io/mattfarina). Please use the attached signatures for verifying this release using `gpg`.

The [Quickstart Guide](https://helm.sh/docs/intro/quickstart/) will get you going from there. For **upgrade instructions** or detailed installation notes, check the install guide](https://helm.sh/docs/intro/install/). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3) on any system with `bash`.

## What's Next

- 3.17.0 is the next feature release and will be on January 15, 2025

## Changelog

- Bump golang.org/x/crypto from 0.30.0 to 0.31.0 7877b45b63f95635153b29a42c0c2f4273ec45ca (dependabot[bot])
- Bump the k8s-io group with 7 updates 848e586c27f05d84bc19d082f395098aba0b7619 (dependabot[bot])
