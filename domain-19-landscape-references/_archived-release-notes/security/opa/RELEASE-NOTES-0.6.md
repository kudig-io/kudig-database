---
title: opa v0.6 Release Notes
description: opa v0.6 Release Notes — Kubernetes 生产运维知识库
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
- opa v0.6 Release Notes 是什么
- 如何 opa v0.6 Release Notes
trigger_keywords:
- opa
- v0.6
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
created: "2026-05-23"
---

# opa v0.6 Release Notes

Source: [v0.6.0](https://github.com/open-policy-agent/opa/releases/tag/v0.6.0)

This release adds initial support for partial evaluation. Partial evaluation
allows callers to mark certain inputs as unknown and then evaluate queries to
produce *new* queries which can be evaluated once inputs become known.

### Features

- Add initial implementation of partial evaluation
- Add sort built-in function ([#465](https://github.com/open-policy-agent/opa/issues/465))
- Add built-in function to check value types

### Fixes

- Fix rule arg type inferencing ([#542](https://github.com/open-policy-agent/opa/issues/542))
- Fix documentation on "else" keyword ([#475](https://github.com/open-policy-agent/opa/issues/475))
- Fix REPL to deduplicate auto-complete paths ([#432](https://github.com/open-policy-agent/opa/pull/432)
- Improve getting started example ([#532](https://github.com/open-policy-agent/opa/issues/532))
- Improve handling of forbidden methods in HTTP server ([#445](https://github.com/open-policy-agent/opa/issues/445))