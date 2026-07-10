---
title: helm v3.12 Release Notes
description: helm v3.12 Release Notes — Kubernetes 生产运维知识库
summary: helm v3.12 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
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
- helm v3.12 Release Notes 是什么
- 如何 helm v3.12 Release Notes
trigger_keywords:
- helm
- v3.12
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




# [[Helm|helm]] v3.12 Release Notes

Source: [v3.12.3](https://github.com/helm/helm/releases/tag/v3.12.3)

Helm v3.12.3 is a patch release. Users are encouraged to upgrade for the best experience. Users are encouraged to upgrade for the best experience.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[entities/kubernetes.md|Kubernetes]] Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [ArtifactHub/packages](https://artifacthub.io/packages/search?kind=0)

## Installation and Upgrading

Download Helm v3.12.3. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.12.3-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.12.3-darwin-amd64.tar.gz.sha256sum) / 1bdbbeec5a12dd0c1cd4efd8948a156d33e1e2f51140e2a51e1e5e7b11b81d47)
- [MacOS arm64](https://get.helm.sh/helm-v3.12.3-darwin-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.12.3-darwin-arm64.tar.gz.sha256sum) / 240b0a7da9cae208000eff3d3fb95e0fa1f4903d95be62c3f276f7630b12dae1)
- Linux amd64](https://get.helm.sh/helm-v3.12.3-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.12.3-linux-amd64.tar.gz.sha256sum) / 1b2313cd198d45eab00cc37c38f6b1ca0a948ba279c29e322bdf426d406129b5)
- [Linux arm](https://get.helm.sh/helm-v3.12.3-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.12.3-linux-arm.tar.gz.sha256sum) / 6b67cf5fc441c1fcb4a860629b2ec613d0e6c8ac536600445f52a033671e985e)
- [Linux arm64](https://get.helm.sh/helm-v3.12.3-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.12.3-linux-arm64.tar.gz.sha256sum) / 79ef06935fb47e432c0c91bdefd140e5b543ec46376007ca14a52e5ed3023088)
- [Linux i386](https://get.helm.sh/helm-v3.12.3-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.12.3-linux-386.tar.gz.sha256sum) / cb789c4753bf66c8426f6be4091349c0780aaf996af0a1de48318f9f8d6b7bc8)
- [Linux ppc64le](https://get.helm.sh/helm-v3.12.3-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.12.3-linux-ppc64le.tar.gz.sha256sum) / 8f2182ae53dd129a176ee15a09754fa942e9e7e9adab41fd60a39833686fe5e6)
- [Linux s390x](https://get.helm.sh/helm-v3.12.3-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.12.3-linux-s390x.tar.gz.sha256sum) / f5d5c7a4e831dedc8dac5913d4c820e0da10e904debb59dec65bde203fad1af0)
- [Windows amd64](https://get.helm.sh/helm-v3.12.3-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.12.3-windows-amd64.zip.sha256sum) / f3e2e9d69bb0549876aef6e956976f332e482592494874d254ef49c4862c5712)

This release was signed with `672C 657B E06B 4B30 969C 4A57 4614 49C2 5E36 B98E ` and can be found at @mattfarina [keybase account](https://keybase.io/mattfarina). Please use the attached signatures for verifying this release using `gpg`.

The [Quickstart Guide](https://helm.sh/docs/intro/quickstart/) will get you going from there. For **upgrade instructions** or detailed installation notes, check the install guide](https://helm.sh/docs/intro/install/). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3) on any system with `bash`.

## What's Next

- 3.13.0 is the next feature release and be on September 13, 2023.

## Changelog

- bump kubernetes modules to v0.27.3 3a31588ad33fe3b89af5a2a54ee1d25bfe6eaa5e (Joe Julian)
- Add priority class to kind sorter fb7415543b910e5661337e187e2be9d3f383638d (Stepan Dohnal)


<!-- risk-assessed -->
