---
title: cert-manager v1.11 Release Notes
description: cert-manager v1.11 Release Notes — Kubernetes 生产运维知识库
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
- cert-manager v1.11 Release Notes 是什么
- 如何 cert-manager v1.11 Release Notes
trigger_keywords:
- cert-manager
- v1.11
- Release
- Notes
- release
- notes
---

# cert-manager v1.11 Release Notes

Source: [v1.11.5](https://github.com/cert-manager/cert-manager/releases/tag/v1.11.5)

v1.11.5 contains an important security fix that addresses [CVE-2023-29409](https://cve.report/CVE-2023-29409).

## Changes since v1.11.4

- Use Go 1.19.9 to fix a security issue in Go's `crypto/tls` library. (#6317, @maelvls)