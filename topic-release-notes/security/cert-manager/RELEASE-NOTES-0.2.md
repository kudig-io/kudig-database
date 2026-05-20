---
title: cert-manager v0.2 Release Notes
description: cert-manager v0.2 Release Notes — Kubernetes 生产运维知识库
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
- cert-manager v0.2 Release Notes 是什么
- 如何 cert-manager v0.2 Release Notes
trigger_keywords:
- cert-manager
- v0.2
- Release
- Notes
- release
- notes
---

# cert-manager v0.2 Release Notes

Source: [v0.2.5](https://github.com/cert-manager/cert-manager/releases/tag/v0.2.5)

[Documentation](https://cert-manager.readthedocs.io/en/release-0.2)

This is a bugfix release which fixes bugs in the way rate limits were handled within the Certificate control loop. This could cause failing authorizations to be retried in quick succession.

It is recommended that all users of v0.2.x upgrade to this release as soon as possible.

## Changelog since v0.2.4

* Fix bug that could cause excessive validation/issuance attempts for failing Certificate resources (#496, @munnerz)
* More aggressively backoff when retry failing certificate requests (#519, @munnerz)
