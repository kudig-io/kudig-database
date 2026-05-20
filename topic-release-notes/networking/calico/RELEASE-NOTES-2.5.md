---
title: calico v2.5 Release Notes
description: calico v2.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- calico
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- calico v2.5 Release Notes 是什么
- 如何 calico v2.5 Release Notes
trigger_keywords:
- calico
- v2.5
- Release
- Notes
- release
- notes
---

# calico v2.5 Release Notes

Source: [v2.5.1](https://github.com/projectcalico/calico/releases/tag/v2.5.1)

# Release notes for Calico v2.5.1

**Attention Kubernetes datastore users upgrading to v2.5.x**:
Users upgrading from Calico v2.4.x or older to v2.5.x or higher with Kubernetes datastore backend must follow the one-time configuration migration task to upgrade the cluster: https://github.com/projectcalico/calico/blob/master/upgrade/v2.5/README.md (@gunjan5)

## Changes to [Felix](https://github.com/projectcalico/felix)
 - [#1538](https://github.com/projectcalico/felix/pull/1538): Add read/write timeouts to Typha connection; fixes that Felix wouldn't spot if TCP connection was dropped without being cleanly shut down.
