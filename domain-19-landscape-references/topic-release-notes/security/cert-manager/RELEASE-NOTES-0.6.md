---
title: cert-manager v0.6 Release Notes
description: cert-manager v0.6 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- webhook
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cert-manager v0.6 Release Notes 是什么
- 如何 cert-manager v0.6 Release Notes
trigger_keywords:
- cert-manager
- v0.6
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- tls-basics
---

# cert-manager v0.6 Release Notes

Source: [v0.6.2](https://github.com/cert-manager/cert-manager/releases/tag/v0.6.2)

This patch release of cert-manager resolves issues when running the webhook component on Amazon EKS.

You can find more information in #1220 

## Changelog since v0.6.1

* Bump Kubernetes apimachinery dependencies to v1.10.12 (#1344, @munnerz)
