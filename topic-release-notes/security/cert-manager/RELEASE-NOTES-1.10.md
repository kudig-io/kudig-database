---
title: cert-manager v1.10 Release Notes
description: cert-manager v1.10 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cert-manager v1.10 Release Notes 是什么
- 如何 cert-manager v1.10 Release Notes
trigger_keywords:
- cert-manager
- v1.10
- Release
- Notes
- release
- notes
---

# cert-manager v1.10 Release Notes

Source: [v1.10.2](https://github.com/cert-manager/cert-manager/releases/tag/v1.10.2)

cert-manager is the easiest way to automatically manage certificates in Kubernetes and OpenShift clusters.

v1.10.2 is primarily a performance enhancement release which might reduce memory consumption by up to 50% in some cases thanks to some brilliant work by @irbekrm! :tada: 

It also patches several vulnerabilities reported by scanners and updates the base images used for cert-manager containers. In addition, it removes a potentially confusing log line which had been introduced in v1.10.0 which implied that an error had occurred when using external issuers even though there'd been no error.

## Changes since `v1.10.1`

### Feature

- Enable support for Kubernetes 1.26 in tests (#5647, @SgtCoDFish)

### Bug or Regression

- Fixes a bug where the cert-manager controller was caching all Secrets twice (#5704, @irbekrm)
- Bump helm version to fix CVE-2022-23525 (#5676, @SgtCoDFish)
- Don't log errors relating to selfsigned issuer checks for external issuers (#5687, @SgtCoDFish)
- Fix `golang.org/x/text` vulnerability (#5592, @SgtCoDfish)
- Upgrade golang/x/net to fix CVE-2022-41717 (#5635, @SgtCoDFish)
- Upgrade to go 1.19.4 to fix CVE-2022-41717 (#5620, @SgtCoDfish)
- Use manually specified tmpdir template when verifying CRDs (#5682, @SgtCoDFish)

### Other (Cleanup or Flake)

- Bump distroless base images to latest versions (#5677, @SgtCoDFish)
