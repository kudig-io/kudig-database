---
title: helm v3.8 Release Notes
description: helm v3.8 Release Notes — Kubernetes 生产运维知识库
summary: helm v3.8 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- helm v3.8 Release Notes 是什么
- 如何 helm v3.8 Release Notes
trigger_keywords:
- helm
- v3.8
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




# [[Helm|helm]] v3.8 Release Notes

Source: [v3.8.2](https://github.com/helm/helm/releases/tag/v3.8.2)

Helm v3.8.2 is a patch release. Users are encouraged to upgrade for the best experience.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[entities/kubernetes.md|Kubernetes]] Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [ArtifactHub/packages](https://artifacthub.io/packages/search?kind=0)

## Installation and Upgrading

Download Helm v3.8.2. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.8.2-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.8.2-darwin-amd64.tar.gz.sha256sum) / 25bb4a70b0d9538a97abb3aaa57133c0779982a8091742a22026e60d8614f8a0)
- [MacOS arm64](https://get.helm.sh/helm-v3.8.2-darwin-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.8.2-darwin-arm64.tar.gz.sha256sum) / dfddc0696597c010ed903e486fe112a18535ab0c92e35335aa54af2360077900)
- Linux amd64](https://get.helm.sh/helm-v3.8.2-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.8.2-linux-amd64.tar.gz.sha256sum) / 6cb9a48f72ab9ddfecab88d264c2f6508ab3cd42d9c09666be16a7bf006bed7b)
- [Linux arm](https://get.helm.sh/helm-v3.8.2-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.8.2-linux-arm.tar.gz.sha256sum) / 3447782673a8dec87f0736d3fcde5c2af6316b0dd19f43b7ffaf873e4f5a486e)
- [Linux arm64](https://get.helm.sh/helm-v3.8.2-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.8.2-linux-arm64.tar.gz.sha256sum) / 238db7f55e887f9c1038b7e43585b84389a05fff5424e70557886cad1635b3ce)
- [Linux i386](https://get.helm.sh/helm-v3.8.2-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.8.2-linux-386.tar.gz.sha256sum) / 4d18731d8c71031b38c4b6579636eda6626b25f5a1965fd3e44b7d5f58c702d5)
- [Linux ppc64le](https://get.helm.sh/helm-v3.8.2-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.8.2-linux-ppc64le.tar.gz.sha256sum) / 144fcfface6dc99295c1cfdd39238f188c601b96472e933e17054eddd1acb8fa)
- [Linux s390x](https://get.helm.sh/helm-v3.8.2-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.8.2-linux-s390x.tar.gz.sha256sum) / 3dece48def23f1a97568936e1099bc626effc9207786b355ea01b274cd8ab2c0)
- [Windows amd64](https://get.helm.sh/helm-v3.8.2-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.8.2-windows-amd64.zip.sha256sum) / 051959311ed5a3d49596b298b9e9618e2a0ad6a9270c134802f205698348ba5e)

This release was signed with `672C 657B E06B 4B30 969C 4A57 4614 49C2 5E36 B98E ` and can be found at @mattfarina [keybase account](https://keybase.io/mattfarina). Please use the attached signatures for verifying this release using `gpg`.

The [Quickstart Guide](https://helm.sh/docs/intro/quickstart/) will get you going from there. For **upgrade instructions** or detailed installation notes, check the install guide](https://helm.sh/docs/intro/install/). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3) on any system with `bash`.

## What's Next

- 3.9.0 is the next feature release and will be released on May 11, 2022.

## Changelog

- Bump oras.land/oras-go from 1.1.0 to 1.1.1 6e3701edea09e5d55a8ca2aae03a68917630e91b (dependabot[bot])
- Fixing downloader plugin error handling 7c3f6516f4d4b2d1dc34c4c9d725b973a5738679 (Zoran Krleza)
- Simplify testdata charts cf45b1a271ec06a2e2fb459afbc1bd86e3e88c78 (Aram Zegerius)
- Simplify testdata charts 7526b0ed8dd4a3f6d5227d5160ec17cf226ae20a (Aram Zegerius)
- Make validation errors easier to fix 7df8251760085ba247fce49415e2be20e6f08ab7 (Damien Nozay)
- fix tarFromLocalDir saving file dependencies in dest path bd77989fb14b017880fde4bf842c7fa459b13f0a (Matthew Fisher)
- Add tests for multi-level dependencies. d263aaadee29bbe394ec4568976ebff7d7e05936 (Aram Zegerius)
- Fix value precedence 5d017e11f1f47345a3559bf70f63d81a7edc981a (Aram Zegerius)
- Bumping Kubernetes package versions 9d3ce9b141b9d8ce57c6e779657731d739b61312 (Matt Farina)
- Updating vcs to latest version c0a645a3a31d4701ba1476d9afe54c723a5168f9 (Matt Farina)
- Dont modify provided transport f4276f45264dbe075567e9ded8f38d1427c0f20f (Matthias Fehr)
- Pass http getter as pointer in tests b216f76899dc9e5f0a1dab02b41b3bbf240eabed (Matthias Fehr)
- Add docs block 65b6e6d1cab1cb495395c91a50a2e7fbd8ed6468 (Matthias Fehr)
- Add transport option and tests ab4dc7861b6af9587f0f1e4149c52f109296924f (Matthias Fehr)
- Reuse http transport 6c5adf1ddf09fd4d478c057d8b0d417ab8137e58 (Matthias Fehr)

<!-- risk-assessed -->
