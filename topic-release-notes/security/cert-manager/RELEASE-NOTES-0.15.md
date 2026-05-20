---
title: cert-manager v0.15 Release Notes
description: cert-manager v0.15 Release Notes — Kubernetes 生产运维知识库
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
- cert-manager v0.15 Release Notes 是什么
- 如何 cert-manager v0.15 Release Notes
trigger_keywords:
- cert-manager
- v0.15
- Release
- Notes
- release
- notes
---

# cert-manager v0.15 Release Notes

Source: [v0.15.2](https://github.com/cert-manager/cert-manager/releases/tag/v0.15.2)

## Changes by Kind
### Other (Bug, Cleanup or Flake)
- Error on venafi CertificateRequest when DN is empty ([#3054](https://github.com/jetstack/cert-manager/pull/3054), [@meyskens](https://github.com/meyskens))
- Fix entrypoint being inside a shell in UBI images ([cert-manager-olm#12](https://github.com/jetstack/cert-manager-olm/pull/12), [@meyskens](https://github.com/meyskens))