---
title: opa v1.10 Release Notes
description: opa v1.10 Release Notes — Kubernetes 生产运维知识库
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
- opa v1.10 Release Notes 是什么
- 如何 opa v1.10 Release Notes
trigger_keywords:
- opa
- v1.10
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

# opa v1.10 Release Notes

Source: [v1.10.1](https://github.com/open-policy-agent/opa/releases/tag/v1.10.1)

This is a bugfix release for the `split` builtin: In v1.10.0, it was looping infinitely when used with an empty-string delimiter.

Reported by @SignalRichard, authored by @srenatus

The release is otherwise identical to v1.10.0.

