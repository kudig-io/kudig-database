---
title: opa v1.0 Release Notes
description: opa v1.0 Release Notes — Kubernetes 生产运维知识库
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
- opa v1.0 Release Notes 是什么
- 如何 opa v1.0 Release Notes
trigger_keywords:
- opa
- v1.0
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
created: "2026-05-23"
---

# opa v1.0 Release Notes

Source: [v1.0.1](https://github.com/open-policy-agent/opa/releases/tag/v1.0.1)

This is a bug fix release addressing the following issues:

- build(go): bump to 1.23.5 (authored by @srenatus).
  Addressing `CVE-2024-45341` and `CVE-2024-45336` vulnerabilities in the Go runtime.
- bundle: Add info about the correct rego version to parse modules on the store, co-authored by @ashutosh-narkar and @johanfylling in [#7278](https://github.com/open-policy-agent/opa/pull/7278).
  Fixing an issue where the rego-version for individual modules was lost during bundle deactivation (bundle lifecycle) if this version diverged from the active runtime rego-version. 
  This could cause reloading of v0 bundles to fail when OPA was not running with the `--v0-compatible` flag.

