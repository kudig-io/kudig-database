---
title: cert-manager v0.4 Release Notes
description: cert-manager v0.4 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cert-manager v0.4 Release Notes 是什么
- 如何 cert-manager v0.4 Release Notes
trigger_keywords:
- cert-manager
- v0.4
- Release
- Notes
- release
- notes
---

# cert-manager v0.4 Release Notes

Source: [v0.4.1](https://github.com/cert-manager/cert-manager/releases/tag/v0.4.1)

[Documentation](https://cert-manager.readthedocs.io/en/release-0.4)

This is a bugfix release for the v0.4 release series.

It fixes a number of issues relating to the ACME Issuer. Notably, it fixes a bug that could cause ACME orders to be handled when they are in the 'valid' state (i.e. already issued).

It is advised all users of the v0.4 series upgrade to v0.4.1 as soon as possible.

* Don't bundle the CA certificate when using the self signed issuer (#811, @munnerz)
* Add renew-before-expiry-duration option to configure how long before expiration a certificate should be attempted to be renewed (#801, @munnerz)
* Fix issue that could cause Certificates to fail re-issuance if triggered before certificate expiry (#800, @munnerz)
* Fix a race that could cause ACME orders to fail despite them being in a 'valid' state (#764, @munnerz)
* Fixed Route53 cleanup errors for already deleted records. (#746, @euank)
* Fix cleanup of Google Cloud DNS hosted zone for dns-01 challenge records (#754, @kragniz)
