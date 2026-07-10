---
title: helm v3.7 Release Notes
description: helm v3.7 Release Notes — Kubernetes 生产运维知识库
summary: helm v3.7 Release Notes — Kubernetes 生产运维知识库
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
- helm v3.7 Release Notes 是什么
- 如何 helm v3.7 Release Notes
trigger_keywords:
- helm
- v3.7
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




# [[Helm|helm]] v3.7 Release Notes

Source: [v3.7.2](https://github.com/helm/helm/releases/tag/v3.7.2)

Helm v3.7.2 is a patch release. Users are encouraged to upgrade for the best experience.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[实体/kubernetes.md|Kubernetes]] Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [ArtifactHub/packages](https://artifacthub.io/packages/search?kind=0)

## Installation and Upgrading

Download Helm v3.7.2. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.7.2-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.7.2-darwin-amd64.tar.gz.sha256sum) / 5a0738afb1e194853aab00258453be8624e0a1d34fcc3c779989ac8dbcd59436)
- [MacOS arm64](https://get.helm.sh/helm-v3.7.2-darwin-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.7.2-darwin-arm64.tar.gz.sha256sum) / 260d4b8bffcebc6562ea344dfe88efe252cf9511dd6da3cccebf783773d42aec)
- Linux amd64](https://get.helm.sh/helm-v3.7.2-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.7.2-linux-amd64.tar.gz.sha256sum) / 4ae30e48966aba5f807a4e140dad6736ee1a392940101e4d79ffb4ee86200a9e)
- [Linux arm](https://get.helm.sh/helm-v3.7.2-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.7.2-linux-arm.tar.gz.sha256sum) / ab73727f1c00903aff010a3557ab4366a1a13ce2d243c9cb191e703fbb76c915)
- [Linux arm64](https://get.helm.sh/helm-v3.7.2-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.7.2-linux-arm64.tar.gz.sha256sum) / b0214eabbb64791f563bd222d17150ce39bf4e2f5de49f49fdb456ce9ae8162f)
- [Linux i386](https://get.helm.sh/helm-v3.7.2-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.7.2-linux-386.tar.gz.sha256sum) / 9bd6f50307fdaa26100bca3fd55aaac3016a985424c8482f37d3a3a4c8a9dbed)
- [Linux ppc64le](https://get.helm.sh/helm-v3.7.2-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.7.2-linux-ppc64le.tar.gz.sha256sum) / a2a44726bee7d69b08fadc72fb3716428b9963f78ea5290711fe6fcb9bac3f14)
- [Linux s390x](https://get.helm.sh/helm-v3.7.2-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.7.2-linux-s390x.tar.gz.sha256sum) / 036167ca03f5e00ac1e8f27dc260c8316d8596d8eb3ddac2c5431b4b692d55af)
- [Windows amd64](https://get.helm.sh/helm-v3.7.2-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.7.2-windows-amd64.zip.sha256sum) / 299165f0af46bece9a61b41305cca8e8d5ec5319a4b694589cd71e6b75aca77e)

This release was signed with `672C 657B E06B 4B30 969C 4A57 4614 49C2 5E36 B98E ` and can be found at @mattfarina [keybase account](https://keybase.io/mattfarina). Please use the attached signatures for verifying this release using `gpg`.

The [Quickstart Guide](https://helm.sh/docs/intro/quickstart/) will get you going from there. For **upgrade instructions** or detailed installation notes, check the install guide](https://helm.sh/docs/intro/install/). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3) on any system with `bash`.

## What's Next

- 3.8.0 is the next feature release and will be released on January 19, 2022.

## Changelog

- Channel should remain open if there is still a routine that wants to write into it 663a896f4a815053445eec4153677ddc24a0a361 (Jerome Küttner)
- Fix memory leak in upgrade action 95c03eecdb87feae2ba5d5651225ef6f53d6892a (Jerome Küttner)
- chore(deps): bump github.com/Masterminds/squirrel from 1.5.1 to 1.5.2 cf8b02d3187c2a190b9f7c1a956a3fe4451c66e9 (dependabot[bot])
- chore(deps): bump github.com/Masterminds/squirrel from 1.5.0 to 1.5.1 013632b2c56c7b45a2e2ea2631d130ee4833808d (dependabot[bot])
- chore(deps): bump github.com/gofrs/flock from 0.8.0 to 0.8.1 339681484da195543461a1b9340b7eb47a0978a0 (dependabot[bot])
- Updating to Kubernetes 1.22.4 packages d5bd91cb91702291e16da3b7975162bfe88cf986 (Matt Farina)
- Fix specifying of Kubernetes version from build scripts bb7f8b2b4092f4040247525ab406f62babc174c5 (Matt Farina)
- allow ldflags to overwrite k8s version 7e750ff4e9d3099f0087719c50e81a76500bb08a (Sverre Boschman)
- Use buffered channel for signal notification dfb24521ac68eb88e55c3d255d9e10727fb39e14 (Martin Hickey)

<!-- risk-assessed -->
