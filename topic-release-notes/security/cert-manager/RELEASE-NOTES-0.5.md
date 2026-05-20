---
title: cert-manager v0.5 Release Notes
description: cert-manager v0.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- ingress
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cert-manager v0.5 Release Notes 是什么
- 如何 cert-manager v0.5 Release Notes
trigger_keywords:
- cert-manager
- v0.5
- Release
- Notes
- release
- notes
---

# cert-manager v0.5 Release Notes

Source: [v0.5.2](https://github.com/cert-manager/cert-manager/releases/tag/v0.5.2)

Two releases in one day!

This release contains a single additional patch over v0.5.1.

In cases where you have defined Ingress resources with multiple different hostnames, that only enable TLS for a subset of those hostnames - if ingress-shim is enabled for these Ingress resources, the hosts that did *not* have TLS enabled would be removed from the Ingress resource.

* Fix bug when cleaning up ingress resources after performing ACME HTTP01 validation (#1082, @munnerz)
