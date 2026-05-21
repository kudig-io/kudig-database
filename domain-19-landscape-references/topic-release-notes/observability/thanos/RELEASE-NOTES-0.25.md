---
title: thanos v0.25 Release Notes
description: thanos v0.25 Release Notes — Kubernetes 生产运维知识库
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
- thanos v0.25 Release Notes 是什么
- 如何 thanos v0.25 Release Notes
trigger_keywords:
- thanos
- v0.25
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# thanos v0.25 Release Notes

Source: [v0.25.2](https://github.com/thanos-io/thanos/releases/tag/v0.25.2)

### Fixed

- [#5202](https://github.com/thanos-io/thanos/pull/5202) Exemplars: Return empty data instead of `nil` if no data available.
- [#5242](https://github.com/thanos-io/thanos/pull/5242) Ruler: Make ruler use the correct WAL directory.


**Full Changelog**: https://github.com/thanos-io/thanos/compare/v0.25.1...v0.25.2