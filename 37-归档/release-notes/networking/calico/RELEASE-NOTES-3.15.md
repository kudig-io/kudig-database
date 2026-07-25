---
title: helm v3.15 Release Notes
description: helm v3.15 Release Notes — Kubernetes 生产运维知识库
summary: helm v3.15 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- docker
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- helm v3.15 Release Notes 是什么
- 如何 helm v3.15 Release Notes
trigger_keywords:
- helm
- v3.15
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




# [[Helm|helm]] v3.15 Release Notes

Source: [v3.15.4](https://github.com/helm/helm/releases/tag/v3.15.4)

Helm v3.15.4 is a patch release. Users are encouraged to upgrade for the best experience. Users are encouraged to upgrade for the best experience.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes]] Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [ArtifactHub/packages](https://artifacthub.io/packages/search?kind=0)

## Installation and Upgrading

Download Helm v3.15.4. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.15.4-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.15.4-darwin-amd64.tar.gz.sha256sum) / 1bc3f354f7ce4d7fd9cfa5bcc701c1f32c88d27076d96c2792d5b5226062aee5)
- [MacOS arm64](https://get.helm.sh/helm-v3.15.4-darwin-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.15.4-darwin-arm64.tar.gz.sha256sum) / 88115846a1fb58f8eb8f64fec5c343d95ca394f1be811602fa54a887c98730ac)
- Linux amd64](https://get.helm.sh/helm-v3.15.4-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.15.4-linux-amd64.tar.gz.sha256sum) / 11400fecfc07fd6f034863e4e0c4c4445594673fd2a129e701fe41f31170cfa9)
- [Linux arm](https://get.helm.sh/helm-v3.15.4-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.15.4-linux-arm.tar.gz.sha256sum) / aa3fb3014d147e5dcf8bfe4f6d5fe8677029ed720d4a4bcc64e54cb745a72206)
- [Linux arm64](https://get.helm.sh/helm-v3.15.4-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.15.4-linux-arm64.tar.gz.sha256sum) / fa419ecb139442e8a594c242343fafb7a46af3af34041c4eac1efcc49d74e626)
- [Linux i386](https://get.helm.sh/helm-v3.15.4-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.15.4-linux-386.tar.gz.sha256sum) / d1457e19fa7b467aaa53433793c446582956905c66d4655655010cc9cef995d3)
- [Linux ppc64le](https://get.helm.sh/helm-v3.15.4-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.15.4-linux-ppc64le.tar.gz.sha256sum) / e4efce93723f52dd858e9046ea836c9c75f346facce1b87b8cf78c817b97e6ac)
- [Linux s390x](https://get.helm.sh/helm-v3.15.4-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.15.4-linux-s390x.tar.gz.sha256sum) / c6e0cdea598196895ac7b627ce972699ef9f06b0eba51dc4db7cc21b3369f24a)
- [Linux riscv64](https://get.helm.sh/helm-v3.15.4-linux-riscv64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.15.4-linux-riscv64.tar.gz.sha256sum) / 5d483ef8c61cf7efeac34278ad90c22a2a1978330723c0ea5f017ee48aee11c4)
- [Windows amd64](https://get.helm.sh/helm-v3.15.4-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.15.4-windows-amd64.zip.sha256sum) / 023b96f02e812cda1a1d5c950cb321834a56f4a50ca90146ff447a81be0ae5b6)

This release was signed with `672C 657B E06B 4B30 969C 4A57 4614 49C2 5E36 B98E ` and can be found at @mattfarina [keybase account](https://keybase.io/mattfarina). Please use the attached signatures for verifying this release using `gpg`.

The [Quickstart Guide](https://helm.sh/docs/intro/quickstart/) will get you going from there. For **upgrade instructions** or detailed installation notes, check the install guide](https://helm.sh/docs/intro/install/). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3) on any system with `bash`.

## What's Next

- 3.16.0 is the next feature release and will be on September 11, 2024.

## Changelog

- Bump the k8s-io group across 1 directory with 7 updates fa9efb07d9d8debbb4306d72af76a383895aa8c4 (dependabot[bot])
- Bump github.com/docker/docker 36a21b18b93d8712f0948d9764f8cd29558b9cb1 (dependabot[bot])


<!-- risk-assessed -->
