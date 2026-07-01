---
title: helm v3.19 Release Notes
description: helm v3.19 Release Notes — Kubernetes 生产运维知识库
summary: helm v3.19 Release Notes — Kubernetes 生产运维知识库
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
- helm v3.19 Release Notes 是什么
- 如何 helm v3.19 Release Notes
trigger_keywords:
- helm
- v3.19
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---



# [[Helm|helm]] v3.19 Release Notes

Source: [v3.19.5](https://github.com/helm/helm/releases/tag/v3.19.5)

Helm v3.19.5 is a patch release. Users are encouraged to upgrade for the best experience.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[entities/kubernetes.md|Kubernetes]] Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [ArtifactHub/packages](https://artifacthub.io/packages/search?kind=0)

## Notable Changes

- Fixed bug where removing subchart value via override resulted in warning #31118
- Fixed bug where helm uninstall with --keep-history did not suspend previous deployed releases https://github.com/helm/helm/issues/12556

## Installation and Upgrading

Download Helm v3.19.5. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.19.5-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.19.5-darwin-amd64.tar.gz.sha256sum) / 57f4a847c349382b7cc742a6434ef25f88f0928a113d8cf49084b464878ef0b9)
- [MacOS arm64](https://get.helm.sh/helm-v3.19.5-darwin-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.19.5-darwin-arm64.tar.gz.sha256sum) / 195e24e587f423f15a78feebab04583ceee68323598575a0e8b3b11b43fd26fe)
- Linux amd64](https://get.helm.sh/helm-v3.19.5-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.19.5-linux-amd64.tar.gz.sha256sum) / a0a5e8c592ed3f376ac110715eff214730c7422f9a44d96cf98117d2b8b0e6c0)
- [Linux arm](https://get.helm.sh/helm-v3.19.5-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.19.5-linux-arm.tar.gz.sha256sum) / 1367926ea842729b4312fbf800234d15bcaa419c92201727b776da4550078a09)
- [Linux arm64](https://get.helm.sh/helm-v3.19.5-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.19.5-linux-arm64.tar.gz.sha256sum) / ce02147ffee6d993bf8ae97a44a22e9e1daf0b69d2d5b69a0c8cf6706445ccf5)
- [Linux i386](https://get.helm.sh/helm-v3.19.5-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.19.5-linux-386.tar.gz.sha256sum) / 54ec170590a6bfb26990c645426f92089d9eb574190c00620ca793d92b5891d5)
- [Linux ppc64le](https://get.helm.sh/helm-v3.19.5-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.19.5-linux-ppc64le.tar.gz.sha256sum) / a51ba349875e2a219c909ae802435db403ea6924ca4725acb73f520da36e5f45)
- [Linux s390x](https://get.helm.sh/helm-v3.19.5-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.19.5-linux-s390x.tar.gz.sha256sum) / 071f19deabaf2326a7ca54c3143934e2001c61bd106fa2949bf53d1e7452ecd0)
- [Linux riscv64](https://get.helm.sh/helm-v3.19.5-linux-riscv64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.19.5-linux-riscv64.tar.gz.sha256sum) / a33b2df76300d33008a2b47107f289a0de31d461e6bfb2354bf1fd747ccecc9b)
- [Windows amd64](https://get.helm.sh/helm-v3.19.5-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.19.5-windows-amd64.zip.sha256sum) / f258b0d17a4c914ad453f9d8cc21643dddd354f4fbad4c7c595cf3480221379e)
- [Windows arm64](https://get.helm.sh/helm-v3.19.5-windows-arm64.zip) ([checksum](https://get.helm.sh/helm-v3.19.5-windows-arm64.zip.sha256sum) / 232bccce9fe4212a22acc210a555edc75e101b26fb9a9eb1442c32bda8d102bf)

This release was signed with `208D D36E D5BB 3745 A167 43A4 C7C6 FBB5 B91C 1155` and can be found at @scottrigby [keybase account](https://keybase.io/r6by). Please use the attached signatures for verifying this release using `gpg`.

The [Quickstart Guide](https://helm.sh/docs/intro/quickstart/) will get you going from there. For **upgrade instructions** or detailed installation notes, check the install guide](https://helm.sh/docs/intro/install/). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3) on any system with `bash`.

## What's Next

- 4.1.0 and 3.20.0 is the next minor releases and will be on January 21, 2026
- 4.1.1 and 3.20.1 are the next patch releases and will be on March 11, 2026

## Changelog

- fix(rollback): `errors.Is` instead of string comp 4a19a5b6fb912c5c28a779e73f2e0880d9e239a4 (Hidde Beydals)
- fix(uninstall): supersede deployed releases 7a00235a0622b6eae1d06fbb87c2a33b718cbd7e (Hidde Beydals)
- fix null merge 578564ee26171e5ca2ee0edd0c06cb58a72fba87 (Ben Foster)