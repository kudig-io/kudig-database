---
title: helm v3.14 Release Notes
description: helm v3.14 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- docker
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- helm v3.14 Release Notes 是什么
- 如何 helm v3.14 Release Notes
trigger_keywords:
- helm
- v3.14
- Release
- Notes
- release
- notes
---

# helm v3.14 Release Notes

Source: [v3.14.4](https://github.com/helm/helm/releases/tag/v3.14.4)

Helm v3.14.4 is a patch release. Users are encouraged to upgrade for the best experience. Users are encouraged to upgrade for the best experience.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [Kubernetes Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [ArtifactHub/packages](https://artifacthub.io/packages/search?kind=0)

## Installation and Upgrading

Download Helm v3.14.4. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.14.4-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.14.4-darwin-amd64.tar.gz.sha256sum) / 73434aeac36ad068ce2e5582b8851a286dc628eae16494a26e2ad0b24a7199f9)
- [MacOS arm64](https://get.helm.sh/helm-v3.14.4-darwin-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.14.4-darwin-arm64.tar.gz.sha256sum) / 61e9c5455f06b2ad0a1280975bf65892e707adc19d766b0cf4e9006e3b7b4b6c)
- [Linux amd64](https://get.helm.sh/helm-v3.14.4-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.14.4-linux-amd64.tar.gz.sha256sum) / a5844ef2c38ef6ddf3b5a8f7d91e7e0e8ebc39a38bb3fc8013d629c1ef29c259)
- [Linux arm](https://get.helm.sh/helm-v3.14.4-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.14.4-linux-arm.tar.gz.sha256sum) / 962297c944c06e1f275111a6e3d80e37c9e9e8fed967d4abec8efcf7fc9fb260)
- [Linux arm64](https://get.helm.sh/helm-v3.14.4-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.14.4-linux-arm64.tar.gz.sha256sum) / 113ccc53b7c57c2aba0cd0aa560b5500841b18b5210d78641acfddc53dac8ab2)
- [Linux i386](https://get.helm.sh/helm-v3.14.4-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.14.4-linux-386.tar.gz.sha256sum) / 2cb3ff032be1c39b7199b324d58d0ae05bc4fe49b9be6aa2fcbeb3fc03f1f9e7)
- [Linux ppc64le](https://get.helm.sh/helm-v3.14.4-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.14.4-linux-ppc64le.tar.gz.sha256sum) / d0d625b43f6650ad376428520b2238baa2400bfedb43b2e0f24ad7247f0f59b5)
- [Linux s390x](https://get.helm.sh/helm-v3.14.4-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.14.4-linux-s390x.tar.gz.sha256sum) / a5750d0cb1ba34ce84ab3be6382a14617130661d15dd2aa1b36630b293437936)
- [Linux riscv64](https://get.helm.sh/helm-v3.14.4-linux-riscv64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.14.4-linux-riscv64.tar.gz.sha256sum) / 925782b159392d52df5ebab88e04e695217325894c6a32a9a779e865b7e32411)
- [Windows amd64](https://get.helm.sh/helm-v3.14.4-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.14.4-windows-amd64.zip.sha256sum) / 0b951db3eadd92dfe336b5a9ddb0640e5cd70d39abdbd7d3125e9fb59b22b669)

This release was signed with `672C 657B E06B 4B30 969C 4A57 4614 49C2 5E36 B98E ` and can be found at @mattfarina [keybase account](https://keybase.io/mattfarina). Please use the attached signatures for verifying this release using `gpg`.

The [Quickstart Guide](https://helm.sh/docs/intro/quickstart/) will get you going from there. For **upgrade instructions** or detailed installation notes, check the [install guide](https://helm.sh/docs/intro/install/). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3) on any system with `bash`.

## What's Next

- 3.15.0 is the next feature release and will be on May 08, 2024.

## Changelog

- refactor: create a helper for checking if a release is uninstalled 81c902a123462fd4052bc5e9aa9c513c4c8fc142 (Alex Petrov)
- fix: reinstall previously uninstalled chart with --keep-history 5a11c768386dab08ff026fb1001e592ab0a033f8 (Alex Petrov)
- chore: remove repetitive words fb3d8805f017d898f9e88667829c21874a8f6166 (deterclosed)
- chore(deps): bump google.golang.org/protobuf from 1.31.0 to 1.33.0 01ac4a2c36d49e691982f85f4243fe449876fb5d (dependabot[bot])
- chore(deps): bump github.com/docker/docker 138602da27a6ba67564d298e7b07f5624a341b88 (dependabot[bot])
- bug: add proxy support for oci getter aa7d95333d5fbc1ea9ed20cc56f011c068e004be (Ricardo Maraschini)
