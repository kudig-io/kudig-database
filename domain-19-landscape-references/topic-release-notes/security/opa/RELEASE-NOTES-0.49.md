---
title: opa v0.49 Release Notes
description: opa v0.49 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- opa
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opa v0.49 Release Notes 是什么
- 如何 opa v0.49 Release Notes
trigger_keywords:
- opa
- v0.49
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

# opa v0.49 Release Notes

Source: [v0.49.2](https://github.com/open-policy-agent/opa/releases/tag/v0.49.2)

This release migrates the [ORAS Go library](oras.land/oras-go/v2) from v1.2.2 to v2.
The earlier version of the library had a dependency on the [docker](github.com/docker/docker)
package. That version of the docker package had some reported vulnerabilities such as
CVE-2022-41716, CVE-2022-41720. The ORAS Go library v2 removes the dependency on the docker package.