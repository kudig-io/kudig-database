---
title: helm v2.2 Release Notes
description: helm v2.2 Release Notes — Kubernetes 生产运维知识库
summary: helm v2.2 Release Notes — Kubernetes 生产运维知识库
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
- helm v2.2 Release Notes 是什么
- 如何 helm v2.2 Release Notes
trigger_keywords:
- helm
- v2.2
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




# [[Helm|helm]] v2.2 Release Notes

Source: [v2.2.3](https://github.com/helm/helm/releases/tag/v2.2.3)

This is a _bugfix release_. Users are encouraged to upgrade for the best experience. _This fixes a critical 'helm delete' bug_ that resulted in data loss.

Additionally, this fixes five other bugs, none of which were marked critical.

Our community is growing rapidly! Jump in!
- Join the discussion in [[实体/kubernetes.md|Kubernetes]] Slack](https://slack.k8s.io/): `#helm` 
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://engineyard.zoom.us/j/366425549)
- Test, debug, and contribute charts: [GitHub/kubernetes/charts](https://github.com/kubernetes/charts)

## Install or Upgrade

Grab a Helm binary and get started.
- [OSX](https://get.helm.sh/helm-v2.2.3-darwin-amd64.tar.gz)
- Linux](https://get.helm.sh/helm-v2.2.3-linux-amd64.tar.gz)
- [Windows](https://get.helm.sh/helm-v2.2.3-windows-amd64.zip)

Once you have it unpacked, run `helm init --upgrade` to upgrade the Tiller server.

The [Quickstart Guide](https://github.com/kubernetes/helm/blob/master/docs/quickstart.md) will get you going from there. For **detailed installation notes**, check the [install guide](https://github.com/kubernetes/helm/blob/master/docs/install.md). You can also use a [script to install](https://raw.githubusercontent.com/kubernetes/helm/master/scripts/get) on any system with `bash`.

## Changelog

Fix helm dep list reporting wrong status c79140874ad122fa87bc5c13d6ee655f581641ac (Qin Wang)
fix(helm): fix bug when helm update can't find release 1. 3a3e3a2598bf4edfef7147b160179d0fd0fbb0e7 (Matt Butcher)
fix(tiller): Fixes problem with `--wait` on headless Services 34c3cd895019563da68ccc5c84b4b2c04b1e7fdb (Taylor Thomas)
fix(tiller): enforce release name length on uninstall 15efd568b23f8fc66b0bb202cf62ee53f84f0360 (Adam Reese)
fix(helm): remove max column width for repo list 91c34afc66d274c1e6beda4aac644eb26b7aa6a9 (Adam Reese)


<!-- risk-assessed -->
