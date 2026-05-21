---
title: kops v1.23 Release Notes
description: kops v1.23 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- controller-manager
- cilium
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.23 Release Notes 是什么
- 如何 kops v1.23 Release Notes
trigger_keywords:
- kops
- v1.23
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cilium-basics
---

# kops v1.23 Release Notes

Source: [v1.23.4](https://github.com/kubernetes/kops/releases/tag/v1.23.4)

## What's Changed
* Automated cherry pick of #14081: aws-ebs-csi-driver: remove preStop hook by @hakman in https://github.com/kubernetes/kops/pull/14086
* cilium: fix wrong pod annotations templating #1.23 by @sterchelen in https://github.com/kubernetes/kops/pull/14105
* Automated cherry pick of #14115: Disable some flags in kube-controller-manager and by @hakman in https://github.com/kubernetes/kops/pull/14120
* Automated cherry pick of #14188: Update runc to v1.1.4 by @hakman in https://github.com/kubernetes/kops/pull/14197
* Release 1.23.4 by @justinsb in https://github.com/kubernetes/kops/pull/14220


**Full Changelog**: https://github.com/kubernetes/kops/compare/v1.23.3...v1.23.4