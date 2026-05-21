---
title: cert-manager v0.16 Release Notes
description: cert-manager v0.16 Release Notes — Kubernetes 生产运维知识库
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
- cert-manager v0.16 Release Notes 是什么
- 如何 cert-manager v0.16 Release Notes
trigger_keywords:
- cert-manager
- v0.16
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- tls-basics
---

# cert-manager v0.16 Release Notes

Source: [v0.16.1](https://github.com/cert-manager/cert-manager/releases/tag/v0.16.1)

## Changes by Kind

### Other (Bug, Cleanup or Flake)

- Ensures Secrets created from the Certificates controller contains the annotation containing the Issuer Group Name. ([#3153](https://github.com/jetstack/cert-manager/pull/3153), [@JoshVanL](https://github.com/JoshVanL))