---
title: thanos v0.37 Release Notes
description: thanos v0.37 Release Notes — Kubernetes 生产运维知识库
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
- thanos v0.37 Release Notes 是什么
- 如何 thanos v0.37 Release Notes
trigger_keywords:
- thanos
- v0.37
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# thanos v0.37 Release Notes

Source: [v0.37.2](https://github.com/thanos-io/thanos/releases/tag/v0.37.2)

v0.37.2 is out now with a few fixes for Sidecar and Store, before the end of the year!

Please try it out and let us know if you find further issues! 🚀

## Changelog

### Fixed

- [#7970](https://github.com/thanos-io/thanos/pull/7970) Sidecar: Respect min-time setting.
- [#7962](https://github.com/thanos-io/thanos/pull/7962) Store: Fix potential deadlock in hedging request.