---
title: opa v0.62 Release Notes
description: opa v0.62 Release Notes — Kubernetes 生产运维知识库
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
- opa v0.62 Release Notes 是什么
- 如何 opa v0.62 Release Notes
trigger_keywords:
- opa
- v0.62
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

# opa v0.62 Release Notes

Source: [v0.62.1](https://github.com/open-policy-agent/opa/releases/tag/v0.62.1)

This is a **security fix release** for the fixes published in [Go 1.22.1](https://groups.google.com/g/golang-announce/c/5pwGVUPoMbg).

OPA servers using `--authentication=tls` would be affected: crafted malicious client certificates could cause a panic in the server.

Also, crafted server certificates could panic OPA's HTTP clients, in bundle plugin, status and decision logs; and `http.send` calls that verify TLS.

This is CVE-2024-24783 (https://pkg.go.dev/vuln/GO-2024-2598).

Note that there are other security fixes in this Golang release, but whether or not OPA is affected is harder to assess. An update is advised.


### Miscellaneous

- Add Trino to OPA ecosystem (authored by @mosabua)
- update: ADOPTERS.md (#6608) (authored by @fredmaggiowski)


