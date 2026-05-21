---
title: kops v1.12 Release Notes
description: kops v1.12 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
- calico
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.12 Release Notes 是什么
- 如何 kops v1.12 Release Notes
trigger_keywords:
- kops
- v1.12
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cni-basics
- etcd-basics
---

# kops v1.12 Release Notes

Source: [1.12.3](https://github.com/kubernetes/kops/releases/tag/1.12.3)

kops 1.12 supports the kubernetes 1.12.x series (and earlier)

Includes significant changes as part of etcd3 migration, please see the [release notes](https://github.com/kubernetes/kops/blob/master/docs/releases/1.12-NOTES.md)


# Significant changes

* etcd3 migration, please see the [release notes](https://github.com/kubernetes/kops/blob/master/docs/releases/1.12-NOTES.md)
* Upgrades calico/canal for security vulnerability
* Upgrades etcd-manager to 3.0.20190801 - better handles "unexpected" etcd versions
* kops now warns if you try to use a version of etcd that is unsupported by etcd-manager
 
# Required Actions

* Please ensure you are running one of etcd 2.2.1, 3.1.12, 3.2.18 or 3.2.24 before upgrading to etcd-manager.  (If you haven't directly specified a different version, you are running 2.2.1)

* Please ensure you have backed up your etcd data before upgrading.

# Changes between 1.12.2 and 1.12.3


* Cherry pick of #7211: Use NodeAuthorizer config options instead of soely [@jacksontj](https://github.com/jacksontj) [#7232](https://github.com/kubernetes/kops/pull/7232)
* Cherry pick of #7219: Make an actual deep-copy of the state [@jacksontj](https://github.com/jacksontj) [#7235](https://github.com/kubernetes/kops/pull/7235)
* Upgrade Calico to 3.7.2 [@asincu](https://github.com/asincu) [#7051](https://github.com/kubernetes/kops/pull/7051)
* Update canal to 3.6.4, for TTA-2019-002 [@justinsb](https://github.com/justinsb) [#7275](https://github.com/kubernetes/kops/pull/7275)
* Bumping calico to 3.7.4. [@michalschott](https://github.com/michalschott) [#7249](https://github.com/kubernetes/kops/pull/7249)
* Cherry pick of #7185: Replace behavior for aws hostnameOverride [@jacksontj](https://github.com/jacksontj) [#7308](https://github.com/kubernetes/kops/pull/7308)
* Calico -> 3.7.4 for older versions [@justinsb](https://github.com/justinsb) [#7282](https://github.com/kubernetes/kops/pull/7282)
* Bump etcd-manager to 3.0.20190801 [@justinsb](https://github.com/justinsb) [#7349](https://github.com/kubernetes/kops/pull/7349)
* Warn/prevent if the version of etcd is unsupported with etcd-manager [@justinsb](https://github.com/justinsb) [#7340](https://github.com/kubernetes/kops/pull/7340)

# Full details

See [full release notes](https://github.com/kubernetes/kops/blob/master/docs/releases/1.12-NOTES.md)

