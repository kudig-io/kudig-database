---
title: kops v1.7 Release Notes
description: kops v1.7 Release Notes — Kubernetes 生产运维知识库
summary: kops v1.7 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.7 Release Notes 是什么
- 如何 kops v1.7 Release Notes
trigger_keywords:
- kops
- v1.7
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# kops v1.7 Release Notes

Source: [1.7.1](https://github.com/kubernetes/kops/releases/tag/1.7.1)

Contains an important vulnerability fix.

# Significant changes

* Updates kube-dns for CVE-2017-14491.  For more details, please see the [CVE Advisory](https://github.com/kubernetes/kops/blob/master/docs/advisories/cve_2017_14491.md).

# Required Actions

* All users are recommended to upgrade to this version of kops (you need not upgrade your version of [[Kubernetes|kubernetes]] to do so.)  Alternatively, there are simple [manual commands to update kube-dns](https://github.com/kubernetes/kops/blob/master/docs/advisories/cve_2017_14491.md#hotfix-instructions).

# Full Release Notes

See [here](https://github.com/kubernetes/kops/blob/master/docs/releases/1.7.1.md) for the full release notes.
