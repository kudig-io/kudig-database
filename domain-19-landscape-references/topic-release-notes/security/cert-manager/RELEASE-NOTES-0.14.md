---
title: cert-manager v0.14 Release Notes
description: cert-manager v0.14 Release Notes — Kubernetes 生产运维知识库
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
- cert-manager v0.14 Release Notes 是什么
- 如何 cert-manager v0.14 Release Notes
trigger_keywords:
- cert-manager
- v0.14
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- tls-basics
---

# cert-manager v0.14 Release Notes

Source: [v0.14.3](https://github.com/cert-manager/cert-manager/releases/tag/v0.14.3)

## Changes by Kind

### Other (Bug, Cleanup or Flake)

- Fix bug in webhook based validation on Kubernetes API servers older than 1.15 ([#2860](https://github.com/jetstack/cert-manager/pull/2860), [@munnerz ](https://github.com/munnerz))
- Fix case where cert-manager.io/issuer doesn't set `Issuer` kind ([#2838](https://github.com/jetstack/cert-manager/pull/2838), [@meyskens](https://github.com/meyskens))
- Fix validatingwebhookconfiguration to use correct URL path and to suport v1alpha3 API objects. ([#2832](https://github.com/jetstack/cert-manager/pull/2832), [@wallrj ](https://github.com/wallrj ))
- Limit `per_page` to 100 in Cloudfare API calls ([#2859](https://github.com/jetstack/cert-manager/pull/2859), [@sileht](https://github.com/sileht))