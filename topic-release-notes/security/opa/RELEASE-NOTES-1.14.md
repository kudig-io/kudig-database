---
title: opa v1.14 Release Notes
description: opa v1.14 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opa v1.14 Release Notes 是什么
- 如何 opa v1.14 Release Notes
trigger_keywords:
- opa
- v1.14
- Release
- Notes
- release
- notes
---

# opa v1.14 Release Notes

Source: [v1.14.1](https://github.com/open-policy-agent/opa/releases/tag/v1.14.1)

This is a patch release collecting two bug fixes and various dependency updates for Golang standard library and common package vulnerabilities.

These bug fixes include a revert of the rule indexer tweaks shipped in 1.14.0, which had caused unexpected lookup failures for some users. (We expect to properly fix the issue in 1.15.0, but for now, a revert is the quicker choice.)

### Changes

- Fix intermittent plugins manager  deadlock on opa.configure (#8407)
- Revert "ast: make rule index track var assignments and `x in {...}` (#8341)" (#8410)
- build: bump deps (go.mod from main)
- build: bump go 1.26.1 (#8409)

