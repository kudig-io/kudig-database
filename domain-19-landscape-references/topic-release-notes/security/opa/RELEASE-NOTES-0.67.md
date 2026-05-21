---
title: opa v0.67 Release Notes
description: opa v0.67 Release Notes — Kubernetes 生产运维知识库
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
- opa v0.67 Release Notes 是什么
- 如何 opa v0.67 Release Notes
trigger_keywords:
- opa
- v0.67
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

# opa v0.67 Release Notes

Source: [v0.67.1](https://github.com/open-policy-agent/opa/releases/tag/v0.67.1)

This is a bug fix release addressing the following issue:

- util+server: Fix bug around chunked request handling ([#6906](https://github.com/open-policy-agent/opa/pull/6906)) authored by @philipaconrad, reported by @David-Wobrock. A request handling bug was introduced in ([#6868](https://github.com/open-policy-agent/opa/pull/6868)), which caused OPA to treat all incoming chunked requests as if they had zero-length request bodies.

