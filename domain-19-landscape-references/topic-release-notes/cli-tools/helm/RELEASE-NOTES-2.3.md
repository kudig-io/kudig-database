---
title: helm v2.3 Release Notes
description: helm v2.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- helm v2.3 Release Notes 是什么
- 如何 helm v2.3 Release Notes
trigger_keywords:
- helm
- v2.3
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

# helm v2.3 Release Notes

Source: [v2.3.1](https://github.com/helm/helm/releases/tag/v2.3.1)

This is a _bugfix release_. Users are encouraged to upgrade for the best experience.

This fixes a segmentation fault that could occur when hoisting variables from a subchart into its parent. A few other minor fixes are included as well. It also removes the duplicate commands that were showing up in help text.

Our community is growing rapidly! Jump in!
- Join the discussion in [Kubernetes Slack](https://slack.k8s.io/): `#helm` 
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://engineyard.zoom.us/j/366425549)
- Test, debug, and contribute charts: [GitHub/kubernetes/charts](https://github.com/kubernetes/charts)

## Install or Upgrade

Grab a Helm binary and get started.
- [OSX](https://get.helm.sh/helm-v2.3.1-darwin-amd64.tar.gz)
- [Linux](https://get.helm.sh/helm-v2.3.1-linux-amd64.tar.gz)
- [Windows](https://get.helm.sh/helm-v2.3.1-windows-amd64.zip)

Once you have it unpacked, run `helm init --upgrade` to upgrade the Tiller server.

The [Quickstart Guide](https://github.com/kubernetes/helm/blob/master/docs/quickstart.md) will get you going from there. For **detailed installation notes**, check the [install guide](https://github.com/kubernetes/helm/blob/master/docs/install.md). You can also use a [script to install](https://raw.githubusercontent.com/kubernetes/helm/master/scripts/get) on any system with `bash`.

## Changelog

- fix(tiller): ignore empty YAML documents during delete db8b1f1418665153459c77a3a3d3ef6c47cb01b5 (Matt Butcher)
- fix(helm): fix nil pointer in requirements.go f3f4f0651c5dc5a58e5f8b656408bace3c74e4af (Matt Butcher)
- fix(tiller): increase maximum size of gRPC message ddf4e23280c4fcdbd00738cd8da1355f16b4b6da (Serguei Bezverkhi)
- Fixes hard-coded linux based file-separator 83d15d13a85861b3d3c7a5a8c80e46b38e237f59 (Sushil Kumar)
- fix(helm): remove duplicate commands 33bdcfdce7c3f5e1f8424f2dc7165f39ff03fb38 (Matt Butcher)