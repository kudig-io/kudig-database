---
title: helm v2.13 Release Notes
description: helm v2.13 Release Notes — Kubernetes 生产运维知识库
summary: helm v2.13 Release Notes — Kubernetes 生产运维知识库
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
- helm v2.13 Release Notes 是什么
- 如何 helm v2.13 Release Notes
trigger_keywords:
- helm
- v2.13
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




# [[Helm|helm]] v2.13 Release Notes

Source: [v2.13.1](https://github.com/helm/helm/releases/tag/v2.13.1)

Helm v2.13.1 is a patch release. Users are encouraged to upgrade for the best experience.

This release was signed with `92AA 783C BAAE 8E3B` and can be found on @bacongobbler's [keybase account](https://keybase.io/bacongobbler). Please use the attached signatures for verifying this release using `gpg`.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[实体/kubernetes.md|Kubernetes]] Slack](https://kubernetes.slack.com):
  - `#helm-users` for questions and just to hang out
  - `#helm-dev` for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [GitHub/helm/charts](https://github.com/helm/charts)

## Installation and Upgrading

Download Helm 2.13.1. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v2.13.1-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v2.13.1-darwin-amd64.tar.gz.sha256) / `c9564c4133349b98a8c1dda42fdb6545f6e4bfdf0980cdfc38cf76d2f8e5e701`)
- Linux amd64](https://get.helm.sh/helm-v2.13.1-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v2.13.1-linux-amd64.tar.gz.sha256) / `c1967c1dfcd6c921694b80ededdb9bd1beb27cb076864e58957b1568bc98925a`)
- [Linux arm](https://get.helm.sh/helm-v2.13.1-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v2.13.1-linux-arm.tar.gz.sha256) / `679e2f5eae334bddb2707bd31f9212e9298dfff25831128e308d8dba82a13af4`)
- [Linux arm64](https://get.helm.sh/helm-v2.13.1-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v2.13.1-linux-arm64.tar.gz.sha256) / `25ed0b72435007976306f9f44724e1e965bd9e7be839fb4f9851156ab69f0092`)
- [Linux i386](https://get.helm.sh/helm-v2.13.1-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v2.13.1-linux-386.tar.gz.sha256) / `b90303f1b4e867e23dd0a5b0a663dfb5eb3b60d8b4196072bb9ca2bee7bf0637`)
- [Linux ppc64le](https://get.helm.sh/helm-v2.13.1-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v2.13.1-linux-ppc64le.tar.gz.sha256) / `7ee4f3fa0d5724c783aa3d1e5e8b1097889ee098ad545bd043ad75acdc9cb33f`)
- [Linux s390x](https://get.helm.sh/helm-v2.13.1-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v2.13.1-linux-s390x.tar.gz.sha256) / `e2643898b1b95410f06268fb0be3813a9ebd2847e6af57998099593897fd27e6`)
- [Windows amd64](https://get.helm.sh/helm-v2.13.1-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v2.13.1-windows-amd64.zip.sha256) / `cf2719fdab73525ebf630ce348e2e4327ccef90e10c8bdf42d47ab3601dbe2b3`)

Once you have the client installed, upgrade Tiller with `helm init --upgrade`.

The [Quickstart Guide](https://docs.helm.sh/using_helm/#quickstart-guide) will get you going from there. For **upgrade instructions** or detailed installation notes, check the install guide](https://docs.helm.sh/using_helm/#installing-helm). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/master/scripts/get) on any system with `bash`.

## What's Next

- v2.13.2 will contain only bug fixes.
- v2.14.0 is the next feature release.

## Changelog

- pkg/chartutil: fix SaveDir for nested templates directories 618447cbf203d147601b4b9bd7f8c37a5d39fbb4 (Joe Lanford)
- Fix #5046 compatible with MacOS (#5406) a6ccbdaa9e47d61111c88dae259a155bc1540f02 (Marc Khouzam)
- restore klog flags (#5411) 20c949f1215402a7d9898a004fc77bd0e169f95f (Matthew Fisher)


<!-- risk-assessed -->
