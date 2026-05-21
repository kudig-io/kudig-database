---
title: cert-manager v1.14 Release Notes
description: cert-manager v1.14 Release Notes — Kubernetes 生产运维知识库
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
- cert-manager v1.14 Release Notes 是什么
- 如何 cert-manager v1.14 Release Notes
trigger_keywords:
- cert-manager
- v1.14
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- tls-basics
---

# cert-manager v1.14 Release Notes

Source: [v1.14.7](https://github.com/cert-manager/cert-manager/releases/tag/v1.14.7)

cert-manager is the easiest way to automatically manage certificates in Kubernetes and OpenShift clusters.

## 📜 Changes since [v1.14.6](https://github.com/cert-manager/cert-manager/releases/tag/v1.14.6)

### Bugfixes

- BUGFIX: fix issue that caused Vault issuer to not retry signing when an error was encountered. (#7113, @cert-manager-bot)

### Other (Cleanup or Flake)

- Update github.com/Azure/azure-sdk-for-go/sdk/azidentity to address CVE-2024-35255 (#7093, @ThatsMrTalbot)
