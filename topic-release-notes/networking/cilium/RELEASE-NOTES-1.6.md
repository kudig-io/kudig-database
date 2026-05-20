---
title: cilium v1.6 Release Notes
description: cilium v1.6 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- istio
- envoy
- cilium
- docker
- ebpf
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cilium v1.6 Release Notes 是什么
- 如何 cilium v1.6 Release Notes
trigger_keywords:
- cilium
- v1.6
- Release
- Notes
- release
- notes
---

# cilium v1.6 Release Notes

Source: [v1.6.12](https://github.com/cilium/cilium/releases/tag/v1.6.12)

We are pleased to release Cilium v1.6.12. This release updates Envoy to 1.14.5, to mitigate CVE-2020-25017.

Summary of Changes
------------------

**Minor Changes:**
* Add hidden --k8s-sync-timeout to set the timeout for initial Kubernetes synchronization (Backport PR #12823, Upstream PR #12822, @joestringer)
* envoy: Require Node only on the first request of a stream (Backport PR #13334, Upstream PR #12522, @jrajahalme)
* Istio integration has been updated to Istio release 1.5.9. (Backport PR #12888, Upstream PR #12861, @jrajahalme)
* k8s: update k8s dependencies to 1.16.15 (#12667, @aanm)

**Bugfixes:**
* Envoy is updated to release 1.14.5 (Backport PR #13334, Upstream PR #13332, @jrajahalme)
* node-init restartPods should use docker if /etc/crictl.yaml not found (Backport PR #13054, Upstream PR #12894, @UnwashedMeme)

**Misc Changes:**
* Add Kubernetes compatibility documentation (Backport PR #12799, Upstream PR #12783, @aanm)
* contrib: Add release helper scripts for preparing micro releases (Backport PR #13250, Upstream PR #13044, @joestringer)
* doc: update #ebpf Slack channel name (Backport PR #12799, Upstream PR #12766, @qmonnet)
* docs/metrics: Correct label typos in metrics.rst (Backport PR #13054, Upstream PR #12901, @sayboras)
* docs: limit copybutton to content area only (Backport PR #13054, Upstream PR #12997, @genbit)
* Upgrade Cilium docs theme version (Backport PR #13054, Upstream PR #12996, @Neelajacques)
* 1.6 special ci-fixing backport (#13111, @nebril)