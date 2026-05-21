---
title: opa v0.57 Release Notes
description: opa v0.57 Release Notes — Kubernetes 生产运维知识库
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
- opa v0.57 Release Notes 是什么
- 如何 opa v0.57 Release Notes
trigger_keywords:
- opa
- v0.57
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
- observability-basics
---

# opa v0.57 Release Notes

Source: [v0.57.1](https://github.com/open-policy-agent/opa/releases/tag/v0.57.1)

This is a bug fix release addressing the following security issues:

### Golang security fix GO-2023-2102

> A malicious HTTP/2 client which rapidly creates requests and immediately resets them can cause excessive server resource consumption.

### OpenTelemetry-Go Contrib security fix CVE-2023-45142

> Denial of service in otelhttp due to unbound cardinality metrics.