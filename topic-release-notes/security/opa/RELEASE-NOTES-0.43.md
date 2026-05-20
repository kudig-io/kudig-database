---
title: opa v0.43 Release Notes
description: opa v0.43 Release Notes — Kubernetes 生产运维知识库
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
- opa v0.43 Release Notes 是什么
- 如何 opa v0.43 Release Notes
trigger_keywords:
- opa
- v0.43
- Release
- Notes
- release
- notes
---

# opa v0.43 Release Notes

Source: [v0.43.1](https://github.com/open-policy-agent/opa/releases/tag/v0.43.1)

This is a security release fixing the following vulnerabilities:

- CVE-2022-36085: Respect unsafeBuiltinMap for 'with' replacements in the compiler

  See https://github.com/open-policy-agent/opa/security/advisories/GHSA-f524-rf33-2jjr for all details.

- CVE-2022-27664 and CVE-2022-32190.

  Fixed by updating the Go version used in our builds to 1.18.6,
  see https://groups.google.com/g/golang-announce/c/x49AQzIVX-s.
  Note that CVE-2022-32190 is most likely not relevant for OPA's usage of net/url.
  But since these CVEs tend to come up in security assessment tooling regardless,
  it's better to get it out of the way.