---
title: kind v0.8 Release Notes
description: kind v0.8 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kind v0.8 Release Notes 是什么
- 如何 kind v0.8 Release Notes
trigger_keywords:
- kind
- v0.8
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# kind v0.8 Release Notes

Source: [v0.8.1](https://github.[[entities/kubernetes.md|kubernetes]]-sigs/kind/releases/tag/v0.8.1)

**This is a tiny patch release to pick up the fix for [Can't create ipv4 clusters if ipv6 is disabled at kernel level](https://github.com/kubernetes-sigs/kind/issues/1544).**

**For full release notes please see [v0.8.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.8.0).**

**Most users will not need to upgrade to this release, this bug is only known to occur on hosts with the `ipv6.disable=1` kernel parameter.**