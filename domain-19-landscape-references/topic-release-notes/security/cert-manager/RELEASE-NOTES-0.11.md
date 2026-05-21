---
title: cert-manager v0.11 Release Notes
description: cert-manager v0.11 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- apiserver
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cert-manager v0.11 Release Notes 是什么
- 如何 cert-manager v0.11 Release Notes
trigger_keywords:
- cert-manager
- v0.11
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- tls-basics
---

# cert-manager v0.11 Release Notes

Source: [v0.11.1](https://github.com/cert-manager/cert-manager/releases/tag/v0.11.1)

This is the only and final patch release of v0.11. It fixes an issue when upgrading from older versions whereby cert-manager will request a new certificate for all Certificate resources immediately if you do not update the `certmanager.k8s.io/issuer-name` and `certmanager.k8s.io/issuer-kind` annotations manually on all Secret resources before upgrading.

It also fixes an issue that will cause Challenge resources to become orphaned if their parent Order resource is deleted.

## Notable Changes

- Ensure secrets using deprecated secret annotations do not cause unneeded re-issuance ([#2404](https://github.com/jetstack/cert-manager/pull/2404), [@JoshVanL](https://github.com/JoshVanL))
- Fix setting ownerReference on Challenge resources created by Orders controller ([#2333](https://github.com/jetstack/cert-manager/pull/2333), [@CoaxVex](https://github.com/CoaxVex))
- Add missing apiVersion to Chart.yaml ([#2300](https://github.com/jetstack/cert-manager/pull/2300), [@yurrriq](https://github.com/yurrriq))
- [Kubernetes APIServer dry-run](https://kubernetes.io/docs/reference/using-api/api-concepts/&#35;dry-run) is supported. ([#2213](https://github.com/jetstack/cert-manager/pull/2213), [@ismailbaskin](https://github.com/ismailbaskin))
- Fix outdated documentation for solver configuration in Issuers and ClusterIssuers ([#2212](https://github.com/jetstack/cert-manager/pull/2212), [@nickbp](https://github.com/nickbp))
