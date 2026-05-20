---
title: helm v3.2 Release Notes
description: helm v3.2 Release Notes — Kubernetes 生产运维知识库
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
- helm v3.2 Release Notes 是什么
- 如何 helm v3.2 Release Notes
trigger_keywords:
- helm
- v3.2
- Release
- Notes
- release
- notes
---

# helm v3.2 Release Notes

Source: [v3.2.4](https://github.com/helm/helm/releases/tag/v3.2.4)

Helm v3.2.4 is the fourth patch release for v3.2, patching a security vulnerability found in Helm's HTTP plugin installer affecting all versions of Helm 3 prior to Helm 3.2.4. Users are encouraged to upgrade to receive the patch.

More information on the security advisory can be found [on the security advisory page](https://github.com/helm/helm/security/advisories/GHSA-qq3j-xp49-j73f).

The community keeps growing, and we'd love to see you there!

- Join the discussion in [Kubernetes Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [GitHub/helm/charts](https://github.com/helm/charts)

## Installation and Upgrading

Download Helm v3.2.4. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.2.4-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.2.4-darwin-amd64.tar.gz.sha256sum))
- [Linux amd64](https://get.helm.sh/helm-v3.2.4-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.2.4-linux-amd64.tar.gz.sha256sum))
- [Linux arm](https://get.helm.sh/helm-v3.2.4-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.2.4-linux-arm.tar.gz.sha256sum))
- [Linux arm64](https://get.helm.sh/helm-v3.2.4-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.2.4-linux-arm64.tar.gz.sha256sum))
- [Linux i386](https://get.helm.sh/helm-v3.2.4-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.2.4-linux-386.tar.gz.sha256sum))
- [Linux ppc64le](https://get.helm.sh/helm-v3.2.4-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.2.4-linux-ppc64le.tar.gz.sha256sum))
- [Linux s390x](https://get.helm.sh/helm-v3.2.4-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.2.4-linux-s390x.tar.gz.sha256sum))
- [Windows amd64](https://get.helm.sh/helm-v3.2.4-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.2.4-windows-amd64.zip.sha256sum))

This release was signed with `967F 8AC5 E221 6F9F 4FD2 70AD 92AA 783C BAAE 8E3B` and can be found at @bacongobbler's [keybase account](https://keybase.io/bacongobbler). Please use the attached signatures for verifying this release using `gpg`.

The [Quickstart Guide](https://docs.helm.sh/using_helm/#quickstart-guide) will get you going from there. For **upgrade instructions** or detailed installation notes, check the [install guide](https://docs.helm.sh/using_helm/#installing-helm). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/master/scripts/get-helm-3) on any system with `bash`.

## What's Next

- 3.3.0 is the next feature release.

## Changelog

- Improve the extractor and add tests b6bbe4f08bbb98eadd6c9cd726b08a5c639908b3 (Matt Butcher)