---
title: helm v3.18 Release Notes
description: helm v3.18 Release Notes — Kubernetes 生产运维知识库
summary: helm v3.18 Release Notes — Kubernetes 生产运维知识库
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
- helm v3.18 Release Notes 是什么
- 如何 helm v3.18 Release Notes
trigger_keywords:
- helm
- v3.18
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




# [[Helm|helm]] v3.18 Release Notes

Source: [v3.18.6](https://github.com/helm/helm/releases/tag/v3.18.6)

Helm v3.18.6 is a patch release. Users are encouraged to upgrade for the best experience. Users are encouraged to upgrade for the best experience.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes]] Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [ArtifactHub/packages](https://artifacthub.io/packages/search?kind=0)

## Installation and Upgrading

Download Helm v3.18.6. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.18.6-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.18.6-darwin-amd64.tar.gz.sha256sum) / 80cad0470e38cf25731cdead7c32dfbeb887bc177bd6fa01e31b065722f8f06b)
- [MacOS arm64](https://get.helm.sh/helm-v3.18.6-darwin-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.18.6-darwin-arm64.tar.gz.sha256sum) / 48e30d236a1f334c6acb78501be5a851eaa2a267fefeb1131b6484eb2f9f30d7)
- Linux amd64](https://get.helm.sh/helm-v3.18.6-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.18.6-linux-amd64.tar.gz.sha256sum) / 3f43c0aa57243852dd542493a0f54f1396c0bc8ec7296bbb2c01e802010819ce)
- [Linux arm](https://get.helm.sh/helm-v3.18.6-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.18.6-linux-arm.tar.gz.sha256sum) / 13156fbe87a9db201b7d8fd0b42dee2a2e76a77388ea3bd59e3074cfa49d3619)
- [Linux arm64](https://get.helm.sh/helm-v3.18.6-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.18.6-linux-arm64.tar.gz.sha256sum) / 5b8e00b6709caab466cbbb0bc29ee09059b8dc9417991dd04b497530e49b1737)
- [Linux i386](https://get.helm.sh/helm-v3.18.6-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.18.6-linux-386.tar.gz.sha256sum) / d245db7c80edaf47bc42d3fab0b79d2e75d63fa49d06bc0ca599686ebdbd1422)
- [Linux ppc64le](https://get.helm.sh/helm-v3.18.6-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.18.6-linux-ppc64le.tar.gz.sha256sum) / 61eac730dac2029d7acf1c53bd0b1056f78ca2c3f915da7fa3995e74c021eae8)
- [Linux s390x](https://get.helm.sh/helm-v3.18.6-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.18.6-linux-s390x.tar.gz.sha256sum) / c6435c220b2577abea11ee9281fda8a5164583fb1f502702f6ef5c2b4347e38f)
- [Linux riscv64](https://get.helm.sh/helm-v3.18.6-linux-riscv64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.18.6-linux-riscv64.tar.gz.sha256sum) / cea696b01b66fad76b24d9ef28a86802b0931cbc25cec687e8469589438995eb)
- [Windows amd64](https://get.helm.sh/helm-v3.18.6-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.18.6-windows-amd64.zip.sha256sum) / 4263320a8d70df0bb1e245534fba1b541f9889005cdb68dec9798b197c3f66b0)
- [Windows arm64](https://get.helm.sh/helm-v3.18.6-windows-arm64.zip) ([checksum](https://get.helm.sh/helm-v3.18.6-windows-arm64.zip.sha256sum) / fdff469f270d8a155d9289843b92f7b00b2ca353b8ec61532e920eeb3fc2b480)

This release was signed with `672C 657B E06B 4B30 969C 4A57 4614 49C2 5E36 B98E ` and can be found at @mattfarina [keybase account](https://keybase.io/mattfarina). Please use the attached signatures for verifying this release using `gpg`.

The [Quickstart Guide](https://helm.sh/docs/intro/quickstart/) will get you going from there. For **upgrade instructions** or detailed installation notes, check the install guide](https://helm.sh/docs/intro/install/). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3) on any system with `bash`.

## What's Next

- 3.19.0 is the next minor release and will be on September 11, 2025

## Changelog

- fix(helm-lint): fmt b76a950f6835474e0906b96c9ec68a2eff3a6430 (Isaiah Lewis)
- fix(helm-lint): Add TLSClientConfig b79a4212e803ad50c66f06799b8bbdb51f918603 (Isaiah Lewis)
- fix(helm-lint): Add HTTP/HTTPS URL support for json schema references b9180e674fccb57e6ea6934ed7deb4448a3c9ddb (Isaiah Lewis)


<!-- risk-assessed -->
