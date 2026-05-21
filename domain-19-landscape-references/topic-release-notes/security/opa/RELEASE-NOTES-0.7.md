---
title: opa v0.7 Release Notes
description: opa v0.7 Release Notes — Kubernetes 生产运维知识库
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
- opa v0.7 Release Notes 是什么
- 如何 opa v0.7 Release Notes
trigger_keywords:
- opa
- v0.7
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

# opa v0.7 Release Notes

Source: [v0.7.1](https://github.com/open-policy-agent/opa/releases/tag/v0.7.1)

### Fixes

- Use rego.ParsedInput to provide input from form ([#571](https://github.com/open-policy-agent/opa/issues/571))

### Miscellaneous

- Add omitempty tag for ad-hoc query result field
- Fix rego package to check capture vars
- Fix root document assignment in REPL
- Update query compiler to deep copy parsed query