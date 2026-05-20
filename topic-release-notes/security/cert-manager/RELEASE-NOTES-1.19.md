---
title: cert-manager v1.19 Release Notes
description: cert-manager v1.19 Release Notes — Kubernetes 生产运维知识库
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
- cert-manager v1.19 Release Notes 是什么
- 如何 cert-manager v1.19 Release Notes
trigger_keywords:
- cert-manager
- v1.19
- Release
- Notes
- release
- notes
---

# cert-manager v1.19 Release Notes

Source: [v1.19.4](https://github.com/cert-manager/cert-manager/releases/tag/v1.19.4)

cert-manager is the easiest way to automatically manage certificates in Kubernetes and OpenShift clusters.

v1.19.4 is a simple patch release to fix some reported vulnerabilities - notably CVE-2026-24051 and CVE-2025-68121. All users should upgrade.

## Changes by Kind

### Bug or Regression

- Bump go to address CVE-2025-68121 (#8526, @SgtCoDFish)
- Bump otel SDK to address GO-2026-4394 (#8531, @SgtCoDFish)