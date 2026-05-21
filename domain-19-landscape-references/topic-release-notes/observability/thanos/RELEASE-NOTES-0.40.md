---
title: thanos v0.40 Release Notes
description: thanos v0.40 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- thanos v0.40 Release Notes 是什么
- 如何 thanos v0.40 Release Notes
trigger_keywords:
- thanos
- v0.40
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# thanos v0.40 Release Notes

Source: [v0.40.1](https://github.com/thanos-io/thanos/releases/tag/v0.40.1)

-- There is still a performance regression in this release https://github.com/thanos-io/thanos/issues/8549 that we are working on fixing --

This fix fixes a performance regression in the gRPC layer.

## What's Changed
* Pull extgrpc fix and release 0.40.1 by @GiedriusS in https://github.com/thanos-io/thanos/pull/8547


**Full Changelog**: https://github.com/thanos-io/thanos/compare/v0.40.0...v0.40.1