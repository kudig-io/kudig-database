---
title: helm v3.20 Release Notes
description: helm v3.20 Release Notes — Kubernetes 生产运维知识库
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
- helm v3.20 Release Notes 是什么
- 如何 helm v3.20 Release Notes
trigger_keywords:
- helm
- v3.20
- Release
- Notes
- release
- notes
---

# helm v3.20 Release Notes

Source: [v3.20.1](https://github.com/helm/helm/releases/tag/v3.20.1)

Helm v3.20.1 is a patch release. Users are encouraged to upgrade for the best experience.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [Kubernetes Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [ArtifactHub/packages](https://artifacthub.io/packages/search?kind=0)

## Notable Changes

- Backport of #31644: Fixed a bug where user-provided nil value was not preserved when chart has an empty map or no default for a key 
- Backport of #31601: Fixed a bug where OCI references with tag+digest failed with "invalid byte" error

## Installation and Upgrading

Download Helm v3.20.1. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v3.20.1-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.20.1-darwin-amd64.tar.gz.sha256sum) / 580515b544d5c966edc6f782c9ae88e21a9e10c786a7d6c5fd4b52613f321076)
- [MacOS arm64](https://get.helm.sh/helm-v3.20.1-darwin-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.20.1-darwin-arm64.tar.gz.sha256sum) / 75cc96ac3fe8b8b9928eb051e55698e98d1e026967b6bffe4f0f3c538a551b65)
- [Linux amd64](https://get.helm.sh/helm-v3.20.1-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.20.1-linux-amd64.tar.gz.sha256sum) / 0165ee4a2db012cc657381001e593e981f42aa5707acdd50658326790c9d0dc3)
- [Linux arm](https://get.helm.sh/helm-v3.20.1-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v3.20.1-linux-arm.tar.gz.sha256sum) / 758375df78fb8f91f4056244bda539710a73be79284b24b4bdad68384348ca33)
- [Linux arm64](https://get.helm.sh/helm-v3.20.1-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.20.1-linux-arm64.tar.gz.sha256sum) / 56b9d1b0e0efbb739be6e68a37860ace8ec9c7d3e6424e3b55d4c459bc3a0401)
- [Linux i386](https://get.helm.sh/helm-v3.20.1-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v3.20.1-linux-386.tar.gz.sha256sum) / 22b350307d5e5897b3a14f096cb6b2212cc03c22ba29ab7b4ee3e64ab9f3f190)
- [Linux ppc64le](https://get.helm.sh/helm-v3.20.1-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v3.20.1-linux-ppc64le.tar.gz.sha256sum) / 77b7d9bc62b209c044b873bc773055c5c0d17ef055e54c683f33209ebbe8883c)
- [Linux s390x](https://get.helm.sh/helm-v3.20.1-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v3.20.1-linux-s390x.tar.gz.sha256sum) / 3c43d45149a425c7bf15ba3653ddee13e7b1a4dd6d4534397b6f317f83c51b58)
- [Linux riscv64](https://get.helm.sh/helm-v3.20.1-linux-riscv64.tar.gz) ([checksum](https://get.helm.sh/helm-v3.20.1-linux-riscv64.tar.gz.sha256sum) / 0eeae246112b4780e61651f9fbe6d778eebf8c8eccca590139b97d167d1b8aeb)
- [Windows amd64](https://get.helm.sh/helm-v3.20.1-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v3.20.1-windows-amd64.zip.sha256sum) / 16d5256f4c2cde0745acb922ba88b7759dfced4bf547b99381084211f81c8629)
- [Windows arm64](https://get.helm.sh/helm-v3.20.1-windows-arm64.zip) ([checksum](https://get.helm.sh/helm-v3.20.1-windows-arm64.zip.sha256sum) / 2aac2b87e92c32d44aa81c6412286d9db7e43b22b4c8ac112b68cf69185429bd)

This release was signed with `208D D36E D5BB 3745 A167 43A4 C7C6 FBB5 B91C 1155` and can be found at @scottrigby [keybase account](https://keybase.io/r6by). Please use the attached signatures for verifying this release using `gpg`.

The [Quickstart Guide](https://helm.sh/docs/intro/quickstart/) will get you going from there. For **upgrade instructions** or detailed installation notes, check the [install guide](https://helm.sh/docs/intro/install/). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3) on any system with `bash`.

## What's Next

- 4.2.0 and 3.21.0 are the next minor releases and will be on May 13, 2026
- 4.1.4 and 3.20.2 are the next patch releases and will be on April 8, 2026

## Changelog

- chore(deps): bump the k8s-io group with 7 updates a2369ca71c0ef633bf6e4fccd66d634eb379b371 (dependabot[bot])
- add image index test 90e10564f7ae746a153f3a03006e7061a54ad490 (Pedro Tôrres)
- fix pulling charts from OCI indices 911f2e908ae40b01ca95b857e94b8894043f64fd (Pedro Tôrres)
- Remove refactorring changes from coalesce_test.go 76dad33fb1a2b6451920429b4f5f2dd575ea71bb (Evans Mungai)
- Fix import 45c12f71407b6054a37d3e425d5293ee79a1ab37 (Evans Mungai)
- Update pkg/chart/common/util/coalesce_test.go 26c6f19f967941dbe53bfb5e52d419b3b3e46075 (Evans Mungai)
- Fix lint warning 09f5129d49a14c9336cea6f33adf5f52889915ef (Evans Mungai)
- Preserve nil values in chart already 417deb2b6b7504357b0f580b76f5eed1bb8a5270 (Evans Mungai)
- fix(values): preserve nil values when chart default is empty map 5417bfaa84871feae9c8171f192e2f9796475054 (Evans Mungai)