---
title: thanos v0.23 Release Notes
description: thanos v0.23 Release Notes — Kubernetes 生产运维知识库
summary: thanos v0.23 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- thanos v0.23 Release Notes 是什么
- 如何 thanos v0.23 Release Notes
trigger_keywords:
- thanos
- v0.23
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Thanos|thanos]] v0.23 Release Notes

Source: [v0.23.2](https://github.com/thanos-io/thanos/releases/tag/v0.23.2)

### Fixed

- [#4795](https://github.com/thanos-io/thanos/pull/4795) Query: Fix deadlock in endpointset.
- [#4962](https://github.com/thanos-io/thanos/pull/4962) Compact/downsample: fix deadlock if error occurs with some backlog of blocks; fixes [this pull request](https://github.com/thanos-io/thanos/pull/4430). Affected versions are 0.22.0 - 0.23.1.