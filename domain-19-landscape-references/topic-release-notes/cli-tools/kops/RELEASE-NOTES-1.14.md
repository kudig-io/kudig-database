---
title: kops v1.14 Release Notes
description: kops v1.14 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
- coredns
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.14 Release Notes 是什么
- 如何 kops v1.14 Release Notes
trigger_keywords:
- kops
- v1.14
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- etcd-basics
---

# kops v1.14 Release Notes

Source: [1.14.1](https://github.com/kubernetes/kops/releases/tag/1.14.1)

Release in 1.14 series of kops, supporting kubernetes 1.14 and earlier.

Please see the [release notes](https://github.com/kubernetes/kops/blob/master/docs/releases/1.14-NOTES.md) for the full list of changes. 

For existing clusters, please update to kubernetes 1.12 before updating to kubernetes 1.13 and 1.14.  Technically kubernetes upgrades can only be done one minor version at a time, but this is particularly important because of the etcd-upgrade that is in kops 1.12.


## Significant Changes

* This release fixes a bug where coredns updates would no longer be applied.

## 1.14.0 to 1.14.1

* fix(upup/models/cloudup/resources/addons/coredns.addons.k8s.io) missing resourceVersion [@phspagiari](https://github.com/phspagiari) [#7477](https://github.com/kubernetes/kops/pull/7477)

