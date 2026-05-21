---
title: thanos v0.28 Release Notes
description: thanos v0.28 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- minio
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- thanos v0.28 Release Notes 是什么
- 如何 thanos v0.28 Release Notes
trigger_keywords:
- thanos
- v0.28
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# thanos v0.28 Release Notes

Source: [v0.28.1](https://github.com/thanos-io/thanos/releases/tag/v0.28.1)

## What's Changed

### Fixed

- [#5702](https://github.com/thanos-io/thanos/pull/5702) Store: Upgrade minio-go/v7 to fix panic caused by leaked goroutines.


**Full Changelog**: https://github.com/thanos-io/thanos/compare/v0.28.0...v0.28.1