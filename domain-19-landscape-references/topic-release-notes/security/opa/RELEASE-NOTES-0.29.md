---
title: opa v0.29 Release Notes
description: opa v0.29 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- opa
- wasm
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opa v0.29 Release Notes 是什么
- 如何 opa v0.29 Release Notes
trigger_keywords:
- opa
- v0.29
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

# opa v0.29 Release Notes

Source: [v0.29.4](https://github.com/open-policy-agent/opa/releases/tag/v0.29.4)

### 🚨 Upgrade Notice: Use `opa_linux_amd64_static` instead of `opa_linux_amd64` 🚨

**What changed and why?**

The v0.29 release changes the pre-built `opa_linux_amd64` binary to better support wasm-based policy execution. The change requires glibc 2.18+ as well as libgcc.

**Who is affected by this change?**

This change affects users running alpine-based Docker images, CentOS 7, etc.

**What should affected users do?**

If you currently deploy the pre-built OPA binaries to one of these systems, update your automation to download `opa_linux_amd64_static` instead of `opa_linux_amd64`. Going forward, the `opa_linux_amd64_static` binary is recommended for systems that do not have the required system libraries.

If you currently build OPA from source and deploy to one of these systems, update your build to produce a statically linked executable by setting the CGO_ENABLED and WASM_ENABLED flags (e.g., `make build WASM_ENABLED=0 CGO_ENABLED=0`).

**Related issues**

[#3499](https://github.com/open-policy-agent/opa/issues/3499)
[#3532](https://github.com/open-policy-agent/opa/issues/3532)
[#3528](https://github.com/open-policy-agent/opa/issues/3528)

### Miscellaneous

- bundle: Implement a DirectoryLoader for fs.FS (#3493) ([#3489](https://github.com/open-policy-agent/opa/issues/3489)) authored by @[simongottschlag](https://github.com/simongottschlag)