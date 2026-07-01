---
title: helm v3.1 Release Notes
description: helm v3.1 Release Notes — Kubernetes 生产运维知识库
summary: helm v3.1 Release Notes — Kubernetes 生产运维知识库
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
- helm v3.1 Release Notes 是什么
- 如何 helm v3.1 Release Notes
trigger_keywords:
- helm
- v3.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---



# [[Helm|helm]] v3.1 Release Notes

Source: [v3.1.3](https://github.com/helm/helm/releases/tag/v3.1.3)

Helm v3.1.3 is the third patch release for Helm 3.1 and it is a security release. Users are encouraged to upgrade.

This release was signed with `672C 657B E06B 4B30 969C 4A57 4614 49C2 5E36 B98E` and can be found at @mattfarina [keybase account](https://keybase.io/mattfarina). Please use the attached signatures for verifying this release using `gpg`.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[entities/kubernetes.md|Kubernetes]] Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [GitHub/helm/charts](https://github.com/helm/charts)

## Installation and Upgrading

Download Helm v3.1.3. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.1.3-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.1.3-darwin-amd64.tar.gz.sha256sum))
- Linux amd64](https://get.helm.sh/helm-v3.1.3-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.1.3-linux-amd64.tar.gz.sha256sum))
- [Linux arm](https://get.helm.sh/helm-v3.1.3-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.1.3-linux-arm.tar.gz.sha256sum))
- [Linux arm64](https://get.helm.sh/helm-v3.1.3-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.1.3-linux-arm64.tar.gz.sha256sum))
- [Linux i386](https://get.helm.sh/helm-v3.1.3-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.1.3-linux-386.tar.gz.sha256sum))
- [Linux ppc64le](https://get.helm.sh/helm-v3.1.3-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.1.3-linux-ppc64le.tar.gz.sha256sum))
- [Linux s390x](https://get.helm.sh/helm-v3.1.3-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.1.3-linux-s390x.tar.gz.sha256sum))
- [Windows amd64](https://get.helm.sh/helm-v3.1.3-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.1.3-windows-amd64.zip.sha256sum))

The [Quickstart Guide](https://docs.helm.sh/using_helm/#quickstart-guide) will get you going from there. For **upgrade instructions** or detailed installation notes, check the install guide](https://docs.helm.sh/using_helm/#installing-helm). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/master/scripts/get-helm-3) on any system with `bash`.