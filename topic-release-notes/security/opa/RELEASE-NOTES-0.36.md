---
title: opa v0.36 Release Notes
description: opa v0.36 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opa v0.36 Release Notes 是什么
- 如何 opa v0.36 Release Notes
trigger_keywords:
- opa
- v0.36
- Release
- Notes
- release
- notes
---

# opa v0.36 Release Notes

Source: [v0.36.1](https://github.com/open-policy-agent/opa/releases/tag/v0.36.1)

This release includes a number of documentation fixes.
It also includes the experimental binary for darwin/arm64.

There are no code changes.

### Documentation

- OpenTelemetry: fix configuration example, authored by @rvalkenaers
- Configuration: fix typo for `tls-cert-refresh-period`, authored by @mattmahn
- SSH and Sudo authorization: Add missing filename
- Integration: fix example policy

### Release

- Build darwin/arm64 in post tag workflow