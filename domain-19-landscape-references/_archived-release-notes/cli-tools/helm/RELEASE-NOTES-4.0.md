---
title: helm v4.0 Release Notes
description: helm v4.0 Release Notes — Kubernetes 生产运维知识库
summary: helm v4.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- helm v4.0 Release Notes 是什么
- 如何 helm v4.0 Release Notes
trigger_keywords:
- helm
- v4.0
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---



# [[Helm|helm]] v4.0 Release Notes

Source: [v4.0.5](https://github.com/helm/helm/releases/tag/v4.0.5)

Helm v4.0.5 is a patch release. Users are encouraged to upgrade for the best experience.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [[entities/kubernetes.md|Kubernetes]] Slack](https://kubernetes.slack.com):
  -  for questions and just to hang out
  -  for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [ArtifactHub/packages](https://artifacthub.io/packages/search?kind=0)

## Notable Changes

- Fixed bug where helm uninstall with --keep-history did not suspend previous deployed releases #12556
- Fixed rollback error when a manifest is removed in a failed upgrade #13437 
- Fixed check to ensure CLI plugin does not load with the same name as an existing Helm command
- Fixed helm test --logs failure with hook-delete-policy "hook-failed" or "hook-succeed" #9098
- Fixed a bug where empty dependency lists were incorrectly treated as present
- Fixed a bug where the watch library did not only watch namespaces associated with the objects
- Fixed regression in downloader plugins environment variables #31612
- Fixed bug where --server-side flag is not respected with helm upgrade --install #31627
- For SDK users: exposed KUBECONFIG to env

## Installation and Upgrading

Download Helm v4.0.5. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v4.0.5-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v4.0.5-darwin-amd64.tar.gz.sha256sum) / 270d906140eadbe95584d2cebae1fa0e46950027d82de0c4db937dc936b564a6)
- [MacOS arm64](https://get.helm.sh/helm-v4.0.5-darwin-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v4.0.5-darwin-arm64.tar.gz.sha256sum) / b4d04ccf68004604e13878fce4a893711490914512f8759879f848136a9f5fca)
- Linux amd64](https://get.helm.sh/helm-v4.0.5-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v4.0.5-linux-amd64.tar.gz.sha256sum) / 730e4e9fbff94168249ddd0b9b1b8c357b7f64815462dd88c6b39f09bf18b814)
- [Linux arm](https://get.helm.sh/helm-v4.0.5-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v4.0.5-linux-arm.tar.gz.sha256sum) / 812668560e720248dc166226c94b8280d54168711eeb4b9d5eb991ed5a30ba36)
- [Linux arm64](https://get.helm.sh/helm-v4.0.5-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v4.0.5-linux-arm64.tar.gz.sha256sum) / 206a7747702d13994a93629eaed4259bd9d0aec6e23ca52d640f47f7edfdc863)
- [Linux i386](https://get.helm.sh/helm-v4.0.5-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v4.0.5-linux-386.tar.gz.sha256sum) / fd0e09cb0468984ab21fa78f7c214408624277face6555b010179494a555c02d)
- [Linux loong64](https://get.helm.sh/helm-v4.0.5-linux-loong64.tar.gz) ([checksum](https://get.helm.sh/helm-v4.0.5-linux-loong64.tar.gz.sha256sum) / ﻿<?xml version="1.0" encoding="utf-8"?><Error><Code>BlobNotFound</Code><Message>The specified blob does not exist.
RequestId:258042f6-701e-0026-72aa-85bd2c000000
Time:2026-01-14T23:09:05.0367123Z</Message></Error>)
- [Linux ppc64le](https://get.helm.sh/helm-v4.0.5-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v4.0.5-linux-ppc64le.tar.gz.sha256sum) / c1e0d6e781a574cebfb8b0b77885523165f4088e23ff7e6cecf69d5f5fbfb89b)
- [Linux s390x](https://get.helm.sh/helm-v4.0.5-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v4.0.5-linux-s390x.tar.gz.sha256sum) / 9d4210cfccb9fa17e6aac9fb9c13d24836dab5859a542a230d14d129ed55e6a2)
- [Linux riscv64](https://get.helm.sh/helm-v4.0.5-linux-riscv64.tar.gz) ([checksum](https://get.helm.sh/helm-v4.0.5-linux-riscv64.tar.gz.sha256sum) / 7f55782db0b2f16d7c9bc134eee521d0fce321ae8dbe1f1884f82160486362e2)
- [Windows amd64](https://get.helm.sh/helm-v4.0.5-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v4.0.5-windows-amd64.zip.sha256sum) / 44035d49d9e7d93c249cef0a056f464c05c8638d074e651d586481e4d4d0208b)
- [Windows arm64](https://get.helm.sh/helm-v4.0.5-windows-arm64.zip) ([checksum](https://get.helm.sh/helm-v4.0.5-windows-arm64.zip.sha256sum) / 45efc334dd5a2e9cae4aed333e9d638baec80c4cedd11a7669fe215991a69eb2)

This release was signed with `208D D36E D5BB 3745 A167 43A4 C7C6 FBB5 B91C 1155` and can be found at @scottrigby [keybase account](https://keybase.io/r6by). Please use the attached signatures for verifying this release using `gpg`.

The [Quickstart Guide](https://helm.sh/docs/intro/quickstart/) will get you going from there. For **upgrade instructions** or detailed installation notes, check the [install guide](https://helm.sh/docs/intro/install/). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3) on any system with `bash`.

## What's Next

- 4.1.0 and 3.20.0 is the next minor releases and will be on January 21, 2026
- 4.1.1 and 3.20.1 are the next patch releases and will be on March 11, 2026

## Changelog

- fix(upgrade): pass --server-side flag to install when using upgrade --install 1b6053d48b51673c5581973f5ae7e104f627fcf5 (Evans Mungai)
- fix(cli): handle nil config in EnvSettings.Namespace() 1e3ee1d2ba5a421165fe053a41aa4071cf69ed62 (Zadkiel AHARONIAN)
- fix(getter): pass settings environment variables 31bd995ce201e295ff2f87e11fdf13bf55fdffd2 (Zadkiel AHARONIAN)
- test(statuswait): fix Copilot code review suggestion for goroutine in tests 41a6b364a1433dabcefca945619b55a43046f6ce (Mohsen Mottaghi)
- test(statuswait): add more tests suggested by Copilot code review 2a2e6f722a95a9c312d9f4133e98a67ceaaa5054 (Mohsen Mottaghi)
- test(statuswait): add some tests for statuswait 3818c02f81cdaf2a0eb9e0082d1f89f2cff9df08 (Mohsen Mottaghi)
- fix: use namespace-scoped watching to avoid cluster-wide LIST permissions 66cab24bb95f68448a83304c6d3297ec45c8fdb7 (Mohsen Mottaghi)
- Use length check for MetaDependencies instead of nil comparison abf2007685501002a5051ded95de9090b5c530f3 (Calvin Bui)
- Deal with golint warning with private executeShutdownFunc 4b3de18628896dc2924643e4a32ab4b9228eb922 (Benoit Tigeot)
- Code review 321277002a2513d841cc81480dff37985f28e41a (Benoit Tigeot)
- Fix linting issue 417aae9c01a79c01f3de8c2a08079fddada6078e (Benoit Tigeot)
- Update pkg/action/hooks.go 6c838b4a18fa345cce5a22367522058d3d2ebcc2 (Michelle Fernandez Bieber)
- added check for nil shutdown c5d87f2f7da0f3d90bd69d71daa70ada29c9d5b9 (Michelle Fernandez Bieber)
- cleaned up empty line 53175b76c784a75610c66c36a17787fc463a242d (Michelle Fernandez Bieber)
- updated comment and made defer of shutdown function return errors as before and not the possible shutdown error d2df1ab59e235c2a2edf27bbfdc72ccc09b3af2c (Michelle Fernandez Bieber)
- added shutdown hook that is executed after the logs have been retrieved 5b223de7c809fe026f1fff03d1fa684187bdfcbe (Michelle Fernandez Bieber)
- Fix TestCliPluginExitCode e845b68fe3ea0c91f429b384e3b78f3f853ae208 (tison)
- Check plugin name is not used 30bfd57c5f677b12ea88a32b5e66735967570e4e (tison)
- Fix rollback for missing resources 0fd2c418b0146c1ea6182c7f7c7676a345554156 (Feruzjon Muyassarov)
- fix: assign KUBECONFIG environment variable value to env.Kubeconfig b456e274652c84316d5bfcf06d6a8b648e3cc23b (LinPr)
- fix(rollback): `errors.Is` instead of string comp e2021f8818d2cf20c118d91fdba8fba5c472c3bb (Hidde Beydals)
- fix(uninstall): supersede deployed releases af7c15303ace3c506c5c790c41186a09df1c8a54 (Hidde Beydals)