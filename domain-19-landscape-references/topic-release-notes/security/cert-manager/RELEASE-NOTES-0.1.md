---
title: cert-manager v0.1 Release Notes
description: cert-manager v0.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cert-manager v0.1 Release Notes 是什么
- 如何 cert-manager v0.1 Release Notes
trigger_keywords:
- cert-manager
- v0.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- tls-basics
- policy-basics
---

# cert-manager v0.1 Release Notes

Source: [v0.1.2](https://github.com/cert-manager/cert-manager/releases/tag/v0.1.2)

[Documentation](https://github.com/jetstack/cert-manager/tree/master/docs) & [User Guides](https://github.com/jetstack/cert-manager/tree/master/docs/user-guides)

## Changelog since v0.1.1

* Fix panic if the secret named in an ACME issuer exists but contains invalid data (or no data) (#165, @munnerz)
* Fix bug in ACME HTTP01 solver causing self-check to return true before paths have propagated (#166, @munnerz)
