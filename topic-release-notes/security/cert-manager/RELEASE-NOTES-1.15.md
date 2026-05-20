---
title: cert-manager v1.15 Release Notes
description: cert-manager v1.15 Release Notes — Kubernetes 生产运维知识库
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
- cert-manager v1.15 Release Notes 是什么
- 如何 cert-manager v1.15 Release Notes
trigger_keywords:
- cert-manager
- v1.15
- Release
- Notes
- release
- notes
---

# cert-manager v1.15 Release Notes

Source: [v1.15.5](https://github.com/cert-manager/cert-manager/releases/tag/v1.15.5)

cert-manager is the easiest way to automatically manage certificates in Kubernetes and OpenShift clusters.

cert-manager v1.15.5 contains simple dependency bumps to address reported CVEs (CVE-2024-45337 and CVE-2024-45338).

We don't believe that cert-manager is actually vulnerable; this release is instead intended to satisfy vulnerability scanners.

## Changes

### Bug or Regression

- Bump golang.org/x/net to address CVE-2024-45337 and CVE-2024-45338 (#7496, @wallrj)

### Other (Cleanup or Flake)

- Bump to go 1.22.10 (#7507, @SgtCoDFish)