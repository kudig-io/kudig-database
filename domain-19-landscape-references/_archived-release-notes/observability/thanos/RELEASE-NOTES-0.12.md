---
title: thanos v0.12 Release Notes
description: thanos v0.12 Release Notes — Kubernetes 生产运维知识库
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
- thanos v0.12 Release Notes 是什么
- 如何 thanos v0.12 Release Notes
trigger_keywords:
- thanos
- v0.12
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# [[Thanos|thanos]] v0.12 Release Notes

Source: [v0.12.2](https://github.com/thanos-io/thanos/releases/tag/v0.12.2)

### Fixed

- [#2459](https://github.com/thanos-io/thanos/issues/2459) Compact: Fixed issue with old blocks being marked and deleted in a (slow) loop.
- [#2533](https://github.com/thanos-io/thanos/pull/2515) Rule: do not wrap reload endpoint with `/`. Makes `/-/reload` accessible again when no prefix has been specified.