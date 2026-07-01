---
title: kops v1.35 Release Notes
description: kops v1.35 Release Notes — Kubernetes 生产运维知识库
summary: kops v1.35 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
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
- kops v1.35 Release Notes 是什么
- 如何 kops v1.35 Release Notes
trigger_keywords:
- kops
- v1.35
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- etcd-basics
---



# kops v1.35 Release Notes

Source: [v1.35.0](https://github.com/kubernetes/kops/releases/tag/v1.35.0)

## What's Changed
* Automated cherry pick of #17945: Create iproute2 symlink for kuberouter on older distros by @rifelpet in https://github.com/kubernetes/kops/pull/17946
* Automated cherry pick of #17956: versionbump: go 1.25.7 by @hakman in https://github.com/kubernetes/kops/pull/17957
* Automated cherry pick of #17861: Feature: pull user defined images for warm pool instances by @hakman in https://github.com/kubernetes/kops/pull/17959
* Automated cherry pick of #17980: chore: Add asset hashes for February 2026 releases by @hakman in https://github.com/kubernetes/kops/pull/17981
* Automated cherry pick of #17966: kube-router: bump version v2.5.0 -> 2.7.1 by @hakman in https://github.com/kubernetes/kops/pull/17979
* Automated cherry pick of #17976: drop cdn.dl.k8s.io as a mirror
#17987: drop storage.googleapis.com/k8s-artifacts-cni as a mirror by @hakman in https://github.com/kubernetes/kops/pull/17988
* Automated cherry pick of #18021: chore: Add hashes for additional February releases by @hakman in https://github.com/kubernetes/kops/pull/18022
* Automated cherry pick of #18026: chore: Bump etcd-manager to v3.0.20260227 by @hakman in https://github.com/kubernetes/kops/pull/18027
* Automated cherry pick of #18043: Fix node bootstrap challenge response hashing by @rifelpet in https://github.com/kubernetes/kops/pull/18044
* Automated cherry pick of #18058: chore: Bump Go to v1.25.8 by @hakman in https://github.com/kubernetes/kops/pull/18059
* Release 1.35.0 by @hakman in https://github.com/kubernetes/kops/pull/18090


**Full Changelog**: https://github.com/kubernetes/kops/compare/v1.35.0-beta.1...v1.35.0