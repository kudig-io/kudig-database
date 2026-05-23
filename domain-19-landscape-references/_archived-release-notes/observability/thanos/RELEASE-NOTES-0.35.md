---
title: thanos v0.35 Release Notes
description: thanos v0.35 Release Notes — Kubernetes 生产运维知识库
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
- thanos v0.35 Release Notes 是什么
- 如何 thanos v0.35 Release Notes
trigger_keywords:
- thanos
- v0.35
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
created: "2026-05-23"
---

# [[Thanos|thanos]] v0.35 Release Notes

Source: [v0.35.1](https://github.com/thanos-io/thanos/releases/tag/v0.35.1)

This patch release bring a few fixes to all components and addresses a security concern! Please try it out and let us know if you face issues! 🚀 

## Changelog

### Fixed

- [#7323](https://github.com/thanos-io/thanos/pull/7323) Sidecar: wait for [[Prometheus|prometheus]] on startup
- [#6948](https://github.com/thanos-io/thanos/pull/6948) Receive: fix goroutines leak during series requests to thanos store api.
- [#7382](https://github.com/thanos-io/thanos/pull/7382) *: Ensure objstore flag values are masked & disable debug/pprof/cmdline
- [#7392](https://github.com/thanos-io/thanos/pull/7392) Query: fix broken min, max for pre 0.34.1 sidecars
- [#7373](https://github.com/thanos-io/thanos/pull/7373) Receive: Fix stats for remote write
- [#7318](https://github.com/thanos-io/thanos/pull/7318) Compactor: Recover from panic to log block ID

**Full Changelog**: https://github.com/thanos-io/thanos/compare/v0.35.0...v0.35.1