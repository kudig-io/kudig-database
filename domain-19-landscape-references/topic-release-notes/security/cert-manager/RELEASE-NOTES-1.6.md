---
title: cert-manager v1.6 Release Notes
description: cert-manager v1.6 Release Notes — Kubernetes 生产运维知识库
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
- cert-manager v1.6 Release Notes 是什么
- 如何 cert-manager v1.6 Release Notes
trigger_keywords:
- cert-manager
- v1.6
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- tls-basics
---

# cert-manager v1.6 Release Notes

Source: [v1.6.3](https://github.com/cert-manager/cert-manager/releases/tag/v1.6.3)

# v1.6.3 Release Notes

1.6.3 is a minor release rebuilding cert-manager 1.6 using the latest version of Go. This eliminates a few security vulnerabilities which have accumulated in Go since the last release.

We don't believe any of those vulnerabilities were practically exploitable or relevant to cert-manager, but we decided to rebuild to keep up to date anyway.

# Changelog since cert-manager 1.6.2

### Bug or Regression

- Bumps the version of Go used to build the cert-manager binaries to 1.17.8, to fix a slew of CVEs (none of which were likely to be exploited) ([#4975](https://github.com/cert-manager/cert-manager/pull/4975), @vhosakot)
- Fixes an expired hardcoded certificate which broke unit tests ([#4977](https://github.com/cert-manager/cert-manager/pull/4977), @SgtCoDFish @jakexks)
