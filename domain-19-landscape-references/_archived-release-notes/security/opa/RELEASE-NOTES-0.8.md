---
title: opa v0.8 Release Notes
description: opa v0.8 Release Notes — Kubernetes 生产运维知识库
summary: opa v0.8 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
- rag
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opa v0.8 Release Notes 是什么
- 如何 opa v0.8 Release Notes
trigger_keywords:
- opa
- v0.8
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---



# opa v0.8 Release Notes

Source: [v0.8.2](https://github.com/open-policy-agent/opa/releases/tag/v0.8.2)

### Fixes

- Fix virtual document cache invalidation ([#736](https://github.com/open-policy-agent/opa/issues/736))
- Fix partial cache invalidation for data changes ([#589](https://github.com/open-policy-agent/opa/issues/589))
- Fix query to path conversion in decision logger ([#783](https://github.com/open-policy-agent/opa/issues/783))
- Fix handling of pointers to structs ([#722](https://github.com/open-policy-agent/opa/issues/722), thanks @srenatus)
- Improve sprintf number handling ([#748](https://github.com/open-policy-agent/opa/issues/748))
- Reduce memory overhead of decision logs ([#705](https://github.com/open-policy-agent/opa/issues/705))
- Set bundle status in case of HTTP 304 ([#794](https://github.com/open-policy-agent/opa/issues/794))

### Miscellaneous

- Add docs on best practices around identity
- Add built-in function to verify JWTs signed with HS246 (thanks @hbouvier)
- Add built-in function to URL encode objects (thanks @vrnmthr)
- Add query parameters to authorization policy input ([#786](https://github.com/open-policy-agent/opa/pull/786))
- Add support for listening on a UNIX domain socket ([#692](https://github.com/open-policy-agent/opa/issues/692), thanks @JAORMX)
- Add trace event for rule index lookups ([#716](https://github.com/open-policy-agent/opa/issues/716))
- Add support for multiple listeners in server (thanks @JAORMX)
- Remove decision log buffer size limit by default
- Update codebase with various go-fmt/ineffassign/mispell fixes (thanks @srenatus)
- Update REPL command to set unknowns
- Update subcommands to support loader filter ([#782](https://github.com/open-policy-agent/opa/issues/782))
- Update evaluator to cache storage reads
- Update object to keep track of groundness