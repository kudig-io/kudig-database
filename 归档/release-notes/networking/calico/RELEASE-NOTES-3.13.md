---
title: helm v3.13 Release Notes
description: helm v3.13 Release Notes — Kubernetes 生产运维知识库
summary: helm v3.13 Release Notes — Kubernetes 生产运维知识库
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
- helm v3.13 Release Notes 是什么
- 如何 helm v3.13 Release Notes
trigger_keywords:
- helm
- v3.13
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




# [[Helm|helm]] v3.13 Release Notes

Source: [v3.13.3](https://github.com/helm/helm/releases/tag/v3.13.3)

Helm v3.13.3 is a patch release. Users are encouraged to upgrade for the best experience. Users are encouraged to upgrade for the best experience.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[实体/kubernetes.md|Kubernetes]] Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [ArtifactHub/packages](https://artifacthub.io/packages/search?kind=0)

## Installation and Upgrading

Download Helm v3.13.3. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.13.3-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.13.3-darwin-amd64.tar.gz.sha256sum) / da654c9e0fd4fcb50cc5dba051c1c9cf398e21ffa5064b47ac89a9697e139d39)
- [MacOS arm64](https://get.helm.sh/helm-v3.13.3-darwin-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.13.3-darwin-arm64.tar.gz.sha256sum) / 61ba210cd65c53be5c0021c8fc8e0b94f4c122aff32f5ed0e4ea81728108ea20)
- Linux amd64](https://get.helm.sh/helm-v3.13.3-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.13.3-linux-amd64.tar.gz.sha256sum) / bbb6e7c6201458b235f335280f35493950dcd856825ddcfd1d3b40ae757d5c7d)
- [Linux arm](https://get.helm.sh/helm-v3.13.3-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.13.3-linux-arm.tar.gz.sha256sum) / 0170b15f3951be399e27e0cfdc21edb211d3b6b2698e078f993d9558d9446e3f)
- [Linux arm64](https://get.helm.sh/helm-v3.13.3-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.13.3-linux-arm64.tar.gz.sha256sum) / 44aaa094ae24d01e8c36e327e1837fd3377a0f9152626da088384c5bc6d94562)
- [Linux i386](https://get.helm.sh/helm-v3.13.3-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.13.3-linux-386.tar.gz.sha256sum) / a92929ba472ff4d31b83bcdd957f94ebb8c396c371c840afd04fa6a7fba61515)
- [Linux ppc64le](https://get.helm.sh/helm-v3.13.3-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.13.3-linux-ppc64le.tar.gz.sha256sum) / 85afc540af42ebbb6e6a4fe270b04ce1fa27fa72845cd1d352feea0f55df1ffc)
- [Linux s390x](https://get.helm.sh/helm-v3.13.3-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.13.3-linux-s390x.tar.gz.sha256sum) / 19dce0dec6225132b80c3f6dfbc9f804cedd8becdbed5e30d197c4bbf20ce3c0)
- [Windows amd64](https://get.helm.sh/helm-v3.13.3-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.13.3-windows-amd64.zip.sha256sum) / abb5e06a3587d8da7cca60c801cfbaa5178f4252c367b2469b3f123da2357cac)

This release was signed with `672C 657B E06B 4B30 969C 4A57 4614 49C2 5E36 B98E ` and can be found at @mattfarina [keybase account](https://keybase.io/mattfarina). Please use the attached signatures for verifying this release using `gpg`.

The [Quickstart Guide](https://helm.sh/docs/intro/quickstart/) will get you going from there. For **upgrade instructions** or detailed installation notes, check the install guide](https://helm.sh/docs/intro/install/). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3) on any system with `bash`.

## What's Next

- 3.14.0 is the next feature release and be on January 17, 2024.

## Changelog

- Updating Helm libraries for k8s 1.28.4 c8b948945e52abba22ff885446a1486cb5fd3474 (Matt Farina)
- Remove excessive logging 2f03d01b7d29d65374838a8376644e2b12066c81 (Sean Mills)
- chore(create): indent to spaces 2e6357665a4100eb8472902b693c8dfa50acc5aa (genofire)


<!-- risk-assessed -->
