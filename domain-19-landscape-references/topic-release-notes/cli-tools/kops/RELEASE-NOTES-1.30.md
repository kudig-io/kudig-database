---
title: kops v1.30 Release Notes
description: kops v1.30 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- cilium
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.30 Release Notes 是什么
- 如何 kops v1.30 Release Notes
trigger_keywords:
- kops
- v1.30
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cilium-basics
---

# kops v1.30 Release Notes

Source: [v1.30.4](https://github.com/kubernetes/kops/releases/tag/v1.30.4)

## What's Changed
* Automated cherry pick of #17141: Only set NTH ManagedASGTag label if it doesn't already exist by @rifelpet in https://github.com/kubernetes/kops/pull/17143
* Automated cherry pick of #17161: Only configure STS region for Route 53 when we obtain it using IDMS by @johngmyers in https://github.com/kubernetes/kops/pull/17165
* Automated cherry pick of #17184: Update Go to v1.23.4 by @hakman in https://github.com/kubernetes/kops/pull/17186
* Automated cherry pick of #17180: fix the random order of block_device_mappings render Include by @hakman in https://github.com/kubernetes/kops/pull/17189
* Automated cherry pick of #17177: Use the same port for hubble-metrics that is used by cilium by @rifelpet in https://github.com/kubernetes/kops/pull/17199
* Automated cherry pick of #17183: Use SDK's built-in resolver for S3Path.GetHTTPsUrl by @rifelpet in https://github.com/kubernetes/kops/pull/17201
* Automated cherry pick of #17217: Update Go to v1.23.5 by @hakman in https://github.com/kubernetes/kops/pull/17219
* Release 1.30.4 by @johngmyers in https://github.com/kubernetes/kops/pull/17242


**Full Changelog**: https://github.com/kubernetes/kops/compare/v1.30.3...v1.30.4