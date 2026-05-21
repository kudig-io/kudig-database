---
title: flux v0.30 Release Notes
description: flux v0.30 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- flux
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- flux v0.30 Release Notes 是什么
- 如何 flux v0.30 Release Notes
trigger_keywords:
- flux
- v0.30
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# flux v0.30 Release Notes

Source: [v0.30.2](https://github.com/fluxcd/flux2/releases/tag/v0.30.2)

Flux v0.30.2 is a patch release with further patches around working with the macOS file-system.

**Note** that [v0.29.0](https://github.com/fluxcd/flux2/releases/tag/v0.29.0) included breaking changes, and [v0.30.0](https://github.com/fluxcd/flux2/releases/tag/v0.30.0) new features.

## CLI Changelog
- PR #2703 - @aryan9600 - Modify tmp dir generation to be absolute on all OSes
- PR #2701 - @stefanprodan - Grant service account read-only access to controllers

