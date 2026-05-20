---
title: cert-manager v1.16 Release Notes
description: cert-manager v1.16 Release Notes — Kubernetes 生产运维知识库
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
- cert-manager v1.16 Release Notes 是什么
- 如何 cert-manager v1.16 Release Notes
trigger_keywords:
- cert-manager
- v1.16
- Release
- Notes
- release
- notes
---

# cert-manager v1.16 Release Notes

Source: [v1.16.5](https://github.com/cert-manager/cert-manager/releases/tag/v1.16.5)

cert-manager is the easiest way to automatically manage certificates in Kubernetes and OpenShift clusters.

This patch release addresses several vulnerabilities reported by the Trivy security scanner. It is built with the latest version of Go 1.23 and includes various dependency updates. 

> 📖 Read the full [cert-manager 1.16 release notes](https://cert-manager.io/docs/releases/release-notes/release-notes-1.16), before installing or upgrading.

## Changes since `v1.16.4`:

### Bug or Regression
- Bump Go to `v1.23.8` to fix `CVE-2025-22871` ([#7706](https://github.com/cert-manager/cert-manager/pull/7706), [`@wallrj`](https://github.com/wallrj))
- Bump `github.com/golang-jwt/jwt/v5` to `v5.2.2` to fix `CVE-2025-30204` ([#7708](https://github.com/cert-manager/cert-manager/pull/7708), [`@wallrj`](https://github.com/wallrj))
- Bump `golang.org/x/net` to fix `CVE-2025-22872` ([#7707](https://github.com/cert-manager/cert-manager/pull/7707), [`@wallrj`](https://github.com/wallrj))
- Bump `go-jose` dependency to address `CVE-2025-27144` ([#7602](https://github.com/cert-manager/cert-manager/pull/7602), [`@SgtCoDFish`](https://github.com/SgtCoDFish))
- Bump `golang.org/x/net` to address `CVE-2025-22870` reported by Trivy ([#7623](https://github.com/cert-manager/cert-manager/pull/7623), [`@SgtCoDFish`](https://github.com/SgtCoDFish))
