---
title: cert-manager v1.18 Release Notes
description: cert-manager v1.18 Release Notes — Kubernetes 生产运维知识库
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
- cert-manager v1.18 Release Notes 是什么
- 如何 cert-manager v1.18 Release Notes
trigger_keywords:
- cert-manager
- v1.18
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- tls-basics
---

# cert-manager v1.18 Release Notes

Source: [v1.18.6](https://github.com/cert-manager/cert-manager/releases/tag/v1.18.6)

cert-manager is the easiest way to automatically manage certificates in Kubernetes and OpenShift clusters.

v1.18.6 is a simple patch release to fix some reported vulnerabilities, most notably [CVE-2025-68121](https://nvd.nist.gov/vuln/detail/CVE-2025-68121).

NB: We didn't attempt to patch [CVE-2026-24051](https://nvd.nist.gov/vuln/detail/CVE-2026-24051) but that vulnerability affects macOS only, so cert-manager will be unaffected. 

## Changes by Kind

### Bug or Regression

- Bump Go to address CVE-2025-68121 (#8525, @SgtCoDFish)