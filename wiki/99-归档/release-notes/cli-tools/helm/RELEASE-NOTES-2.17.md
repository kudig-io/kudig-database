---
title: helm v2.17 Release Notes
description: helm v2.17 Release Notes — Kubernetes 生产运维知识库
summary: helm v2.17 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- docker
- job
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
- helm v2.17 Release Notes 是什么
- 如何 helm v2.17 Release Notes
trigger_keywords:
- helm
- v2.17
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




# [[Helm|helm]] v2.17 Release Notes

Source: [v2.17.0](https://github.com/helm/helm/releases/tag/v2.17.0)

Helm v2.17.0 is a feature release of Helm v2. The focus of this release is the End of Life for Helm v2 support and the deprecation of the stable and incubator Helm chart repositories. The chart repositories are moving to a new location that will serve as a long term archive.

## Notable Changes

- A flag was introduced on `helm init` to skip adding the stable and local chart repositories.
- The stable chart repository is set to use a new location, by default. It can be overridden.
- The old stable and incubator repository locations will be detected in Helm configuration and you will be warned to update them.
- Tiller has a new default location at ghcr.io/helm/tiller. You can still use the previous location in GCR.

## Installation and Upgrading

Download Helm 2.17. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v2.17.0-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v2.17.0-darwin-amd64.tar.gz.sha256) / `104dcda352985306d04d5d23aaf5252d00a85c083f3667fd013991d82f57ae83`)
- Linux amd64](https://get.helm.sh/helm-v2.17.0-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v2.17.0-linux-amd64.tar.gz.sha256) / `f3bec3c7c55f6a9eb9e6586b8c503f370af92fe987fcbf741f37707606d70296`)
- [Linux arm](https://get.helm.sh/helm-v2.17.0-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v2.17.0-linux-arm.tar.gz.sha256) / `bf972150ba0b950119a3fe7ac9ed19d467c703fa552ba4ac79a0ad7f1f9e70c4`)
- [Linux arm64](https://get.helm.sh/helm-v2.17.0-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v2.17.0-linux-arm64.tar.gz.sha256) / `c3ebe8fa04b4e235eb7a9ab030a98d3002f93ecb842f0a8741f98383a9493d7f`)
- [Linux i386](https://get.helm.sh/helm-v2.17.0-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v2.17.0-linux-386.tar.gz.sha256) / `843a0b92441a932be005f4c692f920b24014403c294856a396509c42c90f78aa`)
- [Linux ppc64le](https://get.helm.sh/helm-v2.17.0-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v2.17.0-linux-ppc64le.tar.gz.sha256) / `4e4df66130bb46ab4aa21e01b24f93276dbd6ee6cf4f03218a328f1a79f5cb34`)
- [Linux s390x](https://get.helm.sh/helm-v2.17.0-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v2.17.0-linux-s390x.tar.gz.sha256) / `0a77b6e70a549a09f044ad8abc96eb125338d9e189749e9b97315cec6a519346`)
- [Windows amd64](https://get.helm.sh/helm-v2.17.0-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v2.17.0-windows-amd64.zip.sha256) / `048147ef523f88753ba34170f2f6acd01ac6ec688c6f5973b0e5ffb0b113a232`)

This release was signed with `672C 657B E06B 4B30 969C 4A57 4614 49C2 5E36 B98E ` and can be found at @mattfarina [keybase account](https://keybase.io/mattfarina). Please use the attached signatures for verifying this release using `gpg`.

Once you have the client installed, upgrade Tiller with `helm init --upgrade`.

The [Quickstart Guide](https://docs.helm.sh/using_helm/#quickstart-guide) will get you going from there. For **upgrade instructions** or detailed installation notes, check the install guide](https://docs.helm.sh/using_helm/#installing-helm). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/master/scripts/get) on any system with `bash`.

## What's Next

If a security issue is found in Helm v2.17.0 prior to the end of life a security fix will be released. On November 13, 2020 Helm v2 will reach the end of support. Please upgrade to Helm v3.

## Changelog

- bump version to 2.17 a690bad98af45b015bd3da1a41f6218b1a451dbe (Matt Farina)
- Change default repositories for Helm v2 (#8901) 62d6e4076bf41f90fead1c48a63f67c1915830da (Matt Butcher)
- Re-instating quay Tiller push 11783477f0559310c7dc22bbf32011fa38856593 (Matt Farina)
- Disabling pushing Tiller to quay in CI 905375a8794b06c991384a27371f3cdd46a7b345 (Matt Farina)
- add Docker authentication to CircleCI jobs 911e061883b27ae092ba8529f118fe15136aff3e (Matthew Fisher)
- Moving Tiller to new location 01dc62ecfdaa6df54c9a3c03370ac4d6dabdcd0b (Matt Farina)
- Fix for issue 8761 d46f7bc2ca9b160e9a7ddf51f56be3a77959ee1c (Martin Hickey)
- fix formatting error (#8758) b9566b8799f3981fe68d543fb3a6c2cb0dc0c3dc (Matt Butcher)
- fix: use yaml annotations for yaml.v2 validation a9d1204edd6bb87ad9209694297798bcbd3b4e59 (Matthew Fisher)
- backported fixes from helm3 7c287078c1505fbe662fcb77fcb2b873b01d501f (Matt Butcher)
- Fixing broken vanity URL in dependencies d154b05921fc7c3a1c5074bcb0db184be0820154 (Matt Farina)
- validate plugin metadata before loading b0296c0522e837d65f944beefa3fb64fd08ac304 (Matthew Fisher)
- also don't need the values.yaml in testdata 242bd58066b1d325c5624f58d4ee98113fc443e2 (Jeff Knurek)
- remove unrequired files in testdata 77957fc76492b0b2f4f5cbc3116a37d710078721 (Jeff Knurek)
- the last revert commit still was making an undesired change 5c9ff04da4bc5b685977e761f527a13832a65654 (Jeff Knurek)
- revert the removal of strict param in functions for better backwards compatibility cd90fb4319c0c4483823e444deabe67a88892904 (Jeff Knurek)
- Update Debian repo signing key location e03f1ae502224b0dd2443acba8c08a3e2e8fb9e9 (Matthew Fox)
- fix: removed strict template errors from v2 linter a979ba8c587b4b0511b1822f3f1aa521755b864d (Jeff Knurek)
- [v2] fix stack overflow error in helm template. (#7185) 0b31450452666da00e55c41699b5c320171e9748 (zwwhdls)
- Updating the install instructions e52dbc2c2b31f3d095dddae67e6a2421df88484a (Matt Farina)
- Adding init flad to skip adding repos a03d3e34ac4fc9f31d77647c6273f6b7da35d145 (Matt Farina)
- Updated test cases 416c97d221adc1d39d69fe2c486cb1aec434357b (Stanton Xu)
- Update after review 059aeed8c73b3c026f6b1ff9125304afb23b92bb (Martin Hickey)
- Add more informative error message for removed k8s APIs a90182e3b2dba2172a7ee0a2680de0c7b09ec4ed (Martin Hickey)
- Adding --devel to helm inspect ebbb29eca655c0a0d9dbfdbb970a6561833c06df (Bridget Kromhout)
- backported archive improvements from v3 (#8318) def975f556a8d32307306be119a5f3b0411224d9 (Matt Butcher)
- Consider namespace when comparing resources 0c96138afa423b7d145c310bd1032c5348bd6dba (Fabian Ruff)
- fix(ci): use go 1.14 (#8288) 7606f0879c9eef980e652bd74842c6dcf1ee28a7 (Adam Reese)
- Pull review comments from v3 doc 9247be3b786d417f4ca76bdc168f08f7933cdf6e (Martin Hickey)
- Update review comment 46807c6729708973b1115242297fc702334eb1b4 (Martin Hickey)
- Add deprecated Kubernetes API doc 5152c165179c672778f19b6f97a37c405eaf64c6 (Martin Hickey)
- Adding a counter to the dependencies cache 453ded0f2f9dd32674bef3d4accf8baacf0d87ac (Matt Farina)
- Add Apt (Deb/Ubuntu) installation instructions for Helm 2 628541e0976754a53492701899761460cbc83d01 (Matthew Fox)
- fix(Makefile): disable go modules b6771ab6c4f297cb26092ff3fe507ae7b55e9d79 (Matthew Fisher)
- fix(tiller): Avoid corrupting storage via a lock c32c9a510bce24278ff5c17cc8401e0ff5c32042 (Cristian Klein)

<!-- risk-assessed -->
