---
title: cert-manager v1.4 Release Notes
description: cert-manager v1.4 Release Notes — Kubernetes 生产运维知识库
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
- cert-manager v1.4 Release Notes 是什么
- 如何 cert-manager v1.4 Release Notes
trigger_keywords:
- cert-manager
- v1.4
- Release
- Notes
- release
- notes
---

# cert-manager v1.4 Release Notes

Source: [v1.4.4](https://github.com/cert-manager/cert-manager/releases/tag/v1.4.4)

# Changelog since v1.4.3

### Bug or Regression

- Fixes renewal time issue for certs with skewed duration period. ([#4403](https://github.com/jetstack/cert-manager/pull/4403), @irbekrm). Thanks to @mfmbarros for help with debugging the issue!