---
title: opa v0.4 Release Notes
description: opa v0.4 Release Notes — Kubernetes 生产运维知识库
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
- opa v0.4 Release Notes 是什么
- 如何 opa v0.4 Release Notes
trigger_keywords:
- opa
- v0.4
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

# opa v0.4 Release Notes

Source: [v0.4.10](https://github.com/open-policy-agent/opa/releases/tag/v0.4.10)

This release includes a bunch of new built-in functions to help with string manipulation, JWTs, and more.

The JSON marshalling built-ins have been renamed. Policies that used `json_unmarshal` and `json_marshal` before will need to be updated to use `json.marshal` and `json.unmarshal` respectively.

### Misc

- Add `else` keyword
- Improved undefined built-in error message
- Fixed error message in `-` built-in
- Fixed exit instructions in REPL tutorial
- Relax safety check for built-in outputs