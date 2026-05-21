---
title: cert-manager v0.8 Release Notes
description: cert-manager v0.8 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- ingress
- webhook
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cert-manager v0.8 Release Notes 是什么
- 如何 cert-manager v0.8 Release Notes
trigger_keywords:
- cert-manager
- v0.8
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- tls-basics
---

# cert-manager v0.8 Release Notes

Source: [v0.8.1](https://github.com/cert-manager/cert-manager/releases/tag/v0.8.1)

## Changelog since v0.8.0

* cert-manager-webhook secret exists in cert-manager ns (#1753, @kevinawoo)
* Fix indentation on ACME setup examples (#1785, @lachlancooper)
* Fix ECDSA certificate issuance with ACME issuer (#1757, @munnerz)
* Fix panic in HTTP01 solver if ingress field is not specified (#1758, @munnerz)
* Fix solver selection logic to return the selected solver rather than always returning the last one (#1717, @dobesv)
* Fix logic to select the solver that has the most labels (#1715, @dobesv)