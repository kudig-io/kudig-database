---
title: opa v0.38 Release Notes
description: opa v0.38 Release Notes — Kubernetes 生产运维知识库
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
- opa v0.38 Release Notes 是什么
- 如何 opa v0.38 Release Notes
trigger_keywords:
- opa
- v0.38
- Release
- Notes
- release
- notes
---

# opa v0.38 Release Notes

Source: [v0.38.1](https://github.com/open-policy-agent/opa/releases/tag/v0.38.1)

This is a bug fix release that addresses one issue when using `opa test` with the
`--bundle` (`-b`) flag, and a policy that uses the `every` keyword.

There are no other code changes in this release.

### Fixes

- Compiler: don't raise an error with unused declared+generated vars (every) ([#4420](https://github.com/open-policy-agent/opa/issues/4420)), reported by @kristiansvalland