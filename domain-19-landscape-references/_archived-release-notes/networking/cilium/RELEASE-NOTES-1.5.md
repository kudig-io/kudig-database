---
title: cilium v1.5 Release Notes
description: cilium v1.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- istio
- envoy
- cilium
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cilium v1.5 Release Notes 是什么
- 如何 cilium v1.5 Release Notes
trigger_keywords:
- cilium
- v1.5
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- service-mesh-basics
- cilium-basics
created: "2026-05-23"
---

# [[Cilium|cilium]] v1.5 Release Notes

Source: [v1.5.13](https://github.com/cilium/cilium/releases/tag/v1.5.13)

We are pleased to announce this bugfix release for the Cilium v1.5 series. This version includes various bug fixes as well as security fixes for the recently announced [[Envoy|Envoy]] CVEs. For more information, see the [Cilium security advisory](https://github.com/cilium/cilium/security/advisories/GHSA-6hf9-972x-wff3).

Summary of Changes
------------------

**Bugfixes:**
* Only enable IPv6 forwarding if IPv6 is enabled (Backport PR #10136, Upstream PR #9034, @jrfastab)
* Envoy fixes for CVE-2020-8659, CVE-2020-8660, CVE-2020-8661, CVE-2020-8664 (Backport PR #10445, Upstream PR #10434, @jrajahalme)
* ipam: Protect release from releasing alive IP (Backport PR #10136, Upstream PR #10066, @tgraf)
* pkg/bpf: Protect attr in perf_linux.go with runtime.KeepAlive (#10205, @brb)
* pkg/bpf: Protect each uintptr with runtime.KeepAlive (Backport PR #10253, Upstream PR #10168, @brb)

**CI Changes:**
* test: Wait for [[Istio|Istio]] POD termination before deleting istio-system or cilium (Backport PR #10445, Upstream PR #10325, @jrajahalme)

**Misc Changes:**
* bpf: Fix space hack in Makefile (Backport PR #10253, Upstream PR #10173, @brb)
* doc: Fix links to contributing guide (Backport PR #10445, Upstream PR #10322, @CybrPunk)
* Documentation: Lock dependency to fix build (Backport PR #10439, Upstream PR #10419, @Ropes)
* golang: update to 1.12.17 (#10209, @aanm)
* Update release process steps (Backport PR #10136, Upstream PR #10035, @aanm)
* Use -F flag in git log in check-stable script (Backport PR #10445, Upstream PR #10283, @nebril)

**Other Changes:**
* .github: update github-actions project (#10046, @aanm)