---
title: thanos v0.36 Release Notes
description: thanos v0.36 Release Notes — Kubernetes 生产运维知识库
summary: thanos v0.36 Release Notes — Kubernetes 生产运维知识库
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
- thanos v0.36 Release Notes 是什么
- 如何 thanos v0.36 Release Notes
trigger_keywords:
- thanos
- v0.36
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Thanos|thanos]] v0.36 Release Notes

Source: [v0.36.1](https://github.com/thanos-io/thanos/releases/tag/v0.36.1)

This patch release brings a few fixes! Please try it out and let us know if you face issues! 🚀

## Changelog

### Fixed

- [#7634](https://github.com/thanos-io/thanos/pull/7634) Rule: fix Query and Alertmanager TLS configurations with CA only.
- [#7618](https://github.com/thanos-io/thanos/pull/7618) Proxy: Query goroutine leak when store.response-timeout is set