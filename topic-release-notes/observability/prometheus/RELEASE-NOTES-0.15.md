---
title: prometheus v0.15 Release Notes
description: prometheus v0.15 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v0.15 Release Notes 是什么
- 如何 prometheus v0.15 Release Notes
trigger_keywords:
- prometheus
- v0.15
- Release
- Notes
- release
- notes
---

# prometheus v0.15 Release Notes

Source: [0.15.1](https://github.com/prometheus/prometheus/releases/tag/0.15.1)

- [BUGFIX] Fix vector matching behavior when there is a mix of equality and
  non-equality matchers in a vector selector and one matcher matches no series.
- [ENHANCEMENT] Allow overriding `GOARCH` and `GOOS` in Makefile.INCLUDE.
- [ENHANCEMENT] Update vendored dependencies.
