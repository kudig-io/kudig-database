---
title: kind v0.5 Release Notes
description: kind v0.5 Release Notes — Kubernetes 生产运维知识库
summary: kind v0.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kind v0.5 Release Notes 是什么
- 如何 kind v0.5 Release Notes
trigger_keywords:
- kind
- v0.5
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# kind v0.5 Release Notes

Source: [v0.5.1](https://github.[[entities/kubernetes.md|kubernetes]]-sigs/kind/releases/tag/v0.5.1)

This release is a minor patch to upgrade `kustomize` to `v3.1.1-0.20190821175718-4b67a6de1296`, fixing builds for Windows. This release also contains fixes to our release tooling & CI to ensure we don't regress on this.

**Otherwise, please see the release notes for [v0.5.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.5.0)**.

See: https://github.com/kubernetes-sigs/kind/issues/792 for more details on what went wrong and how we fixed it.