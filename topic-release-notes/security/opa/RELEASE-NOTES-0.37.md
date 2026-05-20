---
title: opa v0.37 Release Notes
description: opa v0.37 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opa v0.37 Release Notes 是什么
- 如何 opa v0.37 Release Notes
trigger_keywords:
- opa
- v0.37
- Release
- Notes
- release
- notes
---

# opa v0.37 Release Notes

Source: [v0.37.2](https://github.com/open-policy-agent/opa/releases/tag/v0.37.2)

This is a bugfix release addressing two bugs:

1. A regression introduced in the formatter fix for CVE-2022-23628.
2. Support indices for appending to an array, conforming to JSON Patch (RFC6902)
   for patch bundles.

### Miscellaneous

- format: generated vars may have a proper location
- storage: Support index for array appends