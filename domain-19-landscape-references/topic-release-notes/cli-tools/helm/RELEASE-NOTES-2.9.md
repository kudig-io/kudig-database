---
title: helm v2.9 Release Notes
description: helm v2.9 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- rbac
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- helm v2.9 Release Notes 是什么
- 如何 helm v2.9 Release Notes
trigger_keywords:
- helm
- v2.9
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

# helm v2.9 Release Notes

Source: [v2.9.1](https://github.com/helm/helm/releases/tag/v2.9.1)

Helm v2.9.1 is a bug fix release. Users are strongly encouraged to upgrade for the best experience.

The community keeps growing, and we'd love to see you there.

- Join the discussion in [Kubernetes Slack](https://slack.k8s.io/):
  - `#helm-users` for questions and just to hang out
  - `#helm-dev` for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/4526666954)
- Test, debug, and contribute charts: [GitHub/kubernetes/charts](https://github.com/kubernetes/charts)

## Installation and Upgrading

Download Helm v2.9.1. The common platform binaries are here:

- [OSX](https://get.helm.sh/helm-v2.9.1-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v2.9.1-darwin-amd64.tar.gz.sha256))
- [Linux](https://get.helm.sh/helm-v2.9.1-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v2.9.1-linux-amd64.tar.gz.sha256))
- [Windows](https://get.helm.sh/helm-v2.9.1-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v2.9.1-windows-amd64.zip.sha256))

Once you have the client installed, upgrade Tiller with `helm init --upgrade`.

The [Quickstart Guide](https://docs.helm.sh/using_helm/#quickstart-guide) will get you going from there. For **upgrade instructions** or detailed installation notes, check the [install guide](https://docs.helm.sh/using_helm/#installing-helm). You can also use a [script to install](https://raw.githubusercontent.com/kubernetes/helm/master/scripts/get) on any system with `bash`.

## Changelog

- bump version to v2.9.1 20adb27c7c5868466912eebdf6664e7390ebe710 (Matthew Fisher)
- Revert "toYaml - Fix #3470 and #3410's trailing \n issues" a00bcc297914fe2f9e7eadab45ea34d1d99f8e87 (Matthew Fisher)
- Revert "Fix tiller deployment on RBAC clusters" c6e7f0335bc083aa298127c5e4d72a28a6822f3f (Matthew Fisher)
