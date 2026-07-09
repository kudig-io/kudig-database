---
title: helm v3.5 Release Notes
description: helm v3.5 Release Notes — Kubernetes 生产运维知识库
summary: helm v3.5 Release Notes — Kubernetes 生产运维知识库
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
- helm v3.5 Release Notes 是什么
- 如何 helm v3.5 Release Notes
trigger_keywords:
- helm
- v3.5
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




# [[Helm|helm]] v3.5 Release Notes

Source: [v3.5.4](https://github.com/helm/helm/releases/tag/v3.5.4)

Helm v3.5.4 is a patch release. Users are encouraged to upgrade for the best experience.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[entities/kubernetes.md|Kubernetes]] Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [ArtifactHub/packages](https://artifacthub.io/packages/search?kind=0)

## Installation and Upgrading

Download Helm v3.5.4. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.5.4-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.5.4-darwin-amd64.tar.gz.sha256sum) / 072c40c743d30efdb8231ca03bab55caee7935e52175e42271a0c3bc37ec0b7b)
- Linux amd64](https://get.helm.sh/helm-v3.5.4-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.5.4-linux-amd64.tar.gz.sha256sum) / a8ddb4e30435b5fd45308ecce5eaad676d64a5de9c89660b56face3fe990b318)
- [Linux arm](https://get.helm.sh/helm-v3.5.4-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.5.4-linux-arm.tar.gz.sha256sum) / 1a9cc09ef06db29a0232d265f73625056a0cb089e5a16b0a5ef8e810e0533157)
- [Linux arm64](https://get.helm.sh/helm-v3.5.4-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.5.4-linux-arm64.tar.gz.sha256sum) / 9db01522150a83a5d65b420171147448d8396c142d2c91af95e5ee77c1694176)
- [Linux i386](https://get.helm.sh/helm-v3.5.4-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.5.4-linux-386.tar.gz.sha256sum) / 0a8366cfd6a51a66122c8705c153b06202a4c13bf590f31dcf15c54f40975267)
- [Linux ppc64le](https://get.helm.sh/helm-v3.5.4-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.5.4-linux-ppc64le.tar.gz.sha256sum) / 228dee9d5799cdeb92a7bc575c2177d2f4367f91dd3ee6ce506c45089fe929f8)
- [Linux s390x](https://get.helm.sh/helm-v3.5.4-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.5.4-linux-s390x.tar.gz.sha256sum) / 18e6c761943b9862704dfe8c914a574e313e4628c0bee6f37176a423b47d46d2)
- [Windows amd64](https://get.helm.sh/helm-v3.5.4-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.5.4-windows-amd64.zip.sha256sum) / 830da2a8fba060ceff95486b3166b11c517035092e213f8d775be4ae2f7c13e0)

This release was signed with `672C 657B E06B 4B30 969C 4A57 4614 49C2 5E36 B98E ` and can be found at @mattfarina [keybase account](https://keybase.io/mattfarina). Please use the attached signatures for verifying this release using `gpg`.

The [Quickstart Guide](https://helm.sh/docs/intro/quickstart/) will get you going from there. For **upgrade instructions** or detailed installation notes, check the install guide](https://helm.sh/docs/intro/install/). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/master/scripts/get-helm-3) on any system with `bash`.

## What's Next

- 3.6.0 is the next feature release and will be released on May 26, 2021.

## Changelog

- Add/update deprecation notices 1b5edb69df3d3a08df77c9902dc17af864ff05d1 (Simon Croome)
- Wrap validation error instead of recreating 29fc83554130762a6f5d70c886891fef1ebea018 (Simon Croome)
- Move default to avoid nil check 9b7322861d651ea5be5b42df6cb84fa934d5d428 (Simon Croome)
- Add name validation rules for object kinds dacb65d7f43723dca43ef4303534127b1fe91d1c (Simon Croome)
- Use kube libraries v0.20.4 c409cf1e987cf5d786ebfbcc3bfaa1d56cdf1c95 (Shoubhik Bose)

<!-- risk-assessed -->
