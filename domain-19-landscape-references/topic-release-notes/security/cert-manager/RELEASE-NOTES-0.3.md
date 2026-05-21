---
title: cert-manager v0.3 Release Notes
description: cert-manager v0.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
- ingress
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cert-manager v0.3 Release Notes 是什么
- 如何 cert-manager v0.3 Release Notes
trigger_keywords:
- cert-manager
- v0.3
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- tls-basics
- policy-basics
---

# cert-manager v0.3 Release Notes

Source: [v0.3.2](https://github.com/cert-manager/cert-manager/releases/tag/v0.3.2)

[Documentation](https://cert-manager.readthedocs.io/en/release-0.3) | [Upgrading guide](https://cert-manager.readthedocs.io/en/release-0.3/admin/upgrading/upgrading-0.2-0.3.html)

This is a bugfix release containing a critical patch for the ACME Issuer implementation.

Let's Encrypt recently made a change to their API to bring it in-line with the latest ACME draft spec, which has caused cert-manager to fail to obtain Certificates in some cases. More information can be found on the [Let's Encrypt forum](https://community.letsencrypt.org/t/acmev2-order-ready-status/62866).

It is advised that all users of v0.3.x upgrade to this release immediately, as without the changes included in this release, certificate renewal **will not be successful**.

As this is a patch release, there have been no breaking changes. Please do not hesitate to upgrade your deployments!

## Changelog since v0.3.1

* ACME: Handle the new Let's Encrypt 'Ready' state for orders (#698, @edevil)
* Fix panic when a Certificate specifies a DNS01 provider that is not present on the Issuer resource (#708, @munnerz)
* Fix bug that could cause changes to Ingress resources when using ingress-shim to not be properly propagated to their respective Certificate resources (#686, @kragniz)
