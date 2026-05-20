---
title: cert-manager v1.9 Release Notes
description: cert-manager v1.9 Release Notes — Kubernetes 生产运维知识库
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
- cert-manager v1.9 Release Notes 是什么
- 如何 cert-manager v1.9 Release Notes
trigger_keywords:
- cert-manager
- v1.9
- Release
- Notes
- release
- notes
---

# cert-manager v1.9 Release Notes

Source: [v1.9.2](https://github.com/cert-manager/cert-manager/releases/tag/v1.9.2)

cert-manager is the easiest way to automatically manage certificates in Kubernetes and OpenShift clusters.

cert-manager `v1.9.2` is a bug fix release which fixes an issue where CertificateRequests marked as InvalidRequest did not properly trigger issuance failure handling leading to 'stuck' requests, and a problem which prevented the Venafi Issuer from connecting to TPP servers where the `vedauth` API endpoints were configured to *accept* client certificates.
It is also compiled with a newer version of Go 1.18 (`v1.18.8`) which fixes some vulnerabilities in the Go standard library.

## Changes since `v1.9.1`

### Bug or Regression

- Fix issue where CertificateRequests marked as InvalidRequest did not properly trigger issuance failure handling leading to 'stuck' requests.
  ([#5371](https://github.com/cert-manager/cert-manager/pull/5371), [@munnerz](https://github.com/munnerz) )
- The Venafi Issuer now supports TLS 1.2 renegotiation, so that it can connect to TPP servers where the `vedauth` API endpoints are configured to *accept* client certificates. (Note: This does not mean that the Venafi Issuer supports client certificate authentication).
  ([#5577](https://github.com/cert-manager/cert-manager/pull/5577), [@wallrj](https://github.com/wallrj))
- Upgrade to latest go patch release.
  ([#5561](https://github.com/cert-manager/cert-manager/pull/5561), [@SgtCoDFish](https://github.com/SgtCoDFish))