---
title: helm v2.12 Release Notes
description: helm v2.12 Release Notes — Kubernetes 生产运维知识库
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
- helm v2.12 Release Notes 是什么
- 如何 helm v2.12 Release Notes
trigger_keywords:
- helm
- v2.12
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
created: "2026-05-23"
---

# [[Helm|helm]] v2.12 Release Notes

Source: [v2.12.3](https://github.com/helm/helm/releases/tag/v2.12.3)

Helm v2.12.3 is a bug fix release. Users are strongly encouraged to upgrade for the best experience.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[entities/kubernetes|Kubernetes]] Slack](https://kubernetes.slack.com):
  - `#helm-users` for questions and just to hang out
  - `#helm-dev` for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [GitHub/helm/charts](https://github.com/helm/charts)

## Installation and Upgrading

Download Helm 2.12.3. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v2.12.3-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v2.12.3-darwin-amd64.tar.gz.sha256))
- Linux amd64](https://get.helm.sh/helm-v2.12.3-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v2.12.3-linux-amd64.tar.gz.sha256))
- [Linux arm](https://get.helm.sh/helm-v2.12.3-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v2.12.3-linux-arm.tar.gz.sha256))
- [Linux arm64](https://get.helm.sh/helm-v2.12.3-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v2.12.3-linux-arm64.tar.gz.sha256))
- [Linux i386](https://get.helm.sh/helm-v2.12.3-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v2.12.3-linux-386.tar.gz.sha256))
- [Linux ppc64le](https://get.helm.sh/helm-v2.12.3-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v2.12.3-linux-ppc64le.tar.gz.sha256))
- [Linux s390x](https://get.helm.sh/helm-v2.12.3-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v2.12.3-linux-s390x.tar.gz.sha256))
- [Windows amd64](https://get.helm.sh/helm-v2.12.3-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v2.12.3-windows-amd64.zip.sha256))

Once you have the client installed, upgrade Tiller with `helm init --upgrade`.

The [Quickstart Guide](https://docs.helm.sh/using_helm/#quickstart-guide) will get you going from there. For **upgrade instructions** or detailed installation notes, check the install guide](https://docs.helm.sh/using_helm/#installing-helm). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/master/scripts/get) on any system with `bash`.

## What's Next

- v2.12.4 will contain only bug fixes.
- v2.13.0 is the next feature release.

## Changelog

- bump version to v2.12.3 eecf22f77df5f65c823aacd2dbd30ae6c65f186e (Matthew Fisher)
- fix: ignore pax header "file"s in chart validation 940400b5a635e9e7e0028786c3d58e3ce2ca4069 (Geoff Baskwill)
- fix: use RFC 1123 subdomains for name verification (#5132) 4268e69a2a7fa69952c02dbc8ad7b77f0bbdc16a (Matthew Fisher)