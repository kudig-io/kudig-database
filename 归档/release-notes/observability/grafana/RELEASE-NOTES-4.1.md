---
title: helm v4.1 Release Notes
description: helm v4.1 Release Notes — Kubernetes 生产运维知识库
summary: helm v4.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- flux
- crd
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- helm v4.1 Release Notes 是什么
- 如何 helm v4.1 Release Notes
trigger_keywords:
- helm
- v4.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Helm|helm]] v4.1 Release Notes

Source: [v4.1.3](https://github.com/helm/helm/releases/tag/v4.1.3)

Helm v4.1.3 is a patch release. Users are encouraged to upgrade for the best experience.

Note there was no 4.1.2 release due to a release automation issue.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[实体/kubernetes.md|Kubernetes]] Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [ArtifactHub/packages](https://artifacthub.io/packages/search?kind=0)

## Notable Changes

- Fixed a bug where --dry-run=server was not respecting generateName #31563
- Fixed a bug where empty CRD resources caused a crash. Now it prints an error #31578 
- Fixed a bug where OCI references with tag+digest failed with "invalid byte" error #31601
- Fixed a bug where user-provided nil value was not preserved when chart has an empty map or no default for a key #31644 
- Fixed a regression since Helm 3.18.0 where Pulling charts from OCI repositories that use an index to store both Container Images and Helm Charts under the same tag failed #31776 
- Fixed a Helm 4 regression where gotemplate white space trimming directly after YAML doc separators, when present after postrendering, caused YAML file corruption #31868 
- Fixed a bug where `FailedStatus` is treated as a terminal state, causing upgrades to fail prematurely when cluster autoscalers needed time to provision nodes, or when pods were being deleted during rolling updates #31897 
- Fixed broken backwards compatibility for deprecated `--atomic` flag on install command #31901
- SDK: Fixed a Windows 'Access Deined' error if multiple processes try to download the same chart version concurrently #31128
- SDK: Fixed a bug where users did not receive any logs from the waiter, causing confusion as several minutes could pass with no user feedback #31717
- SDK: Fixed a bug where server-side apply defaults did not always match the CLI defaults #31732
- SDK: Fixed a Go import issue when downstream tooling attempted to vendor helm.sh/helm/v4/pkg/kube #31852 

## Installation and Upgrading

Download Helm v4.1.3. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v4.1.3-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v4.1.3-darwin-amd64.tar.gz.sha256sum) / 742132e11cc08a81c97f70180cd714ae8376f8c896247a7b14ae1f51838b5a0b)
- [MacOS arm64](https://get.helm.sh/helm-v4.1.3-darwin-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v4.1.3-darwin-arm64.tar.gz.sha256sum) / 21c02fe2f7e27d08e24a6bf93103f9d2b25aab6f13f91814b2cfabc99b108a5e)
- [Linux amd64](https://get.helm.sh/helm-v4.1.3-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v4.1.3-linux-amd64.tar.gz.sha256sum) / 02ce9722d541238f81459938b84cf47df2fdf1187493b4bfb2346754d82a4700)
- [Linux arm](https://get.helm.sh/helm-v4.1.3-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v4.1.3-linux-arm.tar.gz.sha256sum) / 5ea614cd1562e682e213e07f3632b76f9d7b4b0917918e820c515a9030a59951)
- [Linux arm64](https://get.helm.sh/helm-v4.1.3-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v4.1.3-linux-arm64.tar.gz.sha256sum) / 5db45e027cc8de4677ec869e5d803fc7631b0bab1c1eb62ac603a62d22359a43)
- [Linux i386](https://get.helm.sh/helm-v4.1.3-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v4.1.3-linux-386.tar.gz.sha256sum) / 6cbf1f7cca1f4917a0d4a593a22b7c6ec88207e159196eac94f8eaaad8730431)
- [Linux loong64](https://get.helm.sh/helm-v4.1.3-linux-loong64.tar.gz) ([checksum](https://get.helm.sh/helm-v4.1.3-linux-loong64.tar.gz.sha256sum) / ﻿<?xml version="1.0" encoding="utf-8"?><Error><Code>BlobNotFound</Code><Message>The specified blob does not exist.
RequestId:a97d6fdb-301e-0045-72a5-b120d7000000
Time:2026-03-11T22:20:16.6057319Z</Message></Error>)
- [Linux ppc64le](https://get.helm.sh/helm-v4.1.3-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v4.1.3-linux-ppc64le.tar.gz.sha256sum) / 413c21ea07f85beb952807b45cafbcd3bb0ff50aa3ed66e8e87b47bebf2312ce)
- [Linux s390x](https://get.helm.sh/helm-v4.1.3-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v4.1.3-linux-s390x.tar.gz.sha256sum) / c1a5c613429ca50e70ebed3e7535d272805ed4a7aa61c1af20d85121a07e7bcd)
- [Linux riscv64](https://get.helm.sh/helm-v4.1.3-linux-riscv64.tar.gz) ([checksum](https://get.helm.sh/helm-v4.1.3-linux-riscv64.tar.gz.sha256sum) / 0a0beb5b30c24947d71586a7c6bcd774e207ce42b072b046513cf0cff46106a8)
- [Windows amd64](https://get.helm.sh/helm-v4.1.3-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v4.1.3-windows-amd64.zip.sha256sum) / a69356c872fca122650e8c392341c5c49c19da004353514611118087ea2ee7cf)
- [Windows arm64](https://get.helm.sh/helm-v4.1.3-windows-arm64.zip) ([checksum](https://get.helm.sh/helm-v4.1.3-windows-arm64.zip.sha256sum) / 6b05bec8659014df56ede28068a216451b6401d0fbbfd2dadffbef908cd03ff5)

This release was signed with `208D D36E D5BB 3745 A167 43A4 C7C6 FBB5 B91C 1155` and can be found at @scottrigby [keybase account](https://keybase.io/r6by). Please use the attached signatures for verifying this release using `gpg`.

The [Quickstart Guide](https://helm.sh/docs/intro/quickstart/) will get you going from there. For **upgrade instructions** or detailed installation notes, check the [install guide](https://helm.sh/docs/intro/install/). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3) on any system with `bash`.

## What's Next

- 4.2.0 and 3.21.0 are the next minor releases and will be on May 13, 2026
- 4.1.4 and 3.20.2 are the next patch releases and will be on April 8, 2026

## Changelog

- chore(defaults): server-side apply SDK defaults should always match the CLI defaults c94d381b03be117e7e57908edbf642104e00eb8f (Matheus Pimenta)
- whitespace b36d660fb7a5a162d59dcc7877af190b477ecf2b (Austin Abro)
- use logger with waiter 04a91af89e185fb2ff08592057726d54fdd22471 (Austin Abro)
- Remove refactorring changes from coalesce_test.go c3c57dba9878074a7642f2cdf583e809bba7f96e (Evans Mungai)
- Fix import d47cb2b7efaa26090510f4c2289127f40451062e (Evans Mungai)
- Update pkg/chart/common/util/coalesce_test.go 790bf929d5d341df663e0219338fcbf0d5a3119b (Evans Mungai)
- Fix lint warning f7cec12e23fe800495814432da4368b54c6d4552 (Evans Mungai)
- Preserve nil values in chart already d94a5c97cbff6b8e857c578ac7fd29407aa60907 (Evans Mungai)
- fix(values): preserve nil values when chart default is empty map 8c5fe4ef9781c714dc121288a4d499e62cc10bf8 (Evans Mungai)
- chore(deps): bump github.com/cloudflare/circl from 1.6.1 to 1.6.3 217db28a4b75bff615275dc1e3975cf4d389c1d9 (dependabot[bot])
- Restored --atomic flag on install command 7cb43e0c58d8ccf18ca54eadb8d8d6cfb14c06f7 (Travis Leeden)
- fix: bump go.opentelemetry.io/otel/sdk to v1.40.0 for GO-2026-4394 5b26d4f1f99091262fef50ed51133e7ae8e3b011 (Terry Howe)
- fix: bump fluxcd/cli-utils to v0.37.2-flux.1 360c1315ba2d468fcff77193f52764c6ad912c90 (Terry Howe)
- chore(deps): bump sigs.k8s.io/kustomize/kyaml from 0.21.0 to 0.21.1 69a0a92ae3025cb00b480cb1084670d84b9fa018 (dependabot[bot])
- fix: insert newline after doc separators glued to content by template trimming b868e6a7cce696744d3f0c87e3f6bba6a779aff3 (Matheus Pimenta)
- fix: correct import comment in statuswait.go from v3 to v4 dbfbea91e34220911481f82fc6831284d954959a (rohansood10)
- chore(deps): bump the k8s-io group with 7 updates 099192c285233653032e13f557e1b4a7c31e046b (dependabot[bot])
- add image index test 4967ead76215e2ea694c37164d14520da9ba8bde (Pedro Tôrres)
- fix pulling charts from OCI indices 2fe6b106e6d515b6792d53aff517489ae97387be (Pedro Tôrres)
- fix: handle OCI digest algorithm prefix in chart downloader (#31601) e3e2d01ef45c6e4179d0536ea4a475f18fa23846 (Evans Mungai)
- fix(install): check nil for restClientGetter and fix tests c15e7114dc0a0f24e0fda65dff83e7a18e264f60 (Manuel Alonso)
- chore(refactor): better testing and functionality for installing crd df82e68ce1cac6825a59ed43d68d7560d6426979 (Manuel Alonso)
- fix(test): fix tests and check nil for restclient 4b896ca82fa827760086998d6521965e0ace059f (Manuel Alonso)
- fix(test): merge fix correctly 3fc79399c8af02b6704c10cba14be2ced9497cb2 (Manuel Alonso Gonzalez)
- fix(install): add more tests and check nil file data 6017d2b470b07a4f55efd2c55ac0745203ddcd0b (Manuel Alonso)
- fix(test): no check empty resources f451967ab5a5a9b1a26deb699bd1a1f4f2ba6b1e (Manuel Alonso)
- fix(install): check lenght and file nil, add tests fdadff59eb8c3552de76c7647a9c787c248cdbd2 (Manuel Alonso)
- fix(action): crd resources can be empty 10d606726560f1e813617c050c7b9cfbb8b0cb26 (Manuel Alonso)
- fix: casing issue fixed 0fec40f9b61a1dc5799e67bd4fb4ea4321cdd836 (Mujib Ahasan)
- fix: error handled correctly 263749874cb21ba31ebca17fe6c19fb3d0379417 (Mujib Ahasan)
- fix: doc string added 961d7d7cd6b39ccc3a63bce7bba505b94a9be384 (Mujib Ahasan)
- update: test coverage added for helper function validateNameAndGenerateName 29e45067f3f5a772c1400eb756605ba3bc615176 (Mujib Ahasan)
- update: helper function added for the business logic d55b0b9022017aa7a5005d1c6bdc049872bc37f7 (Mujib Ahasan)
- generateName is also considered in logic c1c090e8cead42ee16c36f987f2534c543b556fc (Mujib Ahasan)
- update: business logic respected for skipping object missing name 5e093136f456232f0c80c4d2481e5191b3c72530 (Mujib Ahasan)
- fixed: --dry-run=server now respect generateName f289d1605c9f02c843dd1b5dd03221fdb980f28e (Mujib Ahasan)
- fix(downloader): safely handle concurrent file writes on Windows bfac7393e4cc8b22a36fb015264d10ddb30e03da (Orgad Shaneh)

<!-- risk-assessed -->
