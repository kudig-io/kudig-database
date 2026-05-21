---
title: cert-manager v0.9 Release Notes
description: cert-manager v0.9 Release Notes — Kubernetes 生产运维知识库
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
- cert-manager v0.9 Release Notes 是什么
- 如何 cert-manager v0.9 Release Notes
trigger_keywords:
- cert-manager
- v0.9
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- tls-basics
---

# cert-manager v0.9 Release Notes

Source: [v0.9.1](https://github.com/cert-manager/cert-manager/releases/tag/v0.9.1)

## Changelog since v0.9.0

- Fix concurrent map write panic in certificates controller ([#1980](https://github.com/jetstack/cert-manager/pull/1980), [@munnerz](https://github.com/munnerz))
- Fix panic when an ACME Order fails ([#1965](https://github.com/jetstack/cert-manager/pull/1965), [@munnerz](https://github.com/munnerz))
