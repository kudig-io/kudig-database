---
title: helm v2.7 Release Notes
description: helm v2.7 Release Notes — Kubernetes 生产运维知识库
summary: helm v2.7 Release Notes — Kubernetes 生产运维知识库
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
- helm v2.7 Release Notes 是什么
- 如何 helm v2.7 Release Notes
trigger_keywords:
- helm
- v2.7
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




# [[Helm|helm]] v2.7 Release Notes

Source: [v2.7.2](https://github.com/helm/helm/releases/tag/v2.7.2)

Helm v2.7.2 is a security release and bug release. Users are strongly encouraged
to upgrade.

The Helm Core Maintainers discovered a bug in TLS handling. Versions of Tiller
prior to 2.7.1 had a certificate verification policy that allowed self-signed
certificates to pass the server-side certificate verification phase.

This release contains a fix that *requires* the client certificate to be verified
against Tiller's CA.

Additionally, this release adds documentation for configuring strong [[gRPC|gRPC]] authentication
using TLS. While this feature has been available since 2.3.0, it was not properly
documented.

Finally, this release contains several TLS-related fixes to Helm CLI commands, adding TLS
parameters back to the `helm get *` verbs, and fixing `helm list --tls`.


The community keeps growing, and we'd love to see you there.

- Join the discussion in [[实体/kubernetes.md|Kubernetes]] Slack](https://slack.k8s.io/):
  - `#helm-users` for questions and just to hang out
  - `#helm-dev` for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/4526666954)
- Test, debug, and contribute charts: [GitHub/kubernetes/charts](https://github.com/kubernetes/charts)

## Breaking Changes

This release places much more stringent requirements on certificate auth. It is
possible that some misconfigured Helm SSL configurations that were working are now
broken.

## Installation and Upgrading

Download Helm 2.7.2. The common platform binaries are here:

- [OSX](https://get.helm.sh/helm-v2.7.2-darwin-amd64.tar.gz)
- [Linux](https://get.helm.sh/helm-v2.7.2-linux-amd64.tar.gz)
- [Windows](https://get.helm.sh/helm-v2.7.2-windows-amd64.tar.gz)

Once you have the client installed, upgrade Tiller with `helm init --upgrade`.

The [Quickstart Guide](https://docs.helm.sh/using_helm/#quickstart-guide) will get you going from there. For **upgrade instructions** or detailed installation notes, check the [install guide](https://docs.helm.sh/using_helm/#installing-helm). You can also use a [script to install](https://raw.githubusercontent.com/kubernetes/helm/master/scripts/get) on any system with `bash`.

## Changelog

- fix(helm): fix missing ssl params (#3152) e8e6ac5d7783808cc0bd1adad053bec339849647 (Matt Butcher)

<!-- risk-assessed -->
