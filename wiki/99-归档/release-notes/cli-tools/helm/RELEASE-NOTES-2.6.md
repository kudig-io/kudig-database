---
title: helm v2.6 Release Notes
description: helm v2.6 Release Notes — Kubernetes 生产运维知识库
summary: helm v2.6 Release Notes — Kubernetes 生产运维知识库
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
- helm v2.6 Release Notes 是什么
- 如何 helm v2.6 Release Notes
trigger_keywords:
- helm
- v2.6
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




# [[Helm|helm]] v2.6 Release Notes

Source: [v2.6.2](https://github.com/helm/helm/releases/tag/v2.6.2)

Helm 2.6.2 includes a handful of bug fixes. It is a patch release, and users are strongly encouraged to upgrade.

It is also important to note that this patch release removes a few features from Helm due to licensing issues, most notably #1639 and #2449. These new features relied on libraries licensed under [Facebook's BSD+Patents license](https://code.facebook.com/pages/850928938376556) (also known as a [Category-X license](https://www.apache.org/legal/resolved.html#category-x)). The Facebook BSD+Patents license includes a specification of a PATENTS file that passes along some risk to users of the Helm project. The terms of Facebook BSD+Patents license are not a subset of those found in Apache v2, and therefore cannot be sublicensed as Apache v2 libraries. We are very sorry for breaking [our policy on semantic versioning](https://github.com/kubernetes/helm/blob/master/CONTRIBUTING.md#semver), but in this specific case it is to protect the interests of all users of the Helm project.

Our community is one of the fastest growing [[Kubernetes|Kubernetes]] projects. We'd love to have you as a part!

- Join the discussion in [Kubernetes Slack](https://slack.k8s.io/):
  - `#helm-users` for questions and just to hang out
  - `#helm-devs` for discussing PRs, code, and bugs 
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/4526666954)
- Test, debug, and contribute charts: [GitHub/kubernetes/charts](https://github.com/kubernetes/charts)

## Installation and Upgrading

Download Helm 2.6.2. The common platform binaries are here:

- [OSX](https://get.helm.sh/helm-v2.6.2-darwin-amd64.tar.gz)
- Linux](https://get.helm.sh/helm-v2.6.2-linux-amd64.tar.gz)
- [Windows](https://get.helm.sh/helm-v2.6.2-windows-amd64.zip)

Once you have the client installed, upgrade Tiller with `helm init --upgrade`.

The [Quickstart Guide](https://docs.helm.sh/using_helm/#quickstart-guide) will get you going from there. For **upgrade instructions** or detailed installation notes, check the install guide](https://docs.helm.sh/using_helm/#installing-helm). You can also use a [script to install](https://raw.githubusercontent.com/kubernetes/helm/master/scripts/get) on any system with `bash`.

## What's Next

- 2.6.3 is the next patch release. This release will only happen if important bugs surface between now and the 2.7.0 release date.
- 2.7.0 is the next major release, and is planned to include Kubernetes 1.8 support.

## Changelog

- fix(deps): fix issues when running glide up be3ae4ea91b2960be98c07e8f73754e67e87963c (Matt Farina)
- Load StorageClass before PersistentVolume is loaded 1a13307472db49baccf9729e60179e6bbb9ae2c9 (刘相轩)
- fix(helm): Fix the bug in helm dependency update -verify 7ed614d06d1f6b6ad243c04f9e36ae79ed8f74f0 (@rocky-nupt)
- bug(tiller): sort unknown but different kinds alphabetically based on kind name 685994aecf973e080660c76bb964b72f452f4f36 (Justin Scott)
- fix(helm):Fix dependency aliaes not working 5f1defd07255b98760be568cc380ab0b4e90db18 (@llsheldon)
- Make Memory driver to store copy of releases to stop hiding storage errors during tests 0f18f35d7fe576185e99ca8f9de833a3de7e264a (Maxim Ivanov)
- Correctly persists Release upgrade failure 9b900ad823ec16584008950043e3295e4abc4b96 (Maxim Ivanov)
- remove references to facebookgo/symwalk and facebookgo/atomicfile 821100e ce1ba29 e23f660 (Matt Fisher)

<!-- risk-assessed -->
