---
title: opa v0.25 Release Notes
description: opa v0.25 Release Notes — Kubernetes 生产运维知识库
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
- opa v0.25 Release Notes 是什么
- 如何 opa v0.25 Release Notes
trigger_keywords:
- opa
- v0.25
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

# opa v0.25 Release Notes

Source: [v0.25.2](https://github.com/open-policy-agent/opa/releases/tag/v0.25.2)

This release extends the HTTP server authorizer (`--authorization=basic`) to supply the HTTP message body in the `input` document. See the [Authentication and Authorization](https://www.openpolicyagent.org/docs/edge/security/#authentication-and-authorization) section in the security documentation for details.