---
title: cert-manager v1.13 Release Notes
description: cert-manager v1.13 Release Notes — Kubernetes 生产运维知识库
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
- cert-manager v1.13 Release Notes 是什么
- 如何 cert-manager v1.13 Release Notes
trigger_keywords:
- cert-manager
- v1.13
- Release
- Notes
- release
- notes
---

# cert-manager v1.13 Release Notes

Source: [v1.13.6](https://github.com/cert-manager/cert-manager/releases/tag/v1.13.6)

cert-manager is the easiest way to automatically manage certificates in Kubernetes and OpenShift clusters.

`v1.13.6` fixes a bug in the DigitalOcean DNS-01 provider which could cause incorrect DNS records to be deleted when using a domain with a CNAME. Special thanks to @BobyMCbobs for reporting this issue and testing the fix!

It also patches CVE-2023-45288.

## Known Issues

- ACME Issuer (Let's Encrypt): wrong certificate chain may be used if `preferredChain` is configured: see [1.14 release notes](./release-notes-1.14.md#known-issues) for more information.

## Changes

### Bug or Regression

- DigitalOcean: Ensure that only TXT records are considered for deletion when cleaning up after an ACME challenge (#6892, @SgtCoDFish)
- Bump golang.org/x/net to address [CVE-2023-45288](https://nvd.nist.gov/vuln/detail/CVE-2023-45288) (#6932, @SgtCoDFish)

