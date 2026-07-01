---
title: helm v3.3 Release Notes
description: helm v3.3 Release Notes — Kubernetes 生产运维知识库
summary: helm v3.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- helm v3.3 Release Notes 是什么
- 如何 helm v3.3 Release Notes
trigger_keywords:
- helm
- v3.3
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---



# [[Helm|helm]] v3.3 Release Notes

Source: [v3.3.4](https://github.com/helm/helm/releases/tag/v3.3.4)

Helm v3.3.4 is a bugfix release that fixes several regressions introduced in Helm 3.3.2.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[entities/kubernetes.md|Kubernetes]] Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [GitHub/helm/charts](https://github.com/helm/charts)

## Installation and Upgrading

Download Helm v3.3.4. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.3.4-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.3.4-darwin-amd64.tar.gz.sha256sum) / 9fffc847c61da0e06319788d3998ea173eb86c1cc5600ac3ada8d0d40c911793)
- Linux amd64](https://get.helm.sh/helm-v3.3.4-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.3.4-linux-amd64.tar.gz.sha256sum) / b664632683c36446deeb85c406871590d879491e3de18978b426769e43a1e82c)
- [Linux arm](https://get.helm.sh/helm-v3.3.4-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.3.4-linux-arm.tar.gz.sha256sum) / 9da6cc39a796f85b6c4e6d48fd8e4888f1003bfb7a193bb6c427cdd752ad40bb)
- [Linux arm64](https://get.helm.sh/helm-v3.3.4-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.3.4-linux-arm64.tar.gz.sha256sum) / bdd00b8ff422171b4be5b649a42e5261394a89d7ea57944005fc34d34d1f8160)
- [Linux i386](https://get.helm.sh/helm-v3.3.4-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.3.4-linux-386.tar.gz.sha256sum) / 2c14d4d944c94f4487fa15ae99d974304554850a8decd726419e6a8cb0f9038c)
- [Linux ppc64le](https://get.helm.sh/helm-v3.3.4-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.3.4-linux-ppc64le.tar.gz.sha256sum) / fed9553cd7459f0c37dc99b8566bd397b6893e48dc63f154e63eb8919179b99a)
- [Linux s390x](https://get.helm.sh/helm-v3.3.4-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.3.4-linux-s390x.tar.gz.sha256sum) / 9fffc847c61da0e06319788d3998ea173eb86c1cc5600ac3ada8d0d40c911793)
- [Windows amd64](https://get.helm.sh/helm-v3.3.4-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.3.4-windows-amd64.zip.sha256sum) / 001f38788ed7ecfe336881b991d46bfd73596380185dc70557a1e352f27c0b22)

The [Quickstart Guide](https://docs.helm.sh/using_helm/#quickstart-guide) will get you going from there. For **upgrade instructions** or detailed installation notes, check the install guide](https://docs.helm.sh/using_helm/#installing-helm). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/master/scripts/get-helm-3) on any system with `bash`.

## What's Next

- 3.3.5 will contain only bug fixes.
- 3.4.0 is the next feature release.

## Changelog

- Fixing import package issue a61ce5633af99708171414353ed49547cf05013d (Matt Farina)
- use warning function e5bd79faefedbbaf4a6a5f7faaa13eba4ddc55aa (Matthew Fisher)
- Fixing issue with idempotent repo add 520416adf0723321101235780f86245c3a714c3c (Matt Farina)