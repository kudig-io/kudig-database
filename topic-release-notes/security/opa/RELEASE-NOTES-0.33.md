---
title: opa v0.33 Release Notes
description: opa v0.33 Release Notes — Kubernetes 生产运维知识库
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
- opa v0.33 Release Notes 是什么
- 如何 opa v0.33 Release Notes
trigger_keywords:
- opa
- v0.33
- Release
- Notes
- release
- notes
---

# opa v0.33 Release Notes

Source: [v0.33.1](https://github.com/open-policy-agent/opa/releases/tag/v0.33.1)

This is a bugfix release addressing an issue in the formatting of rego code that contains
object literals. With the last release, those objects would under some conditions have their
keys re-ordered, with some of them put into a single line.

Thanks to @[iainmcgin](https://github.com/iainmcgin) for reporting.

### Fixes

- format: make groupIterable sort by row ([#3849](https://github.com/open-policy-agent/opa/issues/3849)) 
