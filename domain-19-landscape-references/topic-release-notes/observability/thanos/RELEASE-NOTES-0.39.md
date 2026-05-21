---
title: thanos v0.39 Release Notes
description: thanos v0.39 Release Notes — Kubernetes 生产运维知识库
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
- thanos v0.39 Release Notes 是什么
- 如何 thanos v0.39 Release Notes
trigger_keywords:
- thanos
- v0.39
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# thanos v0.39 Release Notes

Source: [v0.39.2](https://github.com/thanos-io/thanos/releases/tag/v0.39.2)

Fixes two issues with the distributed query engine.

Fixed
- [#8374](https://github.com/thanos-io/thanos/pull/8374) Query: fix panic when concurrently accessing annotations map
- [#8375](https://github.com/thanos-io/thanos/pull/8375) Query: fix native histogram buckets in distributed queries

Full Changelog: https://github.com/thanos-io/thanos/compare/v0.39.1...v0.39.2