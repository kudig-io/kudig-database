---
title: helm v2.5 Release Notes
description: helm v2.5 Release Notes — Kubernetes 生产运维知识库
summary: helm v2.5 Release Notes — Kubernetes 生产运维知识库
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
- helm v2.5 Release Notes 是什么
- 如何 helm v2.5 Release Notes
trigger_keywords:
- helm
- v2.5
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




# [[Helm|helm]] v2.5 Release Notes

Source: [v2.5.1](https://github.com/helm/helm/releases/tag/v2.5.1)

This mid-summer Helm release includes several fixes to the Helm 2.5 release. Most notably:

- The deadlock problem with Tiller has been found and fixed. A huge thanks to the dozen community members who submitted use cases and data to help us find the problem.
- The `helm get manifest` and `helm get value` commands are now working properly.
- Plugins will now receive the correct value for `$HELM_HOME` regardless of how the home directory was set.

_Helm v2.5.1 is tested on [[Kubernetes|Kubernetes]] 1.5-1.7. However, the latest Kubernetes 1.7 features (e.g. custom type definitions) are not supported in this release. Full Kubernetes 1.7 support is coming in Helm 2.6._

Our community is one of the fastest growing Kubernetes projects. We'd love to have you as a part!

- Join the discussion in [Kubernetes Slack](https://slack.k8s.io/):
  - `#helm-users` for questions and just to hang out
  - `#helm-devs` for discussing PRs, code, and bugs 
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/4526666954)
- Test, debug, and contribute charts: [GitHub/kubernetes/charts](https://github.com/kubernetes/charts)

## Installation and Upgrading

Download Helm 2.5.1. The common platform binaries are here:

- [OSX](https://get.helm.sh/helm-v2.5.1-darwin-amd64.tar.gz)
- Linux](https://get.helm.sh/helm-v2.5.1-linux-amd64.tar.gz)
- [Windows](https://get.helm.sh/helm-v2.5.1-windows-amd64.zip)

Once you have the client installed, upgrade Tiller with `helm init --upgrade`.

The [Quickstart Guide](https://docs.helm.sh/using_helm/#quickstart-guide) will get you going from there. For **upgrade instructions** or detailed installation notes, check the install guide](https://docs.helm.sh/using_helm/#installing-helm). You can also use a [script to install](https://raw.githubusercontent.com/kubernetes/helm/master/scripts/get) on any system with `bash`.

## Breaking Changes

In this release, we fixed a bug that caused extra empty fields to be added to a `requirements.yaml` file. As a result, in some cases this fix will result in a regeneration of the requirements lock file. This is not technically a breaking change, but may result in surprising behavior.

## What's Next

- 2.6.0 is the next feature release. It is nearing completion.

## Changelog

- fix(tiller): remove locking system from storage and rely on backend controls b6624b78ea7da2aa1ef171963d08e67664f99cd5 (Justin Scott)
- ref(helm): refactor cleanup of environment after tests run aa54325a6249bbd4c784ebf08afeb3637b4329c7 (Adam Reese)
- fix(helm): fix flag parsing once and for all 97857297a6da1d1d4a4df3d2e0ce3a0fa3583484 (Adam Reese)
- fix(helm): fix `helm get` subcommands e960523b3e870a85804acb77750406fe77cb04b7 (Matt Butcher)
- fix(helm): support HELM_HOME during plugin loading d68562faa245403a16d0a3c684dd329042ff7281 (Adam Reese)
- fix(helm): load home from flags during runtime 7692de7ff0b22233b32c9574690dd6ab7a9acb09 (Adam Reese)
- Fix a bug causing 'helm depndency update' to delete required charts 30a5c6c19f31acbf61591fa7beddbaf514b51a04 (Alon Lavi)
- Added tests for different combinations of subcharts & requirements.yaml ebbf1b002235e588ed0a9f68841476f79d3753b3 (Sushil Kumar)
- Adds charts in "charts\" directory to dependencies 39525ce7cc467423a97f2f1f4590888d081d13e4 (Sushil Kumar)
- Added omitempty to Requirements struct f4a101851278ecf2198ced2f99d9321a889e0cc0 (Sushil Kumar)

<!-- risk-assessed -->
