---
title: cert-manager v0.13 Release Notes
description: cert-manager v0.13 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cert-manager v0.13 Release Notes 是什么
- 如何 cert-manager v0.13 Release Notes
trigger_keywords:
- cert-manager
- v0.13
- Release
- Notes
- release
- notes
---

# cert-manager v0.13 Release Notes

Source: [v0.13.1](https://github.com/cert-manager/cert-manager/releases/tag/v0.13.1)

## Bug fixes

- Fix Venafi Cloud URL field being marked required ([#2583](https://github.com/jetstack/cert-manager/pull/2583), [@munnerz](https://github.com/munnerz))
- Fix cainjector.enabled=False override being ignored by the Helm Chart ([#2552](https://github.com/jetstack/cert-manager/pull/2552), [@gtaylor](https://github.com/gtaylor))
- Fix bug that could cause certificates to be incorrectly issued with an invalid public key ([#2543](https://github.com/jetstack/cert-manager/pull/2543), [@munnerz](https://github.com/munnerz))
- Fix GroupVersionKind set on OwnerReference of resources created by HTTP01 challenge solver, causing HTTP01 validations to fail on OpenShift 4.x ([#2554](https://github.com/jetstack/cert-manager/pull/2554), [@munnerz](https://github.com/munnerz))