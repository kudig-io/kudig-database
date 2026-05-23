---
title: opa v0.10 Release Notes
description: opa v0.10 Release Notes — Kubernetes 生产运维知识库
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
- opa v0.10 Release Notes 是什么
- 如何 opa v0.10 Release Notes
trigger_keywords:
- opa
- v0.10
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

# opa v0.10 Release Notes

Source: [v0.10.7](https://github.com/open-policy-agent/opa/releases/tag/v0.10.7)

This release publishes the Hugo-based documentation to GitHub Pages :tada:

### Fixes

- Add `array.slice` built-in function ([#1243](https://github.com/open-policy-agent/opa/issues/1243))
- Add `net.cidr_contains` and `net.cidr_intersects` built-ins
  ([#1289](https://github.com/open-policy-agent/opa/issues/1289)). This
  change deprecates the old `net.cidr_overlap` built-in function. The
  latter will be supported for backwards compatibility but new
  policies should refer to `net.cidr_contains`.

### Miscellaneous

- Bump kube-mgmt container version to 0.8 in tutorial
- Remove unnecessary resizing allocs from AST set and object
- Add [[Kubernetes|Kubernetes]] Admission Control guide