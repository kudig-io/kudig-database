---
title: cert-manager v1.3 Release Notes
description: cert-manager v1.3 Release Notes — Kubernetes 生产运维知识库
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
- cert-manager v1.3 Release Notes 是什么
- 如何 cert-manager v1.3 Release Notes
trigger_keywords:
- cert-manager
- v1.3
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- tls-basics
---

# cert-manager v1.3 Release Notes

Source: [v1.3.3](https://github.com/cert-manager/cert-manager/releases/tag/v1.3.3)

# Release notes for v1.3.3

# Changelog since v1.3.2

## Changes by Kind

### Bug or Regression

- Adds an explicit 10 second timeout when checking HTTP01 challenges for reachability ([#4317](https://github.com/jetstack/cert-manager/pull/4317), [@SgtCoDFish ](https://github.com/SgtCoDFish))

### Other (Cleanup or Flake)

- Clarify the exact supported kubernetes version range for cert-manager 1.3 ([#4314](https://github.com/jetstack/cert-manager/pull/4314), [@SgtCoDFish](https://github.com/SgtCoDFish))
